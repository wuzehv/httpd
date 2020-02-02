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
#include "error.h"
#include "my_socket.h"
#include "rio.h"

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "port not exists\n");
        exit(1);
    }

    struct sockaddr_in clientaddr;
    unsigned int clientlen, connfd;
    char *haddr;

    int port = atoi(argv[1]);
    int listen_queue_len = 1024;

    int listener_d = open_listenfd(port, listen_queue_len);

    while(1){
        clientlen = sizeof(clientaddr);
        // 返回已连接描述符
        connfd = accept(listener_d, (SA *)&clientaddr, &clientlen);

        // 转换成本机表示的ip
        haddr = inet_ntoa(clientaddr.sin_addr);
        printf("server connected to %s\n\n", haddr);

        pid_t pid = fork();

        // 进入子进程
        if(!pid){
            // 关闭主进程套接字
            close(listener_d);

            deal_reques(connfd);

            // 关闭通讯套接字
            close(connfd);
            // 退出子进程
            exit(0);
        }

        wait(NULL);

        close(connfd);
    }

    close(listener_d);

    return 0;
}
