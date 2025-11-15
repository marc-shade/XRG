#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <gdk/gdkkeysyms.h>
#include "core/preferences.h"
#include "core/dataset.h"
#include "core/utils.h"
#include "collectors/cpu_collector.h"
#include "collectors/memory_collector.h"
#include "collectors/network_collector.h"
#include "collectors/disk_collector.h"
#include "collectors/gpu_collector.h"
#include "collectors/aitoken_collector.h"
#include "ui/preferences_window.h"

#define SNAP_DISTANCE 20  /* Pixels from edge to snap */
#define TITLE_BAR_HEIGHT 20

/* Application state */
typedef struct {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *title_bar;
    GtkWidget *resize_grip;
    GtkWidget *cpu_box;
    GtkWidget *cpu_drawing_area;
    GtkWidget *memory_box;
    GtkWidget *memory_drawing_area;
    GtkWidget *network_box;
    GtkWidget *network_drawing_area;
    GtkWidget *disk_box;
    GtkWidget *disk_drawing_area;
    GtkWidget *gpu_box;
    GtkWidget *gpu_drawing_area;
    GtkWidget *aitoken_box;
    GtkWidget *aitoken_drawing_area;
    XRGPreferences *prefs;
    XRGCPUCollector *cpu_collector;
    XRGMemoryCollector *memory_collector;
    XRGNetworkCollector *network_collector;
    XRGDiskCollector *disk_collector;
    XRGGPUCollector *gpu_collector;
    XRGAITokenCollector *aitoken_collector;
    XRGPreferencesWindow *prefs_window;
    guint update_timer_id;

    /* Dragging state */
    gboolean is_dragging;
    gint drag_start_x;
    gint drag_start_y;
    gint window_start_x;
    gint window_start_y;

    /* Resizing state */
    gboolean is_resizing;
    gint resize_start_x;
    gint resize_start_y;
    gint resize_start_width;
    gint resize_start_height;

    /* Layout state */
    XRGLayoutOrientation current_layout_orientation;
    GtkWidget *overlay;  /* Parent of vbox for easy recreation */
} AppState;

/* Forward declarations */
static void on_activate(GtkApplication *app, gpointer user_data);
static GtkWidget* create_title_bar(AppState *state);
static GtkWidget* create_resize_grip(AppState *state);
static void on_title_bar_realize(GtkWidget *widget, gpointer user_data);
static void on_resize_grip_realize(GtkWidget *widget, gpointer user_data);
static gboolean on_window_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data);
static gboolean on_title_bar_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_title_bar_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_title_bar_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean on_draw_title_bar(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_cpu(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_cpu_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_cpu_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void show_cpu_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_memory(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_memory_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_memory_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void show_memory_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_network(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_network_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_network_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void show_network_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_disk(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_disk_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_disk_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void show_disk_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_gpu(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_gpu_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_gpu_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void show_gpu_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_aitoken(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_aitoken_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_aitoken_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static void show_aitoken_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean on_resize_grip_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_resize_grip_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_resize_grip_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean on_draw_resize_grip(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_update_timer(gpointer user_data);
static void on_window_destroy(GtkWidget *widget, gpointer user_data);
static void on_preferences_applied(gpointer user_data);
static void snap_to_edge(GtkWindow *window, gint *x, gint *y);

/**
 * Main entry point
 */
int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.gaucho.xrg-linux", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

/**
 * Create draggable title bar
 */
static GtkWidget* create_title_bar(AppState *state) {
    GtkWidget *event_box = gtk_event_box_new();
    GtkWidget *drawing_area = gtk_drawing_area_new();

    gtk_widget_set_size_request(drawing_area, -1, TITLE_BAR_HEIGHT);
    gtk_container_add(GTK_CONTAINER(event_box), drawing_area);

    /* Make event box visible for events */
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
    gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), TRUE);

    /* Enable events on BOTH event box and drawing area */
    gtk_widget_add_events(event_box,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

    gtk_widget_add_events(drawing_area,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);

    /* Connect signals */
    g_signal_connect(event_box, "realize", G_CALLBACK(on_title_bar_realize), NULL);
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_title_bar_button_press), state);
    g_signal_connect(event_box, "button-release-event", G_CALLBACK(on_title_bar_button_release), state);
    g_signal_connect(event_box, "motion-notify-event", G_CALLBACK(on_title_bar_motion_notify), state);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_title_bar), state);

    return event_box;
}

/**
 * Create resize grip widget (bottom-right corner)
 */
static GtkWidget* create_resize_grip(AppState *state) {
    GtkWidget *event_box = gtk_event_box_new();
    GtkWidget *drawing_area = gtk_drawing_area_new();

    /* Fixed size 20x20 pixels for grip */
    gtk_widget_set_size_request(drawing_area, 20, 20);
    gtk_container_add(GTK_CONTAINER(event_box), drawing_area);

    /* Position in bottom-right corner */
    gtk_widget_set_halign(event_box, GTK_ALIGN_END);
    gtk_widget_set_valign(event_box, GTK_ALIGN_END);

    /* Make event box transparent */
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box), FALSE);
    gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), TRUE);

    /* Enable events */
    gtk_widget_add_events(event_box,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

    gtk_widget_add_events(drawing_area,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);

    /* Connect signals */
    g_signal_connect(event_box, "realize", G_CALLBACK(on_resize_grip_realize), NULL);
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_resize_grip_button_press), state);
    g_signal_connect(event_box, "button-release-event", G_CALLBACK(on_resize_grip_button_release), state);
    g_signal_connect(event_box, "motion-notify-event", G_CALLBACK(on_resize_grip_motion_notify), state);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw_resize_grip), state);

    return event_box;
}

/**
 * Title bar realize (set cursor)
 */
static void on_title_bar_realize(GtkWidget *widget, gpointer user_data) {
    GdkCursor *cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_HAND2);
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    g_object_unref(cursor);
}

/**
 * Draw title bar
 */
static gboolean on_draw_title_bar(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    gint width = allocation.width;
    gint height = allocation.height;
    
    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->background_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    /* Draw title text */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);
    
    cairo_move_to(cr, 5, 14);
    cairo_show_text(cr, "XRG-Linux");
    
    /* Draw drag indicator (three dots) */
    gdouble dot_y = height / 2.0;
    for (gint i = 0; i < 3; i++) {
        gdouble dot_x = width - 30 + (i * 8);
        cairo_arc(cr, dot_x, dot_y, 1.5, 0, 2 * G_PI);
        cairo_fill(cr);
    }
    
    return FALSE;
}

/**
 * Window configure event - fired when window is moved or resized
 */
static gboolean on_window_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    /* Get actual window position (event->x/y don't work on Wayland) */
    gint x, y, width, height;
    gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
    gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

    /* Save new position and size */
    state->prefs->window_x = x;
    state->prefs->window_y = y;
    state->prefs->window_width = width;
    state->prefs->window_height = height - TITLE_BAR_HEIGHT;

    g_message("Window configured: x=%d, y=%d, width=%d, height=%d",
              x, y, width, height - TITLE_BAR_HEIGHT);

    /* Save immediately so position is always current */
    xrg_preferences_save(state->prefs);

    return FALSE;  /* Let other handlers process this event too */
}

/**
 * Title bar button press (start dragging)
 */
static gboolean on_title_bar_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (event->button == 1) {  /* Left click */
        state->is_dragging = TRUE;
        state->drag_start_x = event->x_root;
        state->drag_start_y = event->y_root;

        /* Get current window position */
        gtk_window_get_position(GTK_WINDOW(state->window),
                               &state->window_start_x,
                               &state->window_start_y);

        g_message("Drag started at (%.0f, %.0f), window at (%d, %d)",
                  event->x_root, event->y_root,
                  state->window_start_x, state->window_start_y);

        return TRUE;
    }
    
    return FALSE;
}

/**
 * Title bar button release (stop dragging with snap)
 */
static gboolean on_title_bar_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 1 && state->is_dragging) {
        state->is_dragging = FALSE;

        /* Get current position and snap to edge */
        gint x, y;
        gtk_window_get_position(GTK_WINDOW(state->window), &x, &y);
        snap_to_edge(GTK_WINDOW(state->window), &x, &y);
        gtk_window_move(GTK_WINDOW(state->window), x, y);

        /* Save new position */
        state->prefs->window_x = x;
        state->prefs->window_y = y;
        g_message("Saving window position: x=%d, y=%d", x, y);
        xrg_preferences_save(state->prefs);

        return TRUE;
    }

    return FALSE;
}

