#include <stdio.h>
#include <stdlib.h>

#define SIZE_MAT 3
void print_mat(int** mat){
    for (int i = 0; i < SIZE_MAT;i++){
        for (int j = 0; j < SIZE_MAT; j++){
            printf("%d ", mat[i][j]);
        }
        printf("\n");
    }
}

void canonize(int** mat, int rows, int cols, int pivot_row, int pivot_col){

    int k;
    for(int r = 0; r < rows; ++r){
        if(r == pivot_row){
            k = mat[pivot_row][pivot_col];
        } else {
            k = (-mat[r][pivot_col]/mat[pivot_row][pivot_col]);
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

void maximize(int** mat, int rows, int cols){
    int min = MAX_VAL;
    int pivot_row = -1, pivot_col = -1;;

    for(int c = 1; c < cols-1; ++c){
        if(min > mat[0][c]){
            min = mat[0][c];
            pivot_col = c;
        }
    }
    if (min >= 0)
        return;

    for(int r = 1; r < rows; ++r){
        if(mat[r][pivot_col] > 0 && min > mat[r][pivot_col]){

            min = mat[r][pivot_col];
            pivot_row = r;
        }
    }
    if(pivot_row < 0)
        return;
    canonize(mat, rows, cols, pivot_col, pivot_row);

}

int main(int argc, char* args[]){
    printf("Hello World!\n");
    
    int** mat = malloc(sizeof(int*)*SIZE_MAT);

    for(int i = 0; i < SIZE_MAT; i++)
        mat[i] = calloc(sizeof(int), SIZE_MAT);
    
    mat[0][0] = 1;
    mat[1][0] = 2;
    mat[1][1] = 2;
    mat[1][2] = 2;
    mat[2][0] = 2;
    mat[2][1] = 2;
    mat[2][2] = 2;
  
    
    print_mat(mat);
    for(int i = 0; i < SIZE_MAT; i++)
        free(mat[i]);
    free(mat);
    return 0;
}
