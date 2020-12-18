/**
 * @Author:         yds
 * @Date    :       2020-12-18
*/
#include "epoller.h"

EPoller::EPoller(int max_event) : epoll_fd_(epoll_create(1024)), events_(max_event) {
    assert(epoll_fd_ >= 0 && events_.size() > 0);
}

EPoller::~EPoller() {
    close(epoll_fd_);
}

bool EPoller::add_fd(int fd, uint32_t events) {
    if (fd < 0) return false;

    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

bool EPoller::mod_fd(int fd, uint32_t events) {
    if (fd < 0) return false;

    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

bool EPoller::del_fd(int fd) {
    if (fd < 0) return false;

    struct epoll_event event = {0};
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event);
}

int EPoller::wait(int timeout_ms) {
    return epoll_wait(epoll_fd_, &events_[0], events_.size(), timeout_ms);
}

int EPoller::get_event_fd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t EPoller::get_events(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}