/**
 * Title bar motion (update window position while dragging)
 */
static gboolean on_title_bar_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (state->is_dragging) {
        gint delta_x = event->x_root - state->drag_start_x;
        gint delta_y = event->y_root - state->drag_start_y;
        
        gint new_x = state->window_start_x + delta_x;
        gint new_y = state->window_start_y + delta_y;
        
        gtk_window_move(GTK_WINDOW(state->window), new_x, new_y);
        
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Resize grip realize (set cursor)
 */
static void on_resize_grip_realize(GtkWidget *widget, gpointer user_data) {
    GdkCursor *cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BOTTOM_RIGHT_CORNER);
    gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
    g_object_unref(cursor);
}

/**
 * Draw resize grip
 */
static gboolean on_draw_resize_grip(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw three diagonal lines for grip indicator */
    GdkRGBA *color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, color->red, color->green, color->blue, 0.7);
    cairo_set_line_width(cr, 1.5);

    for (gint i = 0; i < 3; i++) {
        gint offset = i * 5 + 5;
        cairo_move_to(cr, offset, height);
        cairo_line_to(cr, width, offset);
        cairo_stroke(cr);
    }

    return FALSE;
}

/**
 * Resize grip button press - start resize
 */
static gboolean on_resize_grip_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 1) {  /* Left click */
        state->is_resizing = TRUE;
        state->resize_start_x = event->x_root;
        state->resize_start_y = event->y_root;

        gint width, height;
        gtk_window_get_size(GTK_WINDOW(state->window), &width, &height);
        state->resize_start_width = width;
        state->resize_start_height = height;

        return TRUE;
    }

    return FALSE;
}

/**
 * Resize grip button release - stop resize
 */
static gboolean on_resize_grip_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 1 && state->is_resizing) {
        state->is_resizing = FALSE;

        /* Save new size */
        gint width, height;
        gtk_window_get_size(GTK_WINDOW(state->window), &width, &height);
        state->prefs->window_width = width;
        state->prefs->window_height = height - TITLE_BAR_HEIGHT;
        xrg_preferences_save(state->prefs);

        return TRUE;
    }

    return FALSE;
}

/**
 * Resize grip motion - perform resize
 */
static gboolean on_resize_grip_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (state->is_resizing) {
        gint delta_x = event->x_root - state->resize_start_x;
        gint delta_y = event->y_root - state->resize_start_y;

        gint new_width = state->resize_start_width + delta_x;
        gint new_height = state->resize_start_height + delta_y;

        /* Enforce minimum size */
        if (new_width < 150) new_width = 150;
        if (new_height < 100) new_height = 100;

        gtk_window_resize(GTK_WINDOW(state->window), new_width, new_height);

        return TRUE;
    }

    return FALSE;
}

/**
 * Snap window to screen edges
 */
static void snap_to_edge(GtkWindow *window, gint *x, gint *y) {
    GdkScreen *screen = gtk_window_get_screen(window);
    GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
    GdkMonitor *monitor = gdk_display_get_monitor_at_window(
        gdk_screen_get_display(screen), gdk_window);
    
    GdkRectangle workarea;
    gdk_monitor_get_workarea(monitor, &workarea);
    
    gint window_width, window_height;
    gtk_window_get_size(window, &window_width, &window_height);
    
    /* Snap to left edge */
    if (*x < workarea.x + SNAP_DISTANCE) {
        *x = workarea.x;
    }
    /* Snap to right edge */
    else if (*x + window_width > workarea.x + workarea.width - SNAP_DISTANCE) {
        *x = workarea.x + workarea.width - window_width;
    }
    
    /* Snap to top edge */
    if (*y < workarea.y + SNAP_DISTANCE) {
        *y = workarea.y;
    }
    /* Snap to bottom edge */
    else if (*y + window_height > workarea.y + workarea.height - SNAP_DISTANCE) {
        *y = workarea.y + workarea.height - window_height;
    }
}

/**
 * Application activation (create window and initialize)
 */
