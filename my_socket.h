int create_socket();
void bind_port(int socket, int port);
void listen_queue(int socket, int len);
int accept_connect(int socket);
void send_msg(int connect_d, char *msg);
int send_html(int connect_d, char *file);
void parse_header(char *msg, char *html, char *query_string);
void response(char *response, int status, const char *buf);
