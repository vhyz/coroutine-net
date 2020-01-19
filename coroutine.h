#pragma once

#include <functional>

// 运行结束
#define COROUTINE_DEAD 0
// 等待运行
#define COROUTINE_SUSPEND 1
// 正在运行
#define COROUTINE_RUNNING 2
// resume了其他协程的协程
#define COROUTINE_RESUME_OTHER 3

void CoroutineEnvInit(size_t stsize);
void CoroutineEnvDestory();

using CoroutineCallBack = std::function<void()>;

int CoroutineCreate(const CoroutineCallBack& cb);
void CoroutineResume(int);
int CoroutineGo(const CoroutineCallBack& cb);
void CoroutineYield();
int CoroutineRunning();
int CoroutineStatus(int);
