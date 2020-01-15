#ifndef VHYZ_EVENT_H
#define VHYZ_EVENT_H

#include <sys/epoll.h>
#include <functional>

class Event {
   public:
    using EventCallBack = std::function<void(uint32_t)>;

    static const uint32_t NONE_EVENT = 0;
    static const uint32_t READ_EVENT = EPOLLIN | EPOLLPRI;
    static const uint32_t WRITE_EVENT = EPOLLOUT;

    Event() : fd_(-1) {}

    Event(int fd, uint32_t events = NONE_EVENT) : fd_(fd), events_(events) {}

    void setFd(int fd) { fd_ = fd; }

    int getFd() const { return fd_; }

    void setEvents(uint32_t events) { events_ = events; }

    uint32_t getEvents() const { return events_; }

    void enableRead() { events_ |= READ_EVENT; }

    void disableRead() { events_ &= ~READ_EVENT; }

    void enableWrite() { events_ |= WRITE_EVENT; }

    void disableWrite() { events_ &= ~WRITE_EVENT; }

    void setEventCallBack(const EventCallBack& cb) { cb_ = cb; }

    void setEventCallBack(EventCallBack&& cb) { cb_ = std::move(cb); }

    void handleEvent(uint32_t events) {
        if (cb_) {
            cb_(events);
        }
    }

   private:
    int fd_;

    uint32_t events_;

    EventCallBack cb_;
};

#endif