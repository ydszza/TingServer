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

template<typename T>
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

#endif // !__BLOCKQUEUE_H_