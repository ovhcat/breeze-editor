#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <string.h>
#include <stdlib.h>

GtkWidget *statusbar;
GtkWidget *notebook;
gchar *current_file = NULL;
gboolean is_modified = FALSE;
GtkWidget *prefs_dialog;
GtkSourceBuffer *buffer;
GtkWidget *text_view;

#define DEFAULT_AUTO_SAVE_INTERVAL 30000  // Default auto-save every 30 seconds
int auto_save_interval = DEFAULT_AUTO_SAVE_INTERVAL;

// Structure to store preferences
typedef struct {
    int font_size;
    gboolean word_wrap;
    gboolean dark_mode;
    gchar *font_family;
    gchar *theme;
} Preferences;

Preferences prefs = {12, TRUE, FALSE, "Monospace", "Light"};

// Apply syntax highlighting based on file extension
void apply_syntax_highlighting(GtkSourceBuffer *buffer, const gchar *filename) {
    GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();
    GtkSourceLanguage *language = NULL;

    if (filename) {
        const gchar *ext = strrchr(filename, '.');
        if (ext) {
            if (g_strcmp0(ext, ".c") == 0 || g_strcmp0(ext, ".h") == 0) {
                language = gtk_source_language_manager_get_language(lm, "c");
            } else if (g_strcmp0(ext, ".py") == 0) {
                language = gtk_source_language_manager_get_language(lm, "python");
            } else if (g_strcmp0(ext, ".js") == 0) {
                language = gtk_source_language_manager_get_language(lm, "javascript");
            } else if (g_strcmp0(ext, ".html") == 0) {
                language = gtk_source_language_manager_get_language(lm, "html");
            } else if (g_strcmp0(ext, ".css") == 0) {
                language = gtk_source_language_manager_get_language(lm, "css");
            }
            // Add more languages as needed
        }
    }

    if (language) {
        gtk_source_buffer_set_language(buffer, language);
    }
}

// Update status bar with line, column, character count
void update_status_bar(GtkSourceBuffer *buffer, GtkStatusbar *statusbar) {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(buffer), &iter, gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(buffer)));

    int line = gtk_text_iter_get_line(&iter) + 1;
    int col = gtk_text_iter_get_line_offset(&iter) + 1;
    int char_count = gtk_text_buffer_get_char_count(GTK_TEXT_BUFFER(buffer));

    gchar *status = g_strdup_printf("Line: %d  Col: %d  Chars: %d", line, col, char_count);
    gtk_statusbar_pop(statusbar, 0);
    gtk_statusbar_push(statusbar, 0, status);
    g_free(status);
}

// Mark buffer as modified
void mark_as_modified(GtkSourceBuffer *buffer, gpointer data) {
    is_modified = TRUE;
}

// Undo function
void undo(GtkWidget *widget, gpointer data) {
    if (gtk_source_buffer_can_undo(buffer)) {
        gtk_source_buffer_undo(buffer);
    }
}

// Redo function
void redo(GtkWidget *widget, gpointer data) {
    if (gtk_source_buffer_can_redo(buffer)) {
        gtk_source_buffer_redo(buffer);
    }
}

// Save file
void save_file(GtkWidget *widget) {
    if (!current_file) {
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File As",
                                                        GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        "_Save", GTK_RESPONSE_ACCEPT,
                                                        NULL);
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            current_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_widget_destroy(dialog);
        } else {
            gtk_widget_destroy(dialog);
            return;
        }
    }

    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
    gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
    gchar *content = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);
    if (g_file_set_contents(current_file, content, -1, NULL) == FALSE) {
        g_printerr("Failed to save file: %s\n", current_file);
    }
    g_free(content);
    is_modified = FALSE;
}

// Save As function
void save_file_as(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File As",
                                                    GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        g_free(current_file);
        current_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
        gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
        gchar *content = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);
        if (g_file_set_contents(current_file, content, -1, NULL) == FALSE) {
            g_printerr("Failed to save file: %s\n", current_file);
        }
        g_free(content);
        is_modified = FALSE;
    }

    gtk_widget_destroy(dialog);
}

// Import file
void import_file(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Import File",
                                                    GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gchar *content = NULL;

        if (g_file_get_contents(filename, &content, NULL, NULL)) {
            gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), content, -1);
            g_free(content);
            g_free(current_file);
            current_file = filename;
            is_modified = FALSE;
            apply_syntax_highlighting(buffer, filename);  // Apply syntax highlighting based on file type
        } else {
            g_printerr("Failed to open file: %s\n", filename);
            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
}

