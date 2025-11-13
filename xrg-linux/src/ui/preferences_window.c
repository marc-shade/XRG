#include "preferences_window.h"
#include <string.h>

struct _XRGPreferencesWindow {
    GtkWidget *window;
    GtkWidget *notebook;
    XRGPreferences *prefs;

    /* Window tab widgets */
    GtkWidget *window_x_spin;
    GtkWidget *window_y_spin;
    GtkWidget *window_width_spin;
    GtkWidget *window_height_spin;
    GtkWidget *window_opacity_scale;
    GtkWidget *window_always_on_top_check;

    /* CPU module tab widgets */
    GtkWidget *cpu_enabled_check;
    GtkWidget *cpu_height_spin;
    GtkWidget *cpu_update_interval_spin;
    GtkWidget *cpu_fg1_color_button;
    GtkWidget *cpu_fg2_color_button;
    GtkWidget *cpu_bg_color_button;

    /* Memory module tab widgets */
    GtkWidget *memory_enabled_check;
    GtkWidget *memory_height_spin;
    GtkWidget *memory_fg1_color_button;
    GtkWidget *memory_fg2_color_button;
    GtkWidget *memory_fg3_color_button;
    GtkWidget *memory_bg_color_button;

    /* Network module tab widgets */
    GtkWidget *network_enabled_check;
    GtkWidget *network_height_spin;
    GtkWidget *network_fg1_color_button;
    GtkWidget *network_fg2_color_button;
    GtkWidget *network_bg_color_button;

    /* Disk module tab widgets */
    GtkWidget *disk_enabled_check;
    GtkWidget *disk_height_spin;
    GtkWidget *disk_fg1_color_button;
    GtkWidget *disk_fg2_color_button;
    GtkWidget *disk_bg_color_button;

    /* AI Token module tab widgets */
    GtkWidget *aitoken_enabled_check;
    GtkWidget *aitoken_height_spin;
    GtkWidget *aitoken_fg1_color_button;
    GtkWidget *aitoken_fg2_color_button;
    GtkWidget *aitoken_bg_color_button;

    /* Colors tab widgets */
    GtkWidget *bg_color_button;
    GtkWidget *graph_bg_color_button;
    GtkWidget *graph_fg1_color_button;
    GtkWidget *graph_fg2_color_button;
    GtkWidget *graph_fg3_color_button;
    GtkWidget *text_color_button;
    GtkWidget *border_color_button;
};

/* Forward declarations */
static void on_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static GtkWidget* create_window_tab(XRGPreferencesWindow *win);
static GtkWidget* create_cpu_tab(XRGPreferencesWindow *win);
static GtkWidget* create_memory_tab(XRGPreferencesWindow *win);
static GtkWidget* create_network_tab(XRGPreferencesWindow *win);
static GtkWidget* create_disk_tab(XRGPreferencesWindow *win);
static GtkWidget* create_aitoken_tab(XRGPreferencesWindow *win);
static GtkWidget* create_colors_tab(XRGPreferencesWindow *win);
static void load_preferences_to_ui(XRGPreferencesWindow *win);
static void save_ui_to_preferences(XRGPreferencesWindow *win);

/**
 * Create new preferences window
 */
XRGPreferencesWindow* xrg_preferences_window_new(GtkWindow *parent, XRGPreferences *prefs) {
    XRGPreferencesWindow *win = g_new0(XRGPreferencesWindow, 1);
    win->prefs = prefs;

    /* Create dialog window */
    win->window = gtk_dialog_new_with_buttons("XRG-Linux Preferences",
                                               parent,
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               "Cancel", GTK_RESPONSE_CANCEL,
                                               "Apply", GTK_RESPONSE_APPLY,
                                               "OK", GTK_RESPONSE_OK,
                                               NULL);

    gtk_window_set_default_size(GTK_WINDOW(win->window), 600, 500);

    /* Create notebook for tabs */
    win->notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(win->notebook), 10);

    /* Add tabs */
    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_window_tab(win),
                            gtk_label_new("Window"));

    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_cpu_tab(win),
                            gtk_label_new("CPU Module"));

    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_memory_tab(win),
                            gtk_label_new("Memory Module"));

    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_network_tab(win),
                            gtk_label_new("Network Module"));

    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_disk_tab(win),
                            gtk_label_new("Disk Module"));

    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_aitoken_tab(win),
                            gtk_label_new("AI Token Module"));

    gtk_notebook_append_page(GTK_NOTEBOOK(win->notebook),
                            create_colors_tab(win),
                            gtk_label_new("Colors"));

    /* Add notebook to dialog content area */
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(win->window));
    gtk_container_add(GTK_CONTAINER(content_area), win->notebook);

    /* Connect response signal */
    g_signal_connect(win->window, "response", G_CALLBACK(on_response), win);

    /* Load current preferences into UI */
    load_preferences_to_ui(win);

    return win;
}

