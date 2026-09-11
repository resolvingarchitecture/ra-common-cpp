#include <chrono>
#include <thread>

#include "doctest/doctest.h"
#include "ra_common/tasks/task_runner.hpp"

using namespace ra::common::tasks;

TEST_CASE("task runner one-shot + periodic") {
    {
        TaskRunner runner(20);
        int runs = 0;
        runner.AddTask(std::make_shared<LambdaTask>(TaskConfig::Once("c"), [&] {
            runs++;
            return true;
        }));
        runner.Start();
        for (int i = 0; i < 100 && (runs < 1 || runner.TaskCount() > 0); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        CHECK(runs == 1);
        runner.Shutdown();
        CHECK(runner.Status() == RunnerStatus::Shutdown);
    }
    {
        TaskRunner runner(10);
        int n = 0;
        runner.AddTask(std::make_shared<LambdaTask>(TaskConfig::Periodic("p", 10), [&] {
            n++;
            return true;
        }));
        runner.Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        runner.Shutdown();
        CHECK(n >= 2);
    }
}
