#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "simplex.h"
#include "latex_generator.h"
#include "matrix.h"

#define MAX_VAL 1.79769313486231571e+308
int pivot_counter = 0;
int unbound = 0;
int degenerate = 0;
double tolerance = 1e-4;

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


int list_contain(GList *list, Matrix *mat){
  Node *curr;
  for (GList *l = list; l != NULL; l = l->next) {
    curr = (Node *)l->data;

    if(matrix_compare(mat, curr->mat)){ // si ya esta es que se enciclo, entonces nos cambiamos a la otra tabla posible
      return 1;
    }
  }
  return 0;
}

void get_last_state(GList **list, Node **last_out){
  GList *last_link = g_list_last(*list);
  *last_out = (Node *) last_link->data;
  *list = g_list_delete_link(*list, last_link);
}

void  node_list_free(Node *node){
  free_matrix(node->mat);
  free(node);
}

//char tex_buf1[256];
//char tex_buf2[4096];

/*###########################################################################*/

/*###########################################################################*/

void intermediate_table_draw(Latex_Generator *lg, Matrix *mat, char **headers, int pivot_row, int pivot_col){
    char buf[1024];
    buf[0] = '\0';
    //Init the table
    for (int c = 0; c < mat->cols+1; ++c)
        strcat(buf, "|c");
    strcat(buf, "|");
    lg_write(lg, "\\begin{center}\n");
    lg_write(lg, "\\begin{adjustbox}{max width=1\\textwidth,keepaspectratio}\n");
    lg_write(lg, "\\begin{tabular}{%s} \n \\hline \n", buf);
    //headers  
    buf[0] = '\0';
    for(int c = 0; c < mat->cols; ++c){
        if (c == pivot_col)
            strcat(buf, "\\cellcolor{PurpleNoMamado}");
        sprintf(buf + strlen(buf), "$%s$", headers[c]);
        strcat(buf, " & ");
    }
    sprintf(buf + strlen(buf), "$Fraction$");
    strcat(buf, "\\\\ \n \\hline \n");
    lg_write(lg, buf);
    // content
    for (int r = 0; r < mat->rows; ++r){
        buf[0] = '\0';
        // valor de la fila del pivote
        double val = mat->data.f[r][pivot_col];
        double b = mat->data.f[r][mat->cols-1];
        for (int c = 0; c < mat->cols; ++c){
            if (r == pivot_row || c == pivot_col)
                strcat(buf, "\\cellcolor{PurpleNoMamado}");
            sprintf(buf + strlen(buf), "%.2lf", mat->data.f[r][c]);
            strcat(buf, " & ");
        }
        // Si la fraccion  no es valida
        if (val <= 0 || r == 0){
            strcat(buf, "$-$");
        } else {
            if (r == pivot_row)
                strcat(buf, "\\cellcolor{PurpleNoMamado}");
            sprintf(buf + strlen(buf), "%.2lf", b/val);
        }
        strcat(buf, "\\\\ \n \\hline \n");
        lg_write(lg, buf);
    }
    buf[0] = '\0';
    // end the table
    lg_write(lg, "\\end{tabular} \n");
    lg_write(lg, "\\end{adjustbox}\n");
    lg_write(lg, "\\end{center}\n");
}

