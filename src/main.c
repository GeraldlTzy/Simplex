#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <fontconfig/fontconfig.h>
#include <ctype.h>
#include <string.h>
#include "include/simplex.h"
#include "include/matrix.h"
#include "include/utils.h"
#include "include/latex_generator.h"
#include <locale.h>

#define NAME_SIZE 32

GtkWidget* main_window;
GtkWidget* second_window;
GtkWidget* varname_window;
GtkBuilder* builder;
GtkWidget* cmb_objective_func;
GtkWidget* vp_objective_func;
GtkWidget* vp_constraints;
GtkWidget* vp_varnames;
GtkGrid* gd_variables;
GtkGrid* gd_constraints;
GtkGrid* gd_varnames;
GtkToggleButton* intermediate_toggle;

int loaded = 0;
int do_minimize = 0;
char **var_names;
char **headers;
SimplexData *simplex_data;

Latex_Generator l;
Latex_Generator *lg = &l;
GtkListStore *inequalities;
char **ineq_arr;
void initialize(){
	//////////////////////////////// Define the variables
	builder = gtk_builder_new_from_file("ui/main.glade");
	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  cmb_objective_func = GTK_WIDGET(gtk_builder_get_object(builder, "cmb_objective_func"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_objective_func), 0);
  varname_window = GTK_WIDGET(gtk_builder_get_object(builder, "varname_window"));
	second_window = GTK_WIDGET(gtk_builder_get_object(builder, "second_window"));
	vp_objective_func = GTK_WIDGET(gtk_builder_get_object(builder, "vp_objective_func"));
	vp_constraints = GTK_WIDGET(gtk_builder_get_object(builder, "vp_constraints"));
  vp_varnames = GTK_WIDGET(gtk_builder_get_object(builder, "vp_varnames"));
  intermediate_toggle = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "ckbtn_intermediate_tables"));
  inequalities = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  GtkTreeIter iter;
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, "<=", 1, TRUE, -1);
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, "==", 1, FALSE, -1);
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, ">=", 1, FALSE, -1);
  
  gtk_window_set_title(GTK_WINDOW(main_window), "Dynamic Programming Algorithms Hub");
  g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(second_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_builder_connect_signals(builder, NULL);
    //////////////////////////////// latex generator 
}


int main(int argc, char *argv[]){
	//Inicializaciones
  setlocale(LC_NUMERIC, "en_US.UTF-8");
	FcInit();
	gtk_init(&argc, &argv);

  initialize();
	////////////////////////////////// We show the gui
	gtk_widget_show(main_window);
	gtk_main();
	/////////////////////////////////
	return 0;
}

char problem_name[256];
int num_variables;
int num_constraints;

void on_combo_constraint_changed(GtkComboBox *cmb, gpointer user_data){
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(user_data);
  GtkWidget *entry = gtk_bin_get_child(GTK_BIN(cmb));
  if(gtk_combo_box_get_active_iter(cmb, &iter)){
    gboolean active;
    gtk_tree_model_get(model, &iter, 1, &active, -1);//Modelo, indice, devuelvalo aca pa, termina pa
    if(!active){
      gtk_combo_box_set_active(cmb, 0);
      //gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
  }
}

void validate_int(GtkEntry* entry, int max, int min){
    const char* str = gtk_entry_get_text(GTK_ENTRY(entry));

    int has_number = 0;
    for (int i = 0; str[i] != '\0'; i++){
	    if (!isdigit(str[i])) {
	        gtk_entry_set_text(GTK_ENTRY(entry), "");
	        return;
	    }
	    if (str[i] != '0') has_number = 1;
    }

    if (!has_number) {
	    gtk_entry_set_text(GTK_ENTRY(entry), "");
	    return;
    }

    if (atoi(str) > max || atoi(str) < min) {
	    gtk_entry_set_text(GTK_ENTRY(entry), "");
        return;
    }
}

void on_entry_varname_clicked(GtkEntry *e, gpointer user_data) {
    int index = GPOINTER_TO_INT(user_data);
    const char *str = gtk_entry_get_text(e);

    if (strlen(str) >= NAME_SIZE) {
        sprintf(var_names[index], "x_{%d}", index);
        gtk_entry_set_text(e, var_names[index]);
        return;
    }

    if (strcmp(str, "") == 0) {
        sprintf(var_names[index], "x_{%d}", index);
        return; 
    }

    sprintf(var_names[index], str);
}

void on_btn_continue_clicked(GtkButton *b, GtkGrid* gd){
  
  validate_int(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 1)), 15, 2);
  validate_int(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 2)), 15, 2);

  strcpy(problem_name, gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 0))));
  num_variables = atoi(gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 1))));
  num_constraints = atoi(gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 2))));

  
  if (problem_name[0] == '\0' || num_variables == 0 || num_constraints == 0) return;
  
  var_names = malloc(sizeof(char *) * num_variables);
  for (int i = 0; i < num_variables; ++i)
    var_names[i] = malloc(sizeof(char)*NAME_SIZE);
  gd_varnames = GTK_GRID(gtk_grid_new());
  for(int v = 0; v < num_variables; ++v){
    GtkWidget *entry = gtk_entry_new();
    sprintf(var_names[v], "x_{%d}", v+1);
    GtkWidget *label = gtk_label_new(var_names[v]);
    gtk_entry_set_text(GTK_ENTRY(entry), var_names[v]);
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_widget_set_hexpand(entry, TRUE);
    g_signal_connect(entry, "changed", G_CALLBACK(on_entry_varname_clicked), GINT_TO_POINTER(v));

    gtk_grid_attach(gd_varnames, label, 0, v, 1, 1);
    gtk_grid_attach(gd_varnames, entry, 1, v, 1, 1);
  }
  gtk_grid_set_column_spacing(gd_varnames, 5);
  gtk_container_add(GTK_CONTAINER(vp_varnames), GTK_WIDGET(gd_varnames));
 
  
  gtk_widget_hide(main_window);
  gtk_widget_show_all(varname_window);
}

