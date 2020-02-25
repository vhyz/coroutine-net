#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include "event_loop.h"

#define HOOK_FUNC_TYPE(func_name) func_name##_func_t

#define SYSTEM_FUNC(func_name) g_system_func_##func_name

#define HOOK_FUNC_INIT(func_name)                           \
    typedef decltype(func_name) *HOOK_FUNC_TYPE(func_name); \
    static HOOK_FUNC_TYPE(func_name) SYSTEM_FUNC(func_name);

#define HOOK_FUNC(func_name)                                         \
    if (!SYSTEM_FUNC(func_name)) {                                   \
        SYSTEM_FUNC(func_name) =                                     \
            (HOOK_FUNC_TYPE(func_name))dlsym(RTLD_NEXT, #func_name); \
    }

HOOK_FUNC_INIT(socket);
HOOK_FUNC_INIT(accept);
HOOK_FUNC_INIT(close);
HOOK_FUNC_INIT(connect);
HOOK_FUNC_INIT(read);
HOOK_FUNC_INIT(write);
HOOK_FUNC_INIT(send);
HOOK_FUNC_INIT(sendto);
HOOK_FUNC_INIT(sendmsg);
HOOK_FUNC_INIT(recv);
HOOK_FUNC_INIT(recvfrom);
HOOK_FUNC_INIT(recvmsg);
HOOK_FUNC_INIT(poll);
HOOK_FUNC_INIT(select);
HOOK_FUNC_INIT(fcntl);
HOOK_FUNC_INIT(setsockopt);

std::unordered_map<int, std::unique_ptr<SocketContext>> g_socket_context_map;

void NewSocketContext(int fd) {
    g_socket_context_map[fd] =
        std::unique_ptr<SocketContext>(new SocketContext());
}

SocketContext *GetSocketContext(int fd) {
    auto it = g_socket_context_map.find(fd);
    if (it == g_socket_context_map.end()) {
        return nullptr;
    } else {
        return it->second.get();
    }
}

void DeleteSocketContext(int fd) {
    g_socket_context_map.erase(fd);
}

void SetSocketReadTimeout(int fd, int timeout) {
    SocketContext *ctx = GetSocketContext(fd);
    if (ctx) {
        ctx->read_timeout = timeout;
    }
}

void SetSokcetWriteTimeout(int fd, int timeout) {
    SocketContext *ctx = GetSocketContext(fd);
    if (ctx) {
        ctx->write_timeout = timeout;
    }
}

void SetSocketConnectTimeout(int fd, int timeout) {
    SocketContext *ctx = GetSocketContext(fd);
    if (ctx) {
        ctx->write_timeout = timeout;
    }
}

void WaitUntilEvent(int fd, SocketContext *ctx, uint32_t events) {
    int old_evnets = ctx->events;
    ctx->events |= events;
    int timeout;
    Coroutine *co = Coroutine::GetRunningCoroutine();
    if (events == EPOLLIN) {
        timeout = ctx->read_timeout;
        ctx->read_co = co;
    } else {
        timeout = ctx->write_timeout;
        ctx->write_co = co;
    }

    if (old_evnets) {
        // 已存在事件，更新
        EventLoop::GetThreadInstance().UpdateEvent(fd, ctx);
    } else {
        EventLoop::GetThreadInstance().AddEvent(fd, ctx);
    }

    EventLoop::GetThreadInstance().Suspend(timeout);
    ctx->events &= ~events;
    if (ctx->events == 0) {
        // 无事件
        EventLoop::GetThreadInstance().RemoveEvent(fd, ctx);
    } else {
        // 更新事件
        EventLoop::GetThreadInstance().UpdateEvent(fd, ctx);
    }
}

void SetSocketNonblock(int fd) {
    HOOK_FUNC(fcntl);
    int flags = SYSTEM_FUNC(fcntl)(fd, F_GETFL);
    SYSTEM_FUNC(fcntl)(fd, F_SETFL, flags | O_NONBLOCK);
}

int socket(int domain, int type, int protocol) {
    HOOK_FUNC(socket);

    int fd = SYSTEM_FUNC(socket)(domain, type, protocol);
    NewSocketContext(fd);
    SetSocketNonblock(fd);
    return fd;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    HOOK_FUNC(accept);
    SocketContext *ctx = GetSocketContext(sockfd);

    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(accept)(sockfd, addr, addrlen);
    }

    WaitUntilEvent(sockfd, ctx, EPOLLIN);

    int fd = SYSTEM_FUNC(accept)(sockfd, addr, addrlen);
    if (fd < 0) {
        return -1;
    }
    SocketContext *client_ctx = GetSocketContext(fd);
    if (client_ctx == nullptr) {
        NewSocketContext(fd);
        SetSocketNonblock(fd);
    }

    return fd;
}

