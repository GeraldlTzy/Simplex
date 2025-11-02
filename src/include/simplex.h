#ifndef SIMPLEX_H
#define SIMPLEX_H

#include "matrix.h"
#include "latex_generator.h"

typedef struct {
  Matrix *table;
  int rows;
  int cols;
  int variables;
  int slacks;
  int excess;
  int artificials;
  int minimize;
  char **headers;
} SimplexData;

int simplex(SimplexData *data, Latex_Generator *lg);

#endif
