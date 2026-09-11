#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "ra_common/tasks/task_config.hpp"

// Schedules and runs ITask objects. Ports the ra.common.tasks package. C++
// has no standard async runtime (unlike node's event loop or a language with
// async/await), so this uses std::thread throughout - closer to
// ra-common-python's threading-based TaskRunner than ra-common-ts's
// single-threaded async one.
//
// Threads are detached, not joined: a Managed entry is a shared_ptr captured
// by value in its worker's lambda, so the entry stays alive exactly as long
// as the thread needs it regardless of when (or whether) TaskRunner erases it
// from its own tracking vector. Shutdown() waits on an active-worker counter
// instead of std::thread::join() - joining would require holding onto every
// completed thread's std::thread object until Shutdown() runs, and destroying
// a still-joinable std::thread (e.g. when a completed one-shot task's entry
// is erased during normal Pass() cleanup) calls std::terminate().

namespace ra::common::tasks {

enum class RunnerStatus {
    Running,
    Stopping,
    Shutdown,
};

class TaskRunner {
public:
    explicit TaskRunner(int poll_period_ms = 30'000) : poll_ms_(std::max(1, poll_period_ms)) {}

    ~TaskRunner() { Shutdown(); }

    TaskRunner(const TaskRunner&) = delete;
    TaskRunner& operator=(const TaskRunner&) = delete;

    void AddTask(std::shared_ptr<ITask> task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            managed_.push_back(std::make_shared<Managed>(std::move(task), TaskStatus::Ready, false, false));
        }
        if (running_) Pass();
    }

    void Poke() {
        if (running_) Pass();
    }

    void Start() {
        if (running_.exchange(true)) return;
        poll_thread_ = std::thread([this] { PollLoop(); });
        Pass();
    }

    RunnerStatus Status() const {
        if (!running_ && !poll_thread_.joinable()) return RunnerStatus::Shutdown;
        return running_ ? RunnerStatus::Running : RunnerStatus::Stopping;
    }

    size_t TaskCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return managed_.size();
    }

    void Shutdown() {
        if (running_.exchange(false)) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& m : managed_) m->stop.store(true);
            }
            poll_cv_.notify_all();
        }
        if (poll_thread_.joinable()) poll_thread_.join();

        std::unique_lock<std::mutex> lock(workers_mutex_);
        workers_done_cv_.wait(lock, [this] { return active_workers_.load() == 0; });

        std::lock_guard<std::mutex> mlock(mutex_);
        managed_.clear();
    }

private:
    struct Managed {
        std::shared_ptr<ITask> task;
        std::atomic<TaskStatus> status;
        std::atomic<bool> stop;
        std::atomic<bool> scheduled;

        Managed(std::shared_ptr<ITask> t, TaskStatus s, bool st, bool sc)
            : task(std::move(t)), status(s), stop(st), scheduled(sc) {}
    };

    void PollLoop() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (running_) {
            poll_cv_.wait_for(lock, std::chrono::milliseconds(poll_ms_), [this] { return !running_.load(); });
            if (!running_) return;
            lock.unlock();
            Pass();
            lock.lock();
        }
    }

    void Pass() {
        std::vector<std::shared_ptr<Managed>> to_schedule;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            managed_.erase(std::remove_if(managed_.begin(), managed_.end(),
                                           [](const std::shared_ptr<Managed>& m) {
                                               return m->status.load() == TaskStatus::Completed;
                                           }),
                            managed_.end());
            for (auto& m : managed_) {
                if (m->scheduled.exchange(true)) continue;
                auto cfg = m->task->Config();
                if (cfg.periodicity_ms == -1) {
                    m->scheduled = false;  // leave it Ready; re-checked next Pass()
                    continue;
                }
                to_schedule.push_back(m);
            }
        }
        for (auto& m : to_schedule) {
            auto cfg = m->task->Config();
            active_workers_.fetch_add(1);
            std::thread([this, m, cfg] {
                Worker(m, cfg);
                if (active_workers_.fetch_sub(1) == 1) {
                    std::lock_guard<std::mutex> lock(workers_mutex_);
                    workers_done_cv_.notify_all();
                }
            }).detach();
        }
    }

    static void Worker(const std::shared_ptr<Managed>& m, const TaskConfig& cfg) {
        if (cfg.delayed && cfg.delay_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(cfg.delay_ms));
        for (;;) {
            if (m->stop.load() || m->task->ShouldStop()) {
                m->task->OnStop();
                break;
            }
            m->status.store(TaskStatus::Running);
            m->task->Execute();
            if (cfg.periodicity_ms <= 0) break;
            m->status.store(TaskStatus::Ready);
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.periodicity_ms));
        }
        m->status.store(TaskStatus::Completed);
    }

    std::vector<std::shared_ptr<Managed>> managed_;
    mutable std::mutex mutex_;
    std::condition_variable poll_cv_;
    std::atomic<bool> running_{false};
    int poll_ms_;
    std::thread poll_thread_;

    std::atomic<int> active_workers_{0};
    std::mutex workers_mutex_;
    std::condition_variable workers_done_cv_;
};

}  // namespace ra::common::tasks
