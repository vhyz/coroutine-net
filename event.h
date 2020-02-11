#pragma once

#include <sys/epoll.h>
#include <functional>

class Event {
   public:
    using EventCallBack = std::function<void(uint32_t)>;

    static const uint32_t NONE_EVENT = 0;
    static const uint32_t READ_EVENT = EPOLLIN | EPOLLPRI | EPOLLRDHUP;
    static const uint32_t WRITE_EVENT = EPOLLOUT | EPOLLRDHUP;

    Event() : fd_(-1) {}

    Event(int fd, uint32_t events = NONE_EVENT) : fd_(fd), events_(events) {}

    void SetFd(int fd) { fd_ = fd; }

    int GetFd() const { return fd_; }

    void SetEvents(uint32_t events) { events_ = events; }

    uint32_t GetEvents() const { return events_; }

    void EnableRead() { events_ |= READ_EVENT; }

    void DisableRead() { events_ &= ~READ_EVENT; }

    void EnableWrite() { events_ |= WRITE_EVENT; }

    void DisableWrite() { events_ &= ~WRITE_EVENT; }

    void SetEventCallBack(const EventCallBack& cb) { cb_ = cb; }

    void SetEventCallBack(EventCallBack&& cb) { cb_ = std::move(cb); }

    void HandleEvent(uint32_t events) {
        if (cb_) {
            cb_(events);
        }
    }

   private:
    int fd_;

    uint32_t events_;

    EventCallBack cb_;
};
