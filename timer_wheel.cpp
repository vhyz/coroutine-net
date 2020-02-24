#include "timer_wheel.h"
#include <sys/time.h>

static int64_t GetNowTimestamp() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

void Timer::Handle() {
    if (co) {
        co->Resume();
    } else {
        cb();
        cb = nullptr;
    }
}

TimerWheel::TimerWheel()
    : wheel_(kTickSum),
      last_index_(0),
      last_timestamp_(GetNowTimestamp()),
      timer_count_(0) {}

TimerWheel::~TimerWheel() {}

std::weak_ptr<Timer> TimerWheel::AddTimerWithCoroutine(int timeout_ms,
                                                       Coroutine* co) {
    auto timer = std::make_shared<Timer>();
    timer->co = co;
    return AddTimer(std::move(timer), timeout_ms);
}

std::weak_ptr<Timer> TimerWheel::AddTimerWithCallBack(int timeout_ms,
                                                      const TimerCallBack& cb) {
    auto timer = std::make_shared<Timer>();
    timer->cb = cb;
    return AddTimer(std::move(timer), timeout_ms);
}

void TimerWheel::RemoveTimer(std::weak_ptr<Timer> timer) {
    auto t = timer.lock();
    if (t && t->running == false) {
        timer_count_--;
        wheel_[t->index].erase(t->it);
    }
}

std::weak_ptr<Timer> TimerWheel::AddTimer(std::shared_ptr<Timer> timer,
                                          int timeout_ms) {
    timer_count_++;
    std::weak_ptr<Timer> res(timer);
    if (timeout_ms <= 0) {
        timeout_ms = 1;
    }
    int index =
        (GetNowTimestamp() + timeout_ms - last_timestamp_ + last_index_) %
        kTickSum;
    timer->index = index;
    wheel_[index].push_front(timer);
    timer->it = wheel_[index].begin();
    return res;
}

void TimerWheel::Update() {
    int64_t now = GetNowTimestamp();
    int distance = now - last_timestamp_;
    for (int i = 0; i < distance; ++i) {
        last_index_ = (last_index_ + 1) % kTickSum;
        ++last_timestamp_;
        timer_count_ -= wheel_[last_index_].size();
        for (auto timer : wheel_[last_index_]) {
            timer->running = true;
            timer->Handle();
        }
        wheel_[last_index_].clear();
    }
}