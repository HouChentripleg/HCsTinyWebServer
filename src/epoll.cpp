#include "../include/epoll.hpp"

Epoll::Epoll(int maxEvent) : epollfd_(epoll_create(1)), events_(maxEvent) {
    assert(epollfd_ >= 0 && events_.size() > 0);
}

Epoll::~Epoll() {
    close(epollfd_);
}

bool Epoll::addFd(int fd, uint32_t events) {
    if(fd < 0) {
        return false;
    }
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
    return ret == 0;
}

bool Epoll::modFd(int fd, uint32_t events) {
    if(fd < 0) {
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev);
    return ret == 0;
}

bool Epoll::rmFd(int fd) {
    if(fd < 0) {
        return false;
    }
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, NULL);
    return ret == 0;
}

int Epoll::wait(int timeMS) {
    int ret = epoll_wait(epollfd_, &events_[0], static_cast<int>(events_.size()), timeMS);
    return ret; 
}

int Epoll::getEventFd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoll::getEvents(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}