#ifndef LATEX_GENERATOR_H
#define  LATEX_GENERATOR_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    FILE *file;
    char filename[256];
    char pdfname[256];
} Latex_Generator;

int lg_open(Latex_Generator *lg, char *name);
void lg_close(Latex_Generator *lg);
void lg_generate(Latex_Generator *lg); 
void lg_write(Latex_Generator *lg, const char *format, ...);
void lg_define_colors(Latex_Generator *lg);
void lg_write_simplex_info(Latex_Generator *lg);
void lg_simplex_references(Latex_Generator *lg);
void lg_init(Latex_Generator *lg);

void tex_table_draw(Latex_Generator *lg, int rows, int cols, char **headers, double **content, double *big_M, int *skips);
#endif // LATEX_GENERATOR_H
