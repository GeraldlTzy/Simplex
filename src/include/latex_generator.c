#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "latex_generator.h"

char tex_buf[256];
char tex_buf2[4096];

void tex_table_init(Latex_Generator *lg, int cols){
  tex_buf2[0] = '\0';
  for (int c = 0; c < cols; ++c){
    strcat(tex_buf2, "|c");
  }
  strcat(tex_buf2, "|");
  lg_write(lg, "\\begin{center}\n");
  lg_write(lg, "\\begin{adjustbox}{max width=1\\textwidth,keepaspectratio}\n");
  lg_write(lg, "\\begin{tabular}{%s} \n \\hline \n", tex_buf2);
  tex_buf2[0] = '\0';
}

void tex_table_content(Latex_Generator *lg, int rows, int cols, double  **content){
  tex_buf2[0] = '\0';
  for(int r = 0; r < rows; ++r){
    for(int c = 0; c < cols; ++c){
      sprintf(tex_buf2, "%d%s", content[r][c],
              ((c < cols-1) ? (" & ") : ("\n \\hline \n")));
    }
  }
  tex_buf2[0] = '\0';
  lg_write(lg, tex_buf2);
}

void tex_table_headers(Latex_Generator *lg, int cols, char** headers){
  tex_buf2[0] = '\0';
  for(int h = 0; h < cols; ++h){
    strcat(tex_buf2, headers[h]);
    if(h < cols-1)
      strcat(tex_buf2, " & ");
    else
      strcat(tex_buf2, " \n \\hline \n");
  }
  lg_write(lg, tex_buf2);
  tex_buf2[0] = '\0';
}

void tex_table_end(Latex_Generator *lg){
  lg_write(lg, "\\end{tabular} \n");
  lg_write(lg, "\\end{adjustbox}\n");
  lg_write(lg, "\\end{center}\n");
}

void tex_table_draw(Latex_Generator *lg, int rows, int cols, char **headers, double **content){
  tex_table_init(lg, cols);
  tex_table_headers(lg, cols, headers);
  tex_table_content(lg, rows, cols, content);
  tex_table_end(lg);
}


int lg_open(Latex_Generator *lg, char *name) {
    lg->filename[0] = '\0';
    lg->pdfname[0] = '\0';
    snprintf(lg->filename, sizeof(lg->filename), "%s.tex", name);
    snprintf(lg->pdfname, sizeof(lg->pdfname), "%s.pdf", name);

    lg->file = fopen(lg->filename, "w");
    if (!lg->file) return 0;

    return 1; 
}

void lg_close(Latex_Generator *lg) {
    if (!lg->file) return;
    fprintf(lg->file, "\\end{document}");
    fclose(lg->file);
    lg->file = NULL;
}

void lg_generate(Latex_Generator *lg) {
    char command[256];
    snprintf(command, sizeof(command), "pdflatex -output-directory=LaTeX %s", lg->filename);
    system(command);
    system(command);
    snprintf(command, sizeof(command), "evince --presentation --page-label=1 %s &", lg->pdfname);
    system(command);
} 

void lg_write(Latex_Generator *lg, const char *format, ...) {
    if (!lg->file) return;
    va_list args;
    va_start(args, format);
    vfprintf(lg->file, format, args);
    va_end(args);
}

void lg_define_colors(Latex_Generator *lg){
    fprintf(lg->file, "\\definecolor{DarkPurpleMamado}{RGB}{128,0,128}\n");
    fprintf(lg->file, "\\definecolor{PurpleNoMamado}{RGB}{128,70,128}\n");
}

void lg_write_simplex_info(Latex_Generator *lg) {
    fprintf(lg->file, "\\chapter{Simplex Description}\n");
    fprintf(lg->file, "The simplex algorithm is a very simple algorithm.\n");
}

void lg_simplex_references(Latex_Generator *lg){
    /*fprintf(lg->file, "\\begin{thebibliography}{9}\n");
    fprintf(lg->file, "\\bibitem{torres2025}\n"
                      "F. Torres-Rojas, 'class about the equipment replacement', personal "
                      "communication, Investigación de Operaciones, Instituto Tecnológico de "
                      "Costa Rica, San José, Costa Rica, Aug. 29, 2025.\n");
    fprintf(lg->file,"\\end{thebibliography}");*/
}

