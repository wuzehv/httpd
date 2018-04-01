#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include <string.h>
#include <errno.h>

void error(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}
