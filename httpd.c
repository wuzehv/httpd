#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "error.h"
#include "my_socket.h"

#define PORT 80
#define LISTEN_QUEUE_LEN 10

int main(){
  int listener_d = createSocket();
  bindPort(listener_d, PORT);
  listenQueue(listener_d, LISTEN_QUEUE_LEN);

  char buf[500];
  char html[50];
  char query_string[1024];

  while(1){
    int connect_d = acceptConnect(listener_d);

    pid_t pid = fork();

    // 进入子进程
    if(!pid){
      // 关闭主进程套接字
      close(listener_d);

      memset(buf, 0, sizeof(buf));

      // 获取请求头
      recv(connect_d, buf, sizeof(buf), 0);

      // 解析请求头
      parseHeader(buf, html, query_string);

      // 默认请求index.html
      if(strcmp(html, "/") == 0)
        strcpy(html, "/index.php");

      printf("%s, %s\n", html, query_string);

      // 发送html
      sendHtml(connect_d, html);

      // 关闭通讯套接字
      close(connect_d);
      // 退出子进程
      exit(0);
    }

    int pid_status;
    // 等待子进程结束，防止产生僵尸进程
    if(waitpid(pid, &pid_status, 0) == -1)
      error("can't wait process");

    close(connect_d);
  }

  close(listener_d);

  return 0;
}
