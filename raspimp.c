#include <gtk/gtk.h>
#include <gst/gst.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

#include "keyboard.h"

enum {
    COLUMN_NAME,
    COLUMN_COUNT
};

#ifdef __arm__
const char *RASPIMP_GLADE_FILE = "/home/alarm/.config/raspimp/raspimp.glade";
const char *RASPIMP_SQLITE_FILE = "/home/alarm/.config/raspimp/raspimp.db";
const char *RASPIMP_CSS_FILE = "/home/alarm/.config/raspimp/raspimp.css";
const char *RASPIMP_MUSIC_DIR = "/home/alarm/music";
#else
const char *RASPIMP_GLADE_FILE = "raspimp.glade";
const char *RASPIMP_SQLITE_FILE = "raspimp.db";
const char *RASPIMP_CSS_FILE = "raspimp.css";
const char *RASPIMP_MUSIC_DIR = "/home/dirk/Music/Bassdrive";
#endif

GtkLabel *statuslabel = NULL;
GtkLabel *signallabel = NULL;
GtkButton *stopbutton = NULL;
GtkWidget *keyboard = NULL;
GtkTreeModel *streamstore = NULL;
GtkTreeView *streamtree = NULL;
GtkEntry *streamfilterentry = NULL;
GtkTreeModel *musicstore = NULL;
GtkTreeView *musictree = NULL;
GtkEntry *musicfilterentry = NULL;

GMainLoop *loop = NULL;
GstElement *playbin = NULL;
GstBus *bus = NULL;

GtkTreeView *playingtree = NULL;
GtkTreeModel *playingmodel = NULL;
gchar *playingname = NULL;

gint signaltimeout = 0;

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
    playingtree = NULL;
    playingname = NULL;
    playingmodel = NULL;
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

void set_streams(const gchar *filter)
{
    int result;
    sqlite3_stmt *statement;
    GtkTreeIter iter;

    if (filter == NULL || strlen(filter) == 0) {
        sqlite3_prepare_v2(database, "SELECT name FROM stream ORDER BY NAME", -1, &statement, NULL);
    } else {
        char *format = "SELECT name FROM stream WHERE name LIKE '%%%s%%' ORDER BY NAME";
        size_t size = strlen(format) + strlen(filter);
        char query[size];
        snprintf(query, size, format, filter);
        sqlite3_prepare_v2(database, query, -1, &statement, NULL);
    }

    gtk_list_store_clear(GTK_LIST_STORE(streamstore));
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        gtk_list_store_append(GTK_LIST_STORE(streamstore), &iter);
        gtk_list_store_set(GTK_LIST_STORE(streamstore), &iter, COLUMN_NAME, sqlite3_column_text(statement, 0), -1);
    }
    sqlite3_finalize(statement);
}

void set_music(const gchar *filter)
{
    int result;
    sqlite3_stmt *statement;
    GtkTreeIter iter;

    if (filter == NULL || strlen(filter) == 0) {
        sqlite3_prepare_v2(database, "SELECT name FROM music ORDER BY NAME", -1, &statement, NULL);
    } else {
        char *format = "SELECT name FROM music WHERE name LIKE '%%%s%%' ORDER BY NAME";
        size_t size = strlen(format) + strlen(filter);
        char query[size];
        snprintf(query, size, format, filter);
        sqlite3_prepare_v2(database, query, -1, &statement, NULL);
    }

    gtk_list_store_clear(GTK_LIST_STORE(musicstore));
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        gtk_list_store_append(GTK_LIST_STORE(musicstore), &iter);
        gtk_list_store_set(GTK_LIST_STORE(musicstore), &iter, COLUMN_NAME, sqlite3_column_text(statement, 0), -1);
    }
    sqlite3_finalize(statement);
}

gchar *get_selection_name(GtkTreeView *treeview, GtkTreeModel *treemodel)
{
    GtkTreeIter iter;
    GValue value = G_VALUE_INIT;
    gchar *name = NULL;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);

    if (gtk_tree_selection_get_selected(selection, &treemodel, &iter)) {
        gtk_tree_model_get_value(treemodel, &iter, COLUMN_NAME, &value);
        name = (gchar *)g_value_get_string(&value);
    }
    return name;
}

void start_stream(const gchar *url, GtkTreeView *treeview, GtkTreeModel *treemodel)
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
    playingname = get_selection_name(treeview, treemodel);
    playingtree = treeview;
    playingmodel = treemodel;
    g_main_loop_run(loop);
}

void on_streamtree_row_activated()
{
    gchar *name = get_selection_name(streamtree, streamstore);

    if (name != NULL && keyboard == NULL) {
        sqlite3_stmt *statement;
        sqlite3_prepare_v2(database, "SELECT url FROM stream WHERE name=?", -1, &statement, NULL);
        sqlite3_bind_text(statement, 1, name, strlen(name), SQLITE_STATIC);
        sqlite3_step(statement);
        gchar *url = (gchar *)sqlite3_column_text(statement, 0);
        stop_stream();
        start_stream(url, streamtree, streamstore);
        sqlite3_finalize(statement);
    }
}

