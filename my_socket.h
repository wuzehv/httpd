// 通用socket格式
typedef struct sockaddr SA;

void deal_reques(int fd);
void clienterror(int fd, char *method, char *statusnum,
				 char *shortmsg, char *longmsg);
int parse_uri(char *uri);
void get_filetype(char *filename, char *filetype);
void server_static(int fd, char *filename, int filesie);

int open_listenfd(int port, int listenq);