static void on_activate(GtkApplication *app, gpointer user_data) {
    AppState *state = g_new0(AppState, 1);

    /* Load preferences */
    state->prefs = xrg_preferences_new();
    if (!xrg_preferences_load(state->prefs)) {
        g_message("Using default preferences (first run)");
    }

    /* Sanity check: prevent loading black colors (corrupted config) */
    if (state->prefs->graph_fg1_color.red < 0.01 &&
        state->prefs->graph_fg1_color.green < 0.01 &&
        state->prefs->graph_fg1_color.blue < 0.01) {
        g_warning("Config file has corrupted black colors! Restoring Cyberpunk theme defaults...");
        xrg_preferences_apply_theme(state->prefs, "Cyberpunk");
        xrg_preferences_save(state->prefs);  /* Save immediately to fix config file */
    }

    /* Debug: Print loaded colors */
    g_message("Loaded colors - BG: (%.3f, %.3f, %.3f, %.3f)",
              state->prefs->background_color.red,
              state->prefs->background_color.green,
              state->prefs->background_color.blue,
              state->prefs->background_color.alpha);
    g_message("Loaded colors - Graph FG1: (%.3f, %.3f, %.3f, %.3f)",
              state->prefs->graph_fg1_color.red,
              state->prefs->graph_fg1_color.green,
              state->prefs->graph_fg1_color.blue,
              state->prefs->graph_fg1_color.alpha);
    g_message("Loaded colors - Graph FG2: (%.3f, %.3f, %.3f, %.3f)",
              state->prefs->graph_fg2_color.red,
              state->prefs->graph_fg2_color.green,
              state->prefs->graph_fg2_color.blue,
              state->prefs->graph_fg2_color.alpha);

    /* Initialize collectors */
    state->cpu_collector = xrg_cpu_collector_new(200);  /* 200 data points */
    state->memory_collector = xrg_memory_collector_new(200);
    state->network_collector = xrg_network_collector_new(200);
    state->disk_collector = xrg_disk_collector_new(200);
    state->gpu_collector = xrg_gpu_collector_new(200);
    state->aitoken_collector = xrg_aitoken_collector_new(200);

    /* Create main window */
    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "XRG-Linux");
    gtk_window_set_default_size(GTK_WINDOW(state->window),
                                state->prefs->window_width,
                                state->prefs->window_height + TITLE_BAR_HEIGHT);
    gtk_window_move(GTK_WINDOW(state->window),
                    state->prefs->window_x,
                    state->prefs->window_y);

    /* Make window frameless and always-on-top */
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);
    if (state->prefs->window_always_on_top) {
        gtk_window_set_keep_above(GTK_WINDOW(state->window), TRUE);
    }

    /* Set window transparency */
    GdkScreen *screen = gtk_widget_get_screen(state->window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual != NULL && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(state->window, visual);
        gtk_widget_set_opacity(state->window, state->prefs->window_opacity);
    }

    /* Create overlay for resize grip */
    state->overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(state->window), state->overlay);

    /* Store current orientation */
    state->current_layout_orientation = state->prefs->layout_orientation;

    /* Create box container with orientation from preferences */
    GtkOrientation gtk_orientation = (state->prefs->layout_orientation == XRG_LAYOUT_VERTICAL)
        ? GTK_ORIENTATION_VERTICAL
        : GTK_ORIENTATION_HORIZONTAL;
    state->vbox = gtk_box_new(gtk_orientation, 0);
    gtk_container_add(GTK_CONTAINER(state->overlay), state->vbox);

    /* Add title bar */
    state->title_bar = create_title_bar(state);
    gtk_box_pack_start(GTK_BOX(state->vbox), state->title_bar, FALSE, FALSE, 0);

    /* Add resize grip as overlay */
    state->resize_grip = create_resize_grip(state);
    gtk_overlay_add_overlay(GTK_OVERLAY(state->overlay), state->resize_grip);

    /* Create CPU module container */
    state->cpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create CPU graph */
    state->cpu_drawing_area = gtk_drawing_area_new();
    /* In horizontal mode, use square graphs (width = height = graph_width) */
    gint cpu_height = (state->prefs->layout_orientation == XRG_LAYOUT_HORIZONTAL)
                      ? state->prefs->graph_width
                      : state->prefs->graph_height_cpu;
    gtk_widget_set_size_request(state->cpu_drawing_area,
                                state->prefs->graph_width,
                                cpu_height);

    /* Enable tooltips */
    gtk_widget_set_has_tooltip(state->cpu_drawing_area, TRUE);

    /* Enable button press and motion events */
    gtk_widget_add_events(state->cpu_drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->cpu_drawing_area, "draw", G_CALLBACK(on_draw_cpu), state);
    g_signal_connect(state->cpu_drawing_area, "button-press-event", G_CALLBACK(on_cpu_button_press), state);
    g_signal_connect(state->cpu_drawing_area, "motion-notify-event", G_CALLBACK(on_cpu_motion_notify), state);
    gtk_box_pack_start(GTK_BOX(state->cpu_box), state->cpu_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), state->cpu_box, TRUE, TRUE, 0);

    /* Create Memory module container */
    state->memory_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create Memory graph */
    state->memory_drawing_area = gtk_drawing_area_new();
    gint memory_height = (state->prefs->layout_orientation == XRG_LAYOUT_HORIZONTAL)
                         ? state->prefs->graph_width
                         : state->prefs->graph_height_memory;
    gtk_widget_set_size_request(state->memory_drawing_area,
                                state->prefs->graph_width,
                                memory_height);

    /* Enable tooltips */
    gtk_widget_set_has_tooltip(state->memory_drawing_area, TRUE);

    /* Enable button press and motion events */
    gtk_widget_add_events(state->memory_drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->memory_drawing_area, "draw", G_CALLBACK(on_draw_memory), state);
    g_signal_connect(state->memory_drawing_area, "button-press-event", G_CALLBACK(on_memory_button_press), state);
    g_signal_connect(state->memory_drawing_area, "motion-notify-event", G_CALLBACK(on_memory_motion_notify), state);
    gtk_box_pack_start(GTK_BOX(state->memory_box), state->memory_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), state->memory_box, TRUE, TRUE, 0);

    /* Create Network module container */
    state->network_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create Network graph */
    state->network_drawing_area = gtk_drawing_area_new();
    gint network_height = (state->prefs->layout_orientation == XRG_LAYOUT_HORIZONTAL)
                          ? state->prefs->graph_width
                          : state->prefs->graph_height_network;
    gtk_widget_set_size_request(state->network_drawing_area,
                                state->prefs->graph_width,
                                network_height);

    /* Enable tooltips */
    gtk_widget_set_has_tooltip(state->network_drawing_area, TRUE);

    /* Enable button press and motion events */
    gtk_widget_add_events(state->network_drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->network_drawing_area, "draw", G_CALLBACK(on_draw_network), state);
    g_signal_connect(state->network_drawing_area, "button-press-event", G_CALLBACK(on_network_button_press), state);
    g_signal_connect(state->network_drawing_area, "motion-notify-event", G_CALLBACK(on_network_motion_notify), state);
    gtk_box_pack_start(GTK_BOX(state->network_box), state->network_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), state->network_box, TRUE, TRUE, 0);

    /* Create Disk module container */
    state->disk_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create Disk graph */
    state->disk_drawing_area = gtk_drawing_area_new();
    gint disk_height = (state->prefs->layout_orientation == XRG_LAYOUT_HORIZONTAL)
                       ? state->prefs->graph_width
                       : state->prefs->graph_height_disk;
    gtk_widget_set_size_request(state->disk_drawing_area,
                                state->prefs->graph_width,
                                disk_height);

    /* Enable tooltips */
    gtk_widget_set_has_tooltip(state->disk_drawing_area, TRUE);

    /* Enable button press and motion events */
    gtk_widget_add_events(state->disk_drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->disk_drawing_area, "draw", G_CALLBACK(on_draw_disk), state);
    g_signal_connect(state->disk_drawing_area, "button-press-event", G_CALLBACK(on_disk_button_press), state);
    g_signal_connect(state->disk_drawing_area, "motion-notify-event", G_CALLBACK(on_disk_motion_notify), state);
    gtk_box_pack_start(GTK_BOX(state->disk_box), state->disk_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), state->disk_box, TRUE, TRUE, 0);

    /* Create GPU module container */
    state->gpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create GPU graph */
    state->gpu_drawing_area = gtk_drawing_area_new();
    gint gpu_height = (state->prefs->layout_orientation == XRG_LAYOUT_HORIZONTAL)
                      ? state->prefs->graph_width
                      : state->prefs->graph_height_gpu;
    gtk_widget_set_size_request(state->gpu_drawing_area,
                                state->prefs->graph_width,
                                gpu_height);

    /* Enable tooltips */
    gtk_widget_set_has_tooltip(state->gpu_drawing_area, TRUE);

    /* Enable button press and motion events */
    gtk_widget_add_events(state->gpu_drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->gpu_drawing_area, "draw", G_CALLBACK(on_draw_gpu), state);
    g_signal_connect(state->gpu_drawing_area, "button-press-event", G_CALLBACK(on_gpu_button_press), state);
    g_signal_connect(state->gpu_drawing_area, "motion-notify-event", G_CALLBACK(on_gpu_motion_notify), state);
    gtk_box_pack_start(GTK_BOX(state->gpu_box), state->gpu_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), state->gpu_box, TRUE, TRUE, 0);

    /* Create AI Token module container */
    state->aitoken_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create AI Token graph */
    state->aitoken_drawing_area = gtk_drawing_area_new();
    gint aitoken_height = (state->prefs->layout_orientation == XRG_LAYOUT_HORIZONTAL)
                          ? state->prefs->graph_width
                          : state->prefs->graph_height_aitoken;
    gtk_widget_set_size_request(state->aitoken_drawing_area,
                                state->prefs->graph_width,
                                aitoken_height);

    /* Enable tooltips */
    gtk_widget_set_has_tooltip(state->aitoken_drawing_area, TRUE);

    /* Enable button press and motion events */
    gtk_widget_add_events(state->aitoken_drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(state->aitoken_drawing_area, "draw", G_CALLBACK(on_draw_aitoken), state);
    g_signal_connect(state->aitoken_drawing_area, "button-press-event", G_CALLBACK(on_aitoken_button_press), state);
    g_signal_connect(state->aitoken_drawing_area, "motion-notify-event", G_CALLBACK(on_aitoken_motion_notify), state);
    gtk_box_pack_start(GTK_BOX(state->aitoken_box), state->aitoken_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), state->aitoken_box, TRUE, TRUE, 0);

    /* Set initial visibility based on preferences */
    gtk_widget_set_visible(state->cpu_box, state->prefs->show_cpu);
    gtk_widget_set_visible(state->memory_box, state->prefs->show_memory);
    gtk_widget_set_visible(state->network_box, state->prefs->show_network);
    gtk_widget_set_visible(state->disk_box, state->prefs->show_disk);
    gtk_widget_set_visible(state->gpu_box, state->prefs->show_gpu);
    gtk_widget_set_visible(state->aitoken_box, state->prefs->show_aitoken);

    /* Connect keyboard events */
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_key_press), state);

    /* Connect configure event (window moved or resized) */
    g_signal_connect(state->window, "configure-event", G_CALLBACK(on_window_configure), state);

    /* Connect destroy signal */
    g_signal_connect(state->window, "destroy", G_CALLBACK(on_window_destroy), state);

    /* Create preferences window */
    state->prefs_window = xrg_preferences_window_new(GTK_WINDOW(state->window), state->prefs);
    xrg_preferences_window_set_applied_callback(state->prefs_window, on_preferences_applied, state);

    /* Start update timer (1 second) */
    state->update_timer_id = g_timeout_add(state->prefs->normal_update_interval,
                                           on_update_timer, state);

    /* Show window */
    gtk_widget_show_all(state->window);

    g_message("XRG-Linux started - Monitoring %d CPU cores",
              xrg_cpu_collector_get_num_cpus(state->cpu_collector));
}

/**
 * CPU module button press (context menu)
 */
static gboolean on_cpu_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    
    if (event->button == 3) {  /* Right click */
        show_cpu_context_menu(state, event);
        return TRUE;
    }
    
    return FALSE;
}

/**
 * Show CPU context menu
 */
