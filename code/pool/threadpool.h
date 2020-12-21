/**

 * @Date    :       2020-12-18
*/

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <memory>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(int max_thread_num = 1) : pool_(std::make_shared<struct pool>()) {
        for (int i = 0; i < max_thread_num; i++) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> lock(pool->mtx_);
                while (true) {
                    if (pool->tasks.size()) {
                        auto task = pool->tasks.front();
                        pool->tasks.pop();
                        lock.unlock();
                        task();
                        lock.lock();
                    }
                    else if (pool->is_close_) break;
                    else pool->cond.wait(lock);
                }
            }).detach();
        }
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool&& other) = default;
    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> lock(pool_->mtx_);
                pool_->is_close_ = true;
            }
            pool_->cond.notify_all();
        }
    }

    template <typename T>
    void add_task(T&& task) {
        {
            std::lock_guard<std::mutex> lock(pool_->mtx_);
            pool_->tasks.emplace(std::forward<T>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct pool {
        std::mutex mtx_;
        std::condition_variable cond;
        std::queue<std::function<void()>> tasks;
        bool is_close_;
    };
    std::shared_ptr<struct pool> pool_;
};

#endif // !__THREADPOOL_H__