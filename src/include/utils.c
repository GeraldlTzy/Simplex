#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
char* read_text(FILE* file, char start, char end){
  int str_size = 32, index = 0;
  char *str = malloc(str_size);
  char c;
  while((c = fgetc(file)) != start);
  while((c = fgetc(file)) != end && c != 10){
    if (index >= str_size - 1) {
      str_size *= 2;
      char *tmp = realloc(str, str_size);
      if (!tmp) { free(str); return NULL; }
      str = tmp;
    }
    str[index++] = c;
  }
  str[index] = '\0';
  return str;
}