static void show_cpu_context_menu(AppState *state, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();
    
    /* CPU Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("CPU Settings...");
    g_signal_connect_swapped(settings_item, "activate",
                             G_CALLBACK(xrg_preferences_window_show), state->prefs_window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);
    
    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    
    /* Stats */
    gchar *stats_text = g_strdup_printf("Cores: %d | Usage: %.1f%% | Load: %.2f",
                                       xrg_cpu_collector_get_num_cpus(state->cpu_collector),
                                       xrg_cpu_collector_get_total_usage(state->cpu_collector),
                                       xrg_cpu_collector_get_load_average_1min(state->cpu_collector));
    GtkWidget *stats_item = gtk_menu_item_new_with_label(stats_text);
    gtk_widget_set_sensitive(stats_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stats_item);
    g_free(stats_text);
    
    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

/**
 * CPU motion notify (tooltip)
 */
static gboolean on_cpu_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Get datasets */
    XRGDataset *user_dataset = xrg_cpu_collector_get_user_dataset(state->cpu_collector);
    XRGDataset *system_dataset = xrg_cpu_collector_get_system_dataset(state->cpu_collector);
    gint count = xrg_dataset_get_count(user_dataset);

    if (count < 2) {
        gtk_widget_set_tooltip_text(widget, "CPU: Waiting for data...");
        return FALSE;
    }

    /* Calculate which data point the mouse is over */
    gint index = (gint)((event->x / allocation.width) * count);
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    /* Get values at this index */
    gdouble user_val = xrg_dataset_get_value(user_dataset, index);
    gdouble system_val = xrg_dataset_get_value(system_dataset, index);
    gdouble total_val = user_val + system_val;

    /* Set tooltip */
    gchar *tooltip = g_strdup_printf("CPU Usage: %.1f%%\nUser: %.1f%% | System: %.1f%%",
                                     total_val, user_val, system_val);
    gtk_widget_set_tooltip_text(widget, tooltip);
    g_free(tooltip);

    return FALSE;
}

/**
 * Memory module button press (context menu)
 */
static gboolean on_memory_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 3) {  /* Right click */
        show_memory_context_menu(state, event);
        return TRUE;
    }

    return FALSE;
}

/**
 * Show Memory context menu
 */
static void show_memory_context_menu(AppState *state, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();

    /* Memory Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Memory Settings...");
    g_signal_connect_swapped(settings_item, "activate",
                             G_CALLBACK(xrg_preferences_window_show), state->prefs_window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Stats */
    guint64 total_mem = xrg_memory_collector_get_total_memory(state->memory_collector);
    guint64 used_mem = xrg_memory_collector_get_used_memory(state->memory_collector);
    guint64 swap_used = xrg_memory_collector_get_swap_used(state->memory_collector);

    gdouble total_gb = total_mem / (1024.0 * 1024.0 * 1024.0);
    gdouble used_gb = used_mem / (1024.0 * 1024.0 * 1024.0);
    gdouble swap_gb = swap_used / (1024.0 * 1024.0 * 1024.0);

    gchar *stats_text = g_strdup_printf("Used: %.1f/%.1f GB | Swap: %.1f GB",
                                       used_gb, total_gb, swap_gb);
    GtkWidget *stats_item = gtk_menu_item_new_with_label(stats_text);
    gtk_widget_set_sensitive(stats_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stats_item);
    g_free(stats_text);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

/**
 * Memory motion notify (tooltip)
 */
static gboolean on_memory_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Get datasets */
    XRGDataset *used_dataset = xrg_memory_collector_get_used_dataset(state->memory_collector);
    XRGDataset *wired_dataset = xrg_memory_collector_get_wired_dataset(state->memory_collector);
    XRGDataset *cached_dataset = xrg_memory_collector_get_cached_dataset(state->memory_collector);
    gint count = xrg_dataset_get_count(used_dataset);

    if (count < 2) {
        gtk_widget_set_tooltip_text(widget, "Memory: Waiting for data...");
        return FALSE;
    }

    /* Calculate which data point the mouse is over */
    gint index = (gint)((event->x / allocation.width) * count);
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    /* Get values at this index (percentages) */
    gdouble used_val = xrg_dataset_get_value(used_dataset, index);
    gdouble wired_val = xrg_dataset_get_value(wired_dataset, index);
    gdouble cached_val = xrg_dataset_get_value(cached_dataset, index);
    gdouble total_pct = used_val + wired_val + cached_val;

    /* Calculate absolute values in GB */
    guint64 total_mem = xrg_memory_collector_get_total_memory(state->memory_collector);
    gdouble total_gb = total_mem / (1024.0 * 1024.0 * 1024.0);
    gdouble used_gb = (used_val / 100.0) * total_gb;
    gdouble wired_gb = (wired_val / 100.0) * total_gb;
    gdouble cached_gb = (cached_val / 100.0) * total_gb;

    /* Set tooltip */
    gchar *tooltip = g_strdup_printf("Memory Usage: %.1f%% (%.1f GB)\nUsed: %.1f GB | Wired: %.1f GB | Cached: %.1f GB",
                                     total_pct, (total_pct / 100.0) * total_gb,
                                     used_gb, wired_gb, cached_gb);
    gtk_widget_set_tooltip_text(widget, tooltip);
    g_free(tooltip);

    return FALSE;
}

/**
 * Network module button press (context menu)
 */
static gboolean on_network_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 3) {  /* Right click */
        show_network_context_menu(state, event);
        return TRUE;
    }

    return FALSE;
}

/**
 * Show Network context menu
 */
static void show_network_context_menu(AppState *state, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();

    /* Network Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Network Settings...");
    g_signal_connect_swapped(settings_item, "activate",
                             G_CALLBACK(xrg_preferences_window_show), state->prefs_window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Stats */
    const gchar *iface = xrg_network_collector_get_primary_interface(state->network_collector);
    gdouble download = xrg_network_collector_get_download_rate(state->network_collector);
    gdouble upload = xrg_network_collector_get_upload_rate(state->network_collector);

    gchar *stats_text = g_strdup_printf("%s | ↓ %.2f MB/s | ↑ %.2f MB/s",
                                       iface, download, upload);
    GtkWidget *stats_item = gtk_menu_item_new_with_label(stats_text);
    gtk_widget_set_sensitive(stats_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stats_item);
    g_free(stats_text);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

/**
 * Network motion notify (tooltip)
 */
static gboolean on_network_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Get datasets */
    XRGDataset *download_dataset = xrg_network_collector_get_download_dataset(state->network_collector);
    XRGDataset *upload_dataset = xrg_network_collector_get_upload_dataset(state->network_collector);
    gint count = xrg_dataset_get_count(download_dataset);

    if (count < 2) {
        gtk_widget_set_tooltip_text(widget, "Network: Waiting for data...");
        return FALSE;
    }

    /* Calculate which data point the mouse is over */
    gint index = (gint)((event->x / allocation.width) * count);
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    /* Get values at this index (MB/s) */
    gdouble download_val = xrg_dataset_get_value(download_dataset, index);
    gdouble upload_val = xrg_dataset_get_value(upload_dataset, index);

    /* Set tooltip */
    gchar *tooltip = g_strdup_printf("Network Traffic\nDownload: %.2f MB/s\nUpload: %.2f MB/s",
                                     download_val, upload_val);
    gtk_widget_set_tooltip_text(widget, tooltip);
    g_free(tooltip);

    return FALSE;
}

/**
 * Handle button press on Disk drawing area (for context menu)
 */
static gboolean on_disk_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 3) {  /* Right click */
        show_disk_context_menu(state, event);
        return TRUE;
    }

    return FALSE;
}

/**
 * Show Disk context menu
 */
static void show_disk_context_menu(AppState *state, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();

    /* Disk Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("Disk Settings...");
    g_signal_connect_swapped(settings_item, "activate",
                             G_CALLBACK(xrg_preferences_window_show), state->prefs_window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Stats */
    const gchar *device = xrg_disk_collector_get_primary_device(state->disk_collector);
    gdouble read_rate = xrg_disk_collector_get_read_rate(state->disk_collector);
    gdouble write_rate = xrg_disk_collector_get_write_rate(state->disk_collector);

    gchar *stats_text = g_strdup_printf("%s | Read: %.2f MB/s | Write: %.2f MB/s",
                                       device, read_rate, write_rate);
    GtkWidget *stats_item = gtk_menu_item_new_with_label(stats_text);
    gtk_widget_set_sensitive(stats_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stats_item);
    g_free(stats_text);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

/**
 * Disk motion notify (tooltip)
 */
static gboolean on_disk_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Get datasets */
    XRGDataset *read_dataset = xrg_disk_collector_get_read_dataset(state->disk_collector);
    XRGDataset *write_dataset = xrg_disk_collector_get_write_dataset(state->disk_collector);
    gint count = xrg_dataset_get_count(read_dataset);

    if (count < 2) {
        gtk_widget_set_tooltip_text(widget, "Disk: Waiting for data...");
        return FALSE;
    }

    /* Calculate which data point the mouse is over */
    gint index = (gint)((event->x / allocation.width) * count);
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    /* Get values at this index (MB/s) */
    gdouble read_val = xrg_dataset_get_value(read_dataset, index);
    gdouble write_val = xrg_dataset_get_value(write_dataset, index);

    /* Set tooltip */
    gchar *tooltip = g_strdup_printf("Disk Activity\nRead: %.2f MB/s\nWrite: %.2f MB/s",
                                     read_val, write_val);
    gtk_widget_set_tooltip_text(widget, tooltip);
    g_free(tooltip);

    return FALSE;
}

