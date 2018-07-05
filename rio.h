#include <sys/types.h>

#define MAXLINE 2048

#define RIO_BUFSIZE 8092

typedef struct{
  int rio_fd;
  int rio_cnt;
  char *rio_bufptr;
  char rio_buf[RIO_BUFSIZE];
}rio_t;

void rio_readinitb(rio_t *rp, int fd);

ssize_t rio_read(rio_t *rp, char *buf, size_t n);

ssize_t rio_readlineb(rio_t *rp, char *buf, size_t maxlen);

size_t rio_writen(int fd, void *usrbuf, size_t n);

ssize_t rio_readn(int fd, char *usrbuf, size_t n);
