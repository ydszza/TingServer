/**

 * @Date    :       2020-12-18
*/
#include "threadpool.h"

ThreadPool::ThreadPool(int max_thread_num) : pool_(std::make_shared<struct pool>()) {
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

ThreadPool::~ThreadPool() {
    if (static_cast<bool>(pool_)) {
        {
            std::lock_guard<std::mutex> lock(pool_->mtx_);
            pool_->is_close_ = true;
        }
        pool_->cond.notify_all();
    }
}

template <typename T>
void ThreadPool::add_task(T&& task) {
    {
        std::lock_guard<std::mutex> lock(pool_->mtx_);
        pool_->tasks.emplace(std::forward<T>(task));
    }
    pool_->cond.notify_one();
}