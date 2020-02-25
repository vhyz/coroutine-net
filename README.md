# coroutine-net

## Introduction

coroutine-net是一个用C++编写的基于协程的简易网络库，能够像写同步的网络程序一样写异步的网络程序

## Feature

网络库由两个模块构成，分别为协程模块与网络模块

### 协程模块

* 实现了Linux x86_64平台下的上下文切换汇编，实现了resume,yield等协程操作
* 使用私有栈实现，每个协程默认具有128K栈，利用操作系统的虚拟内存原理，实际上每个协程并不占用栈的全部物理内存空间
* 可链式创建协程，如A->B->C，并且每个协程都可resume任意另外一个协程
* 可设置执行完任务的空闲协程链表的最大数量，这样就不必频繁重复申请协程的栈空间

### 网络模块

* 事件循环EventLoop用epoll的LT模式实现
* 采用Linux的hook技术替换了系统的socket,read,write等函数，协程在遇到IO阻塞时会切换到另一个协程
* 内置一个单级时间轮定时器，可提供一分钟的定时，定时精度单位为ms

已hook的系统函数：socket, accept, close, connect, read, write, send, sendto, sendmsg, recv, recvfrom, recvmsg, poll, fcntl, setsockopt

### TODO

* 计划hook的系统函数：select, gethostbyname, gethostbyname_r, dup, dup2
* 定时器优化为多级时间轮的定时器，支持更长的时间定时

## Usage

### 协程模块

* static void InitCoroutineEnv(size_t stack_size = kDefaultStackSize,size_t max_free_co_list_size = kDefaultMaxFreeCoListSize);     
初始化协程模块资源，第一个参数为栈的大小，第二个参数为执行完任务的空闲协程链表的最大数量
* static Coroutine* Create(const CoroutineCallBack& cb)  
创建一个协程，返回协程指针
* void Resume() 
唤醒协程
* static Coroutine* Go(const CoroutineCallBack& cb);
创建一个协程并切换至该协程，实际上是Create和Resume的组合调用
* static void Yield() 
切换到上一个协程
* static Coroutine* GetRunningCoroutine();   
返回当前运行的协程

``` C++
#include <stdio.h>
#include "coroutine.h"

int main() {
    Coroutine::InitCoroutineEnv();
    Coroutine* co1 = Coroutine::Create([]() {
        printf("A\n");
        Coroutine::Yield();
        printf("C\n");
        Coroutine::Yield();
        printf("E\n");
    });
    Coroutine* co2 = Coroutine::Create([]() {
        printf("B\n");
        Coroutine::Yield();
        printf("D\n");
        Coroutine::Yield();
        printf("F\n");
    });
    while (co1->Status() && co2->Status()) {
        co1->Resume();
        co2->Resume();
    }
    return 0;
}
```

### 网络模块

hook了socket相关函数，可使用同步的方式编写异步的网络程序

echo 客户端：
``` C++
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "coroutine.h"
#include "event_loop.h"

int SockaddrInInit(struct sockaddr_in* addr, const char* ip, uint16_t port) {
    memset(addr, 0, sizeof(struct sockaddr_in));
    int err = inet_pton(AF_INET, ip, &addr->sin_addr);
    if (err < 0) {
        return -1;
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    return 0;
}

void ClientEcho() {
    struct sockaddr_in addr;
    SockaddrInInit(&addr, "127.0.0.1", 5000);

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int err = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));

    if (err) {
        close(fd);
        return;
    }

    for (;;) {
        char buf[1024];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        ssize_t n_write = write(fd, buf, n);
        if (n_write < n) {
            break;
        }
        ssize_t n_read = read(fd, buf, n_write);
        if (n_read <= 0) {
            break;
        }
        fwrite(buf, sizeof(char), n_read, stdout);
    }

    close(fd);
}

int main() {
    Coroutine::InitCoroutineEnv();

    Coroutine::Go(std::bind(ClientEcho));

    EventLoop::GetThreadInstance().StartLoop();
}
```

echo服务器：
``` C++
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include "coroutine.h"
#include "event_loop.h"
using namespace std;

void Echo(int fd) {
    for (;;) {
        char buf[1024];
        ssize_t nread = read(fd, buf, sizeof(buf));
        printf("read %d bytes\n", nread);
        if (nread <= 0) {
            break;
        }

        ssize_t nwrite = write(fd, buf, nread);
        printf("writeA %d bytes\n", nwrite);
        if (nwrite < nread) {
            break;
        }
    }
    close(fd);
}

int CreateListenrFd(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind error\n");
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        close(fd);
        fprintf(stderr, "listen error\n");
        return -1;
    }

    return fd;
}

void Listener() {
    int fd = CreateListenrFd(5000);
    for (;;) {
        int client_fd = accept(fd, NULL, NULL);
        if (client_fd >= 0) {
            printf("accept client fd: %d\n", client_fd);
            Coroutine::Go(std::bind(Echo, client_fd));
        }
    }
    close(fd);
}

int main() {
    Coroutine::InitCoroutineEnv();

    Coroutine::Go(std::bind(Listener));

    EventLoop::GetThreadInstance().StartLoop();
}
```