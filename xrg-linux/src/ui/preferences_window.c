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
    GtkWidget *show_activity_bars_check;
    GtkWidget *activity_bar_style_combo;
    GtkWidget *layout_orientation_combo;

    /* CPU module tab widgets */
    GtkWidget *cpu_enabled_check;
    GtkWidget *cpu_height_spin;
    GtkWidget *cpu_update_interval_spin;
    GtkWidget *cpu_style_combo;

    /* Memory module tab widgets */
    GtkWidget *memory_enabled_check;
    GtkWidget *memory_height_spin;
    GtkWidget *memory_style_combo;

    /* Network module tab widgets */
    GtkWidget *network_enabled_check;
    GtkWidget *network_height_spin;
    GtkWidget *network_style_combo;

    /* Disk module tab widgets */
    GtkWidget *disk_enabled_check;
    GtkWidget *disk_height_spin;
    GtkWidget *disk_style_combo;

    /* GPU module tab widgets */
    GtkWidget *gpu_enabled_check;
    GtkWidget *gpu_height_spin;
    GtkWidget *gpu_style_combo;

    /* AI Token module tab widgets */
    GtkWidget *aitoken_enabled_check;
    GtkWidget *aitoken_height_spin;
    GtkWidget *aitoken_style_combo;
    GtkWidget *aitoken_show_model_breakdown_check;

    /* Colors tab widgets */
    GtkWidget *theme_combo;
    GtkWidget *bg_color_button;
    GtkWidget *graph_bg_color_button;
    GtkWidget *graph_fg1_color_button;
    GtkWidget *graph_fg2_color_button;
    GtkWidget *graph_fg3_color_button;
    GtkWidget *text_color_button;
    GtkWidget *border_color_button;
    GtkWidget *activity_bar_color_button;

    /* Callback for when preferences are applied */
    XRGPreferencesAppliedCallback applied_callback;
    gpointer callback_user_data;
};

