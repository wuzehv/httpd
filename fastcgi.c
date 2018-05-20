#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "my_socket.h"
#include "fastcgi.h"

int connectFpm(char *ip, int port){
  int rc;
  int sockfd;
  struct sockaddr_in server_address;

  sockfd = create_socket();
  assert(sockfd > 0);

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = PF_INET;
  server_address.sin_addr.s_addr = inet_addr(ip);
  server_address.sin_port = htons(port);

  rc = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
  assert(rc >= 0);

  return sockfd;
}

FCGI_Header makeHeader(int type, int requestId, int contentLength, int paddingLength){
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

FCGI_BeginRequestBody makeBeginRequestBody(int role, int keepConn){
  FCGI_BeginRequestBody body;
  body.roleB1 = (unsigned char)((role >> 8) & 0xFF);
  body.roleB0 = (unsigned char)(role & 0xFF);
  body.flags = (unsigned char)((keepConn) ? FASTCGI_KEEP_CONN : 0);
  memset(body.reserved, 0, sizeof(body.reserved));

  return body;
}

int sendStartRequestRecord(FCGI *c) {
  int rc;
  FCGI_BeginRequestRecord beginRecord;

  beginRecord.header = makeHeader(FASTCGI_TYPE_BEGIN, c->requestId, sizeof(beginRecord.body),0);
  beginRecord.body = makeBeginRequestBody(PHP_FPM_ROLE_COMMOM, 0);

  rc = write(c->sockfd, (char *)&beginRecord, sizeof(beginRecord));
  assert(rc == sizeof(beginRecord));

  return 1;
}

int makeNameValueBody(char *name, int nameLen,
                      char *value, int valueLen,
                      unsigned char *bodyBuffer, int *bodyLen){
  unsigned char *startBodyBuffer = bodyBuffer;
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

  for(int i = 0; i < strlen(name); i++){
    *bodyBuffer++ = name[i];
  }

  for(int i = 0; i < strlen(value); i++){
    *bodyBuffer++ = value[i];
  }

  *bodyLen = bodyBuffer - startBodyBuffer;

  return 0;
}

int sendParams(FCGI *c, char *name, char *value){
  int bodyLen;
  unsigned char bodyBuffer[1024];
  makeNameValueBody(name, strlen(name), value, strlen(value), bodyBuffer, &bodyLen);

  FCGI_Header header;
  header = makeHeader(FASTCGI_TYPE_ENV, c->requestId, bodyLen, 0);

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


int sendEndRecord(FCGI *c){
  FCGI_Header header;
  header = makeHeader(FASTCGI_TYPE_END, c->requestId, FASTCGI_HEADER_LEN, 0);
  int s = write(c->sockfd, (char *) &header, sizeof(header));
  if(s == -1){
    fprintf(stderr, "error:%s\n", strerror(errno));
    return -1;
  }

  return 0;
}

void readFromFpm(FCGI *c, char *rinfo){
  FCGI_Header responseHeader;
  char content[1024];
  int contentLen;

  while(read(c->sockfd, &responseHeader, FASTCGI_HEADER_LEN) > 0){
    if(responseHeader.type == FASTCGI_TYPE_SUCC){
      contentLen = (responseHeader.contentLengthB1 << 8) + (responseHeader.contentLengthB0);
      memset(content, 0, 1024);

      // 读取内容
      int rc = read(c->sockfd, content, contentLen);
      if(rc != contentLen){
        fprintf(stderr, "error:%s\n", strerror(errno));
      }

      char *delimiter = "\r\n";

      strtok(content, delimiter);

      char *tmp;
      int i = 0;
      int lenstep = 0;
      memset(rinfo, 0, sizeof(rinfo));

      while(tmp = strtok(NULL, "\r\n")){
        if(i > 1){
          memcpy(rinfo + lenstep, tmp, strlen(tmp));
          lenstep += strlen(tmp);
        } else if(i > 0){
          memcpy(rinfo, tmp, strlen(tmp));
          lenstep = strlen(tmp);
        } else {

        }
        i++;
      }

      break;

    } else if (responseHeader.type == FASTCGI_TYPE_ERROR){
    }
  }
}

int parsePhp(int requestId, char *buf){
  // 连接php-fpm
  FCGI init;
  memset(&init, 0, sizeof(init));
  FCGI* c = &init;

  c->requestId = requestId;
  c->sockfd = connectFpm("127.0.0.1", 9000);

  if(c->sockfd == -1){
    return -1;
  }

  sendStartRequestRecord(c);
  // 发送请求参数
  sendParams(c, "SCRIPT_FILENAME", "/home/wuzehui/Documents/httpd/web/index.php");
  sendParams(c, "REQUEST_METHOD", "GET");
  // 发送结束记录
  sendEndRecord(c);

  // 获取php-fpm输出
  readFromFpm(c, buf);

  close(c->sockfd);

  return 0;
}
