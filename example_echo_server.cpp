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