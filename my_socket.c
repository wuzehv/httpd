#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "error.h"
#include "my_socket.h"
#include "fastcgi.h"
#include "rio.h"

void read_requests(rio_t *rp);

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

void dealReques(int fd){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], line[MAXLINE], cgiargs[MAXLINE], filename[MAXLINE];
    struct stat sbuf;
    bzero(buf, sizeof(buf));
    recv(fd, buf, MAXLINE, 0);
    int i;
    for(i = 0; i < MAXLINE; i++) {
        if (buf[i] == '\n')
            break;
        line[i] = buf[i];
    }
    line[i] = '\0';
    sscanf(line, "%s %s %s", method, uri, version);

    // 解析请求头
    char *ptr;
    ptr = index(uri, '?');
    if (ptr) {
        strcpy(cgiargs, ptr+1);
        *ptr = '\0';
    }

    printf("method:%s, uri:%s, version:%s\n", method, uri, version);

    // 请求方法仅支持get和post
    if(strcasecmp(method, "GET") && strcasecmp(method, "POST")){
        clienterror(fd, method, "501", "Not Implemented",
                    "httpd does not implement this method");
        return ;
    }
  
    // 判断是静态资源还是动态资源
    int is_static = 0;
    strcpy(filename, "/home/luorui/Documents/httpd/web/index.php");
    /* is_static = parse_uri(uri, filename); */

    // 验证文件是否存在
    if(stat(filename, &sbuf) < 0) {
        clienterror(fd, method, "404", "Not found",
                    "file not found");
        return ;
    }

    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
        clienterror(fd, method, "403", "Forbiddend",
                    "httpd can't read the file");
        return ;
    }

    if(is_static){
        server_static(fd, filename, sbuf.st_size);
    } else {
        parse_php(fd, filename);
    }
}

void clienterror(int fd, char *method, char *statusnum,
                 char *shortmsg, char *longmsg){
    char buf[MAXLINE], body[MAXLINE];

    sprintf(body, "<html><title>httpd error</title>");
    sprintf(body, "%s<body>\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, statusnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p></body></html>\r\n", body, longmsg, method);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", statusnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
}

// 读取所有的请求头，当前还没有用到请求头信息
void read_requests(rio_t *rp){
    char buf[MAXLINE];

    rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
        bzero(buf, sizeof(buf));
        rio_readlineb(rp, buf, MAXLINE);
    }
}

// 解析uri
int parse_uri(char *uri, char *filename){
    int flag = 1;
    strcpy(filename, strcat("/home/wuzehui/Documents/httpd/web", uri));
    if(uri[strlen(uri)-1] == '/'){
        strcpy(filename, strcat(filename, "index.php"));

        if(strstr(filename, ".php")) {
            flag = 0;
        }
    } else if(strstr(uri, ".php")){
        flag = 0;
    }

    return flag;
}

void server_static(int fd, char *filename, int filesize){
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];

    get_filetype(filename, filetype);

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: %s\r\n", buf, "httpd");
    sprintf(buf, "%sContent-type: %s\r\n", buf, filetype);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, filesize);
    rio_writen(fd, buf, strlen(buf));

    srcfd = open(filename, O_RDONLY, 0);
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype){
    if(strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if(strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if(strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if(strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if(strstr(filename, ".css"))
        strcpy(filetype, "text/css");
}