/**
 * Create Window settings tab
 */
static GtkWidget* create_window_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Window position section */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Window Position</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* X position */
    label = gtk_label_new("X Position:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->window_x_spin = gtk_spin_button_new_with_range(0, 3840, 1);
    gtk_grid_attach(GTK_GRID(grid), win->window_x_spin, 1, row++, 1, 1);

    /* Y position */
    label = gtk_label_new("Y Position:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->window_y_spin = gtk_spin_button_new_with_range(0, 2160, 1);
    gtk_grid_attach(GTK_GRID(grid), win->window_y_spin, 1, row++, 1, 1);

    /* Width */
    label = gtk_label_new("Width:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->window_width_spin = gtk_spin_button_new_with_range(100, 800, 10);
    gtk_grid_attach(GTK_GRID(grid), win->window_width_spin, 1, row++, 1, 1);

    /* Height */
    label = gtk_label_new("Height:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->window_height_spin = gtk_spin_button_new_with_range(100, 1200, 10);
    gtk_grid_attach(GTK_GRID(grid), win->window_height_spin, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Window behavior section */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Window Behavior</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Always on top */
    win->window_always_on_top_check = gtk_check_button_new_with_label("Always on top");
    gtk_grid_attach(GTK_GRID(grid), win->window_always_on_top_check, 0, row++, 2, 1);

    /* Opacity */
    label = gtk_label_new("Opacity:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->window_opacity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 1.0, 0.05);
    gtk_scale_set_value_pos(GTK_SCALE(win->window_opacity_scale), GTK_POS_RIGHT);
    gtk_widget_set_hexpand(win->window_opacity_scale, TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->window_opacity_scale, 1, row++, 1, 1);

    return grid;
}

/**
 * Create CPU module settings tab
 */
static GtkWidget* create_cpu_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Module enabled */
    win->cpu_enabled_check = gtk_check_button_new_with_label("Show CPU Module");
    gtk_grid_attach(GTK_GRID(grid), win->cpu_enabled_check, 0, row++, 2, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Display settings */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Display Settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Graph height */
    label = gtk_label_new("Graph Height:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->cpu_height_spin = gtk_spin_button_new_with_range(40, 300, 10);
    gtk_grid_attach(GTK_GRID(grid), win->cpu_height_spin, 1, row++, 1, 1);

    /* Update interval */
    label = gtk_label_new("Update Interval (ms):");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->cpu_update_interval_spin = gtk_spin_button_new_with_range(100, 5000, 100);
    gtk_grid_attach(GTK_GRID(grid), win->cpu_update_interval_spin, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Colors */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Module Colors</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Background color */
    label = gtk_label_new("Graph Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->cpu_bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->cpu_bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->cpu_bg_color_button, 1, row++, 1, 1);

    /* User CPU color (FG1) */
    label = gtk_label_new("User CPU Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->cpu_fg1_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->cpu_fg1_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->cpu_fg1_color_button, 1, row++, 1, 1);

    /* System CPU color (FG2) */
    label = gtk_label_new("System CPU Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->cpu_fg2_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->cpu_fg2_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->cpu_fg2_color_button, 1, row++, 1, 1);

    /* Info label */
    label = gtk_label_new("Note: Module-specific colors override global colors when set.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_opacity(label, 0.7);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    return grid;
}

/**
 * Create Memory module settings tab
 */
static GtkWidget* create_memory_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Module enabled */
    win->memory_enabled_check = gtk_check_button_new_with_label("Show Memory Module");
    gtk_grid_attach(GTK_GRID(grid), win->memory_enabled_check, 0, row++, 2, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Display settings */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Display Settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Graph height */
    label = gtk_label_new("Graph Height:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->memory_height_spin = gtk_spin_button_new_with_range(40, 300, 10);
    gtk_grid_attach(GTK_GRID(grid), win->memory_height_spin, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Colors */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Module Colors</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Background color */
    label = gtk_label_new("Graph Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->memory_bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->memory_bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->memory_bg_color_button, 1, row++, 1, 1);

    /* Used memory color (FG1) */
    label = gtk_label_new("Used Memory Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->memory_fg1_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->memory_fg1_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->memory_fg1_color_button, 1, row++, 1, 1);

    /* Wired memory color (FG2) */
    label = gtk_label_new("Wired Memory Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->memory_fg2_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->memory_fg2_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->memory_fg2_color_button, 1, row++, 1, 1);

    /* Cached memory color (FG3) */
    label = gtk_label_new("Cached Memory Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->memory_fg3_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->memory_fg3_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->memory_fg3_color_button, 1, row++, 1, 1);

    /* Info label */
    label = gtk_label_new("Note: Module-specific colors override global colors when set.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_opacity(label, 0.7);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    return grid;
}

/**
 * Create Network module settings tab
 */
static GtkWidget* create_network_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Module enabled */
    win->network_enabled_check = gtk_check_button_new_with_label("Show Network Module");
    gtk_grid_attach(GTK_GRID(grid), win->network_enabled_check, 0, row++, 2, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Display settings */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Display Settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Graph height */
    label = gtk_label_new("Graph Height:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->network_height_spin = gtk_spin_button_new_with_range(40, 300, 10);
    gtk_grid_attach(GTK_GRID(grid), win->network_height_spin, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Colors */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Module Colors</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Background color */
    label = gtk_label_new("Graph Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->network_bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->network_bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->network_bg_color_button, 1, row++, 1, 1);

    /* Download color (FG1) */
    label = gtk_label_new("Download Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->network_fg1_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->network_fg1_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->network_fg1_color_button, 1, row++, 1, 1);

    /* Upload color (FG2) */
    label = gtk_label_new("Upload Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->network_fg2_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->network_fg2_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->network_fg2_color_button, 1, row++, 1, 1);

    /* Info label */
    label = gtk_label_new("Note: Module-specific colors override global colors when set.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_opacity(label, 0.7);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    return grid;
}

/**
 * Create Disk module settings tab
 */
static GtkWidget* create_disk_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Module enabled */
    win->disk_enabled_check = gtk_check_button_new_with_label("Show Disk Module");
    gtk_grid_attach(GTK_GRID(grid), win->disk_enabled_check, 0, row++, 2, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Display settings */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Display Settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Graph height */
    label = gtk_label_new("Graph Height:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->disk_height_spin = gtk_spin_button_new_with_range(40, 300, 10);
    gtk_grid_attach(GTK_GRID(grid), win->disk_height_spin, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Colors */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Module Colors</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Background color */
    label = gtk_label_new("Graph Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->disk_bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->disk_bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->disk_bg_color_button, 1, row++, 1, 1);

    /* Read color (FG1) */
    label = gtk_label_new("Read Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->disk_fg1_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->disk_fg1_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->disk_fg1_color_button, 1, row++, 1, 1);

    /* Write color (FG2) */
    label = gtk_label_new("Write Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->disk_fg2_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->disk_fg2_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->disk_fg2_color_button, 1, row++, 1, 1);

    /* Info label */
    label = gtk_label_new("Note: Module-specific colors override global colors when set.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_opacity(label, 0.7);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    return grid;
}

/**
 * Create AI Token module tab
 */
static GtkWidget* create_aitoken_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Module enabled */
    win->aitoken_enabled_check = gtk_check_button_new_with_label("Show AI Token Module");
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_enabled_check, 0, row++, 2, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Display settings */
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Display Settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Graph height */
    label = gtk_label_new("Graph Height:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->aitoken_height_spin = gtk_spin_button_new_with_range(40, 300, 10);
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_height_spin, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Colors */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Module Colors</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Background color */
    label = gtk_label_new("Graph Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->aitoken_bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->aitoken_bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_bg_color_button, 1, row++, 1, 1);

    /* Input tokens color (FG1) */
    label = gtk_label_new("Input Tokens Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->aitoken_fg1_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->aitoken_fg1_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_fg1_color_button, 1, row++, 1, 1);

    /* Output tokens color (FG2) */
    label = gtk_label_new("Output Tokens Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->aitoken_fg2_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->aitoken_fg2_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_fg2_color_button, 1, row++, 1, 1);

    /* Info label */
    label = gtk_label_new("Note: Module-specific colors override global colors when set.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_opacity(label, 0.7);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    return grid;
}

/**
 * Create global colors tab
 */
static GtkWidget* create_colors_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Global Colors</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Background color */
    label = gtk_label_new("Window Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->bg_color_button, 1, row++, 1, 1);

    /* Graph background */
    label = gtk_label_new("Graph Background:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->graph_bg_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->graph_bg_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->graph_bg_color_button, 1, row++, 1, 1);

    /* FG1 color (cyan) */
    label = gtk_label_new("Data Series 1 (Cyan):");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->graph_fg1_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->graph_fg1_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->graph_fg1_color_button, 1, row++, 1, 1);

    /* FG2 color (purple) */
    label = gtk_label_new("Data Series 2 (Purple):");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->graph_fg2_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->graph_fg2_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->graph_fg2_color_button, 1, row++, 1, 1);

    /* FG3 color (amber) */
    label = gtk_label_new("Data Series 3 (Amber):");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->graph_fg3_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->graph_fg3_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->graph_fg3_color_button, 1, row++, 1, 1);

    /* Text color */
    label = gtk_label_new("Text Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->text_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->text_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->text_color_button, 1, row++, 1, 1);

    /* Border color */
    label = gtk_label_new("Border Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->border_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->border_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->border_color_button, 1, row++, 1, 1);

    return grid;
}

/**
 * Load preferences into UI widgets
 */
static void load_preferences_to_ui(XRGPreferencesWindow *win) {
    XRGPreferences *prefs = win->prefs;

    /* Window tab */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->window_x_spin), prefs->window_x);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->window_y_spin), prefs->window_y);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->window_width_spin), prefs->window_width);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->window_height_spin), prefs->window_height);
    gtk_range_set_value(GTK_RANGE(win->window_opacity_scale), prefs->window_opacity);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->window_always_on_top_check), prefs->window_always_on_top);

    /* CPU module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->cpu_enabled_check), prefs->show_cpu);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->cpu_height_spin), prefs->graph_height_cpu);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->cpu_update_interval_spin), prefs->normal_update_interval);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->cpu_bg_color_button), &prefs->graph_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->cpu_fg1_color_button), &prefs->graph_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->cpu_fg2_color_button), &prefs->graph_fg2_color);

    /* Memory module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->memory_enabled_check), prefs->show_memory);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->memory_height_spin), prefs->graph_height_memory);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->memory_bg_color_button), &prefs->memory_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->memory_fg1_color_button), &prefs->memory_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->memory_fg2_color_button), &prefs->memory_fg2_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->memory_fg3_color_button), &prefs->memory_fg3_color);

    /* Network module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->network_enabled_check), prefs->show_network);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->network_height_spin), prefs->graph_height_network);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->network_bg_color_button), &prefs->network_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->network_fg1_color_button), &prefs->network_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->network_fg2_color_button), &prefs->network_fg2_color);

    /* Disk module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->disk_enabled_check), prefs->show_disk);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->disk_height_spin), prefs->graph_height_disk);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->disk_bg_color_button), &prefs->disk_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->disk_fg1_color_button), &prefs->disk_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->disk_fg2_color_button), &prefs->disk_fg2_color);

    /* AI Token module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->aitoken_enabled_check), prefs->show_aitoken);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->aitoken_height_spin), prefs->graph_height_aitoken);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->aitoken_bg_color_button), &prefs->aitoken_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->aitoken_fg1_color_button), &prefs->aitoken_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->aitoken_fg2_color_button), &prefs->aitoken_fg2_color);

    /* Colors tab */
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->bg_color_button), &prefs->background_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_bg_color_button), &prefs->graph_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg1_color_button), &prefs->graph_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg2_color_button), &prefs->graph_fg2_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg3_color_button), &prefs->graph_fg3_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->text_color_button), &prefs->text_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->border_color_button), &prefs->border_color);
}

/**
 * Save UI widget values to preferences
 */
static void save_ui_to_preferences(XRGPreferencesWindow *win) {
    XRGPreferences *prefs = win->prefs;

    /* Window tab */
    prefs->window_x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->window_x_spin));
    prefs->window_y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->window_y_spin));
    prefs->window_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->window_width_spin));
    prefs->window_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->window_height_spin));
    prefs->window_opacity = gtk_range_get_value(GTK_RANGE(win->window_opacity_scale));
    prefs->window_always_on_top = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->window_always_on_top_check));

    /* CPU module tab */
    prefs->show_cpu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->cpu_enabled_check));
    prefs->graph_height_cpu = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->cpu_height_spin));
    prefs->normal_update_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->cpu_update_interval_spin));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->cpu_bg_color_button), &prefs->graph_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->cpu_fg1_color_button), &prefs->graph_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->cpu_fg2_color_button), &prefs->graph_fg2_color);

    /* Memory module tab */
    prefs->show_memory = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->memory_enabled_check));
    prefs->graph_height_memory = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->memory_height_spin));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->memory_bg_color_button), &prefs->memory_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->memory_fg1_color_button), &prefs->memory_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->memory_fg2_color_button), &prefs->memory_fg2_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->memory_fg3_color_button), &prefs->memory_fg3_color);

    /* Network module tab */
    prefs->show_network = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->network_enabled_check));
    prefs->graph_height_network = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->network_height_spin));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->network_bg_color_button), &prefs->network_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->network_fg1_color_button), &prefs->network_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->network_fg2_color_button), &prefs->network_fg2_color);

    /* Disk module tab */
    prefs->show_disk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->disk_enabled_check));
    prefs->graph_height_disk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->disk_height_spin));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->disk_bg_color_button), &prefs->disk_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->disk_fg1_color_button), &prefs->disk_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->disk_fg2_color_button), &prefs->disk_fg2_color);

    /* AI Token module tab */
    prefs->show_aitoken = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->aitoken_enabled_check));
    prefs->graph_height_aitoken = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->aitoken_height_spin));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->aitoken_bg_color_button), &prefs->aitoken_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->aitoken_fg1_color_button), &prefs->aitoken_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->aitoken_fg2_color_button), &prefs->aitoken_fg2_color);

    /* Colors tab */
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->bg_color_button), &prefs->background_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_bg_color_button), &prefs->graph_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_fg1_color_button), &prefs->graph_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_fg2_color_button), &prefs->graph_fg2_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_fg3_color_button), &prefs->graph_fg3_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->text_color_button), &prefs->text_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->border_color_button), &prefs->border_color);

    /* Save to file */
    xrg_preferences_save(prefs);
}

/**
 * Dialog response callback
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    XRGPreferencesWindow *win = (XRGPreferencesWindow *)user_data;

    if (response_id == GTK_RESPONSE_OK || response_id == GTK_RESPONSE_APPLY) {
        save_ui_to_preferences(win);
        g_message("Preferences saved");
    }

    if (response_id == GTK_RESPONSE_OK || response_id == GTK_RESPONSE_CANCEL) {
        gtk_widget_hide(win->window);
    }
}

/**
 * Show preferences window
 */
void xrg_preferences_window_show(XRGPreferencesWindow *win) {
    g_return_if_fail(win != NULL);
    load_preferences_to_ui(win);
    gtk_widget_show_all(win->window);
}

/**
 * Hide preferences window
 */
void xrg_preferences_window_hide(XRGPreferencesWindow *win) {
    g_return_if_fail(win != NULL);
    gtk_widget_hide(win->window);
}

/**
 * Free preferences window
 */
void xrg_preferences_window_free(XRGPreferencesWindow *win) {
    if (win == NULL)
        return;

    if (win->window)
        gtk_widget_destroy(win->window);

    g_free(win);
}