/**
 * Draw Disk graph
 */
static gboolean on_draw_disk(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->disk_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get disk datasets */
    XRGDataset *read_dataset = xrg_disk_collector_get_read_dataset(state->disk_collector);
    XRGDataset *write_dataset = xrg_disk_collector_get_write_dataset(state->disk_collector);

    gint count = xrg_dataset_get_count(read_dataset);
    if (count < 2) {
        /* Not enough data yet */
        return FALSE;
    }

    /* Find maximum rate for auto-scaling */
    gdouble max_rate = 0.1;  /* Minimum 0.1 MB/s */
    for (gint i = 0; i < count; i++) {
        gdouble read_val = xrg_dataset_get_value(read_dataset, i);
        gdouble write_val = xrg_dataset_get_value(write_dataset, i);
        if (read_val > max_rate) max_rate = read_val;
        if (write_val > max_rate) max_rate = write_val;
    }

    /* Draw read rate (cyan - FG1) */
    GdkRGBA *fg1_color = &state->prefs->disk_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = state->prefs->disk_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw write rate (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->disk_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble read_rate = xrg_disk_collector_get_read_rate(state->disk_collector);
    gdouble write_rate = xrg_disk_collector_get_write_rate(state->disk_collector);

    /* Line 1: Disk I/O label */
    gchar *line1 = g_strdup_printf("Disk I/O");
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    /* Line 2: Read rate */
    gchar *line2 = g_strdup_printf("Read: %.2f MB/s", read_rate);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    /* Line 3: Write rate */
    gchar *line3 = g_strdup_printf("Write: %.2f MB/s", write_rate);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Draw activity bar on the right (if enabled) */
    if (state->prefs->show_activity_bars) {
        gint bar_x = width - 20;  /* 20px from right edge */
        gint bar_width = 20;

        /* Draw bar background */
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
        cairo_rectangle(cr, bar_x, 0, bar_width, height);
        cairo_fill(cr);

        /* Draw bar border */
        cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bar_x + 0.5, 0.5, bar_width - 1, height - 1);
        cairo_stroke(cr);

        /* Draw filled bar representing current read rate */
        gdouble current_value = read_rate / max_rate;
        if (current_value > 1.0) current_value = 1.0;  /* Cap at 100% */
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        GdkRGBA *bar_color = &state->prefs->activity_bar_color;
        XRGGraphStyle bar_style = state->prefs->activity_bar_style;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            cairo_rectangle(cr, bar_x, bar_y, bar_width, fill_height);
            cairo_fill(cr);
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 4) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 2) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only - draw dots at top of fill level */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

/**
 * Handle button press on GPU drawing area (for context menu)
 */
static gboolean on_gpu_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 3) {  /* Right click */
        show_gpu_context_menu(state, event);
        return TRUE;
    }

    return FALSE;
}

/**
 * Show GPU context menu
 */
static void show_gpu_context_menu(AppState *state, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();

    /* GPU Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("GPU Settings...");
    g_signal_connect_swapped(settings_item, "activate",
                             G_CALLBACK(xrg_preferences_window_show), state->prefs_window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Stats */
    const gchar *gpu_name = xrg_gpu_collector_get_name(state->gpu_collector);
    gdouble utilization = xrg_gpu_collector_get_utilization(state->gpu_collector);
    gdouble mem_used = xrg_gpu_collector_get_memory_used_mb(state->gpu_collector);
    gdouble mem_total = xrg_gpu_collector_get_memory_total_mb(state->gpu_collector);
    gdouble temp = xrg_gpu_collector_get_temperature(state->gpu_collector);

    gchar *stats_text = g_strdup_printf("%s | %.0f%% | %.0f/%.0f MB | %.0f°C",
                                       gpu_name, utilization, mem_used, mem_total, temp);
    GtkWidget *stats_item = gtk_menu_item_new_with_label(stats_text);
    gtk_widget_set_sensitive(stats_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stats_item);
    g_free(stats_text);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

/**
 * GPU motion notify (tooltip)
 */
static gboolean on_gpu_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Get datasets */
    XRGDataset *util_dataset = xrg_gpu_collector_get_utilization_dataset(state->gpu_collector);
    XRGDataset *mem_dataset = xrg_gpu_collector_get_memory_dataset(state->gpu_collector);
    gint count = xrg_dataset_get_count(util_dataset);

    if (count < 2) {
        gtk_widget_set_tooltip_text(widget, "GPU: Waiting for data...");
        return FALSE;
    }

    /* Calculate which data point the mouse is over */
    gint index = (gint)((event->x / allocation.width) * count);
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    /* Get values at this index */
    gdouble util_val = xrg_dataset_get_value(util_dataset, index);
    gdouble mem_val = xrg_dataset_get_value(mem_dataset, index);

    /* Set tooltip */
    gchar *tooltip = g_strdup_printf("GPU Usage\nUtilization: %.0f%%\nMemory: %.0f%%",
                                     util_val, mem_val);
    gtk_widget_set_tooltip_text(widget, tooltip);
    g_free(tooltip);

    return FALSE;
}

/**
 * Draw GPU graph
 */
