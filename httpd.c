#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "error.h"
#include "my_socket.h"
#include "rio.h"

#define MAX_EVENTS 1024 

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "port not exists\n");
        exit(1);
    }

    struct sockaddr_in clientaddr;
    unsigned int clientlen, connfd, epollfd, nfds;
    char *haddr;

    int port = atoi(argv[1]);
    int listen_queue_len = MAX_EVENTS;

    int listener_d = open_listenfd(port, listen_queue_len);

    struct epoll_event ev, events[MAX_EVENTS];
    epollfd = epoll_create1(0);
    if (epollfd == -1)
        error("epoll create error");

    ev.events = EPOLLIN;
    ev.data.fd = listener_d;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener_d, &ev) == -1)
        error("epoll ctl error");

    while(1){
        clientlen = sizeof(clientaddr);
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
            error("epoll wait error");

        for(int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listener_d) {
                // 返回已连接描述符
                connfd = accept4(listener_d, (SA *)&clientaddr, &clientlen, SOCK_NONBLOCK);
                // 转换成本机表示的ip
                haddr = inet_ntoa(clientaddr.sin_addr);
                printf("server connected to %s\n\n", haddr);
                if (connfd == -1)
                    error("server connect error");

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                    error("epoll_ctl: conn_sock");
            } else {
                deal_reques(events[n].data.fd);
                close(events[n].data.fd);
            }
        }
    }

    close(listener_d);

    return 0;
}
