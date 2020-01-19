#include "event_loop.h"
#include <unistd.h>

EventLoop::EventLoop() {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    num_event_ = 0;
}

EventLoop::~EventLoop() { close(epoll_fd_); }

void EventLoop::AddEvent(Event* event) {
    epoll_event e;
    e.data.ptr = event;
    e.events = event->GetEvents();
    int fd = event->GetFd();

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &e) == 0) {
        num_event_++;
    }
}

void EventLoop::UpdateEvent(Event* event) {
    epoll_event e;
    e.data.ptr = event;
    e.events = event->GetEvents();
    int fd = event->GetFd();

    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &e);
}

void EventLoop::RemoveEvent(Event* event) {
    int fd = event->GetFd();

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == 0) {
        num_event_--;
    }
}

void EventLoop::StartLoop(int timeout) {
    while (num_event_ > 0) {
        int num = epoll_wait(epoll_fd_, event_array_.data(),
                             event_array_.size(), timeout);

        if (num < 0) {
            continue;
        }

        for (int i = 0; i < num; ++i) {
            Event* event = static_cast<Event*>(event_array_[i].data.ptr);
            uint32_t events = event_array_[i].events;

            event->HandleEvent(events);
        }
    }
}