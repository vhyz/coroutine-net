#include "event_loop.h"
#include <unistd.h>

EventLoop::EventLoop() { epollFd_ = epoll_create1(EPOLL_CLOEXEC); }

EventLoop::~EventLoop() { close(epollFd_); }

void EventLoop::addEvent(Event* event) {
    epoll_event e;
    e.data.ptr = event;
    e.events = event->getEvents();
    int fd = event->getFd();

    epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &e);
}

void EventLoop::updateEvent(Event* event) {
    epoll_event e;
    e.data.ptr = event;
    e.events = event->getEvents();
    int fd = event->getFd();

    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &e);
}

void EventLoop::removeEvent(Event* event) {
    int fd = event->getFd();

    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EventLoop::startLoop(int timeout) {
    for (;;) {
        int num = epoll_wait(epollFd_, eventArray_.data(), eventArray_.size(),
                             timeout);

        if (num < 0) {
            continue;
        }

        for (int i = 0; i < num; ++i) {
            Event* event = static_cast<Event*>(eventArray_[i].data.ptr);
            uint32_t events = eventArray_[i].events;

            event->handleEvent(events);
        }
    }
}