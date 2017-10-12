#include <gtk/gtk.h>
#include <string.h>

#ifdef __arm__
const char *KEYBOARD_GLADE_FILE = "/home/alarm/.config/raspimp/keyboard.glade";
#else
const char *KEYBOARD_GLADE_FILE = "keyboard.glade";
#endif

GtkEntry *entry = NULL;
GtkWidget *keyboardwindow = NULL;

void add_entry_value(const gchar *value)
{
    const gchar *text = gtk_entry_get_text(entry);
    gulong size = strlen(text) + strlen(value) + 1;
    gchar result[size];

    g_snprintf(result, size, "%s%s", text, value);
    gtk_entry_set_text(entry, result);
}

void on_zerobutton_clicked()
{
    add_entry_value("0");
}

void on_onebutton_clicked()
{
    add_entry_value("1");
}

void on_twobutton_clicked()
{
    add_entry_value("2");
}

void on_threebutton_clicked()
{
    add_entry_value("3");
}

void on_fourbutton_clicked()
{
    add_entry_value("4");
}

void on_fivebutton_clicked()
{
    add_entry_value("5");
}

void on_sixbutton_clicked()
{
    add_entry_value("6");
}

void on_sevenbutton_clicked()
{
    add_entry_value("7");
}

void on_eightbutton_clicked()
{
    add_entry_value("8");
}

void on_ninebutton_clicked()
{
    add_entry_value("9");
}

void on_abutton_clicked()
{
    add_entry_value("a");
}

void on_bbutton_clicked()
{
    add_entry_value("b");
}

void on_cbutton_clicked()
{
    add_entry_value("c");
}

void on_dbutton_clicked()
{
    add_entry_value("d");
}

void on_ebutton_clicked()
{
    add_entry_value("e");
}

void on_fbutton_clicked()
{
    add_entry_value("f");
}

void on_gbutton_clicked()
{
    add_entry_value("g");
}

void on_hbutton_clicked()
{
    add_entry_value("h");
}

void on_ibutton_clicked()
{
    add_entry_value("i");
}

void on_jbutton_clicked()
{
    add_entry_value("j");
}

void on_kbutton_clicked()
{
    add_entry_value("k");
}

void on_lbutton_clicked()
{
    add_entry_value("l");
}

void on_mbutton_clicked()
{
    add_entry_value("m");
}

void on_nbutton_clicked()
{
    add_entry_value("n");
}

void on_obutton_clicked()
{
    add_entry_value("o");
}

void on_pbutton_clicked()
{
    add_entry_value("p");
}

void on_qbutton_clicked()
{
    add_entry_value("q");
}

void on_rbutton_clicked()
{
    add_entry_value("r");
}

void on_sbutton_clicked()
{
    add_entry_value("s");
}

void on_tbutton_clicked()
{
    add_entry_value("t");
}

void on_ubutton_clicked()
{
    add_entry_value("u");
}

void on_vbutton_clicked()
{
    add_entry_value("v");
}

void on_wbutton_clicked()
{
    add_entry_value("w");
}

void on_xbutton_clicked()
{
    add_entry_value("x");
}

void on_ybutton_clicked()
{
    add_entry_value("y");
}

void on_zbutton_clicked()
{
    add_entry_value("z");
}

void on_aebutton_clicked()
{
    add_entry_value("ä");
}

void on_uebutton_clicked()
{
    add_entry_value("ü");
}

void on_oebutton_clicked()
{
    add_entry_value("ö");
}

void on_spacebutton_clicked()
{
    add_entry_value(" ");
}

void on_closebutton_clicked()
{
    gtk_window_close(GTK_WINDOW(keyboardwindow));
}

void on_returnbutton_clicked()
{
    const gchar *text = gtk_entry_get_text(entry);
    gsize size = strlen(text);

    if (strlen(text) > 0) {
        gchar result[size];
        g_strlcpy(result, text, size);
        gtk_entry_set_text(entry, result);
    }
}

void on_clearbutton_clicked()
{
    gtk_entry_set_text(entry, "");
}

GtkWidget *keyboard_show(GtkEntry *e)
{
    entry = e;
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, KEYBOARD_GLADE_FILE, NULL);
    keyboardwindow = GTK_WIDGET(gtk_builder_get_object(builder, "window"));

    GtkButton *closebutton = GTK_BUTTON(gtk_builder_get_object(builder, "closebutton"));
    gtk_button_set_label(closebutton, "\u2715");
    GtkButton *spacebutton = GTK_BUTTON(gtk_builder_get_object(builder, "spacebutton"));
    gtk_button_set_label(spacebutton, "");
    GtkButton *returnbutton = GTK_BUTTON(gtk_builder_get_object(builder, "returnbutton"));
    gtk_button_set_label(returnbutton, "\u2190");
    GtkButton *clearbutton = GTK_BUTTON(gtk_builder_get_object(builder, "clearbutton"));

    gtk_button_set_label(clearbutton, "\u21a4");
    gtk_window_move(GTK_WINDOW(keyboardwindow), 0, 240);
    gtk_widget_show(keyboardwindow);
    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(builder);
    return keyboardwindow;
}