int close(int fd) {
    HOOK_FUNC(close);
    DeleteSocketContext(fd);

    return SYSTEM_FUNC(close)(fd);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    HOOK_FUNC(connect);

    int err = SYSTEM_FUNC(connect)(sockfd, addr, addrlen);
    if (!err || errno != EINPROGRESS) {
        return err;
    }
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(connect)(sockfd, addr, addrlen);
    }
    WaitUntilEvent(sockfd, ctx, EPOLLOUT);
    socklen_t len = sizeof(err);
    int ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (ret < 0) {
        return -1;
    }
    if (err) {
        ret = -1;
        errno = err;
    }
    return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
    HOOK_FUNC(read);
    SocketContext *ctx = GetSocketContext(fd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(read)(fd, buf, count);
    }
    WaitUntilEvent(fd, ctx, EPOLLIN);

    return SYSTEM_FUNC(read)(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    HOOK_FUNC(write);
    SocketContext *ctx = GetSocketContext(fd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(write)(fd, buf, count);
    }

    ssize_t nleft = count;
    const char *ptr = static_cast<const char *>(buf);
    ssize_t nwrite = SYSTEM_FUNC(write)(fd, ptr, count);
    if (nwrite > 0) {
        nleft -= nwrite;
        ptr += nwrite;
    }
    while (nleft > 0) {
        WaitUntilEvent(fd, ctx, EPOLLOUT);
        nwrite = SYSTEM_FUNC(write)(fd, ptr, nleft);
        if (nwrite <= 0) {
            break;
        }
        ptr += nwrite;
        nleft -= nwrite;
    }
    if (nwrite <= 0 && nleft == count) {
        return nwrite;
    }
    return count - nleft;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    HOOK_FUNC(send);
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(send)(sockfd, buf, len, flags);
    }
    ssize_t nleft = len;
    const char *ptr = static_cast<const char *>(buf);
    ssize_t nwrite = SYSTEM_FUNC(send)(sockfd, ptr, len, flags);
    if (nwrite > 0) {
        nleft -= nwrite;
        ptr += nwrite;
    }
    while (nleft > 0) {
        WaitUntilEvent(sockfd, ctx, EPOLLOUT);
        nwrite = SYSTEM_FUNC(send)(sockfd, ptr, nleft, flags);
        if (nwrite <= 0) {
            break;
        }
        ptr += nwrite;
        nleft -= nwrite;
    }
    if (nwrite <= 0 && nleft == len) {
        return nwrite;
    }
    return len - nleft;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
    HOOK_FUNC(sendto);
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(sendto)(sockfd, buf, len, flags, dest_addr, addrlen);
    }
    WaitUntilEvent(sockfd, ctx, EPOLLOUT);
    return SYSTEM_FUNC(sendto)(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    HOOK_FUNC(sendmsg);
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(sendmsg)(sockfd, msg, flags);
    }
    WaitUntilEvent(sockfd, ctx, EPOLLOUT);
    return SYSTEM_FUNC(sendmsg)(sockfd, msg, flags);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    HOOK_FUNC(recv);
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(recv)(sockfd, buf, len, flags);
    }
    WaitUntilEvent(sockfd, ctx, EPOLLIN);
    return SYSTEM_FUNC(recv)(sockfd, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    HOOK_FUNC(recvfrom);
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(recvfrom)(sockfd, buf, len, flags, src_addr,
                                     addrlen);
    }
    WaitUntilEvent(sockfd, ctx, EPOLLIN);
    return SYSTEM_FUNC(recvfrom)(sockfd, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    HOOK_FUNC(recvmsg);
    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx == nullptr || ctx->user_nonblock) {
        return SYSTEM_FUNC(recvmsg)(sockfd, msg, flags);
    }
    WaitUntilEvent(sockfd, ctx, EPOLLIN);
    return SYSTEM_FUNC(recvmsg)(sockfd, msg, flags);
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    HOOK_FUNC(poll);
    if (timeout == 0) {
        return SYSTEM_FUNC(poll)(fds, nfds, timeout);
    }

    std::unordered_map<int, std::unique_ptr<SocketContext>> tmp_socket_map;
    Coroutine *co = Coroutine::GetRunningCoroutine();
    for (int i = 0; i < nfds; ++i) {
        if (fds[i].fd < 0) {
            continue;
        }
        SocketContext *ctx = GetSocketContext(fds[i].fd);
        if (ctx == nullptr) {
            std::unique_ptr<SocketContext> c(new SocketContext());
            ctx = c.get();
            tmp_socket_map[fds->fd] = std::move(c);
        }
        ctx->events = fds->events;
        ctx->poll_group_fd = fds[0].fd;
        ctx->read_co = co;
        ctx->revents = 0;
        fds[i].revents = 0;

        EventLoop::GetThreadInstance().AddEvent(fds->fd, ctx);
    }

    EventLoop::GetThreadInstance().Suspend(timeout);
    int ret = 0;
    for (int i = 0; i < nfds; ++i) {
        if (fds[i].fd < 0) {
            continue;
        }
        SocketContext *ctx = GetSocketContext(fds[i].fd);
        if (ctx == nullptr) {
            ctx = tmp_socket_map[fds[i].fd].get();
        }
        if (ctx->revents) {
            ret++;
            fds[i].revents = ctx->revents;
        }
        ctx->poll_group_fd = -1;
    }

    return ret;
}