// Auto-save function
gboolean auto_save(gpointer data) {
    if (is_modified && current_file) {
        save_file(NULL);
    }
    return TRUE;
}

// New file
void new_file(GtkWidget *widget, gpointer data) {
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), "", -1);
    g_free(current_file);
    current_file = NULL;
    is_modified = FALSE;
    apply_syntax_highlighting(buffer, NULL);  // Clear syntax highlighting
}

// Update word wrap setting
void toggle_word_wrap(GtkCheckMenuItem *checkmenuitem, GtkTextView *text_view) {
    gboolean is_active = gtk_check_menu_item_get_active(checkmenuitem);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), is_active ? GTK_WRAP_WORD : GTK_WRAP_NONE);
}

// Adjust font size in the text view
void apply_font_size() {
    PangoFontDescription *font_desc = pango_font_description_from_string(prefs.font_family);
    pango_font_description_set_size(font_desc, prefs.font_size * PANGO_SCALE);
    gtk_widget_override_font(text_view, font_desc);
    pango_font_description_free(font_desc);
}

// Increase font size
void increase_font_size(GtkWidget *widget, gpointer data) {
    prefs.font_size += 1;
    apply_font_size();
}

// Decrease font size
void decrease_font_size(GtkWidget *widget, gpointer data) {
    if (prefs.font_size > 1) {  // Minimum font size of 1 to prevent zero or negative size
        prefs.font_size -= 1;
        apply_font_size();
    }
}

// Preferences dialog with font, theme, auto-save settings
void show_preferences_dialog(GtkWidget *widget, gpointer window) {
    prefs_dialog = gtk_dialog_new_with_buttons("Preferences",
                                               GTK_WINDOW(window),
                                               GTK_DIALOG_MODAL,
                                               "_OK", GTK_RESPONSE_OK,
                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                               NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(prefs_dialog));

    // Font Family Dropdown
    GtkWidget *font_label = gtk_label_new("Font Family:");
    gtk_box_pack_start(GTK_BOX(content_area), font_label, FALSE, FALSE, 5);
    GtkWidget *font_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(font_combo), "Monospace");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(font_combo), "Sans-serif");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(font_combo), "Serif");
    gtk_box_pack_start(GTK_BOX(content_area), font_combo, FALSE, FALSE, 5);

    // Theme Selection
    GtkWidget *theme_label = gtk_label_new("Theme:");
    gtk_box_pack_start(GTK_BOX(content_area), theme_label, FALSE, FALSE, 5);
    GtkWidget *theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(theme_combo), "Solarized Dark");
    gtk_box_pack_start(GTK_BOX(content_area), theme_combo, FALSE, FALSE, 5);

    // Auto-Save Interval
    GtkWidget *auto_save_label = gtk_label_new("Auto-Save Interval (seconds):");
    gtk_box_pack_start(GTK_BOX(content_area), auto_save_label, FALSE, FALSE, 5);
    GtkWidget *auto_save_spin = gtk_spin_button_new_with_range(0, 300, 10); // 0 for disabling
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(auto_save_spin), auto_save_interval / 1000);
    gtk_box_pack_start(GTK_BOX(content_area), auto_save_spin, FALSE, FALSE, 5);

    gtk_widget_show_all(prefs_dialog);

    if (gtk_dialog_run(GTK_DIALOG(prefs_dialog)) == GTK_RESPONSE_OK) {
        prefs.font_family = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(font_combo));
        prefs.theme = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(theme_combo));
        auto_save_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(auto_save_spin)) * 1000;

        // Apply font family
        apply_font_size();

        // Apply theme
        if (g_strcmp0(prefs.theme, "Dark") == 0) {
            GdkRGBA color;
            gdk_rgba_parse(&color, "#2e2e2e");
            gtk_widget_override_background_color(text_view, GTK_STATE_FLAG_NORMAL, &color);
        } else if (g_strcmp0(prefs.theme, "Solarized Dark") == 0) {
            GdkRGBA color;
            gdk_rgba_parse(&color, "#073642");
            gtk_widget_override_background_color(text_view, GTK_STATE_FLAG_NORMAL, &color);
        } else {
            gtk_widget_override_background_color(text_view, GTK_STATE_FLAG_NORMAL, NULL);
        }
    }
    gtk_widget_destroy(prefs_dialog);
}

