int createSocket();
void bindPort(int socket, int port);
void listenQueue(int socket, int len);
int acceptConnect(int socket);
void sendMsg(int connect_d, char *msg);
int sendHtml(int connect_d, char *file);
void parseHeader(char *msg, char *html, char *query_string);
void response(char *response, int status, const char *buf);
