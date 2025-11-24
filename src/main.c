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
GtkWidget* entry_problem_name;
GtkWidget* in_num_var;
GtkWidget* in_num_const;

char problem_name[256];
int num_variables = -1;
int num_constraints = -1;
int loaded = 0;
int do_minimize = 0;
char **var_names;
char **headers;
SimplexData *simplex_data;
int exists_grids = 0;

Latex_Generator l;
Latex_Generator *lg = &l;
GtkListStore *inequalities;
char **ineq_arr;
int *canon_number;
#include <regex.h>


void update_grid_varnames(int mode);
/*####################################UTILS##################################*/
typedef struct {
  int inf_limit;
  int sup_limit;
} NumericEntryParams;


void numeric_entry(GtkEditable *editable, const gchar *text,
                            gint length, gint *position, gpointer data) {
  const gchar *current = gtk_entry_get_text(GTK_ENTRY(editable));
  const char *pattern = (const char *) data;

  gchar *new_text = g_strdup_printf("%.*s%.*s%s", 
      *position, current, length, text, current + *position);

  regex_t regex;
  gboolean valid = (regcomp(&regex, pattern, REG_EXTENDED) == 0) &&
                   (regexec(&regex, new_text, 0, NULL, 0) == 0);
  regfree(&regex);
  g_free(new_text);

  if (!valid) {
    g_signal_stop_emission_by_name(editable, "insert-text");
    gdk_display_beep(gdk_display_get_default());
  }
}

static void real_numeric_entry(GtkEditable *editable, const gchar *text,
                            gint length, gint *position, gpointer data) {
  const gchar *current = gtk_entry_get_text(GTK_ENTRY(editable));

  if (text[0] == '-' && *position == 0){
    return;
  }

  gchar *new_text = g_strdup_printf("%.*s%.*s%s", 
      *position, current, length, text, current + *position);

  const char *pattern = "^[-+]?([0-9]+(\\,[0-9]{0,5})?|\\,[0-9]{1,5})$";
  regex_t regex;
  gboolean valid = (regcomp(&regex, pattern, REG_EXTENDED) == 0) &&
                   (regexec(&regex, new_text, 0, NULL, 0) == 0);
  regfree(&regex);
  g_free(new_text);

  if (!valid) {
    g_signal_stop_emission_by_name(editable, "insert-text");
    gdk_display_beep(gdk_display_get_default());
  }
}
/*###########################################################################*/
void delete_data() {
  for (int i = 0; i < num_variables; ++i) free(var_names[i]);
  free(var_names);
  var_names = NULL;

  problem_name[0] = '\0';
  num_variables = -1;
  num_constraints = -1;

  gtk_widget_destroy(GTK_WIDGET(gd_variables));
  gtk_widget_destroy(GTK_WIDGET(gd_constraints));
  gtk_widget_destroy(GTK_WIDGET(gd_varnames));
  //gtk_widget_hide(second_window);
}



void initialize(){
	//////////////////////////////// Define the variables
	builder = gtk_builder_new_from_file("ui/main.glade");
  cmb_objective_func = GTK_WIDGET(gtk_builder_get_object(builder, "cmb_objective_func"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_objective_func), 0);
  varname_window = GTK_WIDGET(gtk_builder_get_object(builder, "varname_window"));
	second_window = GTK_WIDGET(gtk_builder_get_object(builder, "second_window"));
	vp_objective_func = GTK_WIDGET(gtk_builder_get_object(builder, "vp_objective_func"));
	vp_constraints = GTK_WIDGET(gtk_builder_get_object(builder, "vp_constraints"));
  vp_varnames = GTK_WIDGET(gtk_builder_get_object(builder, "vp_varnames"));
  intermediate_toggle = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "ckbtn_intermediate_tables"));
  inequalities = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  
  in_num_const = GTK_WIDGET(gtk_builder_get_object(builder, "in_num_const1"));
  in_num_var = GTK_WIDGET(gtk_builder_get_object(builder, "in_num_var1"));

  char *pattern = "^[0-9]+$";

  g_signal_connect(in_num_const, "insert-text", G_CALLBACK(numeric_entry), (void *) pattern);
  g_signal_connect(in_num_var, "insert-text", G_CALLBACK(numeric_entry), (void *) pattern);


  GtkTreeIter iter;
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, "<=", 1, TRUE, -1);
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, "==", 1, TRUE, -1);
  gtk_list_store_append(inequalities, &iter);
  gtk_list_store_set(inequalities, &iter, 0, ">=", 1, TRUE, -1);
  
  // valores generales
  entry_problem_name = GTK_WIDGET(gtk_builder_get_object(builder, "entry_problem_name"));


  gtk_window_set_title(GTK_WINDOW(second_window), "Dynamic Programming Algorithms Hub");
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
	gtk_widget_show(second_window);
	gtk_main();
	/////////////////////////////////
	return 0;
}


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