void on_btn_var_back_clicked() {
    GtkGrid* gd = GTK_GRID(gtk_builder_get_object(builder, "gd_initial"));
    gtk_entry_set_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 0)), "");
    gtk_entry_set_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 1)), "");
    gtk_entry_set_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 2)), "");
    for (int i = 0; i < num_variables; ++i) free(var_names[i]);
    free(var_names);
    gtk_widget_destroy(GTK_WIDGET(gd_varnames));
    gtk_widget_hide(varname_window);
    gtk_widget_show_all(main_window);
}

void on_btn_var_continue_clicked(){
  gd_variables = GTK_GRID(gtk_grid_new());
  GtkWidget *entry, *label;
  char buff[256];
  int col_i = 0;
  for(int v = 0; v < num_variables; ++v){
    entry = gtk_entry_new();
    sprintf(buff, "%s %s", var_names[v], ((v < num_variables-1) ? "+ " : ""));
    label = gtk_label_new(buff);
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_entry_set_text(GTK_ENTRY(entry), "0");
    // TODO: conectar a una signal que valide el numero
    gtk_grid_attach(gd_variables, entry, col_i++, 0, 1, 1);
    gtk_grid_attach(gd_variables, label, col_i++, 0, 1, 1);
  }
 
  GtkWidget *cmb;
  gd_constraints = GTK_GRID(gtk_grid_new());


  col_i = 0;
  for(int c = 0; c < num_constraints; ++c){
    for(int x = 0; x < num_variables; ++x){
      entry = gtk_entry_new();
      sprintf(buff, "%s %s", var_names[x], ((x < num_variables-1) ? "+ " : ""));
      label = gtk_label_new(buff);
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
      gtk_widget_set_hexpand(entry, TRUE);
      gtk_entry_set_text(GTK_ENTRY(entry), "0");
      // TODO: conectar a una signal que valide el numero
      gtk_grid_attach(gd_constraints, entry, col_i++, c, 1, 1);
      gtk_grid_attach(gd_constraints, label, col_i++, c, 1, 1);
    }

    cmb = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(inequalities));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(cmb), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), 0);

    g_signal_connect(cmb, "changed", G_CALLBACK(on_combo_constraint_changed), inequalities);

    entry = gtk_bin_get_child(GTK_BIN(cmb));
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(entry, FALSE);
    gtk_grid_attach(gd_constraints, cmb, col_i++, c, 1, 1);
    entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_widget_set_hexpand(entry, TRUE);
    // TODO: conectar a una signal que valide el numero, debe ser mayor o igual a 0
    gtk_entry_set_text(GTK_ENTRY(entry), "0");
    gtk_grid_attach(gd_constraints, entry, col_i++, c, 1, 1);
    col_i=0;
  }

  gtk_grid_set_column_spacing(gd_variables, 5);
  gtk_grid_set_column_spacing(gd_constraints, 5);

  gtk_container_add(GTK_CONTAINER(vp_objective_func), GTK_WIDGET(gd_variables));
  gtk_container_add(GTK_CONTAINER(vp_constraints), GTK_WIDGET(gd_constraints));

  gtk_widget_hide(varname_window);
  gtk_widget_show_all(second_window);
}

