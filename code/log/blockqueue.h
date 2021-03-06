/**

 * @Date    :       2020-12-17
*/

#ifndef __BLOCKQUEUE_H_
#define __BLOCKQUEUE_H_

#include <deque>
#include <mutex>
#include <assert.h>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockQueue {
public:
    explicit BlockQueue(size_t max_capacity = 1024);
    ~BlockQueue();

    void clear();
    bool empty();
    bool full();
    void close();

    size_t size() const;
    size_t capacity() const;

    T front();
    T back();

    void push_front(const T& item);
    void push_back(const T& item);
    bool pop(T& item);
    bool pop(T& item, int timeout);

    void flush();

private:   
    std::deque<T> deq_;//阻塞队列底层数据结构双端队列
    size_t capacity_;//队列的容量

    std::mutex mtx_;//队列互斥锁
    std::condition_variable cond_consumer_;//队列pop条件变量
    std::condition_variable cond_producer_;//队列push条件变量

    bool is_close_;//是否打开阻塞队列
};

template<class T>
BlockQueue<T>::BlockQueue(size_t max_capacity) : capacity_(max_capacity), is_close_(false) {
    assert(max_capacity > 0);
}

template<class T>
BlockQueue<T>::~BlockQueue() {
    close();
}

/**
 * 清空队列并且设置标志位关闭
*/
template<class T>
void BlockQueue<T>::close() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        deq_.clear();
        is_close_ = true;
    }
    cond_consumer_.notify_all();
    cond_producer_.notify_all();
}

/**
 * 清空队列
*/
template<class T>
void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    deq_.clear();
}

/**
 * 获取队列是否空
*/
template<class T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.empty();
}

/**
 * 获取队列是否满
*/
template<class T>
bool BlockQueue<T>::full() {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.size() >= capacity_;
}

/**
 * 获取当前队列数量
*/
template<class T>
size_t BlockQueue<T>::size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.size();
}

/**
 * 获取队列的最大容量
*/
template<class T>
size_t BlockQueue<T>::capacity() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return capacity_;
}

/**
 * 获取队首元素
*/
template<class T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.front();
}

/**
 * 获取队尾元素
*/
template<class T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> lock(mtx_);
    return deq_.back();
}

/**
 * push_front
*/
template<class T>
void BlockQueue<T>::push_front(const T& item) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (deq_.size() >= capacity_) {
        cond_producer_.wait(lock);
    }
    deq_.push_front(item);
    cond_consumer_.notify_one();
}

/**
 * push_back
*/
template<class T>
void BlockQueue<T>::push_back(const T& item) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (deq_.size() >= capacity_) {
        cond_producer_.wait(lock);
    }
    deq_.push_back(item);
    cond_consumer_.notify_one();
}

/**
 * pop
*/
template<class T>
bool BlockQueue<T>::pop(T& item) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (deq_.empty()) {
        cond_consumer_.wait(lock);
        if (is_close_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

/**
 * pop并设置超时时间
*/
template<class T>
bool BlockQueue<T>::pop(T& item, int timeout) {
    std::unique_lock<std::mutex> lock(mtx_);
    while (deq_.empty()) {
        if (cond_consumer_.wait_for(lock, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout) {
            return false;
        }
        if (is_close_) return false;
    }
    item = deq_.front();
    deq_.pop_front();
    cond_producer_.notify_one();
    return true;
}

/**
 * 通知一个消费者
*/
template<class T>
void BlockQueue<T>::flush() {
    cond_consumer_.notify_one();
}

#endif // !__BLOCKQUEUE_H_