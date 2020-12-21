/**

 * @Date    :       2020-12-18
*/

#ifndef __HEAPTIMER_H_
#define __HEAPTIMER_H_

#include <queue>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <functional>
#include <time.h>
#include <arpa/inet.h>
#include <assert.h>
#include "../log/log.h"


typedef std::chrono::high_resolution_clock  Clock;
typedef Clock::time_point TimeStamp;
typedef std::chrono::milliseconds MS;
typedef std::function<void()> TimeoutCallback;


struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallback cb;
    bool operator<(const TimerNode &other) {
        return expires < other.expires;
    }
};

class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer();
    void adjust(int id, int new_expires);
    void add(int id, int expires, TimeoutCallback cb);
    void del_and_do_work(int id);
    void clear();
    void tick();
    void pop();
    int get_next_tick();

private:
    void del(size_t i);
    void siftup(size_t i);
    bool siftdown(size_t index, size_t n);
    void swap_node(size_t i, size_t j);

private:
    std::vector<TimerNode> heap_;//最小堆数据结构
    std::unordered_map<int, size_t> ref_;//记录定时器在数组的位置和定时器id的映射
};

#endif // !__HEAPTIMER_H_