static gboolean on_draw_gpu(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get GPU datasets */
    XRGDataset *util_dataset = xrg_gpu_collector_get_utilization_dataset(state->gpu_collector);
    XRGDataset *mem_dataset = xrg_gpu_collector_get_memory_dataset(state->gpu_collector);

    gint count = xrg_dataset_get_count(util_dataset);
    if (count < 2) {
        /* Not enough data yet - draw label */
        GdkRGBA *text_color = &state->prefs->text_color;
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha * 0.5);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10);
        cairo_move_to(cr, 5, 12);
        cairo_show_text(cr, "GPU: Waiting for data...");
        return FALSE;
    }

    /* Draw GPU utilization (cyan - FG1) - bottom layer */
    GdkRGBA *fg1_color = &state->prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = state->prefs->gpu_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(util_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(util_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(util_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(util_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw GPU memory usage on top (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);

    const gchar *gpu_name = xrg_gpu_collector_get_name(state->gpu_collector);
    gdouble utilization = xrg_gpu_collector_get_utilization(state->gpu_collector);
    gdouble mem_used = xrg_gpu_collector_get_memory_used_mb(state->gpu_collector);
    gdouble mem_total = xrg_gpu_collector_get_memory_total_mb(state->gpu_collector);
    gdouble temp = xrg_gpu_collector_get_temperature(state->gpu_collector);

    /* Line 1: GPU name */
    cairo_move_to(cr, 5, 12);
    cairo_show_text(cr, gpu_name);

    /* Line 2: Utilization */
    gchar *util_text = g_strdup_printf("Util: %.0f%%", utilization);
    cairo_move_to(cr, 5, 24);
    cairo_show_text(cr, util_text);
    g_free(util_text);

    /* Line 3: Memory */
    gchar *mem_text = g_strdup_printf("Mem: %.0f/%.0f MB", mem_used, mem_total);
    cairo_move_to(cr, 5, 36);
    cairo_show_text(cr, mem_text);
    g_free(mem_text);

    /* Line 4: Temperature */
    gchar *temp_text = g_strdup_printf("Temp: %.0f°C", temp);
    cairo_move_to(cr, 5, 48);
    cairo_show_text(cr, temp_text);
    g_free(temp_text);

    /* Draw activity bar on the right (if enabled) */
    if (state->prefs->show_activity_bars) {
        gint bar_x = width - 20;  /* 20px from right edge */
        gint bar_width = 20;

        /* Draw bar background */
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
        cairo_rectangle(cr, bar_x, 0, bar_width, height);
        cairo_fill(cr);

        /* Draw bar border */
        cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bar_x + 0.5, 0.5, bar_width - 1, height - 1);
        cairo_stroke(cr);

        /* Draw filled bar representing current GPU utilization */
        gdouble current_value = utilization / 100.0;
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        /* Use activity bar color and style from preferences */
        GdkRGBA *bar_color = &state->prefs->activity_bar_color;
        XRGGraphStyle bar_style = state->prefs->activity_bar_style;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            /* Solid fill */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            cairo_rectangle(cr, bar_x, bar_y, bar_width, fill_height);
            cairo_fill(cr);
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels - dithered with 4px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 4) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots - dithered with 2px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 2) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only - draw dots at top of fill level */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

/**
 * Handle button press on AI Token drawing area (for context menu)
 */
static gboolean on_aitoken_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    if (event->button == 3) {  /* Right click */
        show_aitoken_context_menu(state, event);
        return TRUE;
    }

    return FALSE;
}

/**
 * Show AI Token context menu
 */
static void show_aitoken_context_menu(AppState *state, GdkEventButton *event) {
    GtkWidget *menu = gtk_menu_new();

    /* AI Token Settings */
    GtkWidget *settings_item = gtk_menu_item_new_with_label("AI Token Settings...");
    g_signal_connect_swapped(settings_item, "activate",
                             G_CALLBACK(xrg_preferences_window_show), state->prefs_window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    /* Stats */
    const gchar *source = xrg_aitoken_collector_get_source_name(state->aitoken_collector);
    guint64 total_tokens = xrg_aitoken_collector_get_total_tokens(state->aitoken_collector);
    guint64 session_tokens = xrg_aitoken_collector_get_session_tokens(state->aitoken_collector);

    gchar *stats_text = g_strdup_printf("%s | Total: %lu | Session: %lu",
                                       source, total_tokens, session_tokens);
    GtkWidget *stats_item = gtk_menu_item_new_with_label(stats_text);
    gtk_widget_set_sensitive(stats_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stats_item);
    g_free(stats_text);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}

/**
 * AI Token motion notify (tooltip)
 */
static gboolean on_aitoken_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    /* Get datasets */
    XRGDataset *input_dataset = xrg_aitoken_collector_get_input_dataset(state->aitoken_collector);
    XRGDataset *output_dataset = xrg_aitoken_collector_get_output_dataset(state->aitoken_collector);
    gint count = xrg_dataset_get_count(input_dataset);

    if (count < 2) {
        gtk_widget_set_tooltip_text(widget, "AI Tokens: Waiting for data...");
        return FALSE;
    }

    /* Calculate which data point the mouse is over */
    gint index = (gint)((event->x / allocation.width) * count);
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    /* Get values at this index (tokens/min) */
    gdouble input_val = xrg_dataset_get_value(input_dataset, index);
    gdouble output_val = xrg_dataset_get_value(output_dataset, index);
    gdouble total_val = input_val + output_val;

    /* Set tooltip */
    gchar *tooltip = g_strdup_printf("AI Token Usage: %.0f tokens/min\nInput: %.0f | Output: %.0f",
                                     total_val, input_val, output_val);
    gtk_widget_set_tooltip_text(widget, tooltip);
    g_free(tooltip);

    return FALSE;
}

/**
 * Draw AI Token graph
 */
static gboolean on_draw_aitoken(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get AI Token datasets */
    XRGDataset *input_dataset = xrg_aitoken_collector_get_input_dataset(state->aitoken_collector);
    XRGDataset *output_dataset = xrg_aitoken_collector_get_output_dataset(state->aitoken_collector);

    gint count = xrg_dataset_get_count(input_dataset);
    if (count < 2) {
        /* Not enough data yet */
        /* Draw label indicating waiting for data */
        GdkRGBA *text_color = &state->prefs->text_color;
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha * 0.5);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10);
        cairo_move_to(cr, 5, 12);
        cairo_show_text(cr, "AI Tokens: Waiting for data...");
        return FALSE;
    }

    /* Find maximum rate for scaling (minimum 100 tokens/min) */
    gdouble max_rate = 100.0;
    for (gint i = 0; i < count; i++) {
        gdouble input = xrg_dataset_get_value(input_dataset, i);
        gdouble output = xrg_dataset_get_value(output_dataset, i);
        if (input > max_rate) max_rate = input;
        if (output > max_rate) max_rate = output;
    }

    /* Draw input tokens (cyan - FG1) */
    GdkRGBA *fg1_color = &state->prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = state->prefs->aitoken_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw output tokens (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble tokens_per_min = xrg_aitoken_collector_get_tokens_per_minute(state->aitoken_collector);
    guint64 total_tokens = xrg_aitoken_collector_get_total_tokens(state->aitoken_collector);

    /* Convert tokens per minute to tokens per second */
    gdouble token_rate = tokens_per_min / 60.0;

    /* Line 1: AI Tokens label */
    gchar *line1 = g_strdup_printf("AI Tokens");
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    /* Line 2: Token rate */
    gchar *line2 = g_strdup_printf("Rate: %.0f/s", token_rate);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    /* Line 3: Total tokens */
    gchar *line3 = g_strdup_printf("Total: %lu", (unsigned long)total_tokens);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Draw activity bar on the right (if enabled) */
    if (state->prefs->show_activity_bars) {
        gint bar_x = width - 20;  /* 20px from right edge */
        gint bar_width = 20;

        /* Draw bar background */
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
        cairo_rectangle(cr, bar_x, 0, bar_width, height);
        cairo_fill(cr);

        /* Draw bar border */
        cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bar_x + 0.5, 0.5, bar_width - 1, height - 1);
        cairo_stroke(cr);

        /* Find max token rate in dataset to scale the bar */
        gdouble max_rate = 1.0;  /* Minimum to avoid division by zero */
        for (gint i = 0; i < count; i++) {
            gdouble input_val = xrg_dataset_get_value(input_dataset, i);
            gdouble output_val = xrg_dataset_get_value(output_dataset, i);
            gdouble total_val = input_val + output_val;
            if (total_val > max_rate) {
                max_rate = total_val;
            }
        }

        /* Draw filled bar representing current token rate */
        gdouble current_value = token_rate / max_rate;
        if (current_value > 1.0) current_value = 1.0;  /* Cap at 100% */
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        /* Use activity bar color and style from preferences */
        GdkRGBA *bar_color = &state->prefs->activity_bar_color;
        XRGGraphStyle bar_style = state->prefs->activity_bar_style;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            /* Solid fill */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            cairo_rectangle(cr, bar_x, bar_y, bar_width, fill_height);
            cairo_fill(cr);
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels - dithered with 4px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 4) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots - dithered with 2px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 2) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only - draw dots at top of fill level */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

/**
 * Keyboard shortcut handler
 */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    /* Ctrl+, = Preferences */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_comma) {
        xrg_preferences_window_show(state->prefs_window);
        return TRUE;
    }

    /* Ctrl+Q = Quit */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_q) {
        gtk_widget_destroy(state->window);
        return TRUE;
    }

    /* Ctrl+1 = Toggle CPU */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_1) {
        state->prefs->show_cpu = !state->prefs->show_cpu;
        gtk_widget_set_visible(state->cpu_box, state->prefs->show_cpu);
        xrg_preferences_save(state->prefs);
        g_message("CPU module %s", state->prefs->show_cpu ? "shown" : "hidden");
        return TRUE;
    }

    /* Ctrl+2 = Toggle Memory */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_2) {
        state->prefs->show_memory = !state->prefs->show_memory;
        gtk_widget_set_visible(state->memory_box, state->prefs->show_memory);
        xrg_preferences_save(state->prefs);
        g_message("Memory module %s", state->prefs->show_memory ? "shown" : "hidden");
        return TRUE;
    }

    /* Ctrl+3 = Toggle Network */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_3) {
        state->prefs->show_network = !state->prefs->show_network;
        gtk_widget_set_visible(state->network_box, state->prefs->show_network);
        xrg_preferences_save(state->prefs);
        g_message("Network module %s", state->prefs->show_network ? "shown" : "hidden");
        return TRUE;
    }

    /* Ctrl+4 = Toggle Disk */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_4) {
        state->prefs->show_disk = !state->prefs->show_disk;
        gtk_widget_set_visible(state->disk_box, state->prefs->show_disk);
        xrg_preferences_save(state->prefs);
        g_message("Disk module %s", state->prefs->show_disk ? "shown" : "hidden");
        return TRUE;
    }

    /* Ctrl+5 = Toggle AI Tokens */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_5) {
        state->prefs->show_aitoken = !state->prefs->show_aitoken;
        gtk_widget_set_visible(state->aitoken_box, state->prefs->show_aitoken);
        xrg_preferences_save(state->prefs);
        g_message("AI Token module %s", state->prefs->show_aitoken ? "shown" : "hidden");
        return TRUE;
    }

    /* Ctrl+6 = Toggle GPU */
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_6) {
        state->prefs->show_gpu = !state->prefs->show_gpu;
        gtk_widget_set_visible(state->gpu_box, state->prefs->show_gpu);
        xrg_preferences_save(state->prefs);
        g_message("GPU module %s", state->prefs->show_gpu ? "shown" : "hidden");
        return TRUE;
    }

    return FALSE;
}







