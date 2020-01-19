#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

ssize_t CoRead(int fd, void *buf, size_t count);
ssize_t CoWrite(int fd, const void *buf, size_t count);
ssize_t CoWriteAll(int fd, const void *buf, size_t count);
int CoAccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int CoConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

void CoroutineNetInit();
void CoroutineNetDestory();
void CoroutineNetRun();

int CreateNonblockTcpSocket();
void SetNonblock(int fd);

int CreateListenrFd(uint16_t port);
int SockaddrInInit(struct sockaddr_in *addr, const char *ip, uint16_t port);
