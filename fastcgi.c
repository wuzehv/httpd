#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "my_socket.h"
#include "fastcgi.h"
#include "error.h"
#include "rio.h"

// 本文件主要用来处理httpd与php-fpm通讯

// 连接php-fpm进程
int connect_fpm(char *ip, int port){
    int sockfd;
    struct sockaddr_in server_address;
    struct in_addr iaddr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error("can't create socket");

    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    // 将ip转换为大端法表示数字
    inet_aton(ip, &iaddr);
    server_address.sin_addr = iaddr;
    server_address.sin_port = htons(port);

    if((connect(sockfd, (SA *)&server_address, sizeof(server_address))) < 0)
        error("can't listen port");

    return sockfd;
}

// 制作php-fpm所需的请求头
FCGI_Header make_header(int type, int requestId, int contentLength, int paddingLength){
    FCGI_Header header;
    header.version = FASTCGI_VERSION;
    header.type = (unsigned char) type;
    header.requestIdB0 = (unsigned char)(requestId & 0xFF);
    header.requestIdB1 = (unsigned char)((requestId >> 8) & 0xFF);
    header.contentLengthB0 = (unsigned char)(contentLength & 0xff);
    header.contentLengthB1 = (unsigned char)((contentLength >> 8) & 0xff);
    header.paddingLength = (unsigned char)paddingLength;
    header.reserved = 0;

    return header;
}

// 制作开始请求体
FCGI_BeginRequestBody make_begin_request_body(int role, int keepConn){
    FCGI_BeginRequestBody body;
    body.roleB1 = (unsigned char)((role >> 8) & 0xFF);
    body.roleB0 = (unsigned char)(role & 0xFF);
    body.flags = (unsigned char)((keepConn) ? FASTCGI_KEEP_CONN : 0);
    memset(body.reserved, 0, sizeof(body.reserved));

    return body;
}

// 发送请求消息体
int send_start_request_record(FCGI *c) {
    int rc;
    FCGI_BeginRequestRecord beginRecord;

    beginRecord.header = make_header(FASTCGI_TYPE_BEGIN, c->requestId, sizeof(beginRecord.body),0);
    beginRecord.body = make_begin_request_body(PHP_FPM_ROLE_COMMOM, 0);

    rc = write(c->sockfd, (char *)&beginRecord, sizeof(beginRecord));
    assert(rc == sizeof(beginRecord));

    return 1;
}

// 计算请求参数大小
int make_name_value_body(char *name, int nameLen,
                      char *value, int valueLen,
                      unsigned char *bodyBuffer, int *bodyLen){
    unsigned char *startBodyBuffer = bodyBuffer;
    unsigned int i;
    if(nameLen < 128){
        *bodyBuffer++ = (unsigned char)nameLen;
    } else {
        *bodyBuffer++ = (unsigned char)((nameLen >> 24)|0x80);
        *bodyBuffer++ = (unsigned char)(nameLen >> 16);
        *bodyBuffer++ = (unsigned char)(nameLen >> 8);
        *bodyBuffer++ = (unsigned char)nameLen;
    }

    if(valueLen < 128){
        *bodyBuffer++ = (unsigned char)valueLen;
    } else {
        *bodyBuffer++ = (unsigned char)((valueLen >> 24)|0x80);
        *bodyBuffer++ = (unsigned char)(valueLen >> 16);
        *bodyBuffer++ = (unsigned char)(valueLen >> 8);
        *bodyBuffer++ = (unsigned char)valueLen;
    }

    for(i = 0; i < strlen(name); i++){
        *bodyBuffer++ = name[i];
    }

    for(i = 0; i < strlen(value); i++){
        *bodyBuffer++ = value[i];
    }

    *bodyLen = bodyBuffer - startBodyBuffer;

    return 0;
}

// 发送请求参数
int send_params(FCGI *c, char *name, char *value){
    int bodyLen;
    unsigned char bodyBuffer[1024];
    make_name_value_body(name, strlen(name), value, strlen(value), bodyBuffer, &bodyLen);

    FCGI_Header header;
    header = make_header(FASTCGI_TYPE_ENV, c->requestId, bodyLen, 0);

    int nameValueLen = bodyLen + FASTCGI_HEADER_LEN;
    char nameValueRecord[nameValueLen];

    memcpy(nameValueRecord, (char *)&header, FASTCGI_HEADER_LEN);
    memcpy(nameValueRecord + FASTCGI_HEADER_LEN, bodyBuffer, bodyLen);

    int s = write(c->sockfd, nameValueRecord, nameValueLen);
    if(s == -1){
        fprintf(stderr, "error:%s\n", strerror(errno));
        return -1;
    }

    return 0;
}


// 发送结束消息
int send_end_record(FCGI *c){
    FCGI_Header header;
    header = make_header(FASTCGI_TYPE_END, c->requestId, FASTCGI_HEADER_LEN, 0);
    int s = write(c->sockfd, (char *) &header, sizeof(header));
    if(s == -1){
        fprintf(stderr, "error:%s\n", strerror(errno));
        return -1;
    }

    return 0;
}

// 从php-fpm输出读取响应头与html
void read_from_fpm(FCGI *c){
    FCGI_Header responseHeader;
    char content[MAXLINE], buf[MAXLINE];
    int contentLen;

    while(read(c->sockfd, &responseHeader, FASTCGI_HEADER_LEN) > 0){
        if(responseHeader.type == FASTCGI_TYPE_SUCC){
            contentLen = (responseHeader.contentLengthB1 << 8) + (responseHeader.contentLengthB0);
            memset(content, 0, contentLen);

            // 读取内容
            sprintf(buf, "HTTP/1.1 200 OK\r\n");
            sprintf(buf, "%sServer: %s\r\n", buf, "httpd");

            recv(c->sockfd, content, MAXLINE, 0);
            send(c->requestId, buf, strlen(buf), 0);
            send(c->requestId, content, strlen(content), 0);
            printf("fpm response -------------------------------------\n%s\n", content);
        } else if (responseHeader.type == FASTCGI_TYPE_ERROR){
        }
    }
}

// 发送数据,解析php,并返回
int parse_php(int requestId, char *html){
    FCGI init;
    memset(&init, 0, sizeof(init));
    FCGI* c = &init;

    c->requestId = requestId;
    c->sockfd = connect_fpm("127.0.0.1", 9000);

    if(c->sockfd == -1){
        return -1;
    }

    send_start_request_record(c);
    // 发送请求参数
    send_params(c, "SCRIPT_FILENAME", html);
    send_params(c, "REQUEST_METHOD", "GET");
    // 发送结束记录
    send_end_record(c);

    // 获取php-fpm输出
    read_from_fpm(c);

    close(c->sockfd);

    return 0;
}
