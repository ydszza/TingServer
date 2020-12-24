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
        assert(max_thread_num > 0);
        for (int i = 0; i < max_thread_num; i++) {
            std::thread(std::bind(&ThreadPool::do_task, this)).detach();
        }
    }
    void do_task() {
        std::unique_lock<std::mutex> lock(pool_->mtx_);
        while (true) {
            if (pool_->tasks.size()) {
                auto task = std::move(pool_->tasks.front());
                pool_->tasks.pop();
                lock.unlock();
                task();
                lock.lock();
            }
            else if (pool_->is_close_) break;
            else pool_->cond.wait(lock);
        }
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
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