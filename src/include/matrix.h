#ifndef MATRIX_H
#define MATRIX_H
#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)
#include <stdlib.h>
#include <stdio.h>

typedef enum {INTEGER, FLOAT, STRING, KVPAIR, OPTION} Type;

typedef struct KVPair {
  Type type_f;
  Type type_s;
  union {
    int i;
    float f;
    char *s;
    struct KVPair *pair;
    struct Option *option;
  } first;
  
  union {
    int i;
    float f;
    char *s;
    struct KVPair *pair;
    struct Option *option;
  } second;
} KVPair;

typedef struct Option {
    Type value_type;
    bool is_some;
    union {
        int i;
        float f;
        char *s;
        KVPair *pair;
    } value;
} Option;

typedef struct {
  int rows;
  int cols;
  Type type;
  union {
    KVPair **pair;
    int **i;
    float **f;
    char ***s;
    Option **option;
  } data;
} Matrix;




#ifndef MATRIX_IMPLEMENTATION

void copy_option(Option *dest, Option *src) {
    dest->value = src->value;
    dest->is_some = src->is_some;
    dest->value_type = src->value_type;
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

Matrix new_matrix(int rows, int cols, Type type){
	Matrix matrix;
  matrix.type = type;
  if (type == KVPAIR) {
    matrix.data.pair = malloc(sizeof(KVPair*) * rows);
  }
    
  if (type == OPTION) {
    matrix.data.option = malloc(sizeof(Option*) * rows);
  }
	matrix.rows = rows;
	matrix.cols = cols;

	for (int i = 0; i < rows; i++){ 
		if (type == KVPAIR)
      matrix.data.pair[i] = malloc(sizeof(KVPair) * cols);
    if (type == OPTION)
      matrix.data.option[i] = malloc(sizeof(Option) * cols);
	}	
	return matrix;
}

void free_matrix(Matrix mat){
    //TODO: other frees for other types
	for(int i = 0; i < mat.rows; ++i){
        for(int j = 0; j < mat.cols; ++j){
            if (mat.type == KVPAIR){
                if (mat.data.pair[i][j].second.pair)
                    free(mat.data.pair[i][j].second.pair);
            }
        }
        if (mat.type == KVPAIR) free(mat.data.pair[i]);
        if (mat.type == OPTION) free(mat.data.option[i]) ;
    }
    if (mat.type == OPTION)
        free(mat.data.option);
    if (mat.type == KVPAIR)
        free(mat.data.pair);
}


/*void print_matrix(Matrix matrix){
    printf("Matrix --------\n");
    for (int i = 0; i < matrix.rows; i++){
        for (int j = 0; j < matrix.cols; j++){
            printf("%d, ", matrix.data[i][j]);  
        }
        printf("\n");  // salto de línea al terminar cada fila
    }

    printf("---------------\n");
}*/
void init_matrix_num(Matrix mat, Type type, int num){
    for (int i = 0;i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++){
          switch (type){
            case KVPAIR:
              mat.data.pair[i][j].first.i = num;
              mat.data.pair[i][j].second.i = num;
              mat.data.pair[i][j].first.pair = NULL;
              mat.data.pair[i][j].second.pair = NULL;
              break;
            case OPTION:
              Option opt = option_new(INTEGER);
              opt.is_some = 1;
              opt.value.i = num;
              mat.data.option[i][j] = opt;
          } 
        }
    }
}
/*int get_val(Matrix mat, int r, int c){
        return mat.data[r][c];
}
void set_val(Matrix mat, int val, int r, int c){
    mat.data[r][c] = val;
}

*/
#endif
#endif
