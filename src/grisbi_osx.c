/* ************************************************************************** */
/*                                                                            */
/*     Copyright (C)    2000-2008 Cédric Auger (cedric@grisbi.org)            */
/*          2003-2009 Benjamin Drieu (bdrieu@april.org)                       */
/*          2009-2010Pierre Biava (grisbi@pierre.biava.name)                  */
/*          http://www.grisbi.org                                             */
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/* ************************************************************************** */

/* WARNING this is a copy of test_integration.c (from ige-mac-integration-0.9.5) */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef GTKOSXAPPLICATION

#include "include.h"

/*START_INCLUDE*/
#include "grisbi_osx.h"
#include "export.h"
#include "gsb_account.h"
#include "gsb_assistant_account.h"
#include "gsb_assistant_archive.h"
#include "gsb_assistant_archive_export.h"
#include "gsb_debug.h"
#include "gsb_file.h"
#include "gsb_file_config.h"
#include "gsb_status.h"
#include "gsb_transactions_list.h"
#include "file_obfuscate.h"
#include "file_obfuscate_qif.h"
#include "import.h"
#include "main.h"
#include "menu.h"
#include "parametres.h"
#include "structures.h"
#include "erreur.h"
/*END_INCLUDE*/

typedef struct {
    GtkWindow *window;
    GtkWidget *open_item;
    GtkWidget *edit_item;
    GtkWidget *copy_item;
    GtkWidget *quit_item;
    GtkWidget *help_menu;
    GtkWidget *about_item;
    GtkWidget *preferences_item;
} MenuItems;

typedef struct
{
    gchar *label;
    gpointer item;
} MenuCBData;

typedef struct
{
  GtkOSXApplication *app;
  GtkOSXApplicationAttentionType type;
} AttentionRequest;


/*START_STATIC*/
static void action_activate_cb ( GtkAction *action, gpointer data );
static gboolean attention_cb ( AttentionRequest* req );
static void bounce_cb ( GtkWidget  *button, GtkOSXApplication *app );
static MenuCBData *menu_cbdata_new ( gchar *label, gpointer item );
static void menu_cbdata_delete ( MenuCBData *datum );
static void menu_item_activate_cb ( GtkWidget *item, MenuCBData  *datum );
static void menu_items_destroy ( MenuItems *items );
static MenuItems *menu_items_new ( void );
static void radio_item_changed_cb ( GtkAction* action, GtkAction* current, MenuCBData *datum );
static void view_menu_cb (GtkWidget *button, gpointer user_data);
/*END_STATIC*/


/*START_EXTERN*/
/*END_EXTERN*/


static GQuark menu_items_quark = 0;

/**
 *
 *
 *
 *
 * */
static MenuItems *menu_items_new ( void )
{
    return g_slice_new0 ( MenuItems );
}


/**
 *
 *
 *
 *
 * */
static void menu_items_destroy ( MenuItems *items )
{
    g_slice_free ( MenuItems, items );
}

/**
 *
 *
 *
 *
 * */
static MenuCBData *menu_cbdata_new ( gchar *label, gpointer item )
{
    MenuCBData *datum =  g_slice_new0 ( MenuCBData );
    datum->label = label;
    datum->item = item;
    g_object_ref ( datum->item );

    return datum;
}


/**
 *
 *
 *
 *
 * */
static void menu_cbdata_delete ( MenuCBData *datum )
{
    g_object_unref ( datum->item );
    g_slice_free ( MenuCBData, datum );
}


/**
 *
 *
 *
 *
 * */
static void menu_item_activate_cb ( GtkWidget *item, MenuCBData  *datum )
{
    gboolean visible;
    gboolean sensitive;
    MenuItems *items = g_object_get_qdata ( G_OBJECT ( datum->item ), menu_items_quark );

    if ( GTK_IS_WINDOW ( G_OBJECT ( datum->item ) ) )
        g_print ( "Item activated: %s:%s\n",
                        gtk_window_get_title ( GTK_WINDOW ( datum->item ) ),
                        datum->label);
    else
        g_print ("Item activated %s\n", datum->label);

    if ( !items )
        return;

    g_object_get ( G_OBJECT ( items->copy_item ),
                        "visible", &visible,
                        "sensitive", &sensitive,
                        NULL);

    if ( item == items->open_item )
    {
        gtk_widget_set_sensitive ( items->copy_item, !gtk_widget_get_sensitive ( items->copy_item ) );
    }
}


/**
 *
 *
 *
 *
 * */
static void action_activate_cb ( GtkAction *action, gpointer data )
{
    GtkWindow *window = data;

    g_print ("Window %s, Action %s\n", gtk_window_get_title ( window ), gtk_action_get_name ( action ));
}


/**
 *
 *
 *
 *
 * */
static void radio_item_changed_cb ( GtkAction* action, GtkAction* current, MenuCBData *datum )
{
    g_print ("Radio group %s in window %s changed value: %s is now active.\n",
                        datum->label, gtk_window_get_title ( GTK_WINDOW ( datum->item ) ),
                        gtk_action_get_name ( GTK_ACTION ( current ) ) );
}


/**
 *
 *
 *
 *
 * */
static gboolean attention_cb ( AttentionRequest* req )
{
    gtk_osxapplication_attention_request ( req->app, req->type );
  
    g_free(req);

    return FALSE;
}


/**
 *
 *
 *
 *
 * */
