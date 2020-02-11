#include "coroutine_net.h"
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <unistd.h>
#include <cstring>
#include "coroutine.h"
#include "event_loop.h"

static thread_local EventLoop* g_eventloop;

void CoroutineNetInit() {
    signal(SIGPIPE, SIG_IGN);
    g_eventloop = new EventLoop();
}

void CoroutineNetDestory() {
    delete g_eventloop;
}

void CoroutineNetRun() {
    g_eventloop->StartLoop();
}

ssize_t CoRead(int fd, void* buf, size_t count) {
    Coroutine* runningCo = Coroutine::GetRunningCoroutine();

    Event event(fd);
    event.EnableRead();
    event.SetEventCallBack([runningCo](uint32_t e) {
        if (e & Event::READ_EVENT) {
            runningCo->Resume();
        }
    });

    g_eventloop->AddEvent(&event);
    Coroutine::Yield();

    ssize_t n;
    for (;;) {
        n = read(fd, buf, count);
        if (n < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        } else {
            break;
        }
    }

    g_eventloop->RemoveEvent(&event);
    return n;
}

ssize_t CoWrite(int fd, const void* buf, size_t count) {
    Coroutine* runningCo = Coroutine::GetRunningCoroutine();

    Event event(fd);
    event.EnableWrite();
    event.SetEventCallBack([runningCo](uint32_t e) {
        if (e & Event::WRITE_EVENT) {
            runningCo->Resume();
        }
    });
    g_eventloop->AddEvent(&event);

    ssize_t n;
    Coroutine::Yield();
    for (;;) {
        n = write(fd, buf, count);
        if (n < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        } else {
            break;
        }
    }

    g_eventloop->RemoveEvent(&event);

    return -1;
}

ssize_t CoWriteAll(int fd, const void* buf, size_t count) {
    Coroutine* runningCo = Coroutine::GetRunningCoroutine();

    Event event(fd);
    event.EnableWrite();
    event.SetEventCallBack([runningCo](uint32_t e) {
        if (e & Event::WRITE_EVENT) {
            runningCo->Resume();
        }
    });
    g_eventloop->AddEvent(&event);

    ssize_t nleft = count;
    ssize_t nwrite = 0;
    const char* ptr = static_cast<const char*>(buf);
    while (nleft > 0) {
        Coroutine::Yield();
        nwrite = write(fd, ptr, count);
        if (nwrite < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        }
        nleft -= nwrite;
        ptr += nwrite;
    }

    g_eventloop->RemoveEvent(&event);

    return count - nleft;
}

int CoAccept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    Coroutine* runningCo = Coroutine::GetRunningCoroutine();

    Event event(sockfd);
    event.EnableRead();
    event.SetEventCallBack([runningCo](uint32_t e) {
        if (e & Event::READ_EVENT) {
            runningCo->Resume();
        }
    });

    g_eventloop->AddEvent(&event);
    Coroutine::Yield();

    int client_fd;
    for (;;) {
        client_fd = accept(sockfd, addr, addrlen);
        if (client_fd < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        } else {
            break;
        }
    }

    if (client_fd >= 0) {
        SetNonblock(client_fd);
    }

    g_eventloop->RemoveEvent(&event);
    return client_fd;
}

int CoConnect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    int err = connect(sockfd, addr, addrlen);
    if (!err) {
        return 0;
    }

    Coroutine* runningCo = Coroutine::GetRunningCoroutine();

    Event event(sockfd);
    event.EnableWrite();
    event.SetEventCallBack([runningCo](uint32_t e) {
        if (e & Event::WRITE_EVENT) {
            runningCo->Resume();
        }
    });
    g_eventloop->AddEvent(&event);

    Coroutine::Yield();
    socklen_t len = sizeof(err);
    int ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (ret) {
        ret = -1;
    }
    errno = err;

    if (err) {
        ret = -1;
    }

    g_eventloop->RemoveEvent(&event);

    return ret;
}

int CreateNonblockTcpSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    SetNonblock(fd);
    return fd;
}

void SetNonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int CreateListenrFd(uint16_t port) {
    int fd = CreateNonblockTcpSocket();

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    SetNonblock(fd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind error\n");
        return -1;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        fprintf(stderr, "listen error\n");
        return -1;
    }

    return fd;
}

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