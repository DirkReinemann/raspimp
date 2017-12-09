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
const char *RASPIMP_GLADE_FILE = "/usr/share/raspimp/raspimp.glade";
const char *RASPIMP_CSS_FILE = "/usr/share/raspimp/raspimp.css";
const char *RASPIMP_PAUSE_IMAGE = "/usr/share/raspimp/pause.png";
const char *RASPIMP_PLAY_IMAGE = "/usr/share/raspimp/play.png";
const char *RASPIMP_STOP_IMAGE = "/usr/share/raspimp/stop.png";
const char *RASPIMP_SHUTDOWN_IMAGE = "/usr/share/raspimp/shutdown.png";
const char *RASPIMP_WLAN_IMAGE_FORMAT = "/usr/share/raspimp/wlan%i.png";
const char *RASPIMP_SQL_FILE = "/usr/share/raspimp/raspimp.sql";
const char *RASPIMP_DB_FILENAME = ".raspimp.db";
const char *RASPIMP_MUSIC_DIR = "Music";
#else
const char *RASPIMP_GLADE_FILE = "raspimp.glade";
const char *RASPIMP_CSS_FILE = "raspimp.css";
const char *RASPIMP_PAUSE_IMAGE = "pause.png";
const char *RASPIMP_PLAY_IMAGE = "play.png";
const char *RASPIMP_STOP_IMAGE = "stop.png";
const char *RASPIMP_SHUTDOWN_IMAGE = "shutdown.png";
const char *RASPIMP_WLAN_IMAGE_FORMAT = "wlan%i.png";
const char *RASPIMP_SQL_FILE = "raspimp.sql";
const char *RASPIMP_DB_FILENAME = ".raspimp.db";
const char *RASPIMP_MUSIC_DIR = "Music/Diverse";
#endif

const char *WIRELESS_FILE = "/proc/net/wireless";

GtkWidget *window = NULL;
GtkLabel *statuslabel = NULL;
GtkLabel *positionlabel = NULL;
GtkButton *stopbutton = NULL;
GtkButton *pausebutton = NULL;
GtkWidget *keyboard = NULL;
GtkTreeModel *streamstore = NULL;
GtkTreeView *streamtree = NULL;
GtkEntry *streamfilterentry = NULL;
GtkTreeModel *musicstore = NULL;
GtkTreeView *musictree = NULL;
GtkEntry *musicfilterentry = NULL;
GtkAdjustment *adjustment = NULL;
GtkScale *scale = NULL;
GtkNotebook *notebook = NULL;
GtkImage *pauseimage = NULL;
GtkImage *stopimage = NULL;
GtkImage *shutdownimage = NULL;
GtkImage *wlanimage = NULL;

GMainLoop *loop = NULL;
GstElement *playbin = NULL;
GstBus *bus = NULL;

GtkTreeView *playingtree = NULL;
GtkTreeModel *playingmodel = NULL;
gchar *playingname = NULL;

gint signaltimeout = 0;
gint positiontimeout = 0;
gint paused = 0;

gchar *wlaninterface = NULL;

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
    gtk_widget_set_sensitive(GTK_WIDGET(pausebutton), FALSE);
    gtk_label_set_label(positionlabel, "00:00:00 / 00:00:00");
    gtk_image_set_from_file(pauseimage, RASPIMP_PAUSE_IMAGE);
    paused = 0;
    gtk_adjustment_set_value(adjustment, 0.00);
    gtk_widget_set_sensitive(GTK_WIDGET(scale), FALSE);
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
    gtk_widget_set_sensitive(GTK_WIDGET(pausebutton), TRUE);
    if (gtk_notebook_get_current_page(notebook) == 1)
        gtk_widget_set_sensitive(GTK_WIDGET(scale), TRUE);
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

void on_pausebutton_clicked()
{
    if (playbin != NULL) {
        if (paused) {
            gst_element_set_state(playbin, GST_STATE_PLAYING);
            gtk_image_set_from_file(pauseimage, RASPIMP_PAUSE_IMAGE);
            paused = 0;
        } else {
            gst_element_set_state(playbin, GST_STATE_PAUSED);
            gtk_image_set_from_file(pauseimage, RASPIMP_PLAY_IMAGE);
            paused = 1;
        }
    }
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
    g_source_remove(positiontimeout);
    g_free(wlaninterface);
    stop_stream();
    sqlite3_close(database);
    gtk_main_quit();
}

