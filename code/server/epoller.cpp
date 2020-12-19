/**
 * @Author:         yds
 * @Date    :       2020-12-18
*/
#include "epoller.h"

Epoller::Epoller(int max_event) : epoll_fd_(epoll_create(1024)), events_(max_event) {
    assert(epoll_fd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epoll_fd_);
}

bool Epoller::add_fd(int fd, uint32_t events) {
    if (fd < 0) return false;

    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

bool Epoller::mod_fd(int fd, uint32_t events) {
    if (fd < 0) return false;

    struct epoll_event event = {0};
    event.data.fd = fd;
    event.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

bool Epoller::del_fd(int fd) {
    if (fd < 0) return false;

    struct epoll_event event = {0};
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event);
}

int Epoller::wait(int timeout_ms) {
    return epoll_wait(epoll_fd_, &events_[0], events_.size(), timeout_ms);
}

int Epoller::get_event_fd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoller::get_events(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}