#ifndef VHYZ_COROUTINE_H
#define VHYZ_COROUTINE_H

#include <functional>

// 运行结束
#define COROUTINE_DEAD 0
// 等待运行
#define COROUTINE_SUSPEND 1
// 正在运行
#define COROUTINE_RUNNING 2
// resume了其他协程的协程
#define COROUTINE_RESUME_OTHER 3

void coroutine_env_init(size_t stsize);
void coroutine_env_destory();

using CoroutineCallBack = std::function<void()>;

int coroutine_create(const CoroutineCallBack& cb);
void coroutine_resume(int);
int coroutine_go(const CoroutineCallBack& cb);
void coroutine_yield();
int coroutine_running();
int coroutine_status(int);

#endif