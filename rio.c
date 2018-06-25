#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "rio.h"

void rio_readinitb(rio_t *rp, int fd){
  rp->rio_fd = fd;
  rp->rio_cnt = 0;
  rp->rio_bufptr = rp->rio_buf;
}

// 读取操作，有缓冲区
ssize_t rio_read(rio_t *rp, char *buf, size_t n){
  int cnt;

  // 缓冲区为空的时候，重新填充缓冲区
  while(rp->rio_cnt <= 0){
    rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
    // 读取出错
    if(rp->rio_cnt < 0){
      if(errno != EINTR)
        return -1;
    } else if(rp->rio_cnt == 0) {
      // EOF
      return 0;
    } else {
      rp->rio_bufptr = rp->rio_buf;
    }
  }

  // 使用较小的长度进行读取
  cnt = n;
  if(rp->rio_cnt < n){
    cnt = rp->rio_cnt;
  }

  memcpy(buf, rp->rio_bufptr, cnt);
  rp->rio_cnt -= cnt;
  rp->rio_bufptr += cnt;

  return cnt;
}

ssize_t rio_readlineb(rio_t *rp, char *buf, size_t maxlen){
  int n, rc;
  char c, *usrbuf = buf;

  for(n = 1; n < maxlen; n++){
    if((rc = rio_read(rp, &c, 1)) == 1){
      *usrbuf++ = c;
      if(c == '\n')
        break;
    } else if(rc == 0){
      if(n == 1)
        // 空字符串
        return 0;
      else
        // EOF
        break;
    } else {
      return -1;
    }
  }

  return n;
}

size_t rio_writen(int fd, void *usrbuf, size_t n){
  size_t nleft = n;
  ssize_t nwriten;
  char *buf = usrbuf;

  while(nleft > 0){
    if((nwriten = write(fd, buf, nleft)) <= 0){
      // 遇到错误，置空变量，重新读取
      if(errno == EINTR)
        nwriten = 0;
      else
        return -1;
    }

    nleft -= nwriten;
    usrbuf += nwriten;
  }

  return n;
}

// 读取操作，无缓冲区
ssize_t rio_readn(int fd, char *usrbuf, size_t n){
  size_t nleft = n;
  ssize_t nread;
  char *bufp = usrbuf;

  while(nleft > 0){
    if((nread = read(fd, bufp, nleft)) < 0){
      if(errno == EINTR)
        // 置空，重新读取
        nread = 0;
      else
        // 出错
        return -1;
    } else if (nread == 0)
      // EOF
      break;

    nleft -= nread;
    bufp += nread;
  }

  return (n - nleft);
}
