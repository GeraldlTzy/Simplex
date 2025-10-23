#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "simplex.h"
#include "matrix.h"
#include <glib.h>
int pivot_counter = 0;

void canonize(Matrix mat, int pivot_row, int pivot_col){
    double k;
    printf("piv_r: %d, piv_c: %d\n", pivot_row, pivot_col);
    for(int r = 0; r < mat.rows; ++r){
        if(r == pivot_row){
            k = mat.data.f[pivot_row][pivot_col];
        } else {
            k = ((-1*mat.data.f[r][pivot_col])/mat.data.f[pivot_row][pivot_col]);
        }
        for(int c = 0; c < mat.cols; ++c){
            if(r == pivot_row){
                mat.data.f[r][c] /= k;
            } else {
              mat.data.f[r][c] += (mat.data.f[pivot_row][c] * k);
            }
        }
    }
}


typedef struct{
  Matrix *mat;
  int pv_r;
  int pv_c;
} Node;

GList *list = 0;

int list_contain(Matrix mat){
  Node *curr;
  for (GList *l = list; l != NULL; l = l->next) {
    curr = (Node *)l->data;

    if(matrix_compare(mat, *(curr->mat))){ // si ya esta es que se enciclo, entonces nos cambiamos a la otra tabla posible
      return 1;
      /*list = g_list_remove(list, curr);
      GList *last = g_list_last(list);
      if(!last) return 1;

      mat = (Matrix) last->data->mat;
      pv_r = (int) last->data->pv_r;
      pv_c = (int) last->data->pv_c;

      list = g_list_delete_link(list, last);*/
    }
  }
  return 0;
}

Node *get_last_state(){
  Node *last = (Node *) (g_list_last(list))->data;
  list = g_list_delete_link(list, g_list_last(list));
  return last;
}

void maximize_simplex(Matrix mat){
  Matrix *init = matrix_copy(&mat);
  int ids = 0;
  while(1){
    if(ids == 25) break;
    
    double min = DBL_MAX;
    int pivot_row = -1, pivot_col = -1;;
    
    ids++;
    printf("valid> %d iguales = %d\n", list_contain(mat));
    
    printf("#################################\n");
    for (GList *l = list; l != NULL; l = l->next) {
      Node * curr = (Node *)l->data;
      print_matrix(*(curr->mat));
    }
    printf("#################################\n");

    // no entiendo muy bien este if, es para validar los loops? Tal vez
    if(list_contain(mat) || (matrix_compare(mat, *init) && ids != 1)){
      printf("DEBERIAAA ENTRAR\n");

      Node *node = get_last_state();
      if(!node){
        printf("Loop\n");
        break;
      }
      mat = *(node->mat);
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
      print_matrix(mat);
      printf("NUEVA CARGADA\n");
    } else {
    
    // elegir el mas negativo
    for(int c = 1; c < mat.cols-1; ++c){
      if(min > mat.data.f[0][c]){
        min = mat.data.f[0][c];
        pivot_col = c;
      }
    }
    // ya no hay negativos
    if (min >= 0){
      break;
    }
    // reseta para usarlo en las fracciones
    min =  DBL_MAX;
    double fraction;
    // fracciones opara ver cual elegirt
    for(int r = 1; r < mat.rows; ++r){
      if(mat.data.f[r][pivot_col] > 0){
        fraction = mat.data.f[r][mat.cols-1] / mat.data.f[r][pivot_col];
        if(min > fraction){
          min = fraction;
          pivot_row = r;
        } else if(min == fraction){
          printf("Degenerado: Empate  %d, %d\n", r, pivot_col);
          Node *node = malloc(sizeof(Node));
          node->mat = matrix_copy(&mat);
          node->pv_r = r;
          node->pv_c = pivot_col;
          list = g_list_append(list, node);
        }
      }
    }
    // si nunca se setea el pivote
    if(pivot_row < 0){
      printf("No acotado\n");
      if(list){
        Node *node = get_last_state();
        if(!node){
          printf("Loop\n");
        }
        mat = *(node->mat);
        pivot_row = node->pv_r;
        pivot_col = node->pv_c;
      } else {
        break;
      }
    }
    }
    pivot_counter++;
    printf("Pivoteo(%d)\n", pivot_counter);
    canonize(mat, pivot_row, pivot_col);
    print_matrix(mat);
  }
}

void minimize_simplex(Matrix mat){
  Matrix *init = matrix_copy(&mat);
  int ids = 0;
  while(1){
    if(ids == 25) break;
   
    /* 
     * NOTA EXTRANA:
     * parece que DBL_MIN no es el numero negativo mas pequeno representable por float 
     * es el numero POSITIVO mas peque;o representable por float 
     *
     * entonces cuando se usaba DBL_MIN no servia por eso 
     * si se quiere el numero negativo mas negativo representable por float se usa -DBL_MAX
     * */
    double min_max = -DBL_MAX;
    int pivot_row = -1, pivot_col = -1;;
    
    ids++;
    printf("valid> %d iguales = %d\n", list_contain(mat));
    
    printf("#################################\n");
    for (GList *l = list; l != NULL; l = l->next) {
      Node * curr = (Node *)l->data;
      print_matrix(*(curr->mat));
    }
    printf("#################################\n");

    if(list_contain(mat) || (matrix_compare(mat, *init) && ids != 1)){

      printf("DEBERIAAA ENTRAR\n");

      Node *node = get_last_state();
      if(!node){
        printf("Loop\n");
        break;
      }
      mat = *(node->mat);
      pivot_row = node->pv_r;
      pivot_col = node->pv_c;
      print_matrix(mat);
      printf("NUEVA CARGADA\n");
    } else {

    // elegir el mas positivo
    for(int c = 1; c < mat.cols-1; ++c){
      if(min_max < mat.data.f[0][c]){
        min_max = mat.data.f[0][c];
        pivot_col = c;
      }
    }
    // ya no hay positivos
    if (min_max <= 0){
      break;
    }
    // esta vez se usa para elegir la fraccion minima, igual que antes
    min_max =  DBL_MAX;
    double fraction;
    for(int r = 1; r < mat.rows; ++r){
      if(mat.data.f[r][pivot_col] > 0){
        fraction = mat.data.f[r][mat.cols-1] / mat.data.f[r][pivot_col];
        if(min_max > fraction){
          min_max = fraction;
          pivot_row = r;
        } else if(min_max == fraction){
          printf("Degenerado: Empate  %d, %d\n", r, pivot_col);
          Node *node = malloc(sizeof(Node));
          node->mat = matrix_copy(&mat);
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
        mat = *(node->mat);
        pivot_row = node->pv_r;
        pivot_col = node->pv_c;
      } else {
        break;
      }
    }
    }
    pivot_counter++;
    printf("Pivoteo(%d)\n", pivot_counter);
    canonize(mat, pivot_row, pivot_col);
    print_matrix(mat);
  }
}

int simplex(Matrix mat, int do_minimize){
    print_matrix(mat);
    if (do_minimize) minimize_simplex(mat);
    else maximize_simplex(mat);
    print_matrix(mat);
    return 0;
}
