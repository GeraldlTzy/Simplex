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

void canonize(Matrix *mat, double **big_M, int pivot_row, int pivot_col){
    double k;
    double big_k;
    for(int r = 0; r < mat->rows; ++r){
        if(r == pivot_row){
            k = mat->data.f[pivot_row][pivot_col];
        } else {
            k = ((-1*mat->data.f[r][pivot_col])/mat->data.f[pivot_row][pivot_col]);
        }
        if (r == 0){
            big_k = ((-1*(*big_M)[pivot_col])/mat->data.f[pivot_row][pivot_col]);
        }
        for(int c = 0; c < mat->cols; ++c){
            if(r == pivot_row){
                mat->data.f[r][c] /= k;
            } else {
              mat->data.f[r][c] += (mat->data.f[pivot_row][c] * k);
            }
            // para la M grande
            if (r == 0){
              (*big_M)[c] += (mat->data.f[pivot_row][c] * big_k);
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

void intermediate_table_draw(Latex_Generator *lg, Matrix *mat, double **big_M, char **headers, int pivot_row, int pivot_col, int artificial_start){\
    char buf[1024];
    buf[0] = '\0';
    //Init the table
    for (int c = 0; c < mat->cols+1; ++c){
        // si es artificial pero no es basica, se le hace skip
        if (c > artificial_start && c < mat->cols-1 && !is_basic_var(mat, c)){  
            continue;
        }
        strcat(buf, "|c");
    }
    strcat(buf, "|");
    lg_write(lg, "\\begin{center}\n");
    lg_write(lg, "\\begin{adjustbox}{max width=1\\textwidth,keepaspectratio}\n");
    lg_write(lg, "\\begin{tabular}{%s} \n \\hline \n", buf);
    //headers  
    buf[0] = '\0';
    for(int c = 0; c < mat->cols; ++c){
        // si es artificial pero no es basica, se le hace skip

        if (c > artificial_start && c < mat->cols-1 && !is_basic_var(mat, c)){
            continue;
        }
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
            // si es artificial pero no es basica, se le hace skip
            if (c > artificial_start && c < mat->cols-1 && !is_basic_var(mat, c)){
                continue;
            }
            if (r == pivot_row || c == pivot_col)
                strcat(buf, "\\cellcolor{PurpleNoMamado}");

            if (fabs((*big_M)[c]) > tolerance && fabs(mat->data.f[r][c]) > tolerance && r == 0) {
                sprintf(buf+strlen(buf), "%.2lf*M + %.2lf", (*big_M)[c], mat->data.f[r][c]);
            } else if (fabs((*big_M)[c]) > tolerance && r == 0){
                sprintf(buf+strlen(buf), "%.2lf*M", (*big_M)[c]);
            } else {
                sprintf(buf+strlen(buf), "%.2lf", mat->data.f[r][c]);
            }
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

Matrix *maximize(Matrix *mat, double **big_M, char **headers, int do_intermediates, Latex_Generator *lg, int artificial_start){
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
      double min_M = MAX_VAL;
      for(int c = 1; c < mat->cols-1; ++c){
        if (min_M-(*big_M)[c] > tolerance){
          min_M = (*big_M)[c];
          min = mat->data.f[0][c];
          pivot_col = c;
        // si el M es mas grande no se debe seleccionar, por eso el ==
        } else if(min > mat->data.f[0][c] && fabs(min_M - (*big_M)[c]) < tolerance){
          min_M = (*big_M)[c];
          min = mat->data.f[0][c];
          pivot_col = c;
        }
        /*if (min > mat->data.f[0][c]){
            min = mat->data.f[0][c];
            pivot_col = c;
        }*/
      }
      // si la M mas pequena es mayor a 0, significa que ya no hay negativos
      if (min_M > tolerance) {
        return mat;
      } else if (min >= 0 && fabs(min_M) < tolerance) {
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
                lg_write(lg,    "When you are in the execution of the Simplex algorithm you might find some peculiar tables.\n"
                                "These tables can occur when one of the values on the last column is zero.\n"
                                "When one of those values is zero an interesting phenomenon happens, and that is that a basic variable has the value of zero.\n"
                                "You could think that the only variables that have a value of zero are the non basic ones, but this is totally false.\n"
                                "In some cases you might not see a basic variable whose value is zero, but a basic variable having a value of zero is not an uncommon ocurrence.\n"
                                "When this happens we call the table a degenerate table, and when a problem has at least one degenerate table, we call it a degenerate problem.\\\\\n"
                );
                lg_write(lg,    "Degenerate problems might seem inocent, and for the most part they are.\n"
                                "But these problems come with some peculiarity, that is the reason behind the name, because these are hard to classify.\n"
                                "When a problem is degenerate, you might find that when you pivot the table, the objective function does not increase.\n"
                                "When this happens, do not panic, just keep pivoting, and in most cases you will solve the problem just like any other.\n"
                                "Another peculiarity of these problems is that you could find a draw when choosing a pivot, in that case feel free to choose the one you like, and in most cases you will be fine.\n"
                                "But in some very rare cases you can be very unlucky, and come to a pretty bad realization, and that is that the tables are starting to repeat.\n"
                                "Turns out, that in some extremely rare cases, Simplex can find a loop in it's execution, and thus never end.\n"
                                "If this happens you have two solutions, save every table and compare then to find if you are in a loop and choose a different pivot from last time (this is extremely expensive), or you could simply stop the program if it is taking to long to solve.\n"
                                "Both solutions have their downsides, but this a risk you take when using Simplex, and as far as it is known, this is unavoidable.\\\\\n"
                );

                lg_write(lg, "In the case of this program, when it realizes it is a degenerate problem, it stars to save the tables to see if it has entered a loop.\\\\\n");
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
            if (do_intermediates)
                lg_write(lg, "To appreciate this please see the following table:\\\\\n");
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
          unbound = 1;
          return mat;
        }
      }
    }
    pivot_counter++;
    if (do_intermediates) {
      lg_write(lg, "Pivoting(%d)\n", pivot_counter);
      intermediate_table_draw(lg, mat, big_M, headers, pivot_row, pivot_col, artificial_start);
    }
    canonize(mat, big_M, pivot_row, pivot_col);
  }
}


Matrix *minimize(Matrix *mat, double **big_M, char **headers, int do_intermediates, Latex_Generator *lg, int artificial_start){
  GList *list = 0;
  Matrix *init = matrix_copy(mat);
  //tex_table_draw(lg, mat->rows, mat->cols, headers, mat->data.f);
  double fraction, min_max;
  int pivot_row, pivot_col, pivot_counter = 0;

  while(1){
    min_max = -MAX_VAL; pivot_row = -1; pivot_col = -1;

    if(list_contain(list, mat) || (matrix_compare(mat, init) && pivot_counter != 0)){
      // Si entra al siguiente if es que se enciclo y no hay otras opciones
      Node *node;
      lg_write(lg, "\\textbf{Notice:} the algorithm entered a loop. \n");
      get_last_state(&list, &node);
      
      if(!node){
        lg_write(lg, "No other paths were found, so the program has ended its execution\\\\\n");
        free_matrix(mat);
        return init;
      }
      
      if(mat && mat != node->mat){
        free_matrix(mat);
        mat = NULL;
      }
    
      mat = node->mat;
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
      lg_write(lg, "Another path was found, the program continues its execution using this one to hopefuly find something different. \\\\\n");
    
    } else {
      // elegir el mas positivo
      double max_M = -MAX_VAL;
      for(int c = 1; c < mat->cols-1; ++c){
        if (max_M-(*big_M)[c] < -tolerance){
          max_M = (*big_M)[c];
          min_max = mat->data.f[0][c];
          pivot_col = c;
        // si max era mas grande no se debe seleccionar, por eso el igual
        } else if(min_max < mat->data.f[0][c] && fabs(max_M - (*big_M)[c]) < tolerance){
          max_M = (*big_M)[c];
          min_max = mat->data.f[0][c];
          pivot_col = c;
        }
      }
      // si la M mas grande no es positiva, ya no hay positivos
      if (max_M < tolerance) {
        return mat;
      } else if (min_max <= 0 && fabs(max_M) < tolerance) {
        return mat;                  // termina si no encuentra nuevo valor
      }
      // esta vez se usa para elegir la fraccion minima, igual que antes
      min_max =  MAX_VAL;
      for(int r = 1; r < mat->rows; ++r){
        if(mat->data.f[r][pivot_col] > 0){
          fraction = mat->data.f[r][mat->cols-1] / mat->data.f[r][pivot_col];
          if(min_max > fraction){
            min_max = fraction;
            pivot_row = r;
          } else if(fabs(min_max - fraction) < tolerance){ // Degenerado 
            if (!degenerate) {
                if (do_intermediates)
                    lg_write(lg, "\\textbf{Degenerate Problem Found:}\n");
                else 
                    lg_write(lg, "\\section{Degenerate Problem Found}\n");
                lg_write(lg,    "When you are in the execution of the Simplex algorithm you might find some peculiar tables.\n"
                                "These tables can occur when one of the values on the last column is zero.\n"
                                "When one of those values is zero an interesting phenomenon happens, and that is that a basic variable has the value of zero.\n"
                                "You could think that the only variables that have a value of zero are the non basic ones, but this is totally false.\n"
                                "In some cases you might not see a basic variable whose value is zero, but a basic variable having a value of zero is not an uncommon ocurrence.\n"
                                "When this happens we call the table a degenerate table, and when a problem has at least one degenerate table, we call it a degenerate problem.\\\\\n"
                );
                lg_write(lg,    "Degenerate problems might seem inocent, and for the most part they are.\n"
                                "But these problems come with some peculiarity, that is the reason behind the name, because these are hard to classify.\n"
                                "When a problem is degenerate, you might find that when you pivot the table, the objective function does not increase.\n"
                                "When this happens, do not panic, just keep pivoting, and in most cases you will solve the problem just like any other.\n"
                                "Another peculiarity of these problems is that you could find a draw when choosing a pivot, in that case feel free to choose the one you like, and in most cases you will be fine.\n"
                                "But in some very rare cases you can be very unlucky, and come to a pretty bad realization, and that is that the tables are starting to repeat.\n"
                                "Turns out, that in some extremely rare cases, Simplex can find a loop in it's execution, and thus never end.\n"
                                "If this happens you have two solutions, save every table and compare them to find if you are in a loop and choose a different pivot from last time (this is extremely expensive), or you could simply stop the program if it is taking to long to solve.\n"
                                "Both solutions have their downsides, but this a risk you take when using Simplex, and as far as it is known, this is unavoidable.\\\\\n"
                );

                lg_write(lg, "In the case of this program, when it realizes it is a degenerate problem, it stars to save the tables to see if it has entered a loop.\\\\\n");
            }
            lg_write(lg, "\\textbf{Draw:} the rows %d and %d have a fraction of the same value,"
                            "\\textbf{row %d} (fraction %.5lf) with the %.5lf pivot was \\textbf{choosen} and "
                            "\\textbf{row %d} (fraction %.5lf) with the %.5lf pivot was \\textbf{stored} along with the table in case it is needed.\\\\\n",
                            pivot_row,
                            r,
                            pivot_row,
                            min_max,
                            mat->data.f[pivot_row][pivot_col],
                            r,
                            fraction,
                            mat->data.f[r][pivot_col]
                    );
            if (do_intermediates)
                lg_write(lg, "To appreciate this please see the following table:\\\\\n");
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
          unbound = 1;
          return mat;
        }
      }
    }
    pivot_counter++;
    if (do_intermediates){
      lg_write(lg, "Pivoting(%d)\n", pivot_counter);
      intermediate_table_draw(lg, mat, big_M, headers, pivot_row, pivot_col, artificial_start);
    }
    canonize(mat, big_M, pivot_row, pivot_col);
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
        lg_write(lg, "%s= %.2lf", headers[i+1], sol[i]);
        if (i != size-1) lg_write(lg, "; ");
    }
    lg_write(lg, "$");
    lg_write(lg, "\\\\\n");
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

Matrix *multiple_solutions(Matrix *mat, double **big_M, double **sol_array){
    int pivot_col = -1;
    int pivot_row = -1;
    for (int c = 1; c < mat->cols; ++c) {
        if (is_basic_var(mat, c)) continue;
        // si una no basica tiene un 0
        if (fabs(mat->data.f[0][c]) < tolerance && fabs((*big_M)[c]) < tolerance) {
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

    canonize(mat, big_M, pivot_row, pivot_col);
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

    int *skip = calloc(data->cols, sizeof(int));
    lg_write(lg, "\\section{The initial simplex table}\n");
    tex_table_draw(lg, data->rows, data->cols, data->headers, data->table->data.f, data->big_M, skip);

    if (data->show_intermediates){
      lg_write(lg, "\\section{The intermediate simplex tables}\n");
    }
    unbound = 0;
    degenerate = 0;

    // si hay artificiales canonizarlas primero
    if (data->artificials != 0){
        int start = 1 + data->variables + data->slacks + data->excess;
        for (int i = start; i < start+data->artificials;++i){
            int r;
            // encontrar el 1
            for (int j = 0; j < data->rows; ++j){
                if (data->table->data.f[j][i] == 1){
                    r = j;
                    break;
                }
            }
            canonize(data->table, &data->big_M, r, i);
        }
    }

    int artificial_start = data->variables + data->slacks + data->excess;
 
    if (data->minimize) {
      data->table = minimize(data->table, &data->big_M, data->headers, data->show_intermediates, lg, artificial_start);
    } else {
      data->table = maximize(data->table, &data->big_M, data->headers, data->show_intermediates, lg, artificial_start); 
    }

    for (int i = 0; i < data->cols; ++i){
        if (i > artificial_start && i < data->cols-1 && !is_basic_var(data->table, i))
            skip[i] = 1;
    }

    lg_write(lg, "\\section{The final simplex table}\n");
    tex_table_draw(lg, data->rows, data->cols, data->headers, data->table->data.f, data->big_M, skip);

    int have_solution = 1;
    for (int c = 0; c < data->cols; ++c){
        if (is_basic_var(data->table, c) && fabs(data->big_M[c]) > tolerance){
            have_solution = 0;
        }
    }
    if (fabs(data->big_M[data->cols-1]) > tolerance){
        have_solution = 0;
    }
    
    if (unbound){
        lg_write(lg, "\\section{Unbound solution found}\n");
        lg_write(lg,    "The program found an unbound solution during it's execution, meaning it can grow infinitely.\\\\\n"
                        "When choosing a pivot you first find the column that most benefits you to be pivoted, after doing that you choose the smallest fraction to make sure all restrictions are met.\n"
                        "The fractions you choose during the process are the right sides divided by their corresponding number on the column of the pivot. \n"
                        "More specifically, when choosing the fractions you only take into account the non-negative and non-zero numbers on the pivot column.\n"
                        "But it could happen that all of the options you have are zero or negative. \n"
                        "When this happens it means that the solution is unbound, this means that the objective valuje can grow as much as you want. \\\\\n"
                        "Saying iy in other words, the objective value can be grown to infinity.\n"
                        "At first this could appear like a positive thing, but these solutions also tend to have infinite values for other variables so they are not posible to replicate in real life.\n"
                        "A lot of times an unbound solution could mean that you are missing restrictions in the problem given so you might have to rethink the problem in order to find more solutions so that you can have a solution that can be done in real life.\\\\\n"

        );
        return 1;
    } else if (!have_solution){
        //TODO: en el otro proyecto manejar esto mejor
        lg_write(lg, "\\section{No solution found}\n");
        lg_write(lg,    "The solution found is in an artificial dimension, meaning the problem given has no solution. \\\\\n"
                        "The Simplex algorithm deals with problems that have restrictions of the forms greater or equal, less or equal, and equals.\n"
                        "When solving for restrictions that are less or equal, a slack variable is inserted, this gives the problem a starting solution because these are already canonized.\n"
                        "But when dealing with greater of equal restrictions, excess variables are inserted, these variables do not produce canonical vectors because these are subtracted. \n"
                        "Another problematic restriction is the equals, because these do not propduce any variables.\n"
                        "The simplex algorithm rellies on the matrix having canonical, so something needs to be done about these two restriction types. \\\\\n"
                        "A way to solve this is with the Big M Method, this method inserts artificial variables to make canonical vectors\n"
                        "These variables are added to the objective function with a value M (when minimizing you add -M and when maximizing you add +M) which can be interpreted as a very big number.\n"
                        "When doing this you scale up the ammount of dimensions the problem has, this allows the Simplex algorithm to move in these dimentions, and find a posible solution. \n"
                        "It could happen that the solution the algorithm found has an M in one of it's non-artificial variables, when this happens it means that the solution is in one of the artificial dimensions you inserted.\n"
                        "When this happens it means that the program found no solution inside the initial Simplex body, and thus it means that the poroblem has no solution.\n"

        );
        return 1;
    }
    
    ///////////////////////////
    
    double *solution1 = find_solution(data->table);
    double *solution2 = NULL;
    data->table = multiple_solutions(data->table, &data->big_M, &solution2);
    if (solution2 != NULL){
        lg_write(lg, "\\section{Multiple solutions found}\n");
        lg_write(lg,    "Once you end the execution of the Simplex algorithm, you might think that everything has ended and that the soulution you found is the only way to get the optimal objective value, but this is untrue.\n"
                        "Before asuming that you are finished check if you can pivot one of the non-basic variables without a penalty.\n"
                        "To do this check all of the columns that correspond whith a non-basic variable, if one of them has a zero in the first row, this means that if you pivot it you objective value will not change.\n"
                        "After you have found the row, canonize it, after doing it you will notice that as said, the objective value has not changed, but more importantly, you will find one of the basic variables is different, and that the values of the variables that stayed in the base have changed.\n"
                        "This is another optimal solution to the problem, because the value is still optimal and the variables are different than before.\\\\\n"
                        "If you thought that this is the end, think again, remember that Simplex is a graphic method that jumps between vertices of the body that cointains all feasible solutions.\n"
                        "Which means that the second solution you found is another vertex of the body. \n"
                        "If you join the vertices that have an optimal solution, you will draw a line (or a plane, or another body in multiple dimensions).\n"
                        "The body you drew contains other optimal solutions, infinite of them. \n"
                        "This means that the problem you found can be optimized in an infinite number of ways.\\\\\n"
        );
        lg_write(lg, "\\\\By using the following formula, infinite optimal solutions can be found:\\\\\n"); 
        lg_write(lg, "\\begin{dmath}\n");
        lg_write(lg, "\\alpha*solution1 + (1-\\alpha)*solution2\\\\\n");
        lg_write(lg, "\\end{dmath}\n");
        lg_write(lg, "$$\n");
        lg_write(lg, "0 \\leq \\alpha \\leq 1\n");
        lg_write(lg, "$$\n");
        
        lg_write(lg, "\\\\You can see the second optimal table found:\\\\\n"); 

        tex_table_draw(lg, data->rows, data->cols, data->headers, data->table->data.f, data->big_M, skip);
    }

    lg_write(lg, "\\section{Solution}\n");
    lg_write(lg, "z=%.2lf\\\\\n", data->table->data.f[0][data->cols-1]);

    lg_write(lg, "\\textbf{Solution 1:}\\\\\n");
    write_solution(solution1, data->table->cols-2, data->headers, lg);

    if (solution2 != NULL) {  
        lg_write(lg, "\\textbf{Solution 2:}\\\\\n");
        write_solution(solution2, data->table->cols-2, data->headers, lg);
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
    free(skip);
    free(solution1);
    return 0;
}

void simplex_data_free(SimplexData *data){
  for (int i = 0; i < data->cols; ++i){
    free(data->headers[i]);
  }
  free(data->headers);
  free(data->big_M);
  free_matrix(data->table);
}
