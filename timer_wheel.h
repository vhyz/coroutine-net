#pragma once

#include <functional>
#include <list>
#include <memory>
#include <vector>
#include "coroutine.h"

using TimerCallBack = std::function<void()>;

class Timer {
    friend class TimerWheel;

   public:
    Timer() : co(nullptr), running(false) {}

   private:
    Coroutine* co;
    TimerCallBack cb;
    int index;
    std::list<std::shared_ptr<Timer>>::iterator it;
    bool running;

    void Handle();
};

class TimerWheel {
   public:
    TimerWheel();

    ~TimerWheel();

    std::weak_ptr<Timer> AddTimerWithCoroutine(int timeout_ms, Coroutine* co);

    std::weak_ptr<Timer> AddTimerWithCallBack(int timeout_ms,
                                              const TimerCallBack& cb);

    void RemoveTimer(std::weak_ptr<Timer> timer);

    void Update();

    size_t GetTimerCount() const {
        return timer_count_;
    }

   private:
    std::weak_ptr<Timer> AddTimer(std::shared_ptr<Timer> timer, int timeout_ms);

    static constexpr size_t kTickSum = 60000;  // 1 tick = 1ms

    std::vector<std::list<std::shared_ptr<Timer>>> wheel_;

    int last_index_;

    int64_t last_timestamp_;

    size_t timer_count_;
};