int new_num_variables;
int new_num_constraints;

//GtkGrid *gd_info;

void update_grid_varnames(int mode){
  if(mode < 2){
    gd_varnames = GTK_GRID(gtk_grid_new());
    
    if(mode == 1){
      var_names = malloc(sizeof(char *) * num_variables);
      for (int i = 0; i < num_variables; ++i){
        var_names[i] = malloc(sizeof(char)*NAME_SIZE);
        sprintf(var_names[i], "x_{%d}", i+1);
      }
    }

    for(int v = 0; v < num_variables; ++v){
      GtkWidget *entry = gtk_entry_new();
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
  } else {
    if(num_variables < new_num_variables){
      char **tmp = malloc(sizeof(char *) * new_num_variables);
      for (int i = 0; i < new_num_variables; ++i){
        tmp[i] = malloc(NAME_SIZE);
        if (i < num_variables){
          free(var_names[i]);
        }
      }
      free(var_names);
      var_names = tmp;
      
      for(int v = 0; v < new_num_variables; ++v){
        if(v < num_variables){
          strcpy(var_names[v], gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd_varnames, 1, v))));
        } else{
          sprintf(var_names[v], "x_{%d}", v+1);
          GtkWidget *entry = gtk_entry_new();
          GtkWidget *label = gtk_label_new(var_names[v]);
          gtk_entry_set_text(GTK_ENTRY(entry), var_names[v]);
          gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
          gtk_widget_set_hexpand(entry, TRUE);
          g_signal_connect(entry, "changed", G_CALLBACK(on_entry_varname_clicked), GINT_TO_POINTER(v));

          gtk_grid_attach(gd_varnames, label, 0, v, 1, 1);
          gtk_grid_attach(gd_varnames, entry, 1, v, 1, 1);
        }
      }
    // copiar los datos a un arreglo mas pequno
    // y borrar los que sobran
      } else if(num_variables > new_num_variables){
        char **tmp = malloc(sizeof(char *)*new_num_variables);
        for (int i = 0; i < num_variables; ++i){
          if (i < new_num_variables){
            tmp[i] = malloc(NAME_SIZE);
            memcpy(tmp[i], var_names[i], NAME_SIZE);
          }
          free(var_names[i]);
        }
        free(var_names);
        var_names = tmp;
        for(int v = new_num_variables; v < num_variables; ++v){
          GtkWidget *label = gtk_grid_get_child_at(gd_varnames, 0, v);
          GtkWidget *entry = gtk_grid_get_child_at(gd_varnames, 1, v);
          gtk_container_remove(GTK_CONTAINER(gd_varnames), label);
          gtk_container_remove(GTK_CONTAINER(gd_varnames), entry);
        }
      }
    }
}


void update_grid_variables_and_constraints(int to_update);
void update_problem(GtkButton *b, GtkGrid *gd_info){
  validate_int(GTK_ENTRY(gtk_grid_get_child_at(gd_info, 1, 1)), 15, 2);
  validate_int(GTK_ENTRY(gtk_grid_get_child_at(gd_info, 1, 2)), 15, 2);
  
  strcpy(problem_name, gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd_info, 1, 0))));
  new_num_variables = atoi(gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd_info, 1, 1))));
  new_num_constraints = atoi(gtk_entry_get_text(GTK_ENTRY(gtk_grid_get_child_at(gd_info, 1, 2))));
  
  if (problem_name[0] == '\0' || new_num_variables == 0 || new_num_constraints == 0) return;
  
  // crear el grid de nombres, se hace una sola vez
  if(num_variables == -1){
    num_constraints = new_num_constraints;
    num_variables = new_num_variables;
    update_grid_varnames(1);
  } else {
    update_grid_varnames(2);
  }
  
  if(!exists_grids){
    update_grid_variables_and_constraints(0);
  } else if((num_variables != new_num_variables || num_constraints != new_num_constraints) && num_variables != -1){
    update_grid_variables_and_constraints(1);
  }
  num_variables = new_num_variables;
  num_constraints = new_num_constraints;
  
  int col_i = 0;
  char buff[256];

  for(int v = 0; v < num_variables; ++v){
    sprintf(buff, "%s %s", var_names[v], ((v < num_variables-1) ? "+ " : ""));
    col_i++;
    gtk_label_set_text(GTK_LABEL(gtk_grid_get_child_at(gd_variables, col_i++, 0)), buff);
  }
  col_i = 0; 
  for(int c = 0; c < num_constraints; ++c){
    for(int v = 0; v < num_variables; ++v){
      sprintf(buff, "%s %s", var_names[v], ((v < num_variables-1) ? "+ " : ""));
      col_i++;
      gtk_label_set_text(GTK_LABEL(gtk_grid_get_child_at(gd_constraints, col_i++, c)), buff);
    }
    col_i = 0;
  }
}