// About dialog with additional info
void show_about_dialog(GtkWidget *widget, gpointer window) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "Text Editor\nVersion 2.0.0\n"
                                               "Created by ovhcat\n"
                                               "License: Gpl\n"
                                               "Source code available at https://github.com/ovhcat/breeze-editor/");

    gtk_window_set_title(GTK_WINDOW(dialog), "About");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Function to create the main editor window with tabs
void create_text_editor_window() {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Text Editor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // File Menu
    GtkWidget *fileMenu = gtk_menu_new();
    GtkWidget *fileItem = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileItem), fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileItem);

    GtkWidget *newItem = gtk_menu_item_new_with_label("New");
    g_signal_connect(newItem, "activate", G_CALLBACK(new_file), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newItem);

    GtkWidget *importItem = gtk_menu_item_new_with_label("Import");
    g_signal_connect(importItem, "activate", G_CALLBACK(import_file), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), importItem);

    GtkWidget *saveItem = gtk_menu_item_new_with_label("Save");
    g_signal_connect(saveItem, "activate", G_CALLBACK(save_file), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveItem);

    GtkWidget *saveAsItem = gtk_menu_item_new_with_label("Save As");
    g_signal_connect(saveAsItem, "activate", G_CALLBACK(save_file_as), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAsItem);

    // Preferences Menu
    GtkWidget *settingsMenu = gtk_menu_new();
    GtkWidget *settingsItem = gtk_menu_item_new_with_label("Settings");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(settingsItem), settingsMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), settingsItem);

    GtkWidget *preferencesItem = gtk_menu_item_new_with_label("Preferences");
    g_signal_connect(preferencesItem, "activate", G_CALLBACK(show_preferences_dialog), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(settingsMenu), preferencesItem);

    GtkWidget *increaseFontItem = gtk_menu_item_new_with_label("Increase Font Size");
    g_signal_connect(increaseFontItem, "activate", G_CALLBACK(increase_font_size), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(settingsMenu), increaseFontItem);

    GtkWidget *decreaseFontItem = gtk_menu_item_new_with_label("Decrease Font Size");
    g_signal_connect(decreaseFontItem, "activate", G_CALLBACK(decrease_font_size), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(settingsMenu), decreaseFontItem);

    // About Menu
    GtkWidget *aboutMenu = gtk_menu_new();
    GtkWidget *aboutItem = gtk_menu_item_new_with_label("About");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(aboutItem), aboutMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), aboutItem);

    GtkWidget *aboutInfoItem = gtk_menu_item_new_with_label("About Text Editor");
    g_signal_connect(aboutInfoItem, "activate", G_CALLBACK(show_about_dialog), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(aboutMenu), aboutInfoItem);

    // Tabbed Interface
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    // Text Editor View
    buffer = gtk_source_buffer_new(NULL);
    text_view = gtk_source_view_new_with_buffer(buffer);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(text_view), TRUE);
    gtk_source_buffer_set_highlight_matching_brackets(buffer, TRUE);  // Enable bracket matching
    apply_syntax_highlighting(buffer, NULL);  // Apply initial syntax highlighting
    apply_font_size(); // Apply initial font size

    // Add rulers
    GArray *rulers = g_array_new(FALSE, FALSE, sizeof(gint));
    gint ruler_position = 80;  // Example position for a ruler at column 80
    g_array_append_val(rulers, ruler_position);
    g_object_set(text_view, "right-margin-position", 80, "right-margin", TRUE, NULL);
    gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(text_view), 80);

    // Add minimap
    GtkWidget *source_map = gtk_source_map_new();
    gtk_source_map_set_view(GTK_SOURCE_MAP(source_map), GTK_SOURCE_VIEW(text_view));

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), source_map, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hbox, gtk_label_new("Untitled"));

    // Status Bar
    statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    g_signal_connect(buffer, "changed", G_CALLBACK(update_status_bar), statusbar);
    g_signal_connect(buffer, "changed", G_CALLBACK(mark_as_modified), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    create_text_editor_window();

    // Auto-save timer
    g_timeout_add(auto_save_interval, auto_save, NULL);

    gtk_main();
    return 0;
}
