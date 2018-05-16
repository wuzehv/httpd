// fastcgi版本号
#define FASTCGI_VERSION 1

// 消息类型
#define FASTCGI_TYPE_BEGIN 1 // 请求开始
#define FASTCGI_TYPE_EXCEPTION 2 // 异常断开
#define FASTCGI_TYPE_END 3 // 请求结束
#define FASTCGI_TYPE_ENV 4 // 传递环境变量
#define FASTCGI_TYPE_POST 5 // 发送post数据
#define FASTCGI_TYPE_SUCC 6 // php-fpm正常返回码
#define FASTCGI_TYPE_ERROR 7 // php-fpm异常返回码

// 角色类型, 三个角色
#define PHP_FPM_ROLE_COMMOM 1 // 最常用的，接受http请求，产生响应
#define PHP_FPM_ROLE_AUTH 2
#define PHP_FPM_ROLE_EXTRA 3

// fastcgi协议消息头
typedef struct{
  unsigned char version;
  unsigned char type;
  unsigned char requestIdB0;
  unsigned char requestIdB1;
  unsigned char contentLengthB0;
  unsigned char contentLengthB1;
  unsigned char paddingLength;
  unsigned char reserved;
}FCGI_Header;

// 开始请求体
typedef struct{
  unsigned char roleB0;
  unsigned char roleB1;
  unsigned char flags;
  unsigned char reserved[5];
}FCGI_BeginRequestBody;

// 结束消息体
typedef struct{
  unsigned char appStatusB0;
  unsigned char appStatusB1;
  unsigned char appStatusB2;
  unsigned char appStatusB3;
  unsigned char protocolStatus; // 协议状态
  unsigned char reserved[3];
}FCGI_EndRequestBody;

// 完整的记录都是请求头+对应的请求体

// 开始记录
typedef struct{
  FCGI_Header header;
  FCGI_BeginRequestBody body;
}FCGI_BeginRecord;

// 结束记录
typedef struct{
  FCGI_Header header;
  FCGI_EndRequestBody body;
}FCGI_EndRecord;

// 请求参数记录
typedef struct{
  FCGI_Header header;
  unsigned char nameLength;
  unsigned char valueLength;
  unsigned char data[0];
}FCGI_ParamsRecord;

// 下面是函数
// 绑定php-fpm进行端口
int bind_php_fpm(int *socket, const char *ip, int port);

// 构造请求头部
FCGI_Header makeHeader(int type, int requestId, int contentLength, int paddingLenth);

// 构造开始请求记录协议体
FCGI_BeginRequestBody makeBeginRequestBody(int role, int keepConn);

int sendParams(int php_fpm_socket, int request_id, char *name, char *value);

int makeNameValueBody(char *name, int nameLen, char *value, int valueLen, unsigned char *bodyBuffer, int *bodyLen);

int sendEndRecord(int php_fpm_socket, int request_id);
