/**
 * @Author:         yds
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
    ThreadPool(int max_thread_num = 1);
    ThreadPool() = default;
    ThreadPool(ThreadPool&& other) = default;
    ~ThreadPool();

    template <typename T>
    void add_task(T&& task);

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