void update_grid_variables_and_constraints(int to_update){
  GtkWidget *entry, *label;
  char buff[256];
  int col_i = 0;
  
  if(!to_update){
    gd_variables = GTK_GRID(gtk_grid_new());
    for(int v = 0; v < num_variables; ++v){
      entry = gtk_entry_new();
      sprintf(buff, "%s %s", var_names[v], ((v < num_variables-1) ? "+ " : ""));
      label = gtk_label_new(buff);
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
      gtk_widget_set_hexpand(entry, TRUE);
      gtk_entry_set_text(GTK_ENTRY(entry), "0");
      g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);
      gtk_grid_attach(gd_variables, entry, col_i++, 0, 1, 1);
      gtk_grid_attach(gd_variables, label, col_i++, 0, 1, 1);
    }
    exists_grids = 1;

  } else if(num_variables < new_num_variables){//actualizar grid de variables de la funcion objetivo
    label = gtk_grid_get_child_at(gd_variables, num_variables*2-1, 0);
    sprintf(buff, "%s %s", var_names[num_variables-1], "+ ");
    gtk_label_set_text(GTK_LABEL(label), buff);

    col_i = num_variables*2;
    for(int v = num_variables; v < new_num_variables; ++v){
      entry = gtk_entry_new();
      sprintf(buff, "%s %s", var_names[v], ((v < new_num_variables-1) ? "+ " : ""));
      label = gtk_label_new(buff);
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
      gtk_widget_set_hexpand(entry, TRUE);
      gtk_entry_set_text(GTK_ENTRY(entry), "0");
      g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);
      gtk_grid_attach(gd_variables, entry, col_i++, 0, 1, 1);
      gtk_grid_attach(gd_variables, label, col_i++, 0, 1, 1);
    }
  } else if(num_variables > new_num_variables){
    for(int v = (num_variables*2-1); v >= (new_num_variables*2-1); --v){
      GtkWidget *widget = gtk_grid_get_child_at(gd_variables, v, 0);
      gtk_container_remove(GTK_CONTAINER(gd_variables), widget);
    }
    sprintf(buff, "%s %s", var_names[new_num_variables-1], "");
    label = gtk_label_new(buff);
    gtk_grid_attach(gd_variables, label, new_num_variables*2-1, 0, 1, 1);
  }

  GtkWidget *cmb;
  if (!to_update){
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
      g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);
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
    const char *pattern = "^[0-9]+$";
    g_signal_connect(entry, "insert-text", G_CALLBACK(numeric_entry), (void *) pattern);
    gtk_entry_set_text(GTK_ENTRY(entry), "0");
    gtk_grid_attach(gd_constraints, entry, col_i++, c, 1, 1);
    col_i=0;
  }

  gtk_grid_set_column_spacing(gd_variables, 5);
  gtk_grid_set_column_spacing(gd_constraints, 5);
  gtk_container_add(GTK_CONTAINER(vp_objective_func), GTK_WIDGET(gd_variables));
  gtk_container_add(GTK_CONTAINER(vp_constraints), GTK_WIDGET(gd_constraints));
  //gtk_widget_destroy(GTK_WIDGET(gd_varnames));
  } else {
    // recolocar lo ultimo
    if(num_variables != new_num_variables){
      for (int i = 0; i < num_constraints; ++i){
        cmb = gtk_grid_get_child_at(gd_constraints, num_variables*2, i);
        entry = gtk_grid_get_child_at(gd_constraints, num_variables*2+1, i);
        
        if (num_variables > new_num_variables){
          for (int v = (num_variables*2-1); v > (new_num_variables*2-1); --v){
            GtkWidget *w = gtk_grid_get_child_at(gd_constraints, v, i);
            gtk_container_remove(GTK_CONTAINER(gd_constraints), w);
          }
        }

        if (cmb && entry) {
          g_object_ref(cmb); g_object_ref(entry);

          gtk_container_remove(GTK_CONTAINER(gd_constraints), cmb);
          gtk_container_remove(GTK_CONTAINER(gd_constraints), entry);

          gtk_grid_attach(gd_constraints, cmb, new_num_variables*2, i, 1, 1);
          gtk_grid_attach(gd_constraints, entry, new_num_variables*2+1, i, 1, 1);

          g_object_unref(cmb); g_object_unref(entry);
        }
      }
    }
    for (int i = num_constraints; i < new_num_constraints; ++i){
      cmb = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(inequalities));
      gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(cmb), 0);
      gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), 0);

      g_signal_connect(cmb, "changed", G_CALLBACK(on_combo_constraint_changed), inequalities);

      entry = gtk_bin_get_child(GTK_BIN(cmb));
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_widget_set_can_focus(entry, FALSE);
      gtk_grid_attach(gd_constraints, cmb, new_num_variables*2, i, 1, 1);
      entry = gtk_entry_new();
      gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
      gtk_widget_set_hexpand(entry, TRUE);
      const char *pattern = "^[0-9]+$";
      g_signal_connect(entry, "insert-text", G_CALLBACK(numeric_entry), (void *) pattern);
      gtk_entry_set_text(GTK_ENTRY(entry), "0");
      gtk_grid_attach(gd_constraints, entry, new_num_variables*2+1, i, 1, 1);
    }

    col_i = 0;
    //if (num_constraints < new_num_constraints){
      for(int c = 0; c < new_num_constraints; ++c){
        for(int x = 0; x < new_num_variables; ++x){
          if(c < num_constraints && x < num_variables){
            col_i+=2;
            continue;
          }
          entry = gtk_entry_new();
          sprintf(buff, "%s %s", var_names[x], ((x < num_variables-1) ? "+ " : ""));
          label = gtk_label_new(buff);
          gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);
          gtk_widget_set_hexpand(entry, TRUE);
          gtk_entry_set_text(GTK_ENTRY(entry), "0");
          g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);
          gtk_grid_attach(gd_constraints, entry, col_i++, c, 1, 1);
          gtk_grid_attach(gd_constraints, label, col_i++, c, 1, 1);
        }
        col_i=0;
      }
    //} else {
    if(num_constraints > new_num_constraints){
      for (int row = (num_constraints-1); row >= (new_num_constraints); --row){
        for (int col = 0; col < (new_num_variables*2+2); ++col){
          GtkWidget *w = gtk_grid_get_child_at(gd_constraints, col, row);
          gtk_container_remove(GTK_CONTAINER(gd_constraints), w);
        }
      }
    }
  }
  gtk_widget_show_all(second_window);
}

