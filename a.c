#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
  char f1[100] = "/home";
  char a[1024] = "id=12";
  char f2[] = "GET /index.html HTTP/1.1";
  printf("%s\n", strtok(f2, " "));
  char *p = strtok(NULL, " ");
  printf("%s, %ld\n", p, strlen(a));
  char *w = strchr(a, 'd');
  printf("%s, %ld\n", w, w-a);

  return 0;
}
