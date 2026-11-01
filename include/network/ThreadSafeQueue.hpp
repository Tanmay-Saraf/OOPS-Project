#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <stdexcept>

namespace oops {

    template <typename T> 
    class ThreadSafeQueue {

    private:
        std::queue<T> queue_;
        std::mutex mutex_;
        std::condition_variable condition_;   
        bool stopped = false;
        
    public:

        void push(T item) {
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stopped) {
                    throw std::runtime_error("Queue is stopped");
                }
                queue_.push(std::move(item));
            }

            condition_.notify_one();
        }

        bool tryPop(T &item) {

            std::lock_guard<std::mutex> lock(mutex_);
            if(queue_.empty()) return false;

            item = std::move(queue_.front());
            queue_.pop();
            return true;

        }

        bool waitPop(T& item) {

            std::unique_lock<std::mutex> lock(mutex_);

            condition_.wait(lock, [this]() {
                return !queue_.empty() || stopped;
            });

            if(queue_.empty()) return false;

            item = std::move(queue_.front());
            queue_.pop();
            return true;

        }

        void stop(){

            {
                std::lock_guard<std::mutex> lock(mutex_);
                stopped = true;
            }

            condition_.notify_all();


        }
    
    };

}
