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


char* read_text_multiple_start(FILE* file, char *start, char end, int *selected_start){
  int str_size = 32, index = 0;
  char *str = malloc(str_size);
  char c;
  int can_exit = 0;
  int size = strlen(start);
  while(!can_exit){
      c = fgetc(file);
      for (int i = 0; i < size; ++i){
          if (c == start[i]){
            *selected_start = i;
            can_exit = 1;
            break;
          }
      }

  }
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