/**
 * Draw CPU graph
 */
static gboolean on_draw_cpu(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get CPU datasets */
    XRGDataset *user_dataset = xrg_cpu_collector_get_user_dataset(state->cpu_collector);
    XRGDataset *system_dataset = xrg_cpu_collector_get_system_dataset(state->cpu_collector);

    gint count = xrg_dataset_get_count(user_dataset);
    if (count < 2) {
        /* Not enough data yet */
        return FALSE;
    }

    /* Draw user CPU usage (cyan - FG1) */
    GdkRGBA *fg1_color = &state->prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = state->prefs->cpu_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw system CPU usage on top (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble user_val = xrg_dataset_get_value(user_dataset, i);
            gdouble system_val = xrg_dataset_get_value(system_dataset, i);
            gdouble total_val = user_val + system_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (total_val / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots (stacked on top of user) */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble user_val = xrg_dataset_get_value(user_dataset, i);
            gdouble system_val = xrg_dataset_get_value(system_dataset, i);
            gdouble total_val = user_val + system_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (user_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            /* Fill from user level to total level with dots */
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots (stacked on top of user) */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble user_val = xrg_dataset_get_value(user_dataset, i);
            gdouble system_val = xrg_dataset_get_value(system_dataset, i);
            gdouble total_val = user_val + system_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (user_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            /* Fill from user level to total level with dots */
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots (stacked on top of user) */
        for (gint i = 0; i < count; i++) {
            gdouble user_val = xrg_dataset_get_value(user_dataset, i);
            gdouble system_val = xrg_dataset_get_value(system_dataset, i);
            gdouble total_val = user_val + system_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (total_val / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble total_usage = xrg_cpu_collector_get_total_usage(state->cpu_collector);
    gdouble load_avg = xrg_cpu_collector_get_load_average_1min(state->cpu_collector);

    /* Get user and system from latest values in datasets */
    gdouble user_usage = xrg_dataset_get_latest(user_dataset);
    gdouble system_usage = xrg_dataset_get_latest(system_dataset);

    /* Line 1: Total CPU */
    gchar *line1 = g_strdup_printf("CPU: %.1f%%", total_usage);
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    /* Line 2: User and System */
    gchar *line2 = g_strdup_printf("User: %.1f%% | System: %.1f%%", user_usage, system_usage);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    /* Line 3: Load average */
    gchar *line3 = g_strdup_printf("Load: %.2f", load_avg);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Draw activity bar on the right (if enabled) */
    if (state->prefs->show_activity_bars) {
        gint bar_x = width - 20;  /* 20px from right edge */
        gint bar_width = 20;

        /* Draw bar background */
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
        cairo_rectangle(cr, bar_x, 0, bar_width, height);
        cairo_fill(cr);

        /* Draw bar border */
        cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bar_x + 0.5, 0.5, bar_width - 1, height - 1);
        cairo_stroke(cr);

        /* Draw filled bar representing current CPU usage */
        gdouble current_value = total_usage / 100.0;
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        /* Use activity bar color and style from preferences */
        GdkRGBA *bar_color = &state->prefs->activity_bar_color;
        XRGGraphStyle bar_style = state->prefs->activity_bar_style;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            /* Solid fill */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            cairo_rectangle(cr, bar_x, bar_y, bar_width, fill_height);
            cairo_fill(cr);
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels - dithered with 4px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 4) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots - dithered with 2px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 2) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only - draw dots at top of fill level */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

/**
 * Draw Memory graph
 */
static gboolean on_draw_memory(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get memory datasets */
    XRGDataset *used_dataset = xrg_memory_collector_get_used_dataset(state->memory_collector);
    XRGDataset *wired_dataset = xrg_memory_collector_get_wired_dataset(state->memory_collector);
    XRGDataset *cached_dataset = xrg_memory_collector_get_cached_dataset(state->memory_collector);

    gint count = xrg_dataset_get_count(used_dataset);
    if (count < 2) {
        /* Not enough data yet */
        return FALSE;
    }

    /* Draw used memory (cyan - FG1) - bottom layer */
    GdkRGBA *fg1_color = &state->prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = state->prefs->memory_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw wired memory on top (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble total_val = used_val + wired_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (total_val / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots (stacked on top of used) */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble total_val = used_val + wired_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (used_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            /* Fill from used level to total level with dots */
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots (stacked on top of used) */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble total_val = used_val + wired_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (used_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            /* Fill from used level to total level with dots */
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots (stacked on top of used) */
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble total_val = used_val + wired_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (total_val / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw cached memory on top (amber - FG3) */
    GdkRGBA *fg3_color = &state->prefs->graph_fg3_color;
    cairo_set_source_rgba(cr, fg3_color->red, fg3_color->green, fg3_color->blue, fg3_color->alpha * 0.5);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble cached_val = xrg_dataset_get_value(cached_dataset, i);
            gdouble total_val = used_val + wired_val + cached_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (total_val / 100.0 * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots (stacked on top of used+wired) */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble cached_val = xrg_dataset_get_value(cached_dataset, i);
            gdouble total_val = used_val + wired_val + cached_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - ((used_val + wired_val) / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            /* Fill from used+wired level to total level with dots */
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots (stacked on top of used+wired) */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble cached_val = xrg_dataset_get_value(cached_dataset, i);
            gdouble total_val = used_val + wired_val + cached_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - ((used_val + wired_val) / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            /* Fill from used+wired level to total level with dots */
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots (stacked on top of used+wired) */
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble cached_val = xrg_dataset_get_value(cached_dataset, i);
            gdouble total_val = used_val + wired_val + cached_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (total_val / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    guint64 total_memory = xrg_memory_collector_get_total_memory(state->memory_collector);
    guint64 used_memory = xrg_memory_collector_get_used_memory(state->memory_collector);
    guint64 swap_used = xrg_memory_collector_get_swap_used(state->memory_collector);

    /* Get cached memory from dataset (percentage) and convert to bytes */
    gdouble cached_percent = xrg_dataset_get_latest(cached_dataset);
    guint64 cached_memory = (guint64)(total_memory * cached_percent / 100.0);

    /* Convert from bytes to MB */
    gdouble total_mb = total_memory / (1024.0 * 1024.0);
    gdouble used_mb = used_memory / (1024.0 * 1024.0);
    gdouble cached_mb = cached_memory / (1024.0 * 1024.0);
    gdouble swap_mb = swap_used / (1024.0 * 1024.0);

    /* Line 1: Memory usage */
    gchar *line1 = g_strdup_printf("Memory: %.0f/%.0f MB", used_mb, total_mb);
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    /* Line 2: Cached memory */
    gchar *line2 = g_strdup_printf("Cached: %.0f MB", cached_mb);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    /* Line 3: Swap */
    gchar *line3 = g_strdup_printf("Swap: %.0f MB", swap_mb);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Draw activity bar on the right (if enabled) */
    if (state->prefs->show_activity_bars) {
        gint bar_x = width - 20;  /* 20px from right edge */
        gint bar_width = 20;

        /* Draw bar background */
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
        cairo_rectangle(cr, bar_x, 0, bar_width, height);
        cairo_fill(cr);

        /* Draw bar border */
        cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bar_x + 0.5, 0.5, bar_width - 1, height - 1);
        cairo_stroke(cr);

        /* Draw filled bar representing current memory usage (used + wired + cached) */
        gdouble used_val = xrg_dataset_get_latest(used_dataset);
        gdouble wired_val = xrg_dataset_get_latest(wired_dataset);
        gdouble cached_val = xrg_dataset_get_latest(cached_dataset);
        gdouble current_value = (used_val + wired_val + cached_val) / 100.0;
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        /* Use activity bar color and style from preferences */
        GdkRGBA *bar_color = &state->prefs->activity_bar_color;
        XRGGraphStyle bar_style = state->prefs->activity_bar_style;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            /* Solid fill */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            cairo_rectangle(cr, bar_x, bar_y, bar_width, fill_height);
            cairo_fill(cr);
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels - dithered with 4px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 4) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots - dithered with 2px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 2) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only - draw dots at top of fill level */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

/**
 * Draw Network graph
 */
static gboolean on_draw_network(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    gint width = allocation.width;
    gint height = allocation.height;

    /* Draw background */
    GdkRGBA *bg_color = &state->prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &state->prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get network datasets */
    XRGDataset *download_dataset = xrg_network_collector_get_download_dataset(state->network_collector);
    XRGDataset *upload_dataset = xrg_network_collector_get_upload_dataset(state->network_collector);

    gint count = xrg_dataset_get_count(download_dataset);
    if (count < 2) {
        /* Not enough data yet */
        return FALSE;
    }

    /* Find maximum rate for scaling */
    gdouble max_rate = 0.1;  /* Minimum 0.1 MB/s */
    for (gint i = 0; i < count; i++) {
        gdouble download = xrg_dataset_get_value(download_dataset, i);
        gdouble upload = xrg_dataset_get_value(upload_dataset, i);
        if (download > max_rate) max_rate = download;
        if (upload > max_rate) max_rate = upload;
    }

    /* Draw download rate (cyan - FG1) */
    GdkRGBA *fg1_color = &state->prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = state->prefs->network_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(download_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(download_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(download_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(download_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw upload rate (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area (original behavior) */
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        /* Chunky pixels - fill area with dots */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots - fill area with small dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);

            /* Fill from bottom to the data line with dots */
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Hollow - outline only with dots */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble download_rate = xrg_network_collector_get_download_rate(state->network_collector);
    gdouble upload_rate = xrg_network_collector_get_upload_rate(state->network_collector);

    /* Line 1: Network label */
    gchar *line1 = g_strdup_printf("Network");
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    /* Line 2: Download rate */
    gchar *line2 = g_strdup_printf("↓ %.2f MB/s", download_rate);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    /* Line 3: Upload rate */
    gchar *line3 = g_strdup_printf("↑ %.2f MB/s", upload_rate);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Draw activity bar on the right (if enabled) */
    if (state->prefs->show_activity_bars) {
        gint bar_x = width - 20;  /* 20px from right edge */
        gint bar_width = 20;

        /* Draw bar background */
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
        cairo_rectangle(cr, bar_x, 0, bar_width, height);
        cairo_fill(cr);

        /* Draw bar border */
        cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bar_x + 0.5, 0.5, bar_width - 1, height - 1);
        cairo_stroke(cr);

        /* Find max download rate in dataset to scale the bar */
        gdouble max_rate = 0.1;  /* Minimum to avoid division by zero */
        for (gint i = 0; i < count; i++) {
            gdouble rate = xrg_dataset_get_value(download_dataset, i);
            if (rate > max_rate) {
                max_rate = rate;
            }
        }

        /* Draw filled bar representing current download rate */
        gdouble current_value = download_rate / max_rate;
        if (current_value > 1.0) current_value = 1.0;  /* Cap at 100% */
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        /* Use activity bar color and style from preferences */
        GdkRGBA *bar_color = &state->prefs->activity_bar_color;
        XRGGraphStyle bar_style = state->prefs->activity_bar_style;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            /* Solid fill */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            cairo_rectangle(cr, bar_x, bar_y, bar_width, fill_height);
            cairo_fill(cr);
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels - dithered with 4px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 4) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots - dithered with 2px spacing */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha * 0.8);
            for (gdouble y = height; y >= bar_y; y -= 2) {
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only - draw dots at top of fill level */
            cairo_set_source_rgba(cr, bar_color->red, bar_color->green, bar_color->blue, bar_color->alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

/**
 * Update timer callback (1 second interval)
 */
static gboolean on_update_timer(gpointer user_data) {
    AppState *state = (AppState *)user_data;

    /* Update collectors */
    xrg_cpu_collector_update(state->cpu_collector);
    xrg_memory_collector_update(state->memory_collector);
    xrg_network_collector_update(state->network_collector);
    xrg_disk_collector_update(state->disk_collector);
    xrg_gpu_collector_update(state->gpu_collector);
    xrg_aitoken_collector_update(state->aitoken_collector);

    /* Redraw graphs */
    gtk_widget_queue_draw(state->cpu_drawing_area);
    gtk_widget_queue_draw(state->memory_drawing_area);
    gtk_widget_queue_draw(state->network_drawing_area);
    gtk_widget_queue_draw(state->disk_drawing_area);
    gtk_widget_queue_draw(state->gpu_drawing_area);
    gtk_widget_queue_draw(state->aitoken_drawing_area);

    return G_SOURCE_CONTINUE;  /* Keep timer running */
}

/**
 * Window destroy callback (cleanup)
 */
static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    AppState *state = (AppState *)user_data;

    /* Stop timer */
    if (state->update_timer_id > 0) {
        g_source_remove(state->update_timer_id);
    }

    /* Save window position */
    gint x, y, width, height;
    gtk_window_get_position(GTK_WINDOW(state->window), &x, &y);
    gtk_window_get_size(GTK_WINDOW(state->window), &width, &height);
    state->prefs->window_x = x;
    state->prefs->window_y = y;
    state->prefs->window_width = width;
    state->prefs->window_height = height - TITLE_BAR_HEIGHT;
    g_message("On exit - saving position: x=%d, y=%d, width=%d, height=%d", x, y, width, height - TITLE_BAR_HEIGHT);

    /* Sanity check: prevent saving black colors (corrupted) */
    if (state->prefs->graph_fg1_color.red < 0.01 &&
        state->prefs->graph_fg1_color.green < 0.01 &&
        state->prefs->graph_fg1_color.blue < 0.01) {
        g_warning("Colors corrupted to black! Restoring Cyberpunk theme defaults...");
        xrg_preferences_apply_theme(state->prefs, "Cyberpunk");
    }

    /* Save preferences */
    g_message("Saving on exit - Graph FG1: (%.3f, %.3f, %.3f, %.3f)",
              state->prefs->graph_fg1_color.red, state->prefs->graph_fg1_color.green,
              state->prefs->graph_fg1_color.blue, state->prefs->graph_fg1_color.alpha);
    xrg_preferences_save(state->prefs);

    /* Cleanup */
    xrg_cpu_collector_free(state->cpu_collector);
    xrg_memory_collector_free(state->memory_collector);
    xrg_network_collector_free(state->network_collector);
    xrg_disk_collector_free(state->disk_collector);
    xrg_gpu_collector_free(state->gpu_collector);
    xrg_aitoken_collector_free(state->aitoken_collector);
    xrg_preferences_window_free(state->prefs_window);
    xrg_preferences_free(state->prefs);
    g_free(state);

    g_message("XRG-Linux stopped");
}

/**
 * Preferences applied callback - update module visibility and layout
 */
static void on_preferences_applied(gpointer user_data) {
    AppState *state = (AppState *)user_data;

    /* Update visibility of all module boxes based on preferences */
    gtk_widget_set_visible(state->cpu_box, state->prefs->show_cpu);
    gtk_widget_set_visible(state->memory_box, state->prefs->show_memory);
    gtk_widget_set_visible(state->network_box, state->prefs->show_network);
    gtk_widget_set_visible(state->disk_box, state->prefs->show_disk);
    gtk_widget_set_visible(state->gpu_box, state->prefs->show_gpu);
    gtk_widget_set_visible(state->aitoken_box, state->prefs->show_aitoken);

    /* Check if layout orientation changed */
    if (state->current_layout_orientation != state->prefs->layout_orientation) {
        /* Show dialog to user that restart is needed */
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK_CANCEL,
                                                   "Layout orientation changed");
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                                 "XRG needs to restart for the layout change to take effect.\nRestart now?");

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_OK) {
            /* Restart the application */
            const gchar *appimage_path = g_getenv("APPIMAGE");
            if (appimage_path) {
                /* Running as AppImage - restart via AppImage */
                execl(appimage_path, appimage_path, NULL);
            } else {
                /* Try to restart using argv[0] or program path */
                gchar *program_path = g_find_program_in_path("xrg-linux");
                if (program_path) {
                    execl(program_path, "xrg-linux", NULL);
                    g_free(program_path);
                } else {
                    g_message("Could not find xrg-linux to restart. Please restart manually.");
                }
            }
        }
    }

    g_message("Module visibility updated from preferences");
}
