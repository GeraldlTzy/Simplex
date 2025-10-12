#ifndef MATRIX_H
#define MATRIX_H

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



void copy_option(Option *dest, Option *src);
KVPair pair_max(KVPair p1, KVPair p2);
KVPair pair_new(Type t_f, Type t_s);
Option option_new(Type t);

Matrix new_matrix(int rows, int cols, Type type);
void free_matrix(Matrix mat);
void init_matrix_num(Matrix mat, int num);
void print_matrix(Matrix mat);
#endif