char *drawmin_simplex_2d(char restrictions[][256], int i){
    if (i == num_constraints-1) {
        char *base = malloc(strlen(restrictions[i])+1);
        strcpy(base, restrictions[i]);
        return base;
    }
    char *next = drawmin_simplex_2d(restrictions, i+1);
    int len = strlen("min(,)") + strlen(restrictions[i]) + strlen(next) + 1;
    char *result = malloc(len);
    sprintf(result, "min(%s,%s)", restrictions[i], next);
    free(next);
    return result;
}

void draw_2d_graph(){
    lg_write(lg, "\\begin{tikzpicture}\n");
    lg_write(lg,    "\\begin{axis}[\n"
                    "\tymin = 0, xmin = 0, xmax=100,\n"
                    "\taxis lines = left,\n"
                    "\tgrid = both,"
                    "\tdomain = 0:100\n"
                    "]\n"
    );

    //las restriccione son restricciones perronas
    //hay un label y un entry por cada bichita
    GtkWidget *entry;

    // parseo bien perron de las restricciones
    char restricctions[num_constraints][256];
    char c = 'A';
    const char *str;
    char buf[64];
    int k;
    for (int i = 0; i < num_constraints; ++i){
        // la constante de la restriccion
        entry = gtk_grid_get_child_at(gd_constraints, 2 * num_variables + 1, i);
        str = gtk_entry_get_text(GTK_ENTRY(entry));

        k = 0;
        for (int i = 0; str[i] != '\0'; ++i){
            if (str[i] == ',') buf[k++] = '.';
            else buf[k++] = str[i];
        }
        buf[k] = '\0';
        strcpy(restricctions[i], "(");
        strcat(restricctions[i], buf);

        if (atof(str) != 0){
            // el coeficiente de la primera variable
            entry = gtk_grid_get_child_at(gd_constraints, 0, i);
            str = gtk_entry_get_text(GTK_ENTRY(entry));
            k = 0;
            for (int i = 0; str[i] != '\0'; ++i){
                if (str[i] == ',') buf[k++] = '.';
                else buf[k++] = str[i];
            }
            buf[k] = '\0';
            // como se despeja van con el simbolo opuesto
            if (buf[0] == '-'){
                //strcpy(buf, "+");
                //strcat(buf,str+1);
                buf[0] = '+';
                strcat(restricctions[i], buf);
            } else {
                //strcpy(buf, "-");
                //strcat(buf,str);
                strcat(restricctions[i], "-");
                strcat(restricctions[i], buf);
            }
            strcat(restricctions[i], "*x)/");
            // el coeficiente de la segunda
            entry = gtk_grid_get_child_at(gd_constraints, 2, i);
            str = gtk_entry_get_text(GTK_ENTRY(entry));
            k = 0;
            for (int i = 0; str[i] != '\0'; ++i){
                if (str[i] == ',') buf[k++] = '.';
                else buf[k++] = str[i];
            }
            buf[k] = '\0';
            strcat(restricctions[i], buf);
        }
    }

    // dibuja cada restriccion
    for (int i = 0; i < num_constraints; ++i){
        lg_write(lg,    "\\addplot[\n"
                        "\tcolor=black,\n"
                        "\tname path=R%d\n"
                        "]\n"
                        "{%s};\n",
                        i,
                        restricctions[i]
        ); 
    }

    //Resaltar el cuerpo simplex en un color diferente
    // https://tex.stackexchange.com/questions/326746/how-to-plot-max-min-function-in-latex
    lg_write(lg,    "\\addplot[\n"
                    "\tcolor=red,\n"
                    "\tname path=S\n"
                    "]\n");
    
    lg_write(lg, "{");

    char *simplex = drawmin_simplex_2d(restricctions, 0);
    lg_write(lg, simplex);
    free(simplex);

    lg_write(lg, "};\n");

    lg_write(lg, "\\end{axis}\n");
    lg_write(lg, "\\end{tikzpicture}\n\n");
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
    // constraints x>0
    to_write[0] = '\0';
    for (int i = 0; i < num_variables; ++i){
        buffer[0] = '\0';
        if (i < num_variables-1) sprintf(buffer, "%s,", var_names[i]);
        else sprintf(buffer, "%s", var_names[i]);
        strcat(to_write, buffer);
    }
        lg_write(lg, "\\begin{dmath}\n");
        lg_write(lg, "%s \\geq 0\n", to_write);
        lg_write(lg, "\\end{dmath}\n");

    /*if (num_variables == 2) {
        lg_write(lg, "\n\\section{Graphical representation}\n");
        draw_2d_graph();
    }*/
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
    if (strcmp(ineq, "<=") == 0) {
        simplex_data->slacks++;
    } else if (strcmp(ineq, "==") == 0) {
        simplex_data->artificials++;
    }
    else if (strcmp(ineq, ">=") == 0) {
        simplex_data->artificials++; 
        simplex_data->excess++;
    }
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
  headers[acc] = malloc(2);
  strcpy(headers[acc], "B");
  data->headers = headers;
}

