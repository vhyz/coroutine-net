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