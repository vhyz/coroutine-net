# coroutine-net

## Introduction

coroutine-net是一个用C++编写的基于协程的简易网络库，能够像写同步的网络程序一样写异步的网络程序

## Feature

网络库由两个模块构成，分别为协程模块与网络模块

### 协程模块

* 实现了Linux x86_64平台下的上下文切换汇编，实现了resume,yield等协程操作
* 使用私有栈实现，每个协程默认具有128K栈
* 可链式创建协程，如A->B->C，并且每个协程都可resume任意另外一个协程

### 网络模块

* 事件循环EventLoop用epoll的LT模式实现
* 实现了CoRead,CoWrite,CoConnect,CoAccept等函数，调用它们时如遇到IO阻塞会yield切换到其他的协程

## Usage

### 协程模块

* void CoroutineEnvInit(size_t)         
初始化协程模块资源
* void CoroutineEnvDestory()            
释放协程模块资源
* int CoroutineCreate(CoroutineCallBack)        
创建一个协程，返回一个int类型的协程id
* void CoroutineResume(int)     
传入一个协程id，切换到该协程
* int CoroutineGo(CoroutineCallBack)       
创建一个协程并切换至该协程，实际上是coroutine_create和coroutine_resume的组合
* void CoroutineYield()   
切换到上一个协程
* int CoroutineRunning()    
当前运行的协程id

``` C++
#include <stdio.h>
#include "coroutine.h"

int main() {
    CoroutineEnvInit(0);
    int co1 = CoroutineCreate([]() {
        printf("A\n");
        CoroutineYield();
        printf("C\n");
        CoroutineYield();
        printf("E\n");
    });
    int co2 = CoroutineCreate([]() {
        printf("B\n");
        CoroutineYield();
        printf("D\n");
        CoroutineYield();
        printf("F\n");
    });
    while (CoroutineStatus(co1) && CoroutineStatus(co2)) {
        CoroutineResume(co1);
        CoroutineResume(co2);
    }
    CoroutineEnvDestory();
    return 0;
}
```

### 网络模块

暂时只实现了read,write,connect,accept函数对应的co版本，下面是echo服务器与客户端的例子

echo 客户端：
``` C++
#include <unistd.h>
#include <iostream>
#include "coroutine.h"
#include "coroutine_net.h"

void ClientEcho() {
    struct sockaddr_in addr;
    SockaddrInInit(&addr, "127.0.0.1", 5000);

    int fd = CreateNonblockTcpSocket();

    int err = CoConnect(fd, (const struct sockaddr*)&addr, sizeof(addr));

    if (err) {
        close(fd);
        return;
    }

    for (;;) {
        char buf[1024];
        ssize_t n = CoRead(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        ssize_t n_write = CoWriteAll(fd, buf, n);

        if (n_write < n) {
            break;
        }
        ssize_t n_read = CoRead(fd, buf, n_write);
        if (n_read <= 0) {
            break;
        }
        fwrite(buf, sizeof(char), n_read, stdout);
    }

    close(fd);
}

int main() {
    CoroutineEnvInit(0);
    CoroutineNetInit();

    CoroutineGo(std::bind(ClientEcho));
    CoroutineNetRun();

    CoroutineNetDestory();
    CoroutineEnvDestory();
}
```

echo服务器：
``` C++
#include <unistd.h>
#include <iostream>
#include "coroutine.h"
#include "coroutine_net.h"

void Echo(int fd) {
    for (;;) {
        char buf[1024];
        ssize_t nread = CoRead(fd, buf, sizeof(buf));
        printf("CoRead %d bytes\n", nread);
        if (nread <= 0) {
            break;
        }

        ssize_t nwrite = CoWriteAll(fd, buf, nread);
        printf("CoWriteAll %d bytes\n", nwrite);
        if (nwrite < nread) {
            break;
        }
    }
    close(fd);
}

void Listener() {
    int fd = CreateListenrFd(5000);
    for (;;) {
        int client_fd = CoAccept(fd, NULL, NULL);
        if (client_fd >= 0) {
            printf("CoAccept client fd: %d\n", client_fd);
            CoroutineGo(std::bind(Echo, client_fd));
        }
    }
    close(fd);
}

int main() {
    CoroutineEnvInit(0);
    CoroutineNetInit();

    CoroutineGo(std::bind(Listener));

    CoroutineNetRun();

    CoroutineNetDestory();
    CoroutineEnvDestory();
}
```