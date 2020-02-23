#pragma once

#include <sys/epoll.h>
#include <array>
#include <vector>
#include "event.h"

class EventLoop {
   public:
    EventLoop();

    ~EventLoop();

    void AddEvent(Event* event);

    void UpdateEvent(Event* event);

    void RemoveEvent(Event* event);

    void StartLoop();

   private:
    int epoll_fd_;

    static constexpr size_t kMaxEpollEventNum = 1024;

    std::array<epoll_event, kMaxEpollEventNum> event_array_;

    size_t num_event_;
};
