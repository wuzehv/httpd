#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef struct sockaddr SA;

#define LISTENQ 1024
#define MAXREAD 2048

// 错误输出
void error(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

// 打开一个客户端描述符
int open_clientfd(char *hostname, int port){
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("socket create error");

    if ((hp = gethostbyname(hostname)) == NULL)
        error("gethostbyname error");

    bzero((char *)&serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = port;
    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    
    if (connect(clientfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        error("client connect error");

    return clientfd;
}

// 返回一个监听套接字
int open_listenfd(int port){
    int listenfd, optval = -1;
    struct sockaddr_in serveraddr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("socket create error");

    // 取消30秒的客户端连接请求延迟
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
        error("setsocketopt error");

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        error("bind error");

    if (listen(listenfd, LISTENQ) < 0)
        error("listen error");

    return listenfd;
}

void doit(int connfd){
    char buf[MAXREAD], tmp[MAXREAD], response[MAXREAD];
    bzero(buf, sizeof(buf));
    bzero(response, sizeof(response));
    recv(connfd, buf, MAXREAD, 0);
    printf("%s\n%s\n\n%s\n\n", buf, "---request header print done---", "---please enter response header---");
    while(fgets(tmp, MAXREAD, stdin) != NULL){
        strcat(response, tmp);
    }
    send(connfd, response, strlen(response), 0);
}

int main(int argc, char **argv){
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    if (argc != 2) {
        fprintf(stderr, "port not exists\n");
        exit(1);
    }

    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        doit(connfd);
        close(connfd);
    }
    return 0;
}
