#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "latex_generator.h"

char tex_buf[256];
char tex_buf2[4096];
static double tolerance = 1e-4;

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

void tex_table_content(Latex_Generator *lg, int rows, int cols, double  **content, double *big_M){
  tex_buf2[0] = '\0';
  for(int r = 0; r < rows; ++r){
    for(int c = 0; c < cols; ++c){
      if (r != 0){ 
          sprintf(tex_buf2, "%.2lf%s", content[r][c],
              ((c < cols-1) ? (" & ") : ("\\\\ \n \\hline \n")));
      // primera fila contiene las M
      } else {
        if (fabs(big_M[c]) > tolerance && fabs(content[r][c]) > tolerance) {
          sprintf(tex_buf2, "%.2lf*M + %.2lf%s", big_M[c], content[r][c],
              ((c < cols-1) ? (" & ") : ("\\\\ \n \\hline \n")));
        } else if (fabs(big_M[c]) > tolerance){
          sprintf(tex_buf2, "%.2lf*M%s", big_M[c],
              ((c < cols-1) ? (" & ") : ("\\\\ \n \\hline \n")));
        } else {
          sprintf(tex_buf2, "%.2lf%s", content[r][c],
              ((c < cols-1) ? (" & ") : ("\\\\ \n \\hline \n")));
        }
      }
      lg_write(lg, tex_buf2);
    }
  }
  tex_buf2[0] = '\0';
}

void tex_table_headers(Latex_Generator *lg, int cols, char** headers){
  tex_buf2[0] = '\0';
  for(int h = 0; h < cols; ++h){
    sprintf(tex_buf2 + strlen(tex_buf2), "$%s$", headers[h]);
    if(h < cols-1)
      strcat(tex_buf2, " & ");
    else
      strcat(tex_buf2, "\\\\ \n \\hline \n");
  }
  lg_write(lg, tex_buf2);
  tex_buf2[0] = '\0';
}

void tex_table_end(Latex_Generator *lg){
  lg_write(lg, "\\end{tabular} \n");
  lg_write(lg, "\\end{adjustbox}\n");
  lg_write(lg, "\\end{center}\n");
}

void tex_table_draw(Latex_Generator *lg, int rows, int cols, char **headers, double **content, double *big_M){
  tex_table_init(lg, cols);
  tex_table_headers(lg, cols, headers);
  tex_table_content(lg, rows, cols, content, big_M);
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
    fprintf(lg->file, "\\definecolor{PurpleNoMamado}{RGB}{200,100,200}\n");
}

