#ifndef SIMPLEX_H
#define SIMPLEX_H

#include "matrix.h"
#include "latex_generator.h"

int simplex(Matrix *mat, int minimize, int num_variables, Latex_Generator *lg);

#endif