/* Forward declarations */
static void on_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_theme_changed(GtkComboBox *combo, gpointer user_data);
static GtkWidget* create_window_tab(XRGPreferencesWindow *win);
static GtkWidget* create_cpu_tab(XRGPreferencesWindow *win);
static GtkWidget* create_memory_tab(XRGPreferencesWindow *win);
static GtkWidget* create_network_tab(XRGPreferencesWindow *win);
static GtkWidget* create_disk_tab(XRGPreferencesWindow *win);
static GtkWidget* create_gpu_tab(XRGPreferencesWindow *win);
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
                            create_gpu_tab(win),
                            gtk_label_new("GPU Module"));

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

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Display features section */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Display Features</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Activity bars */
    win->show_activity_bars_check = gtk_check_button_new_with_label("Show Activity Bars");
    gtk_grid_attach(GTK_GRID(grid), win->show_activity_bars_check, 0, row++, 2, 1);

    /* Activity bar style */
    label = gtk_label_new("Activity Bar Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    win->activity_bar_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->activity_bar_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->activity_bar_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->activity_bar_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->activity_bar_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->activity_bar_style_combo, 1, row++, 1, 1);

    /* Layout orientation */
    label = gtk_label_new("Layout Orientation:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    win->layout_orientation_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->layout_orientation_combo), "Vertical (Stacked)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->layout_orientation_combo), "Horizontal (Side-by-Side)");
    gtk_grid_attach(GTK_GRID(grid), win->layout_orientation_combo, 1, row++, 1, 1);

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

    /* Visual Style */
    label = gtk_label_new("Visual Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->cpu_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->cpu_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->cpu_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->cpu_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->cpu_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->cpu_style_combo, 1, row++, 1, 1);

    /* Note: Colors are managed in the Colors tab */

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

    /* Visual Style */
    label = gtk_label_new("Visual Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->memory_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->memory_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->memory_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->memory_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->memory_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->memory_style_combo, 1, row++, 1, 1);

    /* Note: Colors are managed in the Colors tab */

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

    /* Visual Style */
    label = gtk_label_new("Visual Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->network_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->network_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->network_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->network_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->network_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->network_style_combo, 1, row++, 1, 1);

    /* Note: Colors are managed in the Colors tab */

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

    /* Visual Style */
    label = gtk_label_new("Visual Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->disk_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->disk_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->disk_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->disk_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->disk_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->disk_style_combo, 1, row++, 1, 1);

    /* Note: Colors are managed in the Colors tab */

    return grid;
}

/**
 * Create GPU module settings tab
 */
static GtkWidget* create_gpu_tab(XRGPreferencesWindow *win) {
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);

    gint row = 0;

    /* Module enabled */
    win->gpu_enabled_check = gtk_check_button_new_with_label("Show GPU Module");
    gtk_grid_attach(GTK_GRID(grid), win->gpu_enabled_check, 0, row++, 2, 1);

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
    win->gpu_height_spin = gtk_spin_button_new_with_range(40, 300, 10);
    gtk_grid_attach(GTK_GRID(grid), win->gpu_height_spin, 1, row++, 1, 1);

    /* Visual Style */
    label = gtk_label_new("Visual Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->gpu_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->gpu_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->gpu_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->gpu_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->gpu_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->gpu_style_combo, 1, row++, 1, 1);

    /* Note: Colors are managed in the Colors tab */

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

    /* Visual Style */
    label = gtk_label_new("Visual Style:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->aitoken_style_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->aitoken_style_combo), "Solid (Filled)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->aitoken_style_combo), "Pixel (Chunky)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->aitoken_style_combo), "Dot (Fine)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->aitoken_style_combo), "Hollow (Outline)");
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_style_combo, 1, row++, 1, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Data display options */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Data Display Options</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Show model breakdown */
    win->aitoken_show_model_breakdown_check = gtk_check_button_new_with_label("Show Per-Model Token Breakdown");
    gtk_grid_attach(GTK_GRID(grid), win->aitoken_show_model_breakdown_check, 0, row++, 2, 1);

    /* Note: Colors are managed in the Colors tab */

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
    gtk_label_set_markup(GTK_LABEL(label), "<b>Color Theme</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Theme selector */
    label = gtk_label_new("Theme Preset:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    win->theme_combo = gtk_combo_box_text_new();
    for (gint i = 0; i < xrg_preferences_get_theme_count(); i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(win->theme_combo),
                                        xrg_preferences_get_theme_name(i));
    }
    g_signal_connect(win->theme_combo, "changed", G_CALLBACK(on_theme_changed), win);
    gtk_grid_attach(GTK_GRID(grid), win->theme_combo, 1, row++, 1, 1);

    /* Info label */
    label = gtk_label_new("Select a theme preset or customize individual colors below.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_widget_set_opacity(label, 0.7);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    /* Separator */
    gtk_grid_attach(GTK_GRID(grid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, row++, 2, 1);

    /* Global Colors header */
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Custom Colors</b>");
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

    /* Activity bar color */
    label = gtk_label_new("Activity Bar Color:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    win->activity_bar_color_button = gtk_color_button_new();
    gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(win->activity_bar_color_button), TRUE);
    gtk_grid_attach(GTK_GRID(grid), win->activity_bar_color_button, 1, row++, 1, 1);

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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->show_activity_bars_check), prefs->show_activity_bars);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->activity_bar_style_combo), prefs->activity_bar_style);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->layout_orientation_combo), prefs->layout_orientation);

    /* CPU module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->cpu_enabled_check), prefs->show_cpu);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->cpu_height_spin), prefs->graph_height_cpu);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->cpu_update_interval_spin), prefs->normal_update_interval);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->cpu_style_combo), prefs->cpu_graph_style);
    /* Note: CPU colors removed - use Colors tab instead */

    /* Memory module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->memory_enabled_check), prefs->show_memory);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->memory_height_spin), prefs->graph_height_memory);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->memory_style_combo), prefs->memory_graph_style);
    /* Note: Memory colors removed - use Colors tab instead */

    /* Network module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->network_enabled_check), prefs->show_network);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->network_height_spin), prefs->graph_height_network);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->network_style_combo), prefs->network_graph_style);
    /* Note: Network colors removed - use Colors tab instead */

    /* Disk module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->disk_enabled_check), prefs->show_disk);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->disk_height_spin), prefs->graph_height_disk);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->disk_style_combo), prefs->disk_graph_style);
    /* Note: Disk colors removed - use Colors tab instead */

    /* GPU module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->gpu_enabled_check), prefs->show_gpu);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->gpu_height_spin), prefs->graph_height_gpu);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->gpu_style_combo), prefs->gpu_graph_style);
    /* Note: GPU colors removed - use Colors tab instead */

    /* AI Token module tab */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->aitoken_enabled_check), prefs->show_aitoken);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(win->aitoken_height_spin), prefs->graph_height_aitoken);
    gtk_combo_box_set_active(GTK_COMBO_BOX(win->aitoken_style_combo), prefs->aitoken_graph_style);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->aitoken_show_model_breakdown_check), prefs->aitoken_show_model_breakdown);
    /* Note: AIToken colors removed - use Colors tab instead */

    /* Colors tab */
    /* Load color buttons FIRST (before setting theme combo, to avoid triggering callback) */
    g_message("Loading colors to UI - BG: (%.3f, %.3f, %.3f, %.3f)",
              prefs->background_color.red, prefs->background_color.green,
              prefs->background_color.blue, prefs->background_color.alpha);
    g_message("Loading colors to UI - Graph FG1: (%.3f, %.3f, %.3f, %.3f)",
              prefs->graph_fg1_color.red, prefs->graph_fg1_color.green,
              prefs->graph_fg1_color.blue, prefs->graph_fg1_color.alpha);

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->bg_color_button), &prefs->background_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_bg_color_button), &prefs->graph_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg1_color_button), &prefs->graph_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg2_color_button), &prefs->graph_fg2_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg3_color_button), &prefs->graph_fg3_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->text_color_button), &prefs->text_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->border_color_button), &prefs->border_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->activity_bar_color_button), &prefs->activity_bar_color);

    /* Set current theme in dropdown (block signal to avoid overwriting colors) */
    const gchar *current_theme = xrg_preferences_get_current_theme(prefs);
    if (current_theme != NULL) {
        /* Block the "changed" signal while we set the active item */
        g_signal_handlers_block_by_func(win->theme_combo, G_CALLBACK(on_theme_changed), win);

        for (gint i = 0; i < xrg_preferences_get_theme_count(); i++) {
            if (g_strcmp0(xrg_preferences_get_theme_name(i), current_theme) == 0) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(win->theme_combo), i);
                break;
            }
        }

        /* Unblock the signal */
        g_signal_handlers_unblock_by_func(win->theme_combo, G_CALLBACK(on_theme_changed), win);
    }
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
    prefs->show_activity_bars = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->show_activity_bars_check));
    prefs->activity_bar_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->activity_bar_style_combo));
    prefs->layout_orientation = gtk_combo_box_get_active(GTK_COMBO_BOX(win->layout_orientation_combo));

    /* CPU module tab */
    prefs->show_cpu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->cpu_enabled_check));
    prefs->graph_height_cpu = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->cpu_height_spin));
    prefs->normal_update_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->cpu_update_interval_spin));
    prefs->cpu_graph_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->cpu_style_combo));
    /* Note: CPU colors removed - use Colors tab instead */

    /* Memory module tab */
    prefs->show_memory = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->memory_enabled_check));
    prefs->graph_height_memory = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->memory_height_spin));
    prefs->memory_graph_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->memory_style_combo));
    /* Note: Memory colors removed - use Colors tab instead */

    /* Network module tab */
    prefs->show_network = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->network_enabled_check));
    prefs->graph_height_network = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->network_height_spin));
    prefs->network_graph_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->network_style_combo));
    /* Note: Network colors removed - use Colors tab instead */

    /* Disk module tab */
    prefs->show_disk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->disk_enabled_check));
    prefs->graph_height_disk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->disk_height_spin));
    prefs->disk_graph_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->disk_style_combo));
    /* Note: Disk colors removed - use Colors tab instead */

    /* GPU module tab */
    prefs->show_gpu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->gpu_enabled_check));
    prefs->graph_height_gpu = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->gpu_height_spin));
    prefs->gpu_graph_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->gpu_style_combo));
    /* Note: GPU colors removed - use Colors tab instead */

    /* AI Token module tab */
    prefs->show_aitoken = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->aitoken_enabled_check));
    prefs->graph_height_aitoken = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->aitoken_height_spin));
    prefs->aitoken_graph_style = gtk_combo_box_get_active(GTK_COMBO_BOX(win->aitoken_style_combo));
    prefs->aitoken_show_model_breakdown = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->aitoken_show_model_breakdown_check));
    /* Note: AIToken colors removed - use Colors tab instead */

    /* Colors tab */
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->bg_color_button), &prefs->background_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_bg_color_button), &prefs->graph_bg_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_fg1_color_button), &prefs->graph_fg1_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_fg2_color_button), &prefs->graph_fg2_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->graph_fg3_color_button), &prefs->graph_fg3_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->text_color_button), &prefs->text_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->border_color_button), &prefs->border_color);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(win->activity_bar_color_button), &prefs->activity_bar_color);

    g_message("Saving colors from UI - BG: (%.3f, %.3f, %.3f, %.3f)",
              prefs->background_color.red, prefs->background_color.green,
              prefs->background_color.blue, prefs->background_color.alpha);
    g_message("Saving colors from UI - Graph FG1: (%.3f, %.3f, %.3f, %.3f)",
              prefs->graph_fg1_color.red, prefs->graph_fg1_color.green,
              prefs->graph_fg1_color.blue, prefs->graph_fg1_color.alpha);

    /* Save to file */
    xrg_preferences_save(prefs);
}

