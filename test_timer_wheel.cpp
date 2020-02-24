#include <sys/epoll.h>
#include <sys/poll.h>
#include <unistd.h>
#include <iostream>
#include "event_loop.h"
#include "timer_wheel.h"

void print() {
    for (;;) {
        std::cout << "print" << std::endl;
        EventLoop::GetThreadInstance().Suspend(1000);
    }
}

int main() {
    Coroutine::InitCoroutineEnv();
    Coroutine::Go(print);

    EventLoop::GetThreadInstance().StartLoop();
}