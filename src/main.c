#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "include/simplex.h"
#include "include/matrix.h"
#include "include/utils.h"
#include <fontconfig/fontconfig.h>
GtkWidget* main_window;
GtkWidget* second_window;
GtkBuilder* builder;
GtkWidget* cmb_objective_func;
GtkWidget* vp_objective_func;
GtkWidget* vp_constraints;
GtkGrid* gd_variables;
GtkGrid* gd_constraints;

void initialize(){
	//////////////////////////////// Define the variables
	builder = gtk_builder_new_from_file("ui/main.glade");
	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	second_window = GTK_WIDGET(gtk_builder_get_object(builder, "second_window"));
	vp_objective_func = GTK_WIDGET(gtk_builder_get_object(builder, "vp_objective_func"));
	vp_constraints = GTK_WIDGET(gtk_builder_get_object(builder, "vp_constraints"));
	gtk_window_set_title(GTK_WINDOW(main_window), "Dynamic Programming Algorithms Hub");
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(second_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_builder_connect_signals(builder, NULL);
}


int main(int argc, char *argv[]){
	//Inicializaciones
	FcInit();
	gtk_init(&argc, &argv);

  initialize();
	////////////////////////////////// We show the gui
	gtk_widget_show(main_window);
	gtk_main();
	/////////////////////////////////
	return 0;
}

Matrix simplex_table;
/*GtkWidget*
gtk_grid_get_child_at (
  GtkGrid* grid,
  gint left,
  gint top
)*/
/*void
gtk_grid_attach (
  GtkGrid* grid,
  GtkWidget* child,
  gint left,
  gint top,
  gint width,
  gint height
)
*/
char problem_name[50];
int num_variables;
int num_constraints;
char **variables_name;

void on_combo_constraint_changed(GtkComboBox *cmb, gpointer user_data){
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(user_data);
  GtkWidget *entry = gtk_bin_get_child(GTK_BIN(cmb));
  if(gtk_combo_box_get_active_iter(cmb, &iter)){
    gboolean active;
    gtk_tree_model_get(model, &iter, 1, &active, -1);//Modelo, indice, devuelvalo aca pa, termina pa
    if(!active){
      gtk_combo_box_set_active(cmb, -1);
      gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
  }
}


void on_btn_continue_clicked(GtkButton *b, GtkGrid* gd){
  gtk_widget_hide(main_window);
  
  strcpy(problem_name, gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 0))));
  num_variables = atoi(gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 1))));
  num_constraints = atoi(gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd, 1, 2))));


  printf("%s\n%d\n%d\n", problem_name, num_variables, num_constraints);

  gd_variables = GTK_GRID(gtk_grid_new());
  GtkWidget *entry, *label;
  char buff[256];
  int col_i = 0;
  for(int v = 0; v < num_variables; ++v){
    entry = gtk_entry_new();
    sprintf(buff, "x_{%d} %s", v+1, ((v < num_variables-1) ? "+ " : ""));
    label = gtk_label_new(buff);
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_widget_set_hexpand(entry, TRUE);

    gtk_grid_attach(gd_variables, entry, col_i++, 0, 1, 1);
    gtk_grid_attach(gd_variables, label, col_i++, 0, 1, 1);

  }
 
  GtkWidget *cmb;
  GtkListStore *inequalities = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  GtkTreeIter iter;
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, "<=", 1, TRUE, -1);
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, "==", 1, FALSE, -1);
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, ">=", 1, FALSE, -1);
  gd_constraints = GTK_GRID(gtk_grid_new());


  col_i = 0;
  for(int c = 0; c < num_constraints; ++c){
    for(int x = 0; x < num_variables; ++x){
      entry = gtk_entry_new();
      sprintf(buff, "x_{%d} %s", x+1, ((x < num_variables-1) ? "+ " : ""));
      label = gtk_label_new(buff);
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
      gtk_widget_set_hexpand(entry, TRUE);
      gtk_grid_attach(gd_constraints, entry, col_i++, c, 1, 1);
      gtk_grid_attach(gd_constraints, label, col_i++, c, 1, 1);
    }

    cmb = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(inequalities));
    gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(cmb), 0);

    g_signal_connect(cmb, "changed", G_CALLBACK(on_combo_constraint_changed), inequalities);

    entry = gtk_bin_get_child(GTK_BIN(cmb));
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(entry, FALSE);
    gtk_grid_attach(gd_constraints, cmb, col_i++, c, 1, 1);
    entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_grid_attach(gd_constraints, entry, col_i++, c, 1, 1);
    col_i=0;
  }


  gtk_container_add(GTK_CONTAINER(vp_objective_func), GTK_WIDGET(gd_variables));
  gtk_container_add(GTK_CONTAINER(vp_constraints), GTK_WIDGET(gd_constraints));
  

  //simplex_table = new_matrix()
  
  gtk_widget_show_all(second_window);
}

