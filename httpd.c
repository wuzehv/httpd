#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "my_socket.h"

#define PORT 80
#define LISTEN_QUEUE_LEN 10

int main(){
  int listener_d = create_socket();
  bind_port(listener_d, PORT);
  listen_queue(listener_d, LISTEN_QUEUE_LEN);

  char buf[500];
  char html[50];
  char query_string[1024];

  while(1){
    int connect_d = accept_connect(listener_d);

    memset(buf, 0, sizeof(buf));

    // 获取请求头
    recv(connect_d, buf, sizeof(buf), 0);

    // 解析请求头
    parse_header(buf, html, query_string);

    // 默认请求index.html
    if(strcmp(html, "/") == 0)
      strcpy(html, "/index.html");

    printf("%s, %s\n", html, query_string);

    // 发送html
    send_html(connect_d, html);

    close(connect_d);
  }

  return 0;
}
