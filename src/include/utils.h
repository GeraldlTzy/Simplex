#ifndef UTILS_H
#define UTILS_H

char* read_text(FILE* file, char start, char end);
char* read_text_multiple_start(FILE* file, char *start, char end, int *selected_index);

#endif
