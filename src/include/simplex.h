#ifndef SIMPLEX_H
#define SIMPLEX_H

#include "matrix.h"
#include "latex_generator.h"

typedef struct {
  Matrix *table;
  double *big_M;
  int rows;
  int cols;
  int variables;
  int slacks;
  int excess;
  int artificials;
  int minimize;
  int show_intermediates;
  char **headers;
} SimplexData;

int simplex(SimplexData *data, Latex_Generator *lg);
void simplex_data_free(SimplexData *data);
#endif