void on_musictree_row_activated()
{
    gchar *name = get_selection_name(musictree, musicstore);

    if (name != NULL && keyboard == NULL) {
        sqlite3_stmt *statement;
        sqlite3_prepare_v2(database, "SELECT url FROM music WHERE name=?", -1, &statement, NULL);
        sqlite3_bind_text(statement, 1, name, strlen(name), SQLITE_STATIC);
        sqlite3_step(statement);
        gchar *url = (gchar *)sqlite3_column_text(statement, 0);
        stop_stream();
        start_stream(url, musictree, musicstore);
        sqlite3_finalize(statement);
    }
}

void on_stopbutton_clicked()
{
    if (playingtree != NULL) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(playingtree);
        gtk_tree_selection_unselect_all(selection);
    }
    set_status("", FALSE);
    stop_stream();
}

void on_keyboard_destroy()
{
    if (playingtree != NULL && playingmodel != NULL && playingname != NULL) {
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(playingmodel, &iter);
        gboolean found = FALSE;

        GtkTreeSelection *selection = gtk_tree_view_get_selection(playingtree);
        gtk_tree_selection_unselect_all(selection);
        while (valid && !found) {
            GValue value = G_VALUE_INIT;
            gtk_tree_model_get_value(playingmodel, &iter, COLUMN_NAME, &value);
            gchar *name = (gchar *)g_value_get_string(&value);
            if (g_strcmp0(name, playingname) == 0) {
                gtk_tree_selection_select_iter(selection, &iter);
                found = TRUE;
            }
            valid = gtk_tree_model_iter_next(playingmodel, &iter);
        }
    }
    keyboard = NULL;
}

void on_streamfilterentry_button_press_event()
{
    keyboard = keyboard_show(streamfilterentry);
    g_signal_connect(G_OBJECT(keyboard), "destroy", G_CALLBACK(on_keyboard_destroy), NULL);
}

void on_musicfilterentry_button_press_event()
{
    keyboard = keyboard_show(musicfilterentry);
    g_signal_connect(G_OBJECT(keyboard), "destroy", G_CALLBACK(on_keyboard_destroy), NULL);
}

void on_streamfilterentry_insert_text(GtkEditable *editable, gchar *text)
{
    UNUSED(editable);

    set_streams(text);
}

void on_musicfilterentry_insert_text(GtkEditable *editable, gchar *text)
{
    UNUSED(editable);

    set_music(text);
}

void on_window_destroy()
{
    g_source_remove(signaltimeout);
    stop_stream();
    sqlite3_close(database);
    gtk_main_quit();
}

gboolean set_wifi_signal_strength()
{
    FILE *file = popen("iwconfig wlp3s0", "r");

    if (file != NULL) {
        char *line = NULL;
        size_t length = 0;
        ssize_t read;
        const char *rquality = "Quality=[0-9]*/[0-9]*";
        regex_t regex;
        int found = 0;
        regcomp(&regex, rquality, 0);
        while ((read = getline(&line, &length, file)) != -1 && found == 0) {
            regmatch_t matches[1];
            if (!regexec(&regex, line, 1, matches, 0)) {
                int size = matches[0].rm_eo - matches[0].rm_so + 1;
                char cquality[size];
                strncpy(cquality, line + matches[0].rm_so + 8, size - 8);

                int now = 0;
                int max = 0;

                char *token = strtok(cquality, "/");
                if (token != NULL)
                    now = atoi(token);
                token = strtok(NULL, "/");
                if (token != NULL)
                    max = atoi(token);

                double strength = (double)now / max * 100.0;
                gchar sstrength[5];
                g_snprintf(sstrength, 5, "%.0f%%", strength);
                gtk_label_set_text(signallabel, sstrength);
                found = 1;
            }
        }
        free(line);
        pclose(file);
    }
    return TRUE;
}