// TODO
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout) {
    HOOK_FUNC(select);
    return SYSTEM_FUNC(select)(nfds, readfds, writefds, exceptfds, timeout);
}

int fcntl(int fd, int cmd, ...) {
    HOOK_FUNC(fcntl);
    va_list args;
    va_start(args, cmd);
    int ret = SYSTEM_FUNC(fcntl)(fd, cmd, args);
    va_end(args);

    if (cmd == F_SETFL) {
        SocketContext *ctx = GetSocketContext(fd);
        if (ctx != nullptr) {
            va_start(args, cmd);
            int flags = va_arg(args, int);
            if (flags & O_NONBLOCK) {
                ctx->user_nonblock = true;
            }
            va_end(args);
        }
    }

    return ret;
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
    HOOK_FUNC(setsockopt);

    SocketContext *ctx = GetSocketContext(sockfd);
    if (ctx && level == SOL_SOCKET) {
        if (optname == SO_SNDTIMEO) {
            const timeval *tv = (const timeval *)optval;
            int timeout = tv->tv_sec * 1000 + tv->tv_usec / 1000;
            ctx->write_timeout = timeout;
        } else if (optname == SO_RCVTIMEO) {
            const timeval *tv = (const timeval *)optval;
            int timeout = tv->tv_sec * 1000 + tv->tv_usec / 1000;
            ctx->read_timeout = timeout;
        }
    }

    return SYSTEM_FUNC(setsockopt)(sockfd, level, optname, optval, optlen);
}