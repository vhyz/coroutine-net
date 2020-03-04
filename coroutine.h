#pragma once

#include <functional>
#include <iostream>
#include "co_context.h"

// 运行结束
#define COROUTINE_DEAD 0
// 等待运行
#define COROUTINE_SUSPEND 1
// 正在运行
#define COROUTINE_RUNNING 2
// resume了其他协程的协程
#define COROUTINE_RESUME_OTHER 3

using CoroutineCallBack = std::function<void()>;

constexpr size_t kDefaultStackSize = 128 * 1024;

constexpr size_t kDefaultMaxFreeCoListSize = 1024;

// Only call Create() to create a Coroutine
class Coroutine {
    friend class Scheduler;

   public:
    static void InitCoroutineEnv(
        size_t stack_size = kDefaultStackSize,
        size_t max_free_co_list_size = kDefaultMaxFreeCoListSize);

    static Coroutine* Create(const CoroutineCallBack& cb);

    void Resume();

    static void Yield();

    int Status();

    static Coroutine* GetRunningCoroutine();

    static Coroutine* Go(const CoroutineCallBack& cb);

   private:
    CoroutineCallBack cb;
    Coroutine* pre_co;
    char* stack;
    co_context ctx;
    int status;

    Coroutine(size_t stack_size) {
        if (stack_size == 0) {
            stack = nullptr;
        } else {
            stack = static_cast<char*>(operator new(stack_size));
        }
    }

    ~Coroutine() {
        if (stack) {
            operator delete(stack);
        }
    }
};
