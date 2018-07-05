// 通用socket格式
typedef struct sockaddr SA;

void dealReques(int fd);
void clienterror(int fd, char *method, char *statusnum,
				 char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *filename);
void get_filetype(char *filename, char *filetype);
void server_static(int fd, char *filename, int filesie);

int openListenfd(int port, int listenq);