void prepare_simplex_lg(){
    lg_write(lg, "\\chapter{Solving %s}\n", problem_name);
    lg_write(lg, "\\section{Mathematical representation}\n");
    char to_write[1024];
    to_write[0] = '\0';
    char buffer[256];
    buffer[0] = '\0';
    if (do_minimize)
        lg_write(lg, "\\textbf{Minimize:}\n", to_write);
    else
        lg_write(lg, "\\textbf{Maximize:}\n", to_write);

    strcpy(to_write, "z = ");
    GtkWidget *entry, *label, *cmb;
    int col_i = 0;
    const char *str;
    // Funcion Objetivo
    for(int v = 0; v < num_variables; ++v){
        entry = gtk_grid_get_child_at(gd_variables, col_i++, 0);
        label = gtk_grid_get_child_at(gd_variables, col_i++, 0);
        str = gtk_entry_get_text(GTK_ENTRY(entry));
        strcat(to_write, str);
        str = gtk_label_get_text(GTK_LABEL(label));
        strcat(to_write, str);
    }
    lg_write(lg, "\\begin{dmath}\n");
    lg_write(lg, "%s\n", to_write);
    lg_write(lg, "\\end{dmath}\n");

    //Contraints
    lg_write(lg, "\\textbf{Subject To:}\n", to_write);
    for (int i = 0; i < num_constraints; ++i){
        col_i = 0;
        strcpy(to_write, "");
        for (int j = 0; j < num_variables; ++j){
            entry = gtk_grid_get_child_at(gd_constraints, col_i++, i);
            label = gtk_grid_get_child_at(gd_constraints, col_i++, i);
            str = gtk_entry_get_text(GTK_ENTRY(entry));
            strcat(to_write, str);
            str = gtk_label_get_text(GTK_LABEL(label));
            strcat(to_write, str);
        }
        cmb = gtk_grid_get_child_at(gd_constraints, col_i++, i);
        entry = gtk_bin_get_child(GTK_BIN(cmb));
        str = gtk_entry_get_text(GTK_ENTRY(entry));
        if (strcmp(str, "<=") == 0) {
            strcat(to_write, "\\leq");
        } else if (strcmp(str, ">=") == 0){
            strcat(to_write, "\\geq");
        } else {
            strcat(to_write, "=");
        }

        entry = gtk_grid_get_child_at(gd_constraints, col_i++, i);
        str = gtk_entry_get_text(GTK_ENTRY(entry));
        strcat(to_write, str);

        lg_write(lg, "\\begin{dmath}\n");
        lg_write(lg, "%s\n", to_write);
        lg_write(lg, "\\end{dmath}\n");
    }
}
void simplex_data_put_inequalities(SimplexData *simplex_data){
  ineq_arr = malloc(sizeof(char *) * num_constraints);
  GtkWidget *cmb; 
  GtkEntry *entry;
  
  for (int i = 0; i < num_constraints; ++i){
    ineq_arr[i] = malloc(3);
    cmb = gtk_grid_get_child_at(gd_constraints, num_variables*2, i);
    entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cmb)));
    const gchar *ineq = gtk_entry_get_text(entry);
    strcpy(ineq_arr[i], ineq);
    if (strcmp(ineq, "<=") == 0) {simplex_data->slacks++;}
    else if (strcmp(ineq, "=") == 0) {simplex_data->slacks++;}
    else if (strcmp(ineq, ">=") == 0) {simplex_data->artificials++; simplex_data->excess++;}
    printf("Rest: %d, %s\n", i, ineq_arr[i]);
  }
  simplex_data->rows = 1 + num_constraints;
  simplex_data->cols = ((2 + num_variables) + (simplex_data->slacks +
                        simplex_data->artificials + simplex_data->excess));
}
char **simplex_data_put_headers(SimplexData *data){
  char **headers = malloc(sizeof(char *) * data->cols);
  int i, acc = 0;
  
  headers[acc] = malloc(2);
  strcpy(headers[acc++], "Z");
  
  for (i = 0; i < data->variables; i++){
    headers[acc] = malloc(NAME_SIZE);
    strcpy(headers[acc++], var_names[i]);
  }
  for (i = 0; i < data->slacks; i++){
    headers[acc] = malloc(8);
    sprintf(headers[acc++], "s_{%d}", i+1);
  }
  for (i = 0; i < data->excess; i++){
    headers[acc] = malloc(8);
    sprintf(headers[acc++], "e_{%d}", i+1);
  }
  for (i = 0; i < data->artificials; i++){
    headers[acc] = malloc(8);
    sprintf(headers[acc++], "a_{%d}", i+1);
  }
  printf("termina\n");
  headers[acc] = malloc(2);
  strcpy(headers[acc], "B");
  for(i = 0; i < data->cols; ++i)
    printf("%s \t", headers[i]);
  printf("\n");
  data->headers = headers;
}
void on_btn_finish_clicked(){
  simplex_data = malloc(sizeof(SimplexData));
  simplex_data->slacks = 0;
  simplex_data->artificials = 0;
  simplex_data->excess = 0;
  simplex_data->rows = 0;
  simplex_data->cols = 0;
  simplex_data->variables = num_variables;
  simplex_data->minimize = do_minimize;
  simplex_data->show_intermediates = gtk_toggle_button_get_active(intermediate_toggle);
  simplex_data_put_inequalities(simplex_data);
  simplex_data_put_headers(simplex_data);


  int rows = simplex_data->rows;
  int cols = simplex_data->cols;
  simplex_data->table = new_matrix(rows, cols, FLOAT);
  init_matrix_num(simplex_data->table, 0);

  double **table = simplex_data->table->data.f;
  table[0][0] = 1;
  table[0][cols-1] = 0;
      print_matrix(simplex_data->table);
  
  int canonic_i = num_variables + 1;
  GtkWidget *entry;
  
  for(int r = 0; r < rows; ++r){
    for(int c = 1; c < cols; ++c){
      if(r == 0 && c <= num_variables){
        entry = gtk_grid_get_child_at(gd_variables, (c-1)*2, r);      
        table[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry))) * -1;
      } else if(c <= num_variables){
        entry = gtk_grid_get_child_at(gd_constraints, (c-1)*2, (r-1));
        table[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      } else if(r > 0 && c == canonic_i){
        table[r][c] = 1;
      } else if(r > 0 && c == cols-1){
        entry = gtk_grid_get_child_at(gd_constraints, num_variables*2+1, (r-1));
        table[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      }
    }
      print_matrix(simplex_data->table);
    if(r > 0){
      canonic_i++;
    }
  }
  if (!lg_open(lg, "LaTeX/Simplex_Report")) {
    perror("Error al crear el archivo de LaTeX");
    return;
  }
  lg_init(lg);
  prepare_simplex_lg();
  simplex(simplex_data, lg);
  simplex_data_free(simplex_data);
  lg_simplex_references(lg);
  lg_close(lg);
  lg_generate(lg);
}