void on_btn_finish_clicked(){
  int rows = 1 + num_constraints;
  int cols = 2 + num_variables + num_constraints;
  GtkWidget *entry;
  simplex_table = new_matrix(rows, cols, FLOAT);
  init_matrix_num(simplex_table, 0);

  simplex_table.data.f[0][0]=1;
  simplex_table.data.f[0][cols-1] = 0;
  int canonic_i = num_variables + 1;
  for(int r = 0; r < rows; ++r){
    for(int c = 1; c < cols; ++c){
      if(r == 0 && c <= num_variables){
        entry = gtk_grid_get_child_at(gd_variables, (c-1)*2, r);      
        simplex_table.data.f[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry))) * -1;
      } else if(c <= num_variables){
        entry = gtk_grid_get_child_at(gd_constraints, (c-1)*2, (r-1));
        simplex_table.data.f[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      } else if(r > 0 && c == canonic_i){
        simplex_table.data.f[r][c] = 1;
      } else if(r > 0 && c == cols-1){

        entry = gtk_grid_get_child_at(gd_constraints, num_variables*2+1, (r-1));
        simplex_table.data.f[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      }
    }
    if(r > 0){
      canonic_i++;
    }
  }
  print_matrix(simplex_table);
  simplex(simplex_table);
  free_matrix(simplex_table);
}

Matrix load_data(){
  char *filename = "example.txt";
  FILE *file;
  file = fopen(filename, "r");
  strcpy(problem_name, read_text(file, '=', 10));
  num_variables = atoi(read_text(file, '=', '\n'));
  num_constraints = atoi(read_text(file, '=', '\n'));
  variables_name = malloc(sizeof(char*) * num_variables);
  for(int x = 0; x < num_variables; ++x){
      variables_name[x] = read_text(file, '=', '^');
      printf("%s\n", variables_name[x]);
    }
    int x_i;
   

    int rows = 1+num_constraints;
    int cols = 2+num_variables+num_constraints;
    Matrix mat = new_matrix(rows, cols, FLOAT);
    
    int index_diag = num_variables+1;
    
    for(int r = 0; r < rows; ++r){
      for(int c = 0; c < cols; ++c){
        if(r == 0 && c < num_variables+1) {
          if(c == 0){
            mat.data.f[r][c] = 1;
          } else
            mat.data.f[r][c] = atoi(read_text(file, '=', '^')) * -1;
        } else if(c > 0 && c < num_variables+1){
          mat.data.f[r][c] = atoi(read_text(file, '=', '^'));
          //printf("valor agregado: %.3f en [%d][%d]\n", mat[r][c], r, c);
        } else if(r > 0 && c == cols-1) {
          mat.data.f[r][c] = atoi(read_text(file, '<', '^'));
        } else if(r > 0 && c == index_diag){
          mat.data.f[r][index_diag] = 1.0f;
          //printf("valor agregado: %.3f en [%d][%d]\n", mat[r][c], r, c);
        }
      }
      if(r != 0) index_diag++;
    }
    return mat;
    //free(filename);
    //loaded = 1;
    fclose(file);
}


void on_cmb_objective_func_changed(GtkComboBox *cmb, GtkEntry* e){
  printf("text: %s\n", gtk_entry_get_text(e));
}
