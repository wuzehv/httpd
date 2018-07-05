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
// 这两个目前没有用到
#define PHP_FPM_ROLE_AUTH 2
#define PHP_FPM_ROLE_EXTRA 3

#define FASTCGI_KEEP_CONN 1 // 是否为长连接
#define FASTCGI_HEADER_LEN 8 // 消息头长度

// fastcgi结构体
typedef struct{
  int requestId; // 请求id
  int sockfd; // 客户端连接id
}FCGI;

typedef struct {
  unsigned char version;          //版本
  unsigned char type;             //操作类型
  unsigned char requestIdB1;      //请求id
  unsigned char requestIdB0;
  unsigned char contentLengthB1;  //内容长度
  unsigned char contentLengthB0;
  unsigned char paddingLength;    //填充字节长度
  unsigned char reserved;         //保留字节
}FCGI_Header;   //消息头

// 开始请求体
typedef struct{
  unsigned char roleB1;
  unsigned char roleB0;
  unsigned char flags;
  unsigned char reserved[5];
}FCGI_BeginRequestBody;

// 结束消息体
typedef struct{
  unsigned char appStatusB3;
  unsigned char appStatusB2;
  unsigned char appStatusB1;
  unsigned char appStatusB0;
  unsigned char protocolStatus; // 协议状态
  unsigned char reserved[3];
}FCGI_EndRequestBody;

// 完整的记录都是请求头+对应的请求体

// 开始记录
typedef struct{
  FCGI_Header header;         //消息头
  FCGI_BeginRequestBody body; //开始请求体
}FCGI_BeginRequestRecord;

// 请求参数记录
typedef struct{
  FCGI_Header header;
  unsigned char nameLength;
  unsigned char valueLength;
  unsigned char data[0];
}FCGI_ParamsRecord;

int parse_php(int requestId, char *html);
