#include <arpa/inet.h>
#include <coroutine_net.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

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

int main(int argc, char* argv[]) {
    if (argc != 7) {
        fprintf(stderr,
                "Uasge: <ip> <port> <thread_num> <buf_size> <timeout> "
                "<client_count> \n");
        exit(127);
    }

    const char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    int thread_num = atoi(argv[3]);
    int buf_size = atoi(argv[4]);
    int timeout = atoi(argv[5]);
    int client_count = atoi(argv[6]);

    int64_t bytes_count = 0;
    EventLoop::GetThreadInstance().GetTimerWheel().AddTimerWithCallBack(
        timeout * 1000, [&bytes_count, timeout]() {
            std::cout << static_cast<double>(bytes_count) /
                             (timeout * 1024 * 1024)
                      << "MB/s" << std::endl;
            exit(0);
        });
    Coroutine::InitCoroutineEnv();
    for (int i = 0; i < client_count; ++i) {
        Coroutine::Go([ip, port, buf_size, &bytes_count]() {
            sockaddr_in addr;
            SockaddrInInit(&addr, ip, port);
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            int err = connect(fd, (const sockaddr*)&addr, sizeof(addr));
            if (err) {
                fprintf(stderr, "connect error, errno msg:%s", strerror(errno));
                close(fd);
                return;
            }
            std::vector<char> buf(buf_size);
            int n = buf_size;
            for (;;) {
                write(fd, buf.data(), n);
                n = read(fd, buf.data(), buf.size());
                bytes_count += n;
            }
        });
    }

    EventLoop::GetThreadInstance().StartLoop();
}