#pragma once

#include <functional>
#include <string>

namespace ra::common::tasks {

enum class TaskStatus {
    Ready,
    Running,
    Completed,
};

struct TaskConfig {
    std::string name;
    int periodicity_ms = 0;
    bool delayed = false;
    int delay_ms = 0;
    bool fixed_delay = false;
    bool long_running = false;

    static TaskConfig Once(std::string name) { return TaskConfig{std::move(name), 0, false, 0, false, false}; }

    static TaskConfig Periodic(std::string name, int period_ms) {
        auto cfg = Once(std::move(name));
        cfg.periodicity_ms = period_ms;
        return cfg;
    }
};

/// A schedulable unit of work. Ports ra.common.tasks.Task. Concrete tasks can
/// be a lambda-backed `LambdaTask` (see below) or a custom subclass.
class ITask {
public:
    virtual ~ITask() = default;
    virtual TaskConfig Config() const = 0;
    virtual bool Execute() = 0;
    virtual bool ShouldStop() const { return false; }
    virtual void OnStop() {}
};

/// Wraps a config + a `bool()` callable as an ITask, for the common case of
/// not wanting a full subclass.
class LambdaTask : public ITask {
public:
    LambdaTask(TaskConfig config, std::function<bool()> execute) : config_(std::move(config)), execute_(std::move(execute)) {}

    TaskConfig Config() const override { return config_; }
    bool Execute() override { return execute_(); }

private:
    TaskConfig config_;
    std::function<bool()> execute_;
};

}  // namespace ra::common::tasks
