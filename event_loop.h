#ifndef VHYZ_EVENT_LOOP_H
#define VHYZ_EVENT_LOOP_H

#include <sys/epoll.h>
#include <array>
#include <vector>
#include "event.h"

class EventLoop {
   public:
    EventLoop();

    ~EventLoop();

    void addEvent(Event* event);

    void updateEvent(Event* event);

    void removeEvent(Event* event);

    void startLoop(int timeout = -1);

   private:
    int epollFd_;

    static constexpr size_t MAX_EPOLL_EVENT_NUM = 1024;

    std::array<epoll_event, MAX_EPOLL_EVENT_NUM> eventArray_;
};

#endif