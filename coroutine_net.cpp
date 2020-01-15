#include "coroutine_net.h"
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cstring>
#include "coroutine.h"
#include "event_loop.h"

static thread_local EventLoop* gEventLoop;

void coroutine_net_init() { gEventLoop = new EventLoop(); }

void coroutine_net_destory() { delete gEventLoop; }

void coroutine_net_run() { gEventLoop->startLoop(); }

ssize_t co_read(int fd, void* buf, size_t count) {
    int runningCo = coroutine_running();

    Event event(fd);
    event.enableRead();
    event.setEventCallBack([runningCo](uint32_t e) {
        if (e & Event::READ_EVENT) {
            coroutine_resume(runningCo);
        }
    });

    gEventLoop->addEvent(&event);
    coroutine_yield();

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

    gEventLoop->removeEvent(&event);
    return n;
}

ssize_t co_write(int fd, const void* buf, size_t count) {
    int runningCo = coroutine_running();

    Event event(fd);
    event.enableWrite();
    event.setEventCallBack([runningCo](uint32_t e) {
        if (e & Event::WRITE_EVENT) {
            coroutine_resume(runningCo);
        }
    });
    gEventLoop->addEvent(&event);

    ssize_t n;
    coroutine_yield();
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

    gEventLoop->removeEvent(&event);

    return -1;
}

ssize_t co_write_all(int fd, const void* buf, size_t count) {
    int runningCo = coroutine_running();

    Event event(fd);
    event.enableWrite();
    event.setEventCallBack([runningCo](uint32_t e) {
        if (e & Event::WRITE_EVENT) {
            coroutine_resume(runningCo);
        }
    });
    gEventLoop->addEvent(&event);

    ssize_t nleft = count;
    ssize_t nwrite = 0;
    const char* ptr = static_cast<const char*>(buf);
    while (nleft > 0) {
        coroutine_yield();
        nwrite = write(fd, ptr, count);
        if (nwrite < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                break;
            }
        }
        nleft -= nwrite;
        ptr += nwrite;
    }

    gEventLoop->removeEvent(&event);

    return count - nleft;
}

int co_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int runningCo = coroutine_running();

    Event event(sockfd);
    event.enableRead();
    event.setEventCallBack([runningCo](uint32_t e) {
        if (e & Event::READ_EVENT) {
            coroutine_resume(runningCo);
        }
    });

    gEventLoop->addEvent(&event);
    coroutine_yield();

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
        set_nonblock(client_fd);
    }

    gEventLoop->removeEvent(&event);
    return client_fd;
}

int co_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    int err = connect(sockfd, addr, addrlen);
    if (!err) {
        return 0;
    }

    int runningCo = coroutine_running();

    Event event(sockfd);
    event.enableWrite();
    event.setEventCallBack([runningCo](uint32_t e) {
        if (e & Event::WRITE_EVENT) {
            coroutine_resume(runningCo);
        }
    });
    gEventLoop->addEvent(&event);

    coroutine_yield();
    socklen_t len = sizeof(err);
    int ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (ret) {
        ret = -1;
    }
    errno = err;

    if (err) {
        ret = -1;
    }

    gEventLoop->removeEvent(&event);

    return ret;
}

int create_nonblock_tcp_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblock(fd);
    return fd;
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_listenr_fd(uint16_t port) {
    int fd = create_nonblock_tcp_socket();

    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    set_nonblock(fd);

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

int sockaddr_in_init(struct sockaddr_in* addr, const char* ip, uint16_t port) {
    memset(addr, 0, sizeof(struct sockaddr_in));
    int err = inet_pton(AF_INET, ip, &addr->sin_addr);
    if (err < 0) {
        return -1;
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    return 0;
}