/*#########################MANEJO DE ARCHIVOS################################*/

void select_file(char **filename, char *oper, GtkWidget *parent){
  GtkWidget *chooser_window;
  GtkFileChooserAction open_window = GTK_FILE_CHOOSER_ACTION_SAVE;
  int response;
  chooser_window = gtk_file_chooser_dialog_new(oper,
                                                GTK_WINDOW(parent),
                                                open_window,
                                                "_Cancel",
                                                GTK_RESPONSE_CANCEL,
                                                "_Open",
                                                GTK_RESPONSE_ACCEPT,
                                                NULL);
  response = gtk_dialog_run(GTK_DIALOG(chooser_window));
  if (response == GTK_RESPONSE_ACCEPT){
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(chooser_window);
    *filename = gtk_file_chooser_get_filename(chooser);
  } else {
    *filename = NULL;
  }
  gtk_widget_destroy(chooser_window);
}
void load_data(char *filename){
  if (exists_grids){
    delete_data();
    exists_grids = 0;
  }
  FILE *file;
  file = fopen(filename, "r");
  char objective[9];
  strcpy(problem_name, read_text(file, '=', '\n'));
  gtk_entry_set_text(GTK_ENTRY(entry_problem_name), problem_name);

  strcpy(objective, read_text(file, '=', '\n'));
  if(strcmp(objective, "Maximize") == 0){
    do_minimize = 0;
    gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_objective_func), 0);
  } else {
    do_minimize = 1;
    gtk_combo_box_set_active(GTK_COMBO_BOX(cmb_objective_func), 1);
  }
  char *num_var_str = read_text(file, '=', '\n');
  num_variables = atoi(num_var_str);
  gtk_entry_set_text(GTK_ENTRY(in_num_var), num_var_str);

  char *num_const_str = read_text(file, '=', '\n');
  num_constraints = atoi(num_const_str);
  gtk_entry_set_text(GTK_ENTRY(in_num_const), num_const_str);

  var_names = malloc(sizeof(char*) * num_variables);

  gd_variables = GTK_GRID(gtk_grid_new());
  gd_constraints = GTK_GRID(gtk_grid_new());
  GtkWidget *label, *cmb; 
  GtkEntry *entry;
  char buf[256];

  for(int x = 0; x < num_variables; ++x){
    var_names[x] = read_text(file, '=', '^');
  }
  update_grid_varnames(0);
  exists_grids = 1;

  int gd_left = 0;
  int gd_top = 0;

  for(int x = 0; x < num_variables; ++x){
    //Cargar los valores de las variables al grid
    entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_width_chars(entry, 5);
    gtk_widget_set_hexpand(GTK_WIDGET(entry), TRUE);
    sprintf(buf, "%.5lf", atof(read_text(file, '=', '^')));
    gtk_entry_set_text(entry, buf);
    g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);
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
      g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);
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
    g_signal_connect(entry, "insert-text", G_CALLBACK(real_numeric_entry), NULL);

    int selected_index = 0;
    sprintf(buf, "%.5lf", atof(read_text_multiple_start(file, "<=>", '^', &selected_index)));
    gtk_entry_set_text(entry, buf);

    gtk_combo_box_set_active(GTK_COMBO_BOX(cmb), selected_index);

    gtk_grid_attach(gd_constraints, cmb, gd_left++, gd_top, 1, 1);
    gtk_grid_attach(gd_constraints, GTK_WIDGET(entry), gd_left++, gd_top++, 1, 1);
    gd_left = 0;
  }
  fclose(file);
  gtk_grid_set_column_spacing(gd_variables, 5);
  gtk_grid_set_column_spacing(gd_constraints, 5);

  gtk_container_add(GTK_CONTAINER(vp_objective_func), GTK_WIDGET(gd_variables));
  gtk_container_add(GTK_CONTAINER(vp_constraints), GTK_WIDGET(gd_constraints));

  gtk_widget_show_all(second_window);
}

