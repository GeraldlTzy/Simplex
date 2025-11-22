#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)
#include <stdlib.h>
#include <stdio.h>
#include "matrix.h"
#include <math.h>

static double tolerance = 1e-4;
void copy_option(Option *dest, Option *src) {
    dest->value = src->value;
    dest->is_some = src->is_some;
    dest->value_type = src->value_type;
}

int is_basic_var(Matrix *mat, int col) {
    int one_ammount = 0;
    double value, tmp;
    for (int r = 0; r < mat->rows; ++r) {
        value = mat->data.f[r][col];
        tmp = value;
        if(value < 0) tmp = value * -1;
        
        if (tmp <= tolerance) {
            continue;
        } else if (fabs(value - 1.0) <= tolerance) {
            ++one_ammount;
            if (one_ammount > 1) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    return 1;
}
KVPair pair_max(KVPair p1, KVPair p2){
  if (max(p1.first.i, p2.first.i) == p1.first.i)
    return p1;
  else
    return p2;
}

KVPair pair_new(Type t_f, Type t_s){
  KVPair pair;
  pair.type_f = t_f;
  pair.type_s = t_s;
  return pair;
}

Option option_new(Type t) {
    Option option;
    option.is_some = 0;
    option.value_type = t;
    return option;
}


/*void * pair_get_fst(Type t_f){
  switch (t_f){
    case INTEGER:
      return first.i;
    case FLOAT:
      return first.f
    case STRING:
      return first.s

  }
}*/

Matrix *new_matrix(int rows, int cols, Type type){
	Matrix *matrix = malloc(sizeof(Matrix));
  matrix->type = type;
	matrix->rows = rows;
	matrix->cols = cols;

  switch(type){
    case KVPAIR:
      matrix->data.pair = malloc(sizeof(KVPair*) * rows);
      break;
    case OPTION:
      matrix->data.option = malloc(sizeof(Option*) * rows);
      break;
    case FLOAT:
      matrix->data.f = malloc(sizeof(double*) * rows);
  }

	for (int i = 0; i < rows; i++){ 
		switch(type){
      case KVPAIR:
        matrix->data.pair[i] = malloc(sizeof(KVPair) * cols);
        break;
      case OPTION:
        matrix->data.option[i] = malloc(sizeof(Option) * cols);
        break;
      case FLOAT:
        matrix->data.f[i] = malloc(sizeof(double) * cols);
        break;
    }
	}	
	return matrix;
}

Matrix *matrix_copy(const Matrix *src) {
    Matrix *dest = malloc(sizeof(Matrix));
    dest->rows = src->rows;
    dest->cols = src->cols;
    dest->type = src->type;

    switch (src->type) {
        case FLOAT:
            dest->data.f = malloc(src->rows * sizeof(double *));
            for (int i = 0; i < src->rows; i++) {
                dest->data.f[i] = malloc(src->cols * sizeof(double));
                for (int j = 0; j < src->cols; j++) {
                    dest->data.f[i][j] = src->data.f[i][j];
                }
            }
            break;

        default:
            free(dest);
            return NULL;
    }
    return dest;
}


void free_matrix(Matrix *mat){
    //TODO: other frees for other types
  for(int i = 0; i < mat->rows; ++i){
    switch(mat->type){
      case OPTION:
        //TODO
        break;
      case KVPAIR:
        //TODO
        break;
      case FLOAT:
        free(mat->data.f[i]);
    }
  }
  switch(mat->type){
    case OPTION:
      free(mat->data.option);
      break;
    case KVPAIR:
      free(mat->data.pair);
      break;
    case FLOAT:
      free(mat->data.f);
  }
  free(mat);
}


int matrix_compare(const Matrix *self, const Matrix *other){
  double err = 1e-5;
  for(int r = 0; r < self->rows; ++r){
    for(int c = 0; c < self->cols; ++c){
      if(fabs(self->data.f[r][c] - other->data.f[r][c]) > err){return 0;}
    }
  }
  return 1;
}


void init_matrix_num(Matrix *mat, int num){
    for (int i = 0;i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++){
          switch (mat->type){
            case KVPAIR:
              mat->data.pair[i][j].first.i = num;
              mat->data.pair[i][j].second.i = num;
              mat->data.pair[i][j].first.pair = NULL;
              mat->data.pair[i][j].second.pair = NULL;
              break;
            case OPTION:
              Option opt = option_new(INTEGER);
              opt.is_some = 1;
              opt.value.i = num;
              mat->data.option[i][j] = opt;
              break;
            case FLOAT:
              mat->data.f[i][j] = 0.0f;
          }
        }
    }
}

void print_matrix(Matrix *mat){
  for (int r = 0; r < mat->rows; ++r) {
    for (int c = 0; c < mat->cols; ++c){
      switch (mat->type){
        case KVPAIR:
          break;
        case OPTION:
          break;
        case FLOAT:
          printf("%.4lf|\t", mat->data.f[r][c]);
          break;
      }
    }
    printf("\n");
  }
}

/*int get_val(Matrix mat, int r, int c){
        return mat.data[r][c];
}
void set_val(Matrix mat, int val, int r, int c){
    mat.data[r][c] = val;
}

*/