void lg_write_simplex_info(Latex_Generator *lg) {
    fprintf(lg->file, "\\chapter{Simplex Description}\n");
    fprintf(lg->file, "\\section{History}\n");
    fprintf(lg->file,
            "The Simplex algorithm is an algorithm discovered by George Dantzig. "
            "This algorithm is used find the optimal solution for a linear programming problem\\cite{torres2025}."
            "Dantzig was an American mathmatician and Operations Research and Computer Science profesor at the Stanford University."
            "He is called the father of Linear Programming thanks to the revolutionary Simplex algorithm."
            "Before Dantzig it was known that the optimal solution to a linear programming problem could be found in a vertex of the feasible region,"
            "this made it posible to check every single vertex to find the solution but it was too slow to be used in practice."
            "Then George Dantzig proved the feasible region is a convex shape, and after that he easily proposed a greedy algorithm that find the optimal solution to the problem."
            "The property of being convex, allowed for the algorithm to be greedy and still be able to find the best solution."
    );
    fprintf(lg->file,   "\\begin{figure}[H]%%[!ht]\n"
                        "\\begin {center}\n"
                        "\\includegraphics[width=0.7\\textwidth]{DANTZIG.jpeg}\n"
                        "\\caption{Image of George Dantzig}\n"
                        "\\label{fig:dantzig}\n"
                        "\\end {center}\n"
                        "\\end{figure}\n"
    );
    fprintf(lg->file, "\\section{Description}\n");
    fprintf(lg->file,
            "The Simplex algorithm uses matrix operations to find the optimal solution to a linear programming problem, or to find out if it does not have a feasible solution \\cite{torres2025}."
            "A linear programming problem has desition variables and restrictions under them, so you would think that you could solve the problem simply by using a normal method to find the value of the variables."
            "Sadly, in these types of problems thre are usually far more varaibles than equations, to this aproach is not good enough."
            "Using the equations you can find the feasible region of the problem, where all the posible solutions are."
            "Simplex starts from a vertex of the feasible region and jumps to adjacent vertices that give a better value for the objective function."
            "Once you can not find a vertex that upgrades the objective value, that is the optimal solution.\\\\\n"
            "The simplex algorithm has a set of variables that are called basic, these variables can be found in canonical columns \\cite{torres2025}."
            "The first step is to level out any inecuations with slack or excess variables depending on what you need,"
            "when one of the restrictions is equals or greater than you will also need to add artificial variables, because these forms of restrictions do not give enough basic variables."
            "After leveling out the restrictions, you need to clear out the z variable from the objective function, this will give you an extra equation to work with.\\\\\n"
            "Once you have made that you can build the simplex table, giving each variable a column and each equation a row."
            "To maximize the objective value you will find the smallest negative number in the first row (or the biggest positive number if you are minimizing)."
            "If you did not find any numbers that meet the criteria, congratulations, you found the optimal solution."
            "However if you did, then you need to divide all the numbers in that column by the numbers of the final column (ignore negative numbers and 0's) and choose the smallest fraction to meet all the restrictions, the number on the column and row is called the pivot."
            "Once you have a pivot you need to canonize the column, keep in mid that in this process you want the pivot to be the one of the canonical column, this process is called pivoting."
            "Once you have canonized the column, repeat the process until you have ended \\cite{torres2025}.\\\\\n"
            "Sadly, not every problem is as simplex(ba-dum-tss), if in the process of calculating the fractions you do not find any positive numbers, you have found an unbounded problem and it indicates that the objective value can keep growing to infinity, this usually indicates that you might be lacking a restriction."
            "If at the end of a problem you see a non-basic variable that has a 0 in it's column it means that if you canonize that column with the apropiate pivot, the objective value will not go down, this indicates that the problem has multiple optimal solutions and you can choose any solution in between the initial two you found."
            "If at any point of the execution of the algorithm you find a draw between which pivot to choose you have found yourself a degenerate problem, these are (in most cases) harmless, and you might see that the objective value does not upgrade in an iteration, do not worry, just keep pivoting."
            "However, if you are really unlucky, you might find a degenerate problem that makes the algorithm enter an infinite loop, in these cases keep an eye on the tables, if they are repeating, you are indeed in a loop."
            "But managing the extremely rare cases of the program entering a loop can be extremely expensive because you do not know when this will happen so you need to store every table of the execution, in these cases the usual solution is to look the other way and if a problem is taking too long to solve, just cut it's execution."
    );
    fprintf(lg->file, "\\section{Big M Algorithm}\n");
    fprintf(lg->file,   "The description of the simplex algorithm given in the last section seems simple enough, but it has a pretty big problem.\n"
                        "The explanation given ealier only supports linear programming problems that have restrictions of less or equal, meaning that it cannot solve the ones that use greater or equal and equals.\n"
                        "This problem arrives because of how you turn the inequations into equations, in less or equal restrictions you add some value as a slack variable, sice it sums to the restriction it is positive and thus produces a 1 in its position and a 0 in all other restrictions, making a canonical column.\n"
                        "In greater or equal restrictions you use an excess variable, which subtracts from the restriction and thus produces a -1 on it's position, because of this it does not produces a canonical column\n"
                        "Furthermore, equals restrictions do not have the need of extra variables, and thus they do not give canonical vectors.\n"
                        "The problem here is that Simplex needs enough canonical vector to find the solution, but these types of restrictions do not produce canonical columns.\\\\\n"
                        "To deal with this you use the Big M Algorithm, this algorithm simply introduces the canonical vectors needed, but it has a catch, when adding these, you need to add their value in the objective function as a M (-M if you are maximizind and +M if you are minimizing).\n"
                        "Every restriction of the equals of greater of equal types, add a new artificial variable in order to have enough canonical vectors, when adding these to the table you will see that these vectors are not canonical, because they have M's in the first row, so you need to tackle this before anything.\n"
                        "Once you build the simplex table, before execution simplex you need to canonize the columns of the artificial variables, you will see that other variables are starting to be filled with M's, but do not panic, this is completely normal.\n"
                        "Once you have canonized all the columns of the artificial variables, you are free to use the Simplex algorithm, taking into accout that the M's should be interpreted as a very big value, a value so bigger than any other you have in the table.\n"
                        "During the simplex execution you might see that the M's are starting to move out of the original variables and that they are staying in the artificial ones, which is good news.\n"
                        "And take into account that once an artificial variable exits the base, it will never enter it again.\n"
                        "So, what does the M means?\n"
                        "As said before, the M is a value you can interpret as bigger than any other value in the table, and it adds dimensions to the problem to move around in them and hopefully find a solution that does not need the extra dimensions.\\\\\n"
                        "When using this algorithm you might see problems that never got the artificial variables out of their solution, or that still have M's but you cannot canonize it any further.\n "
                        "When this happens, it means the solution is in another dimension, this means that the solution is out of the dimension of the original problem, and thus it means that the original problem has no feasible solution.\n"
    );

}

void lg_simplex_references(Latex_Generator *lg){
    fprintf(lg->file, "\\begin{thebibliography}{9}\n");
    fprintf(lg->file, "\\bibitem{torres2025}\n"
                      "F. Torres-Rojas, 'class about the simplex algorithm', personal "
                      "communication, Investigación de Operaciones, Instituto Tecnológico de "
                      "Costa Rica, San José, Costa Rica, Sep. 10, 2025.\n");
    fprintf(lg->file,"\\end{thebibliography}");
}

void lg_init(Latex_Generator *lg) {
fprintf(lg->file, "\\documentclass[12pt,a4paper]{report}\n"); 

    fprintf(lg->file, "\\usepackage{listings}\n");
    fprintf(lg->file, "\\usepackage{hyperref}\n");
    fprintf(lg->file, "\\usepackage{graphicx}\n");
    fprintf(lg->file, "\\usepackage{float}\n");
    fprintf(lg->file, "\\usepackage{url}\n");
    fprintf(lg->file, "\\usepackage{tikz}\n");
    fprintf(lg->file, "\\usepackage{pgfplots}\n");
    fprintf(lg->file, "\\usepgfplotslibrary{fillbetween}\n");

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
    fprintf(lg->file, "\\def \\titulo{Project 05}\n");
    fprintf(lg->file, "\\def \\subtitulo{Otro SIMPLEX más - Parte 2}\n");
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
