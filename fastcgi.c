#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "my_socket.h"
#include "fastcgi.h"

int php_fpm_socket = 0;
int request_id = 3;

int bind_php_fpm(int *socket, const char *ip, int port){
  int s = create_socket();
  struct sockaddr_in si;
  memset(&si, 0, sizeof(si));
  si.sin_family = PF_INET;
  si.sin_addr.s_addr = inet_addr(ip);
  si.sin_port = htons(port);
  int c = connect(s, (struct sockaddr *)&si, sizeof(si));
  if(c == -1){
    fprintf(stderr, "error:%s\n", strerror(errno));
    return -1;
  }
  *socket = s;
  printf("socket:%d\n", *socket);
  return 0;
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

  printf("verion:%d\n", header.version);
  printf("type:%d\n", header.type);
  printf("requestIdB1:%d\n", header.requestIdB1);
  printf("requestIdB0:%d\n", header.requestIdB0);
  printf("contentLengthB1:%d\n", header.contentLengthB1);
  printf("contentLengthB0:%d\n", header.contentLengthB0);
  printf("paddingLength:%d\n", header.paddingLength);

  return header;
}

FCGI_BeginRequestBody makeBeginRequestBody(int role, int keepConn){
  FCGI_BeginRequestBody body;
  body.roleB1 = (unsigned char)((role >> 8) & 0xFF);
  body.roleB0 = (unsigned char)(role & 0xFF);
  body.flags = (unsigned char)((keepConn) ? 1 : 0);
  memset(body.reserved, 0, sizeof(body.reserved));
  printf("roleB1:%d\n", body.roleB1);
  printf("roleB0:%d\n", body.roleB0);
  printf("flags:%d\n", body.flags);
  int i;
  for(i = 0; i < 5;i++){
    printf("reserved:%d\n", body.reserved[i]);
  }

  return body;
}

int sendParams(int php_fpm_socket, int request_id, char *name, char *value){
  int bodyLen;
  char bodyBuffer[1024];
  makeNameValueBody(name, strlen(name), value, strlen(value), bodyBuffer, &bodyLen);
  FCGI_Header header;
  header = makeHeader(FASTCGI_TYPE_ENV, request_id, bodyLen, 0);
  int nameValueLen = bodyLen + sizeof(header);
  char nameValueRecord[nameValueLen];
  memcpy(nameValueRecord, (char *)&header, sizeof(header));
  memcpy(nameValueRecord + sizeof(header), bodyBuffer, bodyLen);

  int s = write(php_fpm_socket, bodyBuffer, nameValueLen);
  printf("send_len:%d\n", s);
  printf("namevaluelen:%d\n", nameValueLen);
  printf("namevaluerecord:%s\n", nameValueRecord);
  if(s == -1){
    fprintf(stderr, "error:%s\n", strerror(errno));
    return -1;
  }

  return 0;
}

int makeNameValueBody(char *name, int nameLen, char *value, int valueLen, unsigned char *bodyBuffer, int *bodyLen){
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

int sendEndRecord(int php_fpm_socket, int request_id){
  FCGI_Header header;
  header = makeHeader(FASTCGI_TYPE_END, request_id, 8, 0);
  int s = write(php_fpm_socket, (char *) &header, sizeof(header));
  if(s == -1){
    fprintf(stderr, "error:%s\n", strerror(errno));
    return -1;
  }

  return 0;
}

int main(){
  // 连接php-fpm
  int cs = bind_php_fpm(&php_fpm_socket, "127.0.0.1", 9000);
  if(cs == -1){
    return -1;
  }

  FCGI_BeginRecord beginRecord;
  beginRecord.header = makeHeader(FASTCGI_TYPE_BEGIN, request_id, sizeof(beginRecord.body), 0);
  beginRecord.body = makeBeginRequestBody(PHP_FPM_ROLE_COMMOM, 1);

  // 发送开始消息记录
  int s = write(php_fpm_socket, (char *) &beginRecord, sizeof(beginRecord));
  if(s == -1){
    fprintf(stderr, "error:%s\n", strerror(errno));
    return -1;
  }

  // 发送请求参数
  sendParams(php_fpm_socket, request_id, "SCRIPT_FILENAME", "/home/wuzehui/Desktop/clearn/httpd/web/index.php");
  sendParams(php_fpm_socket, request_id, "REQUEST_METHOD", "GET");
  // 发送结束记录
  sendEndRecord(php_fpm_socket, request_id);
  // 获取php-fpm输出
  /* char buf[1024]; */
  /* recv(php_fpm_socket, buf, sizeof(buf), 0); */
  /* printf("%s\n", buf); */
  fprintf(stdout, "succ");
  close(php_fpm_socket);

  return 0;
}
