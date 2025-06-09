#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <memory>
#include <future>
#include <atomic>

namespace nano_std {

    template<typename T>
    class TSafeQueue {
    private:
        std::queue<T> data_queue;
        mutable std::mutex mut;

    public:
        void push(T value) {
            std::lock_guard<std::mutex> lock(mut);
            data_queue.push(std::move(value));
        }

        bool try_pop(T& value) {
            std::lock_guard<std::mutex> lock(mut);
            if (data_queue.empty())
                return false;
            value = std::move(data_queue.front());
            data_queue.pop();
            return true;
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mut);
            return data_queue.size();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mut);
            return data_queue.empty();
        }

        void clear() {
            std::lock_guard<std::mutex> lock(mut);
            while (!data_queue.empty()) data_queue.pop();
        }
    };

    class WorkerThread {
    private:
        std::thread t;
        std::atomic<bool> running;
    public:
        WorkerThread() : running(true) {}
        ~WorkerThread() {
            stop();
        }

        void run(std::function<void()> loop) {
            t = std::thread([this, loop]() {
                while (running) {
                    loop();
                }
            });
        }

        void stop() {
            running = false;
            if (t.joinable())
                t.join();
        }
    };

    class ThreadPool {
    private:
        TSafeQueue<std::function<void(void)>> queue;
        std::vector<std::unique_ptr<WorkerThread>> workers;
        std::mutex queue_mutex;
        std::condition_variable cv_task;
        std::atomic<bool> stopped{false};
        unsigned int current_index{0};
        unsigned int max_index{0};

    public:
        explicit ThreadPool(unsigned int count) {
            max_index = count;
            for (unsigned int i = 0; i < count; i++) {
                auto worker = std::make_unique<WorkerThread>();
                worker->run([this]() {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        cv_task.wait(lock, [this] {
                            return stopped || !queue.empty();
                        });
                        if (stopped && queue.empty()) return;
                        if (!queue.try_pop(task)) return;
                    }
                    task();
                });
                workers.push_back(std::move(worker));
            }
        }

        ~ThreadPool() {
            stopped = true;
            cv_task.notify_all();
            for (auto& w : workers) {
                w->stop();
            }
        }

        void doAsync(std::function<void(void)> task) {
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                queue.push(std::move(task));
            }
            cv_task.notify_one();
        }

        void doSync(std::function<void(void)> task) {
            std::mutex mtx;
            std::unique_lock<std::mutex> lock(mtx);
            doAsync([&task, &mtx]() {
                task();
                mtx.unlock();
            });
            mtx.lock(); // wait until unlocked by task
        }

        void syncGroup(std::vector<std::function<void(void)>> &tasks, int batch_size = 1) {
            std::vector<std::function<void(void)>> real_tasks;
            if (batch_size > 1) {
                int real_size = tasks.size() / batch_size + (tasks.size() % batch_size != 0);
                real_tasks.resize(real_size);
                for (int i = 0; i < real_size; i++) {
                    real_tasks[i] = [&tasks, i, batch_size]() {
                        for (int j = 0; j < batch_size; j++) {
                            int index = i * batch_size + j;
                            if (index < (int)tasks.size()) {
                                tasks[index]();
                            }
                        }
                    };
                }
            } else {
                real_tasks = tasks;
            }

            int count = real_tasks.size();
            std::mutex mx;
            std::condition_variable cv_done;

            for (auto& task : real_tasks) {
                doAsync([&task, &count, &mx, &cv_done]() {
                    task();
                    std::unique_lock<std::mutex> lock(mx);
                    if (--count == 0) {
                        cv_done.notify_one();
                    }
                });
            }

            std::unique_lock<std::mutex> lock(mx);
            cv_done.wait(lock, [&count]() {
                return count == 0;
            });
        }
    };

}

#endif // THREAD_POOL_H
