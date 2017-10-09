#include <gtk/gtk.h>
#include <gst/gst.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

enum {
    COLUMN_NAME,
    COLUMN_COUNT
};

#ifdef __arm__
const char *UIFILE = "/home/alarm/.config/raspimp/raspimp.glade";
const char *DBFILE = "/home/alarm/.config/raspimp/raspimp.db";
const char *STFILE = "/home/alarm/.config/raspimp/raspimp.css";
#else
const char *UIFILE = "raspimp.glade";
const char *DBFILE = "raspimp.db";
const char *STFILE = "raspimp.css";
#endif

GtkTreeModel *streamstore = NULL;
GtkTreeView *streamtree = NULL;
GtkLabel *statuslabel = NULL;
GtkButton *stopbutton = NULL;

GMainLoop *loop = NULL;
GstElement *playbin = NULL;
GstBus *bus = NULL;

sqlite3 *database = NULL;

#define UNUSED(x) (void)(x)

void set_status(const gchar *status, gboolean error)
{
    gchar *text;
    gchar *value;
    gchar color[6];

    value = g_ascii_strup(status, strlen(status));

    if (error)
        g_strlcpy(color, "red", 4);
    else
        g_strlcpy(color, "white", 6);
    text = g_markup_printf_escaped("<span foreground=\"\%s\">\%s</span>", color, value);
    gtk_label_set_markup(statuslabel, text);
    g_free(text);
    g_free(value);
}

void stop_stream()
{
    if (loop != NULL) {
        g_main_loop_quit(loop);
        g_main_loop_unref(loop);
        loop = NULL;
    }
    if (playbin != NULL) {
        gst_element_set_state(playbin, GST_STATE_NULL);
        gst_object_unref(playbin);
        playbin = NULL;
    }
    if (bus != NULL) {
        gst_bus_remove_watch(bus);
        gst_object_unref(bus);
        bus = NULL;
    }
    gtk_widget_set_sensitive(GTK_WIDGET(stopbutton), FALSE);
}

gboolean on_bus_message(GstBus *bus, GstMessage *message, gpointer data)
{
    UNUSED(bus);
    UNUSED(data);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *error;
        gchar *debug;

        gst_message_parse_error(message, &error, &debug);
        set_status(error->message, TRUE);
        g_error_free(error);
        g_free(debug);
        stop_stream();
        break;
    }
    case GST_MESSAGE_EOS:
        stop_stream();
        set_status("END OF STREAM", TRUE);
        break;
    case GST_MESSAGE_TAG: {
        GstTagList *tags = NULL;
        gchar *value = 0;

        gst_message_parse_tag(message, &tags);
        if (gst_tag_list_get_string(tags, "title", &value))
            set_status(value, FALSE);
        g_free(value);
        gst_tag_list_unref(tags);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

void set_streams()
{
    int result;
    sqlite3_stmt *statement;
    GtkTreeIter iter;

    sqlite3_prepare_v2(database, "SELECT name FROM stream", -1, &statement, NULL);
    gtk_list_store_clear(GTK_LIST_STORE(streamstore));
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        gtk_list_store_append(GTK_LIST_STORE(streamstore), &iter);
        gtk_list_store_set(GTK_LIST_STORE(streamstore), &iter, COLUMN_NAME, sqlite3_column_text(statement, 0), -1);
    }
    sqlite3_finalize(statement);
}

void start_stream(const gchar *url)
{
    gst_init(NULL, NULL);
    playbin = gst_element_factory_make("playbin", "playbin");
    g_object_set(playbin, "uri", url, NULL);
    bus = gst_element_get_bus(playbin);
    gst_bus_add_watch(bus, on_bus_message, NULL);
    gst_element_set_state(playbin, GST_STATE_PLAYING);
    loop = g_main_loop_new(NULL, FALSE);
    set_status("", FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(stopbutton), TRUE);
    g_main_loop_run(loop);
}

void on_streamtree_cursor_changed()
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    sqlite3_stmt *statement;
    gchar *name;
    gchar *url;
    GValue value = G_VALUE_INIT;

    selection = gtk_tree_view_get_selection(streamtree);
    if (gtk_tree_selection_get_selected(selection, &streamstore, &iter)) {
        gtk_tree_model_get_value(streamstore, &iter, COLUMN_NAME, &value);
        name = (gchar *)g_value_get_string(&value);
        sqlite3_prepare_v2(database, "SELECT url FROM stream WHERE name=?", -1, &statement, NULL);
        sqlite3_bind_text(statement, 1, name, strlen(name), SQLITE_STATIC);
        sqlite3_step(statement);
        url = (gchar *)sqlite3_column_text(statement, 0);
        stop_stream();
        start_stream(url);
        sqlite3_finalize(statement);
    }
}

void on_stopbutton_clicked()
{
    stop_stream();
    set_status("", FALSE);
}

void on_window_destroy()
{
    stop_stream();
    sqlite3_close(database);
    gtk_main_quit();
}

void is_file(const char *FILENAME)
{
    if (access(FILENAME, R_OK) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                   "Error while opening the file '%s'.", FILENAME);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        exit(1);
    }
}

void initialize_gtk()
{
    GtkBuilder *builder;
    GtkCssProvider *provider;
    GtkWidget *window;

    gtk_init(NULL, NULL);

    is_file(STFILE);
    is_file(DBFILE);
    is_file(UIFILE);

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, UIFILE, NULL);
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    gtk_builder_connect_signals(builder, NULL);
    provider = gtk_css_provider_get_default();
    gtk_css_provider_load_from_path(provider, STFILE, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    streamtree = GTK_TREE_VIEW(gtk_builder_get_object(builder, "streamtree"));
    streamstore = GTK_TREE_MODEL(gtk_builder_get_object(builder, "streamstore"));
    statuslabel = GTK_LABEL(gtk_builder_get_object(builder, "statuslabel"));
    stopbutton = GTK_BUTTON(gtk_builder_get_object(builder, "stopbutton"));

    g_object_unref(builder);
    sqlite3_open(DBFILE, &database);
    set_streams();
    gtk_widget_show(window);
    gtk_main();
}

int main()
{
    initialize_gtk();
    return 0;
}
