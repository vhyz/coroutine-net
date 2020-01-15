# coroutine-net

## Introduction

coroutine-net是一个用C++编写的基于协程的简易网络库，能够像写同步的网络程序一样写异步的网络程序

## Feature

网络库由两个模块构成，分别为协程模块与网络模块

### 协程模块

* 使用ucontext接口实现的有栈协程，实现了resume,yield等协程操作
* 使用私有栈实现，每个协程默认具有128K栈
* 可链式创建协程，如A->B->C，并且每个协程都可resume任意另外一个协程

### 网络模块

* 事件循环EventLoop用epoll的LT模式实现
* 实现了co_read,co_write,co_connect,co_accept等函数，调用它们时如遇到IO阻塞会yield切换到其他的协程

## Usage

### 协程模块

* void coroutine_env_init(size_t)         
初始化协程模块资源
* void coroutine_env_destory()            
释放协程模块资源
* int coroutine_create(CoroutineCallBack)        
创建一个协程，返回一个int类型的协程id
* void coroutine_resume(int)     
传入一个协程id，切换到该协程
* int coroutine_go(CoroutineCallBack)       
创建一个协程并切换至该协程，实际上是coroutine_create和coroutine_resume的组合
* void coroutine_yield()   
切换到上一个协程
* int coroutine_running()    
当前运行的协程id

``` C++
#include <stdio.h>
#include "coroutine.h"

int main() {
    coroutine_env_init(0);
    int co1 = coroutine_create([]() {
        printf("A\n");
        coroutine_yield();
        printf("C\n");
        coroutine_yield();
        printf("E\n");
    });
    int co2 = coroutine_create([]() {
        printf("B\n");
        coroutine_yield();
        printf("D\n");
        coroutine_yield();
        printf("F\n");
    });
    while (coroutine_status(co1) && coroutine_status(co2)) {
        coroutine_resume(co1);
        coroutine_resume(co2);
    }
    coroutine_env_destory();
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

using namespace std;

void client_echo() {
    struct sockaddr_in addr;
    sockaddr_in_init(&addr, "127.0.0.1", 5000);

    int fd = create_nonblock_tcp_socket();

    int err = co_connect(fd, (const struct sockaddr*)&addr, sizeof(addr));

    if (err) {
        return;
    }

    for (;;) {
        char buf[1024];
        ssize_t n = co_read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        ssize_t n_write = co_write_all(fd, buf, n);

        if (n_write < n) {
            break;
        }
        ssize_t n_read = co_read(fd, buf, n_write);
        if (n_read <= 0) {
            break;
        }
        fwrite(buf, sizeof(char), n_read, stdout);
    }

    close(fd);
}

int main() {
    coroutine_env_init(0);
    coroutine_net_init();

    coroutine_go(std::bind(client_echo));
    coroutine_net_run();

    coroutine_net_destory();
    coroutine_env_destory();
}
```

echo服务器：
``` C++
#include <unistd.h>
#include <iostream>
#include "coroutine.h"
#include "coroutine_net.h"

using namespace std;

void echo(int fd) {
    for (;;) {
        char buf[1024];
        ssize_t nread = co_read(fd, buf, sizeof(buf));
        printf("co_read %d bytes\n", nread);
        if (nread <= 0) {
            break;
        }

        ssize_t nwrite = co_write_all(fd, buf, nread);
        printf("co_write %d bytes\n", nwrite);
        if (nwrite < nread) {
            break;
        }
    }
    close(fd);
}

void listener() {
    int fd = create_listenr_fd(5000);
    for (;;) {
        int client_fd = co_accept(fd, NULL, NULL);
        if (client_fd >= 0) {
            printf("co_accept client fd: %d\n", client_fd);
            coroutine_go(std::bind(echo, client_fd));
        }
    }
}

int main() {
    coroutine_env_init(0);
    coroutine_net_init();

    coroutine_go(std::bind(listener));

    coroutine_net_run();

    coroutine_net_destory();
    coroutine_env_destory();
}
```