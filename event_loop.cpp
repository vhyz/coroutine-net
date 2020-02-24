#include "event_loop.h"
#include <unistd.h>
#include <unordered_map>

EventLoop::EventLoop() {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    num_event_ = 0;
}

EventLoop::~EventLoop() {
    close(epoll_fd_);
}

void EventLoop::AddEvent(int fd, SocketContext* ctx) {
    epoll_event event;
    event.data.ptr = ctx;
    event.events = ctx->events;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0) {
        num_event_++;
    }
}

void EventLoop::UpdateEvent(int fd, SocketContext* ctx) {
    epoll_event event;
    event.data.ptr = ctx;
    event.events = ctx->events;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

void EventLoop::RemoveEvent(int fd, SocketContext* ctx) {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == 0) {
        num_event_--;
    }
}

void EventLoop::StartLoop() {
    while (num_event_ > 0 || timer_wheel_.GetTimerCount() > 0) {
        int num =
            epoll_wait(epoll_fd_, event_array_.data(), event_array_.size(), 1);

        if (num < 0) {
            continue;
        }

        std::unordered_map<int, SocketContext*> poll_map;
        for (int i = 0; i < num; ++i) {
            SocketContext* ctx =
                static_cast<SocketContext*>(event_array_[i].data.ptr);
            uint32_t events = event_array_[i].events;
            if (ctx->poll_group_fd >= 0) {
                auto it = poll_map.find(ctx->poll_group_fd);
                if (it == poll_map.end()) {
                    poll_map[ctx->poll_group_fd] = ctx;
                }
                ctx->revents = events;
            } else {
                if (events & EPOLLIN) {
                    ctx->read_co->Resume();
                }
                if (events & EPOLLOUT) {
                    ctx->write_co->Resume();
                }
            }
        }
        for (auto it : poll_map) {
            it.second->read_co->Resume();
        }

        timer_wheel_.Update();
    }
}

void EventLoop::Suspend(int timeout) {
    std::weak_ptr<Timer> timer;
    if (timeout >= 0) {
        timer = timer_wheel_.AddTimerWithCoroutine(
            timeout, Coroutine::GetRunningCoroutine());
    }
    Coroutine::Yield();
    if (timeout >= 0) {
        timer_wheel_.RemoveTimer(timer);
    }
}