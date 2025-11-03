#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <assert.h>

#include "simplex.h"
#include "latex_generator.h"
#include "matrix.h"

#define MAX_VAL 1.79769313486231571e+308
int pivot_counter = 0;
Matrix *init;

void canonize(Matrix *mat, int pivot_row, int pivot_col){
    double k;
    for(int r = 0; r < mat->rows; ++r){
        if(r == pivot_row){
            k = mat->data.f[pivot_row][pivot_col];
        } else {
            k = ((-1*mat->data.f[r][pivot_col])/mat->data.f[pivot_row][pivot_col]);
        }
        for(int c = 0; c < mat->cols; ++c){
            if(r == pivot_row){
                mat->data.f[r][c] /= k;
            } else {
              mat->data.f[r][c] += (mat->data.f[pivot_row][c] * k);
            }
        }
    }
}

struct node_t {
  void * data;
  struct node_t **childs;
  int n_childs;
  int s_childs;
};

typedef struct node_t node_t;

typedef struct {
  node_t *root;
} tree_t;



typedef struct{
  Matrix *mat;
  int pv_r;
  int pv_c;
} Node;

GList *list = 0;

int list_contain(Matrix *mat){
  Node *curr;
  for (GList *l = list; l != NULL; l = l->next) {
    curr = (Node *)l->data;

    if(matrix_compare(mat, curr->mat)){ // si ya esta es que se enciclo, entonces nos cambiamos a la otra tabla posible
      return 1;
    }
  }
  return 0;
}

Node *get_last_state(){
  Node *last = (Node *) (g_list_last(list))->data;
  list = g_list_delete_link(list, g_list_last(list));
  return last;
}

void  node_list_free(Node *node){
  free_matrix(node->mat);
  free(node);
}

//char tex_buf1[256];
//char tex_buf2[4096];

/*###########################################################################*/

/*###########################################################################*/
void maximize(Matrix *mat, char **headers, int do_intermediates, Latex_Generator *lg){
  init = matrix_copy(mat);
  int ids = 0;
  while(1){
    ids++;
    double min = MAX_VAL;
    int pivot_row = -1, pivot_col = -1;;
    
    printf("#################################\n");
    for (GList *l = list; l != NULL; l = l->next) {
      Node * curr = (Node *)l->data;
      print_matrix(curr->mat);
    }
    printf("#################################\n");

    if(list_contain(mat) || (matrix_compare(mat, init) && ids != 1)){
      printf("ENCICLADO CAMBIA DE MAT\n");
      // Si entra al siguiente if es que se enciclo y no hay otras opciones
      Node *node = get_last_state();
      if(!node){
        printf("NO HAY OPCIONES\n");
        break;
      }
      
      if(mat && mat != node->mat){
        free_matrix(mat);
        mat = NULL;
      }
      mat = node->mat;
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
      print_matrix(mat);
    } else {

      for(int c = 1; c < mat->cols-1; ++c){
        if(min > mat->data.f[0][c]){
          min = mat->data.f[0][c];
          pivot_col = c;
        }
      }
      if (min >= 0){
        break;
      }
      min =  MAX_VAL;
      double fraction;
      for(int r = 1; r < mat->rows; ++r){
        if(mat->data.f[r][pivot_col] > 0){
          fraction = mat->data.f[r][mat->cols-1] / mat->data.f[r][pivot_col];
          if(min > fraction){
            min = fraction;
            pivot_row = r;
          } else if(min == fraction){
            printf("Degenerado: Empate  %d, %d\n", r, pivot_col);
            Node *node = malloc(sizeof(Node));
            node->mat = matrix_copy(mat);
            node->pv_r = r;
            node->pv_c = pivot_col;
            list = g_list_append(list, node);
          }
        }
      }

      // Si es no acotado y hay opciones cambia 
      // Si no hay opciones termina
      if(pivot_row < 0){
        printf("No acotado\n");
        print_matrix(mat);
        if(list){
          Node *node = get_last_state();
          if(mat && mat != node->mat){
            free_matrix(mat);
            mat = NULL;
          } 
          mat = node->mat;
          pivot_row = node->pv_r;
          pivot_col = node->pv_c;
        } else {
          printf("Algo1\n");
          print_matrix(mat);
          return;
        }
      }
    }
    pivot_counter++;
    //printf("Pivoteo(%d)\n", pivot_counter);
    canonize(mat, pivot_row, pivot_col);
    if (do_intermediates) {
      lg_write(lg, "Pivoting(%d)\n", pivot_counter);
      tex_table_draw(lg, mat->rows, mat->cols, headers, mat->data.f);
    }
    //print_matrix(mat);
  }
}