Matrix *maximize(Matrix *mat, char **headers, int do_intermediates, int *have_solution, Latex_Generator *lg){
  GList *list = 0;
  Matrix *init = matrix_copy(mat);
  double min, fraction;
  int pivot_counter = 0, pivot_row, pivot_col;
  
  while(1){
    min = MAX_VAL; pivot_row = -1; pivot_col = -1;
    // Cambia de matrix si es que se enciclo
    if(list_contain(list, mat) || (matrix_compare(mat, init) && pivot_counter != 0)){ 
      Node *node;
      lg_write(lg, "\\textbf{Notice:} the algorithm entered a loop. \n");
      get_last_state(&list, &node);
      if(!node){                                // Retorna si no hay mas opciones
        lg_write(lg, "No other paths were found, so the program has ended its execution\\\\\n");
        *have_solution = 0;
        return mat;
      }
      
      if(mat && mat != node->mat){              // Asigna la ultima bifurcacion
        free_matrix(mat);
        mat = NULL;
      }
      
      mat = node->mat;
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
      lg_write(lg, "Another path was found, the program continues its execution using this one to hopefuly find something different. \\\\\n");
    
    } else {                                    // Busca la columna del pivote
      for(int c = 1; c < mat->cols-1; ++c){
        if(min > mat->data.f[0][c]){
          min = mat->data.f[0][c];
          pivot_col = c;
        }
      }
      if (min >= 0) {
        *have_solution = 1;
        return mat;                  // termina si no encuentra nuevo valor
      }
      min =  MAX_VAL;
      
      for(int r = 1; r < mat->rows; ++r){
        if(mat->data.f[r][pivot_col] > 0){
          fraction = mat->data.f[r][mat->cols-1] / mat->data.f[r][pivot_col];
          if(min > fraction){
            min = fraction;
            pivot_row = r;
          } else if(fabs(min - fraction) < tolerance){ // Degenerado
            if (!degenerate) {
                if (do_intermediates)
                    lg_write(lg, "\\textbf{Degenerate Problem Found:}\n");
                else 
                    lg_write(lg, "\\section{Degenerate Problem Found}\n");
                lg_write(lg, "A draw when choosing a pivot ocurred during the Simplex execution.\n");
                lg_write(lg, "To manage this, one of the pivots was choosen and the other table was stored in case a loop is found.\\\\\n");
            }
            lg_write(lg, "\\textbf{Draw:} the rows %d and %d have a fraction of the same value,"
                            "\\textbf{row %d} (fraction %.5lf) with the %.5lf pivot was \\textbf{choosen} and "
                            "\\textbf{row %d} (fraction %.5lf) with the %.5lf pivot was \\textbf{stored} along with the table in case it is needed.\\\\\n",
                            pivot_row,
                            r,
                            pivot_row,
                            min,
                            mat->data.f[pivot_row][pivot_col],
                            r,
                            fraction,
                            mat->data.f[r][pivot_col]
                    );
            lg_write(lg, "To apretiate this please see the following table:\\\\\n");
            degenerate = 1;
            Node *node = malloc(sizeof(Node));  // Se guarda la matriz y el pivote para volver en caso
            node->mat = matrix_copy(mat);       // de enciclarse o encontrarse con problema no acotado
            node->pv_r = r;
            node->pv_c = pivot_col;
            list = g_list_append(list, node);
          }
        }
      }
      // Si no hay opciones termina
      if(pivot_row < 0){                        // Si es no acotado y hay opciones cambia 
        if(list){
          lg_write(lg, "\\textbf{Notice:} No other paths were found and there are no pivots to choose, so the program has ended its execution.\\\\\n");
          Node *node;
          get_last_state(&list, &node);
          if(mat && mat != node->mat){
            free_matrix(mat);
            mat = NULL;
          } 
          mat = node->mat;
          pivot_row = node->pv_r;
          pivot_col = node->pv_c;
        } else {
          //free_matrix(mat);
          //TODO: esto es una solucion no acotada, no que no tiene solucion
          *have_solution = 0;
          unbound = 1;
          return mat;
        }
      }
    }
    pivot_counter++;
    if (do_intermediates) {
      lg_write(lg, "Pivoting(%d)\n", pivot_counter);
      intermediate_table_draw(lg, mat, headers, pivot_row, pivot_col);
    }
    canonize(mat, pivot_row, pivot_col);
  }
}


