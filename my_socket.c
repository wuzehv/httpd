#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "error.h"
#include "my_socket.h"
#include <unistd.h>
#include "fastcgi.h"

// 创建socket
int create_socket(){
  int listener_d = socket(PF_INET, SOCK_STREAM, 0);
  if(listener_d == -1)
    error("can't create socket");

  return listener_d;
}

// 绑定端口
void bind_port(int socket, int port){
  struct sockaddr_in name;
  name.sin_family = PF_INET;
  name.sin_port = (in_port_t)htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  // 重用端口
  int reuse = 1;
  if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1)
    error("can't reuse port");

  int c = bind(socket, (struct sockaddr *)&name, sizeof(name));
  if(c == -1)
    error("can't bind port");
}

// 创建监听队列, 限制同时请求连接的个数
void listen_queue(int socket, int len){
  if(listen(socket, len) == -1)
    error("can't listen port");
}

// 接受客户端连接, 返回客户端套接字描述符
int accept_connect(int socket){
  struct sockaddr_storage client_addr;
  unsigned int address_size = sizeof(client_addr);
  int connect_d = accept(socket, (struct sockaddr *)&client_addr, &address_size);
  if(connect_d == -1)
    error("can't connect socket");

  return connect_d;
}

// 解析html，获取请求的html文件与查询字符串
// "GET /index.html?id=1 HTTP/1.1"
void parse_header(char *msg, char *html, char *query_string){
  char *delimiter = " ";
  strtok(msg, delimiter);
  // 临时存储index.html?id=1或index.html
  char *tmp_file = strtok(NULL, delimiter);
  if(strchr(tmp_file, '?')){
    strcpy(html, strtok(tmp_file, "?"));
    strcpy(query_string, strtok(NULL, "?"));
  } else {
    strcpy(html, tmp_file);
    strcpy(query_string, "");
  }
}

// 发送请求的html到浏览器
int send_html(int connect_d, char *file){
  char root[100] = "/home/wuzehui/Documents/httpd/web/";
  char *html_file = strcat(root, file);
  char html_response[1024], buf[1024];
  memset(buf, 0, sizeof(buf));
  memset(html_response, 0, sizeof(html_response));

  // 响应状态码
  int status = 200;

  FILE *html;
  if(strcmp(file, "/favicon.ico") == 0 || !(html = fopen(html_file, "r"))){
    fprintf(stderr, "%s not found\n", html_file);
    status = 404;
  } else if(strstr(file, "php")){
    // 解析php
    parsePhp(connect_d, buf);
  } else{
    fread(buf, sizeof(int), sizeof(buf), html);
  }

  response(html_response, status, buf);
  send_msg(connect_d, html_response);

  if(html)
    fclose(html);

  return 0;
}

// 返回响应头
void response(char *html_response, int status, const char *buf){
  char *status_info = "";
  if(status == 200)
    status_info = "OK";
  else if(status == 404)
    status_info = "Not Found";

  char *response_header = "HTTP/1.1 %d %s\r\nContent-Type: text/html;charset=utf-8\r\nContent-Length: %d\r\n\r\n%s";
  sprintf(html_response, response_header, status, status_info, strlen(buf), buf);
}

// 发消息
void send_msg(int connect_d, char *msg){
  if(send(connect_d, msg, strlen(msg), 0) == -1)
    fprintf(stderr, "%s: %s\n", "can't send message", strerror(errno));
}