// TODO: signal para validar numeros
void load_data(char *filename){
  FILE *file;
  file = fopen(filename, "r");
  char objective[9];
  strcpy(problem_name, read_text(file, '=', '\n'));
  strcpy(objective, read_text(file, '=', '\n'));
  if(strcmp(objective, "maximize") == 0){
    do_minimize = 0;
  } else {
    do_minimize = 1;
  }
  num_variables = atoi(read_text(file, '=', '\n'));
  num_constraints = atoi(read_text(file, '=', '\n'));
  var_names = malloc(sizeof(char*) * num_variables);

  gd_variables = GTK_GRID(gtk_grid_new());
  gd_constraints = GTK_GRID(gtk_grid_new());
  GtkWidget *label, *cmb; 
  GtkEntry *entry;
  char buf[256];

  for(int x = 0; x < num_variables; ++x){
    var_names[x] = read_text(file, '=', '^');
  }
  int gd_left = 0;
  int gd_top = 0;

  for(int x = 0; x < num_variables; ++x){
    //Cargar los valores de las variables al grid
    entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_width_chars(entry, 5);
    gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);
    sprintf(buf, "%.5lf", atof(read_text(file, '=', '^')));
    gtk_entry_set_text(entry, buf);
    gtk_grid_attach(gd_variables, GTK_WIDGET(entry), gd_left++, 0, 1, 1);
    //Cargar los nombres de las variables al grid 
    sprintf(buf, "%s %s", var_names[x], ((x < num_variables-1) ? ("+ ") : ("")));
    label = gtk_label_new(buf);
    gtk_grid_attach(gd_variables, label, gd_left, 0, 1, 1);
    //Carga los nombres tambien al grid de restricciones
    for (int y = 0; y < num_constraints; ++y){
      label = gtk_label_new(buf);
      gtk_grid_attach(gd_constraints, label, gd_left, gd_top++, 1, 1);
    }
    gd_left++;
    gd_top = 0;
  }
  // reiniciar indices de los grid
  gd_top = 0; gd_left = 0;

  // agregar las restricciones
  for(int i = 0; i < num_constraints; ++i){
    for(int j = 0; j < num_variables; ++j){
      //Cargar los valores de las variables al grid
      entry = GTK_ENTRY(gtk_entry_new());
      gtk_entry_set_width_chars(entry, 5);
      gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);
      sprintf(buf, "%.5lf", atof(read_text(file, '=', '^')));
      gtk_entry_set_text(entry, buf);
      gtk_grid_attach(gd_constraints, GTK_WIDGET(entry), gd_left, gd_top, 1, 1);
      gd_left += 2;
    }
    // Configurar combo box de la desigualdad
    cmb = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(inequalities));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(cmb), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), 0);
    g_signal_connect(cmb, "changed", G_CALLBACK(on_combo_constraint_changed), inequalities);

    entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cmb)));
    gtk_entry_set_width_chars(entry, 5);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(entry), FALSE);

    // Agregar la entry del valor de la variable
    entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_width_chars(entry, 5);
    gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);

    sprintf(buf, "%.5lf", atof(read_text(file, '<', '^')));
    gtk_entry_set_text(entry, buf);

    gtk_grid_attach(gd_constraints, cmb, gd_left++, gd_top, 1, 1);
    gtk_grid_attach(gd_constraints, GTK_WIDGET(entry), gd_left++, gd_top++, 1, 1);
    gd_left = 0;
  }
  fclose(file);
  gtk_grid_set_column_spacing(gd_variables, 5);
  gtk_grid_set_column_spacing(gd_constraints, 5);

  gtk_container_add(GTK_CONTAINER(vp_objective_func), GTK_WIDGET(gd_variables));
  gtk_container_add(GTK_CONTAINER(vp_constraints), GTK_WIDGET(gd_constraints));

  gtk_widget_hide(main_window);
  gtk_widget_show_all(second_window);
}

