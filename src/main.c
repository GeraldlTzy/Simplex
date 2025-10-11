#include <stdio.h>
#include <stdlib.h>
#define MAX_VAL 200

#define SIZE_MAT 3
int pivot_counter = 0;
void print_mat(double** mat){
    for (int i = 0; i < SIZE_MAT;i++){
        for (int j = 0; j < SIZE_MAT; j++){
            printf("%.3f ", mat[i][j]);
        }
        printf("\n");
    }
}

void canonize(double** mat, int rows, int cols, int pivot_row, int pivot_col){
    double k;
    for(int r = 0; r < rows; ++r){
        if(r == pivot_row){
            k = mat[pivot_row][pivot_col];
        } else {
            k = ((-1*mat[r][pivot_col])/mat[pivot_row][pivot_col]);
        }
        for(int c = 0; c < cols; ++c){
            if(r == pivot_row){
                mat[r][c] /= k;
            } else {
              mat[r][c] += (mat[pivot_row][pivot_col] * k);
            }
        }
    }
}


void maximize(double** mat, int rows, int cols){
  while(1){
    double min = MAX_VAL;
    int pivot_row = -1, pivot_col = -1;;

    for(int c = 1; c < cols-1; ++c){
      if(min > mat[0][c]){
        min = mat[0][c];
        pivot_col = c;
      }
    }
    if (min >= 0){
      break;
    }
    min =  MAX_VAL;
    double fraction;
    for(int r = 1; r < rows; ++r){
      if(mat[r][pivot_col] > 0){
        fraction = mat[r][cols-1] / mat[r][pivot_col];
        if(min > fraction){
          min = fraction;
          pivot_row = r;
        }
      }
    }
    if(pivot_row < 0)
      break;
    pivot_counter++;
    printf("Pivoteo(%d)\n", pivot_counter);
    canonize(mat, rows, cols, pivot_row, pivot_col);
  }
}

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
char *problem_name;
int count_variables;
int count_constraints;
char **variables_name;

void load_data(){
    char *filename = "example.txt";
    FILE *file;
    file = fopen(filename, "r");
    problem_name = read_text(file, '=', 10);
    count_variables = atoi(read_text(file, '=', '\n'));
    count_constraints = atoi(read_text(file, '=', '\n'));
    variables_name = malloc(sizeof(char*) * count_variables);
    for(int x = 0; x < count_variables; ++x){
      variables_name[x] = read_text(file, '=', '^');
      printf("%s\n", variables_name[x]);
    }
    //free(filename);
    //loaded = 1;
    fclose(file);
}

int main(int argc, char* args[]){
    double** mat = malloc(sizeof(double*)*SIZE_MAT);
    load_data();
    for(int i = 0; i < SIZE_MAT; i++)
        mat[i] = calloc(sizeof(double), SIZE_MAT);
    
    mat[0][0] = 1;
    mat[0][1] = -3;
    //mat[]

    mat[1][0] = 2;
    mat[1][1] = 2;
    mat[1][2] = 2;
    mat[2][0] = 2;
    mat[2][1] = 2;
    mat[2][2] = 2;
  
    
    print_mat(mat);
    maximize(mat, 3, 3);
    print_mat(mat);
    for(int i = 0; i < SIZE_MAT; i++)
        free(mat[i]);
    free(mat);
    return 0;
}
