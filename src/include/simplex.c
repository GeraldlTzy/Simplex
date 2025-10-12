#include <stdio.h>
#include <stdlib.h>
#include "simplex.h"
#include "matrix.h"
#define MAX_VAL 200
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


void maximize(Matrix mat){
  while(1){
    double min = MAX_VAL;
    int pivot_row = -1, pivot_col = -1;;

    for(int c = 1; c < mat.cols-1; ++c){
      if(min > mat.data.f[0][c]){
        min = mat.data.f[0][c];
        pivot_col = c;
      }
    }
    if (min >= 0){
      break;
    }
    min =  MAX_VAL;
    double fraction;
    for(int r = 1; r < mat.rows; ++r){
      if(mat.data.f[r][pivot_col] > 0){
        fraction = mat.data.f[r][mat.cols-1] / mat.data.f[r][pivot_col];
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
    canonize(mat, pivot_row, pivot_col);
  }
}

int simplex(Matrix mat){
    print_matrix(mat);
    maximize(mat);
    print_matrix(mat);
    return 0;
}
