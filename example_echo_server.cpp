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