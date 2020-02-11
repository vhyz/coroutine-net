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
    Coroutine::InitCoroutineEnv();
    CoroutineNetInit();

    Coroutine::Go(std::bind(ClientEcho));
    CoroutineNetRun();

    CoroutineNetDestory();
}