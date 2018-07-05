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
#include "common.h"
#include "rio.h"

int main(){
  struct sockaddr_in clientaddr;
  unsigned int clientlen, connfd;
  struct hostent *hp;
  char *haddr;

  int port = atoi(getConfig("listen"));
  int listen_queue_len = atoi(getConfig("listen_queue_len"));

  int listener_d = openListenfd(port, listen_queue_len);

  while(1){
    clientlen = sizeof(clientaddr);
    // 返回已连接描述符
    connfd = accept(listener_d, (SA *)&clientaddr, &clientlen);

    // 获取客户端地址信息
    hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                       sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    // 转换成本机表示的ip
    haddr = inet_ntoa(clientaddr.sin_addr);
    printf("server connected to %s (%s)\n", hp->h_name, haddr);

    pid_t pid = fork();

    // 进入子进程
    if(!pid){
      // 关闭主进程套接字
      close(listener_d);

      dealReques(connfd);

      // 关闭通讯套接字
      close(connfd);
      // 退出子进程
      exit(0);
    }

    int pid_status;
    // 等待子进程结束，防止产生僵尸进程
    if(waitpid(pid, &pid_status, 0) == -1)
      error("can't wait process");

    close(connfd);
  }

  close(listener_d);

  return 0;
}
