#pragma once

#include "network/ThreadSafeQueue.hpp"

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <stdexcept>

namespace oops {

    class ThreadPool {
    public:

        explicit ThreadPool(std::size_t worker_count);
        ~ThreadPool();

        void enqueue(std::function<void()> task);

    private:

        ThreadSafeQueue<std::function<void()>> tasks_;
        std::vector<std::thread> workers_;

        void workerLoop();
    };

}