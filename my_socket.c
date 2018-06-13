#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "error.h"
#include "my_socket.h"
#include "fastcgi.h"
#include "common.h"

// 创建socket
int openListenfd(int port, int listenq){
  int listenfd, optval = 1;
  struct sockaddr_in serveraddr;

  if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    error("can't create socket");

  if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                (const void *)&optval, sizeof(int)) < 0)
    error("can't reuse port");

  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  // 转换为网络字节序列（大端法）,端口同理
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);

  if(bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
    error("can't bind port");

  // 将主动套接字转换为监听套接字
  if(listen(listenfd, listenq) < 0)
    error("can't listen port");

  return listenfd;
}

// 解析html，获取请求的html文件与查询字符串
// "GET /index.html?id=1 HTTP/1.1"
void parseHeader(char *msg, char *html, char *query_string){
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
int sendHtml(int connect_d, char *file){
  char root[100];
  strcpy(root, getConfig("root_path"));
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
    parsePhp(connect_d, html_file, buf, sizeof(buf));
  } else{
    fread(buf, sizeof(int), sizeof(buf), html);
  }

  response(html_response, status, buf);
  sendMsg(connect_d, html_response);

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

  char *response_header = "HTTP/1.1 %d %s\r\nContent-Type: image/jpeg;charset=utf-8\r\nContent-Length: %d\r\n\r\n%s";
  sprintf(html_response, response_header, status, status_info, strlen(buf), buf);
}

// 发消息
void sendMsg(int connect_d, char *msg){
  if(send(connect_d, msg, strlen(msg), 0) == -1)
    fprintf(stderr, "%s: %s\n", "can't send message", strerror(errno));
}