static void bounce_cb ( GtkWidget  *button, GtkOSXApplication *app )
{
    AttentionRequest *req = g_new0 ( AttentionRequest, 1 );

    req->app = app;
    req->type = CRITICAL_REQUEST;
    g_timeout_add_seconds ( 2, (GSourceFunc) attention_cb, req );
    g_print("Now switch to some other application\n");
}


/**
 *
 *
 *
 *
 * */
void grisbi_osx_app_active_cb ( GtkOSXApplication* app, gboolean* data )
{
    g_print("Application became %s\n", *data ? "active" : "inactive");
}


/**
 *
 *
 *
 *
 * */
gboolean grisbi_osx_app_should_quit_cb ( GtkOSXApplication *app, gpointer data )
{
    static gboolean abort = TRUE;
    gboolean answer;

    answer = abort;
    abort = FALSE;
    g_print ("Application has been requested to quit, %s\n", answer ? "but no!" : "it's OK.");

    return answer;
}


/**
 *
 *
 *
 *
 * */
void grisbi_osx_app_will_quit_cb ( GtkOSXApplication *app, gpointer data )
{
    g_print ("Quitting Now\n");

    gtk_main_quit();
}


/**
 *
 *
 *
 *
 * */
GtkWidget *grisbi_osx_create_window ( gchar *title )
{
    gpointer dock = NULL;
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *statusbar;
    GtkWidget *menubar;
    GtkWidget *bbox;
    GtkWidget *button;
    GtkWidget *sep;
    MenuItems *items = menu_items_new ();
    GtkUIManager *mgr = gtk_ui_manager_new ();
    GtkActionGroup *actions = gtk_action_group_new ("TestActions");
    guint mergeid;

    GtkOSXApplication *theApp = g_object_new ( GTK_TYPE_OSX_APPLICATION, NULL );

    /* create the menus */
/*     menubar = grisbi_osx_init_menus ( window, vbox );  */

    /* unsensitive the necessaries menus */
/*     menus_sensitifs ( FALSE );  */

    /* charge les raccourcis claviers */
    gtk_accel_map_load ( C_PATH_CONFIG_ACCELS );

    /* set the last opened files */
    affiche_derniers_fichiers_ouverts ( );

    /* set the size of the window */
    if ( conf.main_width && conf.main_height )
        gtk_window_set_default_size ( GTK_WINDOW ( window ), conf.main_width, conf.main_height );
    else
        gtk_window_set_default_size ( GTK_WINDOW ( window ), 900, 600 );

    /* display window at position */
    gtk_window_move ( GTK_WINDOW ( window ), conf.root_x, conf.root_y );

    /* set the full screen if necessary */
    if ( conf.full_screen )
        gtk_window_maximize ( GTK_WINDOW ( window ) );

    return window;
}

/**
 *
 *
 *
 *
 * */
GtkWidget *grisbi_osx_init_menus ( GtkWidget *window, GtkWidget *vbox )
{
    GtkWidget *menubar;
    GtkWidget *sep;
    MenuItems *items;
    GtkUIManager *ui_manager;
    GtkOSXApplication *theApp;

    theApp = g_object_new ( GTK_TYPE_OSX_APPLICATION, NULL );

    menubar = init_menus ( vbox );

    ui_manager = gsb_menu_get_ui_manager ( );
    items = menu_items_new ( );

    items->open_item = gtk_ui_manager_get_widget ( ui_manager, "/menubar/FileMenu/Open" );
    items->edit_item = gtk_ui_manager_get_widget ( ui_manager, "/menubar/EditMenu" );
    items->help_menu = gtk_ui_manager_get_widget ( ui_manager, "/menubar/Help/Manual" );
    items->quit_item = gtk_ui_manager_get_widget ( ui_manager, "/menubar/FileMenu/Quit" );
    items->about_item = gtk_ui_manager_get_widget ( ui_manager, "/menubar/Help/About" );
    items->preferences_item = gtk_ui_manager_get_widget ( ui_manager, "/menubar/EditMenu/Preferences" );
    gtk_widget_hide ( menubar );

    gtk_osxapplication_set_menu_bar ( theApp, GTK_MENU_SHELL ( menubar ) );
    gtk_osxapplication_insert_app_menu_item  ( theApp, items->about_item, 0 );
  
    sep = gtk_separator_menu_item_new ( );
    g_object_ref ( sep );
    gtk_osxapplication_insert_app_menu_item  ( theApp, sep, 1 );
    gtk_osxapplication_insert_app_menu_item  ( theApp, items->preferences_item, 2);
  
    sep = gtk_separator_menu_item_new();
    g_object_ref(sep);
    gtk_osxapplication_insert_app_menu_item  ( theApp, sep, 3 );

    gtk_osxapplication_set_help_menu ( theApp, GTK_MENU_ITEM ( items->help_menu ) );
    gtk_osxapplication_set_window_menu ( theApp, NULL );

    if ( !menu_items_quark )
        menu_items_quark = g_quark_from_static_string ( "MenuItem" );
    g_object_set_qdata_full ( G_OBJECT ( window ), menu_items_quark, items,
                        (GDestroyNotify ) menu_items_destroy );

    return menubar;
}


/**
 *
 *
 *
 *
 * */
#endif /* GTKOSXAPPLICATION */
/**
 *
 *
 *
 *
 * */
/* Local Variables: */
/* c-basic-offset: 4 */
/* End: */