int save_data(char *filename){
  FILE *f = fopen(filename, "w");
  if (!f) return 1;
  fprintf(f, "name=%s\n", problem_name);
  fprintf(f, "objective=%s\n", (do_minimize == 1 ? "Minimize" : "Maximize"));
  fprintf(f, "count_variables=%d\n", num_variables);
  fprintf(f, "count_constraints=%d\n", num_constraints);
  for (int x = 0; x < num_variables; ++x){
    fprintf(f, "x_%d=%s%s", x+1, var_names[x], (x != num_variables-1 ? "^" : "\n"));
  }

  GtkWidget *entry, *cmb;
  double value;
  const char *ineq;

  for (int c = 0; c <= num_constraints; ++c){
    for (int x = 0; x < num_variables; ++x){
      if (c == 0){
        entry = gtk_grid_get_child_at(gd_variables, x*2, 0);
        value = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
        fprintf(f, "x_%d=%.5lf%s", x+1, value, (x != num_variables-1 ? "^" : "\n"));
      } else {
        entry = gtk_grid_get_child_at(gd_constraints, x*2, c-1);
        value = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
        fprintf(f, "x_%d=%.5lf^", x+1, value);
      }
    }
    if (c != 0){
      cmb = gtk_grid_get_child_at(gd_constraints, num_variables*2, c-1);
      entry = gtk_bin_get_child(GTK_BIN(cmb));
      ineq = gtk_entry_get_text(GTK_ENTRY(entry));
      entry = gtk_grid_get_child_at(gd_constraints, num_variables*2+1, c-1);
      value = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      fprintf(f, "%c%.5lf\n", ineq[0], value);
    }
  }
  fclose(f);
}