Matrix *minimize(Matrix *mat, char **headers, int do_intermediates, int *have_solution, Latex_Generator *lg){
  GList *list = 0;
  Matrix *init = matrix_copy(mat);
  tex_table_draw(lg, mat->rows, mat->cols, headers, mat->data.f);
  double fraction, min_max;
  int pivot_row, pivot_col, pivot_counter = 0;

  while(1){
    min_max = -MAX_VAL; pivot_row = -1; pivot_col = -1;

    if(list_contain(list, mat) || (matrix_compare(mat, init) && pivot_counter != 0)){
      // Si entra al siguiente if es que se enciclo y no hay otras opciones
      Node *node;
      get_last_state(&list, &node);
      
      if(!node){
        free_matrix(mat);
        *have_solution = 0;
        return init;
      }
      
      if(mat && mat != node->mat){
        free_matrix(mat);
        mat = NULL;
      }
    
      mat = node->mat;
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
    
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
        *have_solution = 1;
        return mat;
      }
      // esta vez se usa para elegir la fraccion minima, igual que antes
      min_max =  MAX_VAL;
      for(int r = 1; r < mat->rows; ++r){
        if(mat->data.f[r][pivot_col] > 0){
          fraction = mat->data.f[r][mat->cols-1] / mat->data.f[r][pivot_col];
          if(min_max > fraction){
            min_max = fraction;
            pivot_row = r;
          } else if(min_max == fraction){ // Degenerado 
            degenerate = 1;
            Node *node = malloc(sizeof(Node));
            node->mat = matrix_copy(mat);
            node->pv_r = r;
            node->pv_c = pivot_col;
            list = g_list_append(list, node);
          }
        }
      }
      // Si no hay opciones termina
      if(pivot_row < 0){
        if(list){
          Node *node;
          get_last_state(&list, &node);
          if(mat && mat != node->mat){
            free_matrix(mat);
            mat = NULL;
          }
          mat = node->mat;
          pivot_row = node->pv_r;
          pivot_col = node->pv_c;
        } else {
          //free_matrix(mat);
          //TODO: esto es una solucion no acotada, no que no tiene solucion
          *have_solution = 0;
          unbound = 1;
          return mat;
        }
      }
    }
    pivot_counter++;
    if (do_intermediates){
      lg_write(lg, "Pivoting(%d)\n", pivot_counter);
      intermediate_table_draw(lg, mat, headers, pivot_row, pivot_col);
    }
    canonize(mat, pivot_row, pivot_col);
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
    lg_write(lg, "$");
    for (int i = 0; i < size; ++i) {
        lg_write(lg, "%s: %.2lf", headers[i+1], sol[i]);
        if (i != size-1) lg_write(lg, ", ");
    }
    lg_write(lg, "$");
    lg_write(lg, "\\\\\n");
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

double *find_solution(Matrix *mat) {
    double *solution = malloc(sizeof(double)*(mat->cols-2));
    // el valor de z no entra en el vector solucion
    for (int c = 1; c < mat->cols-1; ++c) {
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

Matrix *multiple_solutions(Matrix *mat, double **sol_array){
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
    if (pivot_col == -1) return mat;
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
    *sol_array = find_solution(mat);
    return mat;
}


double *generate_solution(double *sol1, double *sol2, int num_variables, double factor) {
    double *sol3 = malloc(sizeof(double)*num_variables);
    for (int i = 0; i < num_variables; ++i) {
        sol3[i] = factor*sol1[i] + (1-factor)*sol2[i];
    }
    return sol3;
}

int simplex(SimplexData *data, Latex_Generator *lg){
    pivot_counter = 0;

    lg_write(lg, "\\section{The initial simplex table}\n");
    tex_table_draw(lg, data->rows, data->cols, data->headers, data->table->data.f);

    if (data->show_intermediates){
      lg_write(lg, "\\section{The intermediates simplex tables}\n");
    }
    int have_solution = 0;
    unbound = 0;
    degenerate = 0;

    if (data->minimize) {
      data->table = minimize(data->table, data->headers, data->show_intermediates, &have_solution, lg);
    } else {
      data->table = maximize(data->table, data->headers, data->show_intermediates, &have_solution, lg);
    }

    lg_write(lg, "\\section{The final simplex table}\n");
    tex_table_draw(lg, data->rows, data->cols, data->headers, data->table->data.f);

    if (unbound){
        lg_write(lg, "\\section{Unbound Solution}\n");
        lg_write(lg, "The solution found is infinite, this happened because when choosing the pivot, all posible rows to choose had a non-positive value.\n");
        return 1;
    } else if (!have_solution){
        //TODO: en el otro proyecto manejar esto mejor
        lg_write(lg, "\\section{No solution found}\n");
        lg_write(lg, "The solution found is in another dimension, and can not posibly exist using the existing restrictions and desicion variables.\n");
        return 1;
    }
    double *solution1 = find_solution(data->table);
    double *solution2 = NULL;
    data->table = multiple_solutions(data->table, &solution2);
    if (solution2 != NULL){
        lg_write(lg, "\\section{The final simplex table 2}\n");
        lg_write(lg, "Multiple optimal solutions where found because one of the non basic functions can be pivoted without penalty. When pivoting said column, the folowing table can be obtained.\n"); 
        tex_table_draw(lg, data->rows, data->cols, data->headers, data->table->data.f);
    }

    lg_write(lg, "\\section{Solution}\n");
    lg_write(lg, "z=%.2lf\\\\\n", data->table->data.f[0][data->cols-1]);

    lg_write(lg, "\\textbf{Solution 1:}\\\\\n");
    write_solution(solution1, data->table->cols-2, data->headers, lg);

    if (solution2 != NULL) {  
        lg_write(lg, "\\textbf{Solution 2:}\\\\\n");
        write_solution(solution2, data->table->cols-2, data->headers, lg);
        lg_write(lg, "\\\\By using the following formula, infinite optimal solutions can be found:\\\\\n"); 
        lg_write(lg, "\\begin{dmath}\n");
        lg_write(lg, "\\alpha*solution1 + (1-\\alpha)*solution2\\\\\n");
        lg_write(lg, "\\end{dmath}\n");
        lg_write(lg, "$$\n");
        lg_write(lg, "0 \\leq \\alpha \\leq 1\n");
        lg_write(lg, "$$\n");
        double *solution3 = generate_solution(solution1, solution2, data->table->cols-2, 0.25);
        lg_write(lg, "\\textbf{Solution 3:}\\\\\n");
        write_solution(solution3, data->table->cols-2, data->headers, lg);
        double *solution4 = generate_solution(solution1, solution2, data->table->cols-2, 0.5);
        lg_write(lg, "\\textbf{Solution 4:}\\\\\n");
        write_solution(solution4, data->table->cols-2, data->headers, lg);
        double *solution5 = generate_solution(solution1, solution2, data->table->cols-2, 0.75);
        lg_write(lg, "\\textbf{Solution 5:}\\\\\n");
        write_solution(solution5, data->table->cols-2, data->headers, lg);
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