void on_scale_button_release_event()
{
    gint64 len;

    if (playbin != NULL) {
        if (gst_element_query_duration(playbin, GST_FORMAT_TIME, &len)) {
            gdouble value = gtk_adjustment_get_value(adjustment);
            gint64 pos = len / 100 * value;
            gst_element_seek(playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, pos,
                             GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
            gtk_window_set_focus(GTK_WINDOW(window), GTK_WIDGET(stopbutton));
        }
    }
}

void on_shutdownbutton_clicked()
{
    system("sudo poweroff");
}

char *get_wlan_interface(gchar *name)
{
    FILE *file = popen("ip link", "r");

    if (file != NULL) {
        char *line = NULL;
        size_t length = 0;
        ssize_t read = 0;
        char *token = NULL;
        int found = 0;
        regex_t regex;
        regcomp(&regex, "^ wl[0-9a-z]*$", 0);
        while ((read = getline(&line, &length, file)) != -1 && found == 0) {
            token = strtok(line, ":");
            while (token != NULL && found == 0) {
                if (!regexec(&regex, token, 0, NULL, 0)) {
                    gint size = strlen(token);
                    name = malloc(sizeof(token) * size);
                    g_strlcpy(name, token + 1, size);
                    found = 1;
                }
                token = strtok(NULL, ":");
            }
        }
        fclose(file);
    }
    return name;
}

gboolean set_wifi_signal_strength()
{
    int strength = 0;

    if (access(WIRELESS_FILE, F_OK) != -1) {
        FILE *file = fopen(WIRELESS_FILE, "r");
        if (file != NULL) {
            char *line = NULL;
            size_t size = 0;
            ssize_t read = 0;
            regex_t regex;
            regcomp(&regex, wlaninterface, 0);
            while ((read = getline(&line, &size, file)) != -1)
                if (!regexec(&regex, line, 0, NULL, 0)) {
                    char *token = strtok(line, " ");
                    int i = 0;
                    while (token && i < 4) {
                        if (i == 3)
                            strength = atol(token);
                        token = strtok(NULL, " ");
                        i++;
                    }
                }
            fclose(file);
            if (strength < 0)
                strength = 110 + strength;
        }
    }
    int image = 0;
    if (strength > 90)
        image = 5;
    else if (strength > 70)
        image = 4;
    else if (strength > 50)
        image = 3;
    else if (strength > 30)
        image = 2;
    else if (strength > 10)
        image = 1;

    ssize_t size = strlen(RASPIMP_WLAN_IMAGE_FORMAT);
    gchar imagename[size];
    g_snprintf(imagename, size, RASPIMP_WLAN_IMAGE_FORMAT, image);
    gtk_image_set_from_file(wlanimage, imagename);
    return TRUE;
}

gchar *format_time(gint64 value, gchar *format)
{
    gint hours = value / (GST_SECOND * 60 * 60);
    gint minutes = (value / (GST_SECOND * 60)) % 60;
    gint seconds = (value / GST_SECOND) % 60;
    gint size = 9;

    format = g_malloc(size * sizeof(gchar));
    g_snprintf(format, size, "%02i:%02i:%02i", hours, minutes, seconds);
    return format;
}

gboolean set_position()
{
    gint64 pos = 0, len = 0;
    gchar *fpos = NULL, *flen = NULL;

    if (playbin != NULL) {
        if (gst_element_query_position(playbin, GST_FORMAT_TIME, &pos) &&
            gst_element_query_duration(playbin, GST_FORMAT_TIME, &len)) {
            fpos = format_time(pos, fpos);
            flen = format_time(len, flen);
            gint size = strlen(fpos) + strlen(flen) + 4;
            gchar position[size];
            g_snprintf(position, size, "%s / %s", fpos, flen);
            g_free(fpos);
            g_free(flen);
            gtk_label_set_label(positionlabel, position);
            gdouble percent = (gdouble)pos / (gdouble)len * 100.0;
            if (percent > 0)
                gtk_adjustment_set_value(adjustment, percent);
        }
    }
    return TRUE;
}

void set_music_database(const gchar *path)
{
    GError *error = NULL;
    GDir *dir = g_dir_open(path, 0, &error);

    if (dir != NULL) {
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
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

                    sqlite3_stmt *statement;
                    gchar *iformat = "INSERT INTO music(name, url) VALUES(?, ?)";
                    gulong isize = strlen(iformat) + strlen(filename) + strlen(url);
                    gchar iquery[isize];
                    g_snprintf(iquery, isize, iformat, filename, url);
                    sqlite3_prepare_v2(database, iquery, -1, &statement, NULL);
                    sqlite3_bind_text(statement, 1, filename, strlen(filename), SQLITE_STATIC);
                    sqlite3_bind_text(statement, 2, url, strlen(url), SQLITE_STATIC);
                    sqlite3_step(statement);
                    sqlite3_finalize(statement);
                }
            }
        }
        g_dir_close(dir);
    }
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

gboolean on_window_key_press_event(GtkWidget *widget, GdkEventKey *event)
{
    UNUSED(widget);
    switch (event->keyval) {
    case GDK_KEY_i: {
        // use this keypress event to test a functionality
    }
    }
    return FALSE;
}