void lg_init(Latex_Generator *lg) {
fprintf(lg->file, "\\documentclass[12pt,a4paper]{report}\n"); 

    fprintf(lg->file, "\\usepackage{listings}\n");
    fprintf(lg->file, "\\usepackage{hyperref}\n");
    fprintf(lg->file, "\\usepackage{graphicx}\n");
    fprintf(lg->file, "\\usepackage{float}\n");
    fprintf(lg->file, "\\usepackage{url}\n");
    //fprintf(lg->file, "\\usepackage{tikz}\n");
    //fprintf(lg->file, "\\usetikzlibrary{tikzmark, calc}\n");

    fprintf(lg->file, "\\usepackage[dvipsnames]{xcolor}\n");
    fprintf(lg->file, "\\usepackage[table]{xcolor}\n");
    fprintf(lg->file, "\\usepackage{amsmath}\n");
    fprintf(lg->file, "\\usepackage{breqn}\n");
    fprintf(lg->file, "\\usepackage{adjustbox}\n");
    fprintf(lg->file, "\\usepackage{makecell}\n");
    fprintf(lg->file, "\\usepackage[a4paper, margin=2cm]{geometry}\n");
    
    lg_define_colors(lg);
    fprintf(lg->file, "\\lstset{\n");
    fprintf(lg->file, "    language=C,\n");
    fprintf(lg->file, "    basicstyle=\\footnotesize,\n");
    fprintf(lg->file, "    numbers=left,\n");
    fprintf(lg->file, "    stepnumber=1,\n");
    fprintf(lg->file, "    showstringspaces=false,\n");
    fprintf(lg->file, "    tabsize=1,\n");
    fprintf(lg->file, "    breaklines=true,\n");
    fprintf(lg->file, "    breakatwhitespace=false,\n");
    fprintf(lg->file, "}\n\n");

    fprintf(lg->file, "\\def \\unidad{Escuela de Ingeniería en Computación}\n");
    fprintf(lg->file, "\\def \\programa{Ingeniería en Computación}\n");
    fprintf(lg->file, "\\def \\curso{IC-6400 - Investigación de Operaciones}\n");
    fprintf(lg->file, "\\def \\titulo{Project 04}\n");
    fprintf(lg->file, "\\def \\subtitulo{Simplex Report}\n");
    fprintf(lg->file, "\\def \\autores{\n");
    fprintf(lg->file, "    Gerald Calderón Castro\\\\\n");
    fprintf(lg->file, "    gecalderon@estudiantec.cr\\\\\n");
    fprintf(lg->file, "    2023125197\\\\\n\n");
    fprintf(lg->file, "    \\vspace{0.5cm}\n\n");
    fprintf(lg->file, "    Óscar Obando Umaña\\\\\n");
    fprintf(lg->file, "    osobando@estudiantec.cr\\\\\n");
    fprintf(lg->file, "    2023091684\n");
    fprintf(lg->file, "}\n");
    fprintf(lg->file, "\\def \\fecha{November 2025}\n");
    fprintf(lg->file, "\\def \\lugar{San José , Costa Rica}\n\n");

    fprintf(lg->file, "\\begin{document}\n\n");

    fprintf(lg->file,
        "\\begin{titlepage}\n"
        "    \\begin{center}\n"
        "        \\vspace*{1cm}\n\n"
        "        %% La imagen está en el mismo directorio que el .tex\n"
        "        \\includegraphics[width=0.8\\linewidth]{%s}\\\\\n\n"
        "        \\LARGE\n"
        "        \\unidad\\\\\n"
        "        \\programa\\\\\n"
        "        \\curso\n\n"
        "        \\vspace{1cm}\n\n"
        "        \\Huge\n"
        "        \\textbf{\\titulo}\n\n"
        "        \\vspace{0.5cm}\n"
        "        \\LARGE\n"
        "        \\subtitulo\n\n"
        "        \\vspace{1.5cm}\n\n"
        "        \\large\n"
        "        \\autores\n\n"
        "        \\vfill\n\n"
        "        \\lugar\\\\\n"
        "        \\fecha\n\n"
        "    \\end{center}\n"
        "\\end{titlepage}\n",
        "logo_tec.jpg"
    );

    fprintf(lg->file, "\\tableofcontents\n\n");

    lg_write_simplex_info(lg);
}