void set_music_database(const gchar *path)
{
    GError *error;
    const gchar *filename;
    GDir *dir = g_dir_open(path, 0, &error);

    while ((filename = g_dir_read_name(dir))) {
        gchar *pformat = "%s/%s";
        gulong psize = strlen(pformat) + strlen(path) + strlen(filename);
        gchar npath[psize];
        g_snprintf(npath, psize, pformat, path, filename);
        if (g_file_test(npath, G_FILE_TEST_IS_DIR) == TRUE) {
            set_music_database(npath);
        } else {
            if (g_str_has_suffix(filename, ".mp3")) {
                gchar *uformat = "file://%s";
                gulong usize = strlen(uformat) + strlen(npath);
                gchar url[usize];
                g_snprintf(url, usize, uformat, npath);

                sqlite3_stmt *sstatement;
                gchar *sformat = "SELECT COUNT(*) FROM music WHERE name=? AND url=?";
                gulong ssize = strlen(sformat) + strlen(filename) + strlen(url);
                gchar squery[ssize];
                g_snprintf(squery, ssize, sformat, filename, url);
                sqlite3_prepare_v2(database, squery, -1, &sstatement, NULL);
                sqlite3_bind_text(sstatement, 1, filename, strlen(filename), SQLITE_STATIC);
                sqlite3_bind_text(sstatement, 2, url, strlen(url), SQLITE_STATIC);
                sqlite3_step(sstatement);
                gint result = sqlite3_column_int(sstatement, 0);
                sqlite3_finalize(sstatement);

                if (result == 0) {
                    sqlite3_stmt *istatement;
                    gchar *iformat = "INSERT INTO music(name, url) VALUES(?, ?)";
                    gulong isize = strlen(iformat) + strlen(filename) + strlen(url);
                    gchar iquery[isize];
                    g_snprintf(iquery, isize, iformat, filename, url);
                    sqlite3_prepare_v2(database, iquery, -1, &istatement, NULL);
                    sqlite3_bind_text(istatement, 1, filename, strlen(filename), SQLITE_STATIC);
                    sqlite3_bind_text(istatement, 2, url, strlen(url), SQLITE_STATIC);
                    sqlite3_step(istatement);
                    sqlite3_finalize(istatement);
                }
            }
        }
    }
    g_dir_close(dir);
}

void get_files_in_directory(const gchar *path, gdouble *count)
{
    GError *error;
    const gchar *filename;
    GDir *dir = g_dir_open(path, 0, &error);

    while ((filename = g_dir_read_name(dir))) {
        gchar *format = "%s/%s";
        gulong size = strlen(format) + strlen(path) + strlen(filename);
        gchar subpath[size];
        g_snprintf(subpath, size, format, path, filename);
        if (g_file_test(subpath, G_FILE_TEST_IS_DIR) == TRUE)
            get_files_in_directory(subpath, count);
        else
            (*count)++;
    }
    g_dir_close(dir);
}

void on_rescanbutton_clicked()
{
    gdouble count = 0;

    get_files_in_directory(RASPIMP_MUSIC_DIR, &count);
    gchar *format = "Scanning mp3 files from directory '%s'.";
    gulong size = strlen(format) + strlen(RASPIMP_MUSIC_DIR);
    gchar message[size];
    snprintf(message, size, format, RASPIMP_MUSIC_DIR);
    sqlite3_exec(database, "DELETE FROM music", NULL, 0, NULL);
    set_music_database(RASPIMP_MUSIC_DIR);
}

void is_file(const char *FILENAME)
{
    if (access(FILENAME, R_OK) != 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE, "Error while opening the file '%s'.", FILENAME);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        exit(1);
    }
}

void initialize_gtk()
{
    gtk_init(NULL, NULL);

    is_file(RASPIMP_CSS_FILE);
    is_file(RASPIMP_SQLITE_FILE);
    is_file(RASPIMP_GLADE_FILE);
    is_file(KEYBOARD_GLADE_FILE);
    is_file(RASPIMP_MUSIC_DIR);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, RASPIMP_GLADE_FILE, NULL);
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    gtk_builder_connect_signals(builder, NULL);
    GtkCssProvider *provider = gtk_css_provider_get_default();
    gtk_css_provider_load_from_path(provider, RASPIMP_CSS_FILE, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    statuslabel = GTK_LABEL(gtk_builder_get_object(builder, "statuslabel"));
    signallabel = GTK_LABEL(gtk_builder_get_object(builder, "signallabel"));
    stopbutton = GTK_BUTTON(gtk_builder_get_object(builder, "stopbutton"));
    streamtree = GTK_TREE_VIEW(gtk_builder_get_object(builder, "streamtree"));
    streamstore = GTK_TREE_MODEL(gtk_builder_get_object(builder, "streamstore"));
    streamfilterentry = GTK_ENTRY(gtk_builder_get_object(builder, "streamfilterentry"));
    musictree = GTK_TREE_VIEW(gtk_builder_get_object(builder, "musictree"));
    musicstore = GTK_TREE_MODEL(gtk_builder_get_object(builder, "musicstore"));
    musicfilterentry = GTK_ENTRY(gtk_builder_get_object(builder, "musicfilterentry"));

    g_object_unref(builder);
    sqlite3_open(RASPIMP_SQLITE_FILE, &database);
    set_music_database(RASPIMP_MUSIC_DIR);
    set_streams(NULL);
    set_music(NULL);
    signaltimeout = g_timeout_add(5000, set_wifi_signal_strength, NULL);
    gtk_widget_show(window);
    gtk_main();
}

int main()
{
    initialize_gtk();
    return 0;
}
