#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>
#include <pthread.h>

namespace nano_std {
    template<typename T>
    class TSafeQueue {
    private:
        std::mutex mut;
        std::queue<T> data_queue;

    public:
        TSafeQueue() {
        }

        TSafeQueue(TSafeQueue const &other) {
            std::lock_guard<std::mutex> lock(other.mut);
            data_queue = other.data_queue;
        }

        void push(T value) {
            std::lock_guard<std::mutex> lock(mut);
            data_queue.push(value);
        }

        std::shared_ptr<T> pop() {
            std::lock_guard<std::mutex> lock(mut);
            std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
            data_queue.pop();
            return res;
        }

        std::shared_ptr<T> try_pop() {
            std::lock_guard<std::mutex> lock(mut);
            if (data_queue.empty())
                return std::shared_ptr<T>();
            std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
            data_queue.pop();
            return res;
        }

        bool size() {
            std::lock_guard<std::mutex> lock(mut);
            return data_queue.size();
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(mut);
            return data_queue.empty();
        }

        void clear() {
            std::lock_guard<std::mutex> lock(mut);
            while (!data_queue.empty())data_queue.pop();
        }
    };

    class WorkerThread {
    private:
        TSafeQueue<std::function<void(void)>> queue;
        std::thread t;
        std::mutex mux;
        std::condition_variable thread_returned;
        std::thread::id tid;
        std::string name;
        bool is_running;
        bool thread_stopped;

        bool isRunning() {
            std::unique_lock<std::mutex> lock(mux);
            return is_running;
        }

        void isRunning(bool running) {
            std::lock_guard<std::mutex> lock(mux);
            is_running = running;
            if (running) {
                thread_stopped = false;
            }
        }

    public:
        WorkerThread() {
            is_running = false;
            thread_stopped = true;
        };

        ~WorkerThread() {
            queue.clear();
            stop();
        }

        void run() {
            isRunning(true);
            t = std::move(std::thread(&WorkerThread::mainLoop, this));
            tid = t.get_id();
            t.detach();
        }

        void setName(std::string n) {
            std::unique_lock<std::mutex> lock(mux);
            this->name = n;
        }

        std::string getName() {
            std::unique_lock<std::mutex> lock(mux);
            return this->name;
        }


        void stop() {
            isRunning(false);
            std::unique_lock<std::mutex> lock(mux);
            thread_returned.wait(lock, [this] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return thread_stopped;
            });
        }

        void insertTask(std::function<void(void)> &task) {
            queue.push(task);
        }

        int remainTasks() {
            return queue.size();
        }

        // main loop
        void mainLoop() {
            std::string n = getName();
            if (n.size() > 0) {
                pthread_setname_np(getName().c_str());
            }
            do {
                if (isRunning() && !queue.empty()) {
                    std::function<void(void)> task = *(queue.pop());
                    task();
                } else {
                    // else sleep for a while
                    std::this_thread::sleep_for(std::chrono::milliseconds (1));
                }
            } while (isRunning());
            std::unique_lock<std::mutex> lk(mux);
            thread_stopped = true;
            thread_returned.notify_one();
        }
    };
static int wid = 0;
    class ThreadPool {
    private:
        TSafeQueue<std::function<void(void)>> queue;
        std::vector<WorkerThread *> workers;
        std::condition_variable is_empty;
        unsigned int current_index;
        unsigned int max_index;

    public:
        explicit ThreadPool(unsigned int count) {
            max_index = count;
            current_index = 0;
            for (unsigned int i = 0; i < count; i++) {
                auto *worker = new WorkerThread();
                worker->setName("nano" + std::to_string(wid++));
                worker->run();
                workers.push_back(worker);
            }
        };

        ~ThreadPool() {
            for (auto w: workers) {
                w->stop();
                delete w;
            }
        }

        void doAsync(std::function<void(void)> task) {
            current_index = (current_index + 1) % max_index;
            workers[current_index]->insertTask(task);
        }

        void doSync(std::function<void(void)> task) {
            std::mutex mux;
            mux.lock();
            doAsync([&mux, &task]() {
                if (task) {
                    task();
                }
                mux.unlock();
            });
            mux.lock();
        }

        void syncGroup(std::vector<std::function<void(void)>> &tasks, int batch_size = 1) {
            std::vector<std::function<void(void)>> real_tasks;
            if (batch_size > 1) {
                int real_size = tasks.size() / batch_size + 1;
                real_tasks.resize(real_size);
                for (unsigned int i = 0; i < real_size; i++) {
                    real_tasks[i] = [&tasks, i, batch_size]() {
                        for (int j = 0; j < batch_size; j++) {
                            int index = i * batch_size + j;
                            if (index < tasks.size()) {
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
            for (auto task: real_tasks) {
                doAsync([task, this, &mx, &count]() {
                    task();
                    std::unique_lock<std::mutex> lock(mx);
                    if ((--count) == 0) {
                        is_empty.notify_one();
                    }
                });
            }
            std::unique_lock<std::mutex> lock(mx);
            is_empty.wait(lock, [&count] {
                return count == 0;
            });
        }
    };
};

#endif // THREAD_POOL_H