/*###############################SIGNALS#####################################*/

void on_cmb_objective_func_changed(GtkComboBox *cmb, GtkEntry* e){
  const char* str = gtk_entry_get_text(e);
  if (strcmp(str, "Maximize") == 0) do_minimize = 0;
  else do_minimize = 1;
}


void on_btn_save_clicked(){
  char *filename;
  select_file(&filename, "Save File", second_window);
  if(filename){
    save_data(filename);
  }
}

void on_btn_load_clicked(){
  char *filename;
  select_file(&filename, "Open File", second_window);
  if (filename){
    load_data(filename);
  }
}
void on_btn_finish_clicked(){
  if (!exists_grids) return;
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
  simplex_data->big_M = malloc(sizeof(double) * cols);
  for (int i = 0; i < cols; ++i) simplex_data->big_M[i] = 0.0;

  double **table = simplex_data->table->data.f;
  table[0][0] = 1;
  table[0][cols-1] = 0;

  int canonic_i = num_variables + 1; 

  int start_slacks = num_variables;
  int start_excess = num_variables + simplex_data->slacks;
  int start_artificials = num_variables + simplex_data->slacks + simplex_data->excess;

  // un 0 es que esta vacio y que se puede poner el indice 
  // un 1 es que ya esta lleno y que hay que poner en otra parte
  int *artificials_mark = calloc(simplex_data->artificials, sizeof(int));
  
  int slack_i = start_slacks + 1;
  int excess_i = start_excess + 1;
  int artificial_i = start_artificials + 1;

  GtkWidget *entry;
  for(int r = 0; r < rows; ++r){
    int added_artificial = 0;
    int add_canon = 0;
    for(int c = 1; c < cols; ++c){
      int is_slack = simplex_data->slacks > 0 && c > start_slacks && c <= start_excess;
      int is_excess = simplex_data->excess > 0 && c > start_excess && c <= start_artificials;
      int is_artificial = simplex_data->artificials > 0 && c > start_artificials && c != cols-1; 


      // los negativos en las varaibles en la funcion objetivo 
      if(r == 0 && c <= num_variables){
        entry = gtk_grid_get_child_at(gd_variables, (c-1)*2, r);      
        table[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry))) * -1;
      // los coeficientes de las variables 
      } else if(c <= num_variables){
        entry = gtk_grid_get_child_at(gd_constraints, (c-1)*2, (r-1));
        table[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      // las B
      } else if (r > 0 && c == cols-1) {
        entry = gtk_grid_get_child_at(gd_constraints, num_variables*2+1, (r-1));
        table[r][c] = atof(gtk_entry_get_text(GTK_ENTRY(entry)));
      // slack 1
      } else if (r > 0 && is_slack && c == canonic_i){
          if (strcmp(ineq_arr[r-1], "<=") == 0) {
            add_canon = 1;
            table[r][c] = 1;
          }
      // excess -1 
      } else if (r > 0 && is_excess && c == canonic_i){
          if (strcmp(ineq_arr[r-1], ">=") == 0) {
            add_canon = 1;
            table[r][c] = -1;
          }
      // artificial 1
      } else if (r > 0 && is_artificial && !added_artificial) {
          char *ineq = ineq_arr[r-1];
          added_artificial = 1;
          if (strcmp(ineq, ">=") == 0 || strcmp(ineq, "==") == 0) {
            // agrega en la siguiente artificial que tenga espacio disponible
            for (int i = 0; i < simplex_data->artificials; ++i) {
                // si esta vacio se pone 
                if (!artificials_mark[i]) {
                    artificials_mark[i] = 1;
                    table[r][c+i] = 1;
                    break;
                }
            }
          }
      // artificial M 
      } if (r == 0 && is_artificial) {
          if (do_minimize)
            simplex_data->big_M[c] = -1;
          else 
            simplex_data->big_M[c] = 1;
      }
    }
    if (r > 0 && add_canon) ++canonic_i;
  }

  free(artificials_mark);

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

