// 通用socket格式
typedef struct sockaddr SA;

void dealReques(int fd);
void clienterror(int fd, char *method, char *statusnum,
				 char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *filename);
void get_filetype(char *filename, char *filetype);
void server_static(int fd, char *filename, int filesie);

int openListenfd(int port, int listenq);
void sendMsg(int connect_d, char *msg);
int sendHtml(int connect_d, char *file);
void parseHeader(char *msg, char *html, char *query_string);
void response(char *response, int status, const char *buf);