/**
 * Theme selection callback - applies theme and updates color buttons
 */
static void on_theme_changed(GtkComboBox *combo, gpointer user_data) {
    XRGPreferencesWindow *win = (XRGPreferencesWindow *)user_data;
    gchar *theme_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));

    if (theme_name == NULL) {
        return;  /* No selection */
    }

    /* Apply the theme to preferences */
    xrg_preferences_apply_theme(win->prefs, theme_name);

    /* Update all color buttons to reflect the new theme */
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->bg_color_button), &win->prefs->background_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_bg_color_button), &win->prefs->graph_bg_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg1_color_button), &win->prefs->graph_fg1_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg2_color_button), &win->prefs->graph_fg2_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->graph_fg3_color_button), &win->prefs->graph_fg3_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->text_color_button), &win->prefs->text_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->border_color_button), &win->prefs->border_color);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(win->activity_bar_color_button), &win->prefs->activity_bar_color);

    g_message("Applied theme: %s", theme_name);
    g_free(theme_name);
}

/**
 * Dialog response callback
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    XRGPreferencesWindow *win = (XRGPreferencesWindow *)user_data;

    if (response_id == GTK_RESPONSE_OK || response_id == GTK_RESPONSE_APPLY) {
        save_ui_to_preferences(win);
        g_message("Preferences saved");

        /* Call the applied callback if set */
        if (win->applied_callback) {
            win->applied_callback(win->callback_user_data);
        }
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
 * Set callback to be called when preferences are applied
 */
void xrg_preferences_window_set_applied_callback(XRGPreferencesWindow *win,
                                                   XRGPreferencesAppliedCallback callback,
                                                   gpointer user_data) {
    g_return_if_fail(win != NULL);
    win->applied_callback = callback;
    win->callback_user_data = user_data;
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