void minimize(Matrix *mat, char **headers, int do_intermediates, Latex_Generator *lg){
  Matrix *init = matrix_copy(mat);
  tex_table_draw(lg, mat->rows, mat->cols, headers, mat->data.f);

  while(1){
    double min_max = -MAX_VAL;
    int pivot_row = -1, pivot_col = -1;;
    
    printf("#################################\n");
    for (GList *l = list; l != NULL; l = l->next) {
      Node * curr = (Node *)l->data;
      print_matrix(curr->mat);
    }
    printf("#################################\n");

    if(list_contain(mat) || (matrix_compare(mat, init) && pivot_counter != 0)){
      printf("ENCICLADO CAMBIA DE MAT\n");
      // Si entra al siguiente if es que se enciclo y no hay otras opciones
      Node *node = get_last_state();
      if(!node){
        printf("NO HAY OPCIONES\n");
        break;
      }
      
      if(mat && mat != node->mat){
        free_matrix(mat);
        mat = NULL;
      }
      mat = node->mat;
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
      print_matrix(mat);
    } else {
      // elegir el mas positivo
      for(int c = 1; c < mat->cols-1; ++c){
        if(min_max < mat->data.f[0][c]){
          min_max = mat->data.f[0][c];
          pivot_col = c;
        }
      }
      // ya no hay positivos
      if (min_max <= 0){
        break;
      }
      // esta vez se usa para elegir la fraccion minima, igual que antes
      min_max =  MAX_VAL;
      double fraction;
      for(int r = 1; r < mat->rows; ++r){
        if(mat->data.f[r][pivot_col] > 0){
          fraction = mat->data.f[r][mat->cols-1] / mat->data.f[r][pivot_col];
          if(min_max > fraction){
            min_max = fraction;
            pivot_row = r;
          } else if(min_max == fraction){
            printf("Degenerado: Empate  %d, %d\n", r, pivot_col);
            Node *node = malloc(sizeof(Node));
            node->mat = matrix_copy(mat);
            node->pv_r = r;
            node->pv_c = pivot_col;
            list = g_list_append(list, node);
          }
        }
      }
      // si nunca se setea el pivote
      if(pivot_row < 0){
        printf("No acotado aaaaaaaaa\n");
        if(list){
          Node *node = get_last_state();
          if(!node){
            printf("Loop\n");
          }
          mat = node->mat;
          pivot_row = node->pv_r;
          pivot_col = node->pv_c;
        } else {
          break;
        }
        // Si es no acotado y hay opciones cambia 
        // Si no hay opciones termina
        if(pivot_row < 0){
          printf("No acotado\n");
          print_matrix(mat);
          if(list){
            Node *node = get_last_state();
            if(mat && mat != node->mat){
              free_matrix(mat);
              mat = NULL;
            } 
            mat = node->mat;
            pivot_row = node->pv_r;
            pivot_col = node->pv_c;
          } else {
            return;
          }
        }
      }
    }
    //for(int i = 0; i < mat->cols; ++i)
    //printf("%s \t", headers[i]);
    //printf("\n");
 
    pivot_counter++;
    canonize(mat, pivot_row, pivot_col);
    if (do_intermediates){
      lg_write(lg, "Pivoting(%d)\n", pivot_counter);
      tex_table_draw(lg, mat->rows, mat->cols, headers, mat->data.f);
    }
  }
}

