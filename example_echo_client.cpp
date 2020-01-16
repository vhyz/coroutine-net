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
        close(fd);
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