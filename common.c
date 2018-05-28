#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "error.h"
#include "common.h"

#define DELIMITER "="

char *getConfig(const char *key){
  FILE *config_file;
  char line[1024];
  if(!(config_file = fopen("./httpd.conf", "r"))){
    error("config file not exists");
  }

  while(fscanf(config_file, "%[^\n]\n", line) == 1){
    // 简单处理注释，最好保证想要的数据中不要出现#号 :)
    if(strstr(line, "#")){
      continue;
    }
    if(strstr(line, key) == line){
      strtok(line, DELIMITER);
      return strtok(NULL, DELIMITER);
      break;
    }
  }
}