void initialize_database(const gchar *musicdir)
{
#ifdef __arm__
    char *home = getenv("HOME");
    int dsize = strlen(home) + strlen(RASPIMP_DB_FILENAME) + 2;
    char databasefile[dsize];
    snprintf(databasefile, dsize, "%s/%s", home, RASPIMP_DB_FILENAME);
#else
    const char *databasefile = RASPIMP_DB_FILENAME;
#endif

    if (access(databasefile, R_OK) != 0) {
        int csize = strlen(databasefile) + strlen(RASPIMP_SQL_FILE) + 12;
        char command[csize];
        snprintf(command, csize, "sqlite3 %s < %s", databasefile, RASPIMP_SQL_FILE);
        system(command);
    }

    sqlite3_open(databasefile, &database);
    sqlite3_exec(database, "DELETE FROM music", NULL, 0, NULL);
    set_music_database(musicdir);
}

void initialize_gtk()
{
    gtk_init(NULL, NULL);

    gchar *home = (gchar *)getenv("HOME");
    gint msize = strlen(home) + strlen(RASPIMP_MUSIC_DIR) + 2;
    gchar musicdir[msize];
    g_snprintf(musicdir, msize, "%s/%s", home, RASPIMP_MUSIC_DIR);

    is_file(RASPIMP_CSS_FILE);
    is_file(RASPIMP_SQL_FILE);
    is_file(RASPIMP_GLADE_FILE);
    is_file(KEYBOARD_GLADE_FILE);
    is_file(RASPIMP_PLAY_IMAGE);
    is_file(RASPIMP_PAUSE_IMAGE);
    is_file(RASPIMP_STOP_IMAGE);
    is_file(RASPIMP_SHUTDOWN_IMAGE);
    is_file(musicdir);

    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, RASPIMP_GLADE_FILE, NULL);
    gtk_builder_connect_signals(builder, NULL);
    GtkCssProvider *provider = gtk_css_provider_get_default();
    gtk_css_provider_load_from_path(provider, RASPIMP_CSS_FILE, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    statuslabel = GTK_LABEL(gtk_builder_get_object(builder, "statuslabel"));
    positionlabel = GTK_LABEL(gtk_builder_get_object(builder, "positionlabel"));
    stopbutton = GTK_BUTTON(gtk_builder_get_object(builder, "stopbutton"));
    pausebutton = GTK_BUTTON(gtk_builder_get_object(builder, "pausebutton"));
    streamtree = GTK_TREE_VIEW(gtk_builder_get_object(builder, "streamtree"));
    streamstore = GTK_TREE_MODEL(gtk_builder_get_object(builder, "streamstore"));
    streamfilterentry = GTK_ENTRY(gtk_builder_get_object(builder, "streamfilterentry"));
    musictree = GTK_TREE_VIEW(gtk_builder_get_object(builder, "musictree"));
    musicstore = GTK_TREE_MODEL(gtk_builder_get_object(builder, "musicstore"));
    musicfilterentry = GTK_ENTRY(gtk_builder_get_object(builder, "musicfilterentry"));
    adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment"));
    scale = GTK_SCALE(gtk_builder_get_object(builder, "scale"));
    notebook = GTK_NOTEBOOK(gtk_builder_get_object(builder, "notebook"));
    pauseimage = GTK_IMAGE(gtk_builder_get_object(builder, "pauseimage"));
    stopimage = GTK_IMAGE(gtk_builder_get_object(builder, "stopimage"));
    shutdownimage = GTK_IMAGE(gtk_builder_get_object(builder, "shutdownimage"));
    wlanimage = GTK_IMAGE(gtk_builder_get_object(builder, "wlanimage"));

    g_object_unref(builder);
    gtk_image_set_from_file(pauseimage, RASPIMP_PAUSE_IMAGE);
    gtk_image_set_from_file(stopimage, RASPIMP_STOP_IMAGE);
    gtk_image_set_from_file(shutdownimage, RASPIMP_SHUTDOWN_IMAGE);

    gchar imagename[strlen(RASPIMP_WLAN_IMAGE_FORMAT)];
    g_snprintf(imagename, strlen(RASPIMP_WLAN_IMAGE_FORMAT), RASPIMP_WLAN_IMAGE_FORMAT, 0);
    gtk_image_set_from_file(wlanimage, imagename);
    wlaninterface = get_wlan_interface(wlaninterface);
    initialize_database(musicdir);
    set_streams(NULL);
    set_music(NULL);
    signaltimeout = g_timeout_add(5000, set_wifi_signal_strength, NULL);
    positiontimeout = g_timeout_add(500, set_position, NULL);
    gtk_widget_show(window);
    gtk_main();
}

int main()
{
    initialize_gtk();
    return 0;
}