int simplex_table_cols;

void on_btn_load_clicked(){
  char *filename;
  GtkWidget *chooser_window;
  GtkFileChooserAction open_window = GTK_FILE_CHOOSER_ACTION_SAVE;
  int response;
  chooser_window = gtk_file_chooser_dialog_new("Open File",
                                                GTK_WINDOW(main_window),
                                                open_window,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Open",
                                                GTK_RESPONSE_ACCEPT,
                                                NULL);
  response = gtk_dialog_run(GTK_DIALOG(chooser_window));
  if (response == GTK_RESPONSE_ACCEPT){
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(chooser_window);
    filename = gtk_file_chooser_get_filename(chooser);
    load_data(filename);
  }
  gtk_widget_destroy(chooser_window);
}


void on_cmb_objective_func_changed(GtkComboBox *cmb, GtkEntry* e){
  const char* str = gtk_entry_get_text(e);
  if (strcmp(str, "Maximize") == 0) do_minimize = 0;
  else do_minimize = 1;
}

void on_back_button_clicked() {
  GtkGrid* gd = GTK_GRID(gtk_builder_get_object(builder, "gd_initial"));
  gtk_entry_set_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 0)), "");
  gtk_entry_set_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 1)), "");
  gtk_entry_set_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 2)), "");
  gtk_toggle_button_set_active(intermediate_toggle, 0);
 
  for (int i = 0; i < num_variables; ++i) free(var_names[i]);
  free(var_names);
  
  gtk_widget_destroy(GTK_WIDGET(gd_varnames));
  
  problem_name[0] = '\0';
  num_variables = 0;
  num_constraints = 0;

  gtk_widget_destroy(GTK_WIDGET(gd_variables));
  gtk_widget_destroy(GTK_WIDGET(gd_constraints));
  gtk_widget_hide(second_window);
  gtk_widget_show_all(main_window);

}
