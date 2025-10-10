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

void canonize(){
    mat[r][c] += (mat[pivot_row][c] / -mat[r][c])
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
