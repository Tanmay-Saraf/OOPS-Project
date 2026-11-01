#include "network/ThreadPool.hpp"
#include <iostream>
#include <exception>
#include <utility>

namespace oops {

    ThreadPool::ThreadPool(std::size_t worker_count) {
        if (worker_count == 0) {
            throw std::invalid_argument("ThreadPool requires at least one worker");
        }

        try {
            for (std::size_t i = 0; i < worker_count; ++i) {
                workers_.emplace_back(&ThreadPool::workerLoop, this);
            }
        } catch (...) {
            tasks_.stop();

            for (auto& worker : workers_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }

            throw;
        }
    }

    void ThreadPool::workerLoop() {
        std::function<void()> task;

        while (tasks_.waitPop(task)) {
            try {
                task();
            }catch(const std::exception& e) {
                std::cerr << "Worker task failed: " << e.what() << '\n';
            } catch(...) {
                std::cerr << "Worker task failed with an unknown exception\n";
            }
        }
    }

    void ThreadPool::enqueue(std::function<void()> task) {
        tasks_.push(std::move(task));
    }

    ThreadPool::~ThreadPool() {
        tasks_.stop();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

}