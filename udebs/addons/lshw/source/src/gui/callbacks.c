#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "version.h"
#include "engine.h"
#include <string.h>

static GtkWidget *about = NULL;
extern GtkWidget *mainwindow;

static char *id = "@(#) $Id: callbacks.c 1907 2007-10-13 22:23:16Z lyonel $";

void
refresh_display                        (GtkMenuItem     *menuitem,
gpointer         user_data)
{
  refresh(mainwindow);
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
gpointer         user_data)
{
  if(!GTK_IS_WIDGET(about))
  {
    about = create_aboutlshw();
    gtk_widget_show(about);
  }
}


void
on_aboutclose_activate          (GtkButton       *button,
gpointer         user_data)
{
  if(GTK_IS_WIDGET(about))
  {
    gtk_widget_destroy(about);
  }
}


void
on_version_realize                     (GtkWidget       *widget,
gpointer         user_data)
{
  const char *latest = checkupdates();

  gtk_label_set_text(GTK_LABEL(widget), getpackageversion());

  if(latest)
  {
    if(strcmp(latest, getpackageversion()) != 0)
    {
      static GtkWidget *dialog = NULL;

      if(!GTK_IS_WIDGET(dialog))
      {
        dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(mainwindow),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,
                                  "The latest version is <tt>%s</tt>.\n\nYou can visit <span foreground=\"blue\"><u>http://www.ezix.org/</u></span> for more information.",
                                  latest);

        gtk_window_set_title(GTK_WINDOW(dialog), "Update available");
        /* Destroy the dialog when the user responds to it (e.g. clicks a button) */
        g_signal_connect_swapped (dialog, "response",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog);
      }

      gtk_widget_show(dialog);
    }
  }
}


void
on_treeview1_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(1, treeview);
}


void
on_treeview2_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(2, treeview);
}


void
on_treeview3_row_activated             (GtkTreeView     *treeview,
GtkTreePath     *path,
GtkTreeViewColumn *column,
gpointer         user_data)
{
  browse(3, treeview);
}


void
on_treeview1_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(1, treeview);
}


void
on_treeview2_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(2, treeview);
}


void
on_treeview3_cursor_changed            (GtkTreeView     *treeview,
gpointer         user_data)
{
  change_selection(3, treeview);

  (void) &id;                                     // avoid warning "id defined but not used"
}


void
go_up                                  (GtkToolButton   *toolbutton,
gpointer         user_data)
{
  go_back(mainwindow);
}


void
on_lshw_map                            (GtkWidget       *widget,
gpointer         user_data)
{
  refresh(mainwindow);
}

void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save_as(mainwindow);
}


void
on_savebutton_clicked                  (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
  on_save_activate(NULL, NULL);
}

