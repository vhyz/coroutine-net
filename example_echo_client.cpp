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