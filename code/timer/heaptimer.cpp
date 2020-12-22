/**

 * @Date    :       2020-12-18
*/

#include "heaptimer.h"

HeapTimer::HeapTimer() {
    heap_.reserve(64);
}

HeapTimer::~HeapTimer() {
    clear();
}

/**
 * 交换节点
*/
void HeapTimer::swap_node(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size() && j >= 0 && j < heap_.size());
    if (i == j) return;

    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = j;
    ref_[heap_[j].id] = i;
}

/**
 * 上虑
*/
void HeapTimer::siftup(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) break;
        swap_node(j, i);
        i = j;
        j = (i - 1) / 2;
    }
}

/**
 * 下滤
*/
bool HeapTimer::siftdown(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index, j = index * 2 * 1;
    while (j < n) {
        //找出最小的子节点
        if (j+1 < n && heap_[j+1] < heap_[j]) j++;
        //如果下滤节点小于当前子节点最小值则下滤结束
        if (heap_[i] < heap_[j]) break;
        swap_node(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

/**
 * 调整最小堆
*/
void HeapTimer::adjust(int id, int new_expires) {
    assert(!heap_.empty() && ref_.count(id));
    //更新定时器时间并且下滤，新的定时时间一定大于原本的吗？？？
    heap_[ref_[id]].expires = Clock::now() + MS(new_expires);
    siftdown(ref_[id], heap_.size());
}

/**
 * 添加新的定时
*/
void HeapTimer::add(int id, int expires, TimeoutCallback cb) {
    assert(id > 0 && expires > 0);
    size_t i;
    //是新的定时器还是已经存在的
    if (ref_.count(id) == 0) {
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now()+MS(expires), cb});
        siftup(i);
    }
    else {
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(expires);
        heap_[i].cb = cb;
        if (!siftdown(i, heap_.size())) {
            siftup(i);
        }
    }
}

/**
 * 删除定时器
*/

void HeapTimer::del(size_t i) {
    assert(i >= 0 && i < heap_.size());
    //将要删除的节点和最后一个节点交换然后调整堆
    size_t n = heap_.size() - 1;
    if (i < n) {
        swap_node(i, n);
        if (!siftdown(i, n)) {
            siftup(i);
        }
    }
    ref_.erase(heap_[n].id);
    heap_.pop_back();
}

/**
 * 删除定时器并且执行定时器回调
*/
void HeapTimer::del_and_do_work(int id) {
    assert(id > 0);
    int i = ref_[id];
    heap_[i].cb();
    del(i);
}

/**
 * 清空定时器
*/
void HeapTimer::clear() {
    heap_.clear();
    ref_.clear();
}

/**
 * 清楚超时节点
*/
void HeapTimer::tick() {
    if (heap_.empty()) return;
    while (heap_.size()) {
        if (std::chrono::duration_cast<MS>(heap_[0].expires - Clock::now()).count() > 0) {
            break;
        }
        heap_[0].cb();
        pop();
    }
}

/**
 * 堆顶出队
*/
void HeapTimer::pop() {
    assert(heap_.size());
    del(0);
}

/**
 * 执行超时回调并获取下一个定时时间
*/
int HeapTimer::get_next_tick() {
    tick();
    size_t ret = -1;
    if (heap_.size()) {
        ret = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if(ret < 0) { ret = 0; }
    }
    return ret;
}