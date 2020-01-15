#ifndef COROUTINE_NET_H
#define COROUTINE_NET_H

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

ssize_t co_read(int fd, void *buf, size_t count);
ssize_t co_write(int fd, const void *buf, size_t count);
ssize_t co_write_all(int fd, const void *buf, size_t count);
int co_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int co_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

void coroutine_net_init();
void coroutine_net_destory();
void coroutine_net_run();

int create_nonblock_tcp_socket();
void set_nonblock(int fd);

int create_listenr_fd(uint16_t port);
int sockaddr_in_init(struct sockaddr_in *addr, const char *ip, uint16_t port);

#endif