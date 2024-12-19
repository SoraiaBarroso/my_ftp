#ifndef THREAD_POOL_HPP 
#define THREAD_POOL_HPP 
    #include <cstddef>
    #include <iostream>
    #include <thread>
    #include <vector>
    #include <queue>
    #include <functional>
    #include <mutex>
    #include <condition_variable>

    class ThreadPool {
        public:
            // Calculate the optimal number of threads based on the current load or CPU count
            ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) : stop_(false) {
                for (size_t i = 0; i < num_threads; ++i) {
                    threads_.emplace_back([this] {
                        while (true) {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(queue_mutex_);
                                condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                                if (stop_ && tasks_.empty()) return;
                                task = std::move(tasks_.front());
                                tasks_.pop();
                            }

                            task();
                        }
                    });
                }
            };

            ~ThreadPool() {
               stop();               
            }   

            void stop() {
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    stop_ = true;
                }
                // Notify all threads to stop
                condition_.notify_all();
                for (std::thread &thread : threads_) {
                    if (thread.joinable()) thread.join();
                }
            }

            // Enqueue task for execution by the thread pool
            void enqueue(std::function<void()> task) {
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    tasks_.push(std::move(task));
                }
                // Notify a waiting thread
                condition_.notify_one(); 
            }


        private:
            std::vector<std::thread> threads_;
            std::queue<std::function<void()>> tasks_;
            std::mutex queue_mutex_;
            std::condition_variable condition_;
            bool stop_;
    };
#endif