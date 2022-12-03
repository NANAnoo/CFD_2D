#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>

namespace nano_std
{
    template <typename T>
    class TSafeQueue
    {
    private:
        std::mutex mut;
        std::queue<T> data_queue;

    public:
        TSafeQueue()
        {
        }
        TSafeQueue(TSafeQueue const &other)
        {
            std::lock_guard<std::mutex> lock(other.mut);
            data_queue = other.data_queue;
        }
        void push(T value)
        {
            std::lock_guard<std::mutex> lock(mut);
            data_queue.push(value);
        }
        std::shared_ptr<T> pop()
        {
            std::lock_guard<std::mutex> lock(mut);
            std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
            data_queue.pop();
            return res;
        }
        std::shared_ptr<T> try_pop()
        {
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
        bool empty()
        {
            std::lock_guard<std::mutex> lock(mut);
            return data_queue.empty();
        }
    };

    class WorkerThread
    {
    private:
        TSafeQueue<std::function<void(void)>> queue;
        std::thread t;
        std::mutex mux;
        bool is_running;
        bool isRunning()
        {
            std::unique_lock<std::mutex> lock(mux);
            return is_running;
        }
        void setRuning(bool running)
        {
            std::lock_guard<std::mutex> lock(mux);
            is_running = running;
        }

    public:
        WorkerThread()
        {
            is_running = false;
        };

        ~WorkerThread()
        {
            stop();
        }

        void run()
        {
            setRuning(true);
            t = std::move(std::thread(&WorkerThread::mainLoop, this));
            t.detach();
        }

        void stop()
        {
            setRuning(false);
        }

        void insertTask(std::function<void(void)> &task)
        {
            queue.push(task);
        }

        int remainTasks()
        {
            return queue.size();
        }

        // main loop
        void mainLoop()
        {
            do
            {
                if (isRunning() && !queue.empty())
                {
                    std::function<void(void)> task = *(queue.pop());
                    task();
                } else
                {
                    // else sleep for a while
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            } while (isRunning());
        }
    };

    class ThreadPool
    {
    private:
        TSafeQueue<std::function<void(void)>> queue;
        std::vector<WorkerThread *> workers;
        std::condition_variable is_empty;
        unsigned int current_index;
        unsigned int max_index;

    public:
        explicit ThreadPool(unsigned int count)
        {
            max_index = count;
            current_index = 0;
            for (unsigned int i = 0; i < count; i++)
            {
                auto *worker = new WorkerThread();
                worker->run();
                workers.push_back(worker);
            }
        };
        void doAsync(std::function<void(void)> task)
        {
            current_index = (current_index + 1) % max_index;
            workers[current_index]->insertTask(task);
        }
        void doSync(std::function<void(void)> task)
        {
            std::mutex mux;
            mux.lock();
            doAsync([&mux, &task]()
                    {
                        if (task) {
                            task();
                        }
                        mux.unlock(); });
            mux.lock();
        }

        void syncGroup(std::vector<std::function<void(void)>> &tasks, int batch_size = 1)
        {
            std::vector<std::function<void(void)>> real_tasks;
            if (batch_size > 1) {
                int real_size = tasks.size() / batch_size + 1;
                real_tasks.resize(real_size);
                for (unsigned int i = 0; i < real_size; i ++) {
                    real_tasks[i] = [&tasks, i, batch_size](){
                        for (int j = 0; j < batch_size; j ++) {
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
            for (auto task : real_tasks)
            {
                doAsync([task, this, &mx, &count](){
                    task();
                    std::unique_lock<std::mutex> lock(mx);
                    if ((--count) == 0) {
                        is_empty.notify_one();
                    }
                });
            }
            std::unique_lock<std::mutex> lock(mx);
            is_empty.wait(lock, [&count]
            {
                return count == 0;
            });
        }
    };
};

#endif // THREAD_POOL_H