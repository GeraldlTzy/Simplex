#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "include/simplex.h"
#include <fontconfig/fontconfig.h>
GtkWidget* main_window;
GtkBuilder* builder;
GtkWidget* cmb_objective_func;



void initialize(){
	//////////////////////////////// Define the variables
	builder = gtk_builder_new_from_file("ui/main.glade");
	main_window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	gtk_window_set_title(GTK_WINDOW(main_window), "Dynamic Programming Algorithms Hub");
  g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_builder_connect_signals(builder, NULL);
}


int main(int argc, char *argv[]){
	//Inicializaciones
	FcInit();
	gtk_init(&argc, &argv);
	////////////////////////////////// We show the gui
	gtk_widget_show(main_window);
	gtk_main();
	/////////////////////////////////
	return 0;
}

void on_cmb_objective_func_changed(GtkComboBox *cmb, GtkEntry* e){
  printf("text: %s\n", gtk_entry_get_text(e));
}
