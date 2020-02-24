#pragma once

#include <sys/epoll.h>
#include <array>
#include <vector>
#include "timer_wheel.h"

struct SocketContext {
    bool user_nonblock;
    int read_timeout;
    int write_timeout;
    Coroutine* read_co;
    Coroutine* write_co;
    uint32_t events;
    uint32_t revents;
    int poll_group_fd;

    SocketContext()
        : user_nonblock(false),
          read_timeout(-1),
          write_timeout(-1),
          read_co(nullptr),
          write_co(nullptr),
          events(0),
          revents(0),
          poll_group_fd(-1) {}
};

void SetSocketReadTimeout(int fd, int timeout);

void SetSokcetWriteTimeout(int fd, int timeout);

void SetSocketConnectTimeout(int fd, int timeout);

class EventLoop {
   public:
    EventLoop();

    ~EventLoop();

    void AddEvent(int fd, SocketContext* ctx);

    void UpdateEvent(int fd, SocketContext* ctx);

    void RemoveEvent(int fd, SocketContext* ctx);

    void StartLoop();

    void Suspend(int timeout);

    TimerWheel& GetTimerWheel() {
        return timer_wheel_;
    }

    static EventLoop& GetThreadInstance() {
        static thread_local EventLoop loop;
        return loop;
    }

   private:
    int epoll_fd_;

    static constexpr size_t kMaxEpollEventNum = 1024;

    std::array<epoll_event, kMaxEpollEventNum> event_array_;

    size_t num_event_;

    TimerWheel timer_wheel_;
};