void print_solution(double *sol, int size) {
    printf("---- Solution: ");
    for (int i = 0; i < size; ++i) {
        printf("%.3lf ", sol[i]);
    }
    printf("---- \n");
}
void write_solution(double *sol, int size, char** headers, Latex_Generator* lg) {
    for (int i = 0; i < size; ++i) {
        lg_write(lg, "%s: %.5lf", headers[i+1], sol[i]);
        if (i != size-1) lg_write(lg, ", ");
    }
    lg_write(lg, "\\\\\n");
}
int is_basic_var(Matrix *mat, int col) {
    int one_ammount = 0;
    for (int r = 0; r < mat->rows; ++r) {
        double value = mat->data.f[r][col];
        if (value == 0.0) {
            continue;
        } else if (value == 1.0) {
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

double *find_solution(Matrix *mat, int num_variables) {
    double *solution = malloc(sizeof(double)*num_variables);
    // el valor de z no entra en el vector solucion
    for (int c = 1; c < num_variables+1; ++c) {
        if (!is_basic_var(mat, c)) {
            solution[c-1] = 0.0;
            continue;
        }
        // si es basica se busca el 1
        for (int r = 0; r < mat->rows; ++r){
            if (mat->data.f[r][c] != 1) continue;
            solution[c-1] = mat->data.f[r][mat->cols-1];
            break;
        }
    }
    return solution;
}

double *multiple_solutions(Matrix *mat, int num_variables){
    int pivot_col = -1;
    int pivot_row = -1;
    for (int c = 1; c < mat->cols; ++c) {
        if (is_basic_var(mat, c)) continue;
        // si una no basica tiene un 0
        if (mat->data.f[0][c] == 0) {
            pivot_col = c;
            break;
        }
    }
    // si no tiene, no hay nada que hacer
    if (pivot_col == -1) return NULL;
    // encontrar la fraccion 
    double min =  MAX_VAL;
    double fraction;
    for(int r = 1; r < mat->rows; ++r){
        if(mat->data.f[r][pivot_col] > 0){
            fraction = mat->data.f[r][mat->cols-1] / mat->data.f[r][pivot_col];
            if(min > fraction){
                min = fraction;
                pivot_row = r;
            }
        }
    }
    canonize(mat, pivot_row, pivot_col);
    return find_solution(mat, num_variables);
}


double *generate_solution(double *sol1, double *sol2, int num_variables, double factor) {
    double *sol3 = malloc(sizeof(double)*num_variables);
    for (int i = 0; i < num_variables; ++i) {
        sol3[i] = factor*sol1[i] + (1-factor)*sol2[i];
    }
    return sol3;
}

int simplex(SimplexData *data, Latex_Generator *lg){
    Matrix *mat = data->table;
    pivot_counter = 0;

    print_matrix(mat);
    lg_write(lg, "\\section{The initial simplex table}\n");
    tex_table_draw(lg, mat->rows, mat->cols, data->headers, mat->data.f);

    if (data->show_intermediates){
      lg_write(lg, "\\section{The intermediates simplex tables}\n");
    }

    if (data->minimize) {
      node_t initial = {NULL, malloc(sizeof(node_t *) * 5), 0, 5};
      tree_t forks = {&initial};
      minimize(mat, data->headers, data->show_intermediates, lg);
    } else {
      node_t initial = {NULL, malloc(sizeof(node_t *) * 5), 0, 5};
      tree_t forks = {&initial};
      maximize(mat, data->headers, data->show_intermediates, lg);
    }

    printf("EXIT \n");

    if (mat == NULL){
        //TODO: en el otro proyecto manejar esto mejor
        lg_write(lg, "\\section{No solution found}\n");
        return 1;
    }

    //Se cae al acceder data.f pero justo antes del return de la funcion si lo deja acceder
    printf("%d\n", mat->data.f[0]); 
    printf("EXIT \n");

    print_matrix(mat);
   

    lg_write(lg, "\\section{The final simplex table}\n");
    tex_table_draw(lg, mat->rows, mat->cols, data->headers, mat->data.f);
    lg_write(lg, "\\section{Solution}\n");
    double *solution1 = find_solution(mat, data->variables);
    lg_write(lg, "\\textbf{Solution 1:}\\\\\n");
    write_solution(solution1, data->variables, data->headers, lg);
    double *solution2 = multiple_solutions(mat, data->variables);



    if (solution2 != NULL) {  
        lg_write(lg, "\\\\Multiple optimal solutions where found because one of the non basic functions can be pivoted without penalty.\\\\\\\\\n"); 
        lg_write(lg, "\\textbf{Solution 2:}\\\\\n");
        write_solution(solution2, data->variables, data->headers, lg);
        lg_write(lg, "\\\\By using the following formula, infinite optimal solutions can be found:\\\\\n"); 
        lg_write(lg, "\\begin{dmath}\n");
        lg_write(lg, "\\alpha*solution1 + (1-\\alpha)*solution2\\\\\n");
        lg_write(lg, "\\end{dmath}\n");
        lg_write(lg, "$$\n");
        lg_write(lg, "0 \\leq \\alpha \\leq 1\n");
        lg_write(lg, "$$\n");
        double *solution3 = generate_solution(solution1, solution2, data->variables, 0.25);
        lg_write(lg, "\\textbf{Solution 3:}\\\\\n");
        write_solution(solution3, data->variables, data->headers, lg);
        double *solution4 = generate_solution(solution1, solution2, data->variables, 0.5);
        lg_write(lg, "\\textbf{Solution 4:}\\\\\n");
        write_solution(solution4, data->variables, data->headers, lg);
        double *solution5 = generate_solution(solution1, solution2, data->variables, 0.75);
        lg_write(lg, "\\textbf{Solution 5:}\\\\\n");
        write_solution(solution5, data->variables, data->headers, lg);
        free(solution2);
        free(solution3);
        free(solution4);
        free(solution5);
    }
    free(solution1);

    return 0;
}


void simplex_data_free(SimplexData *data){
  for (int i = 0; i < data->cols; ++i){
    free(data->headers[i]);
  }
  free(data->headers);
  free_matrix(data->table);
}
