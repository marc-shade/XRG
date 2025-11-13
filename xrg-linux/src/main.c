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
    GtkWidget *cpu_drawing_area;
    GtkWidget *cpu_activity_bar;
    GtkWidget *memory_drawing_area;
    GtkWidget *memory_activity_bar;
    GtkWidget *network_drawing_area;
    GtkWidget *network_activity_bar;
    GtkWidget *disk_drawing_area;
    GtkWidget *disk_activity_bar;
    GtkWidget *aitoken_drawing_area;
    GtkWidget *aitoken_activity_bar;
    XRGPreferences *prefs;
    XRGCPUCollector *cpu_collector;
    XRGMemoryCollector *memory_collector;
    XRGNetworkCollector *network_collector;
    XRGDiskCollector *disk_collector;
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
} AppState;

/* Forward declarations */
static void on_activate(GtkApplication *app, gpointer user_data);
static GtkWidget* create_title_bar(AppState *state);
static GtkWidget* create_resize_grip(AppState *state);
static void on_title_bar_realize(GtkWidget *widget, gpointer user_data);
static void on_resize_grip_realize(GtkWidget *widget, gpointer user_data);
static gboolean on_title_bar_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_title_bar_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_title_bar_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean on_draw_title_bar(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_cpu_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_cpu(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_cpu_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void show_cpu_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_memory_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_memory(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_memory_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void show_memory_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_network_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_network(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_network_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void show_network_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_disk_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_disk(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_disk_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void show_disk_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_draw_aitoken_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_draw_aitoken(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_aitoken_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void show_aitoken_context_menu(AppState *state, GdkEventButton *event);
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean on_resize_grip_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_resize_grip_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean on_resize_grip_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean on_draw_resize_grip(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static gboolean on_update_timer(gpointer user_data);
static void on_window_destroy(GtkWidget *widget, gpointer user_data);
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

    /* Initialize collectors */
    state->cpu_collector = xrg_cpu_collector_new(200);  /* 200 data points */
    state->memory_collector = xrg_memory_collector_new(200);
    state->network_collector = xrg_network_collector_new(200);
    state->disk_collector = xrg_disk_collector_new(200);
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
    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(state->window), overlay);

    /* Create vertical box container */
    state->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(overlay), state->vbox);

    /* Add title bar */
    state->title_bar = create_title_bar(state);
    gtk_box_pack_start(GTK_BOX(state->vbox), state->title_bar, FALSE, FALSE, 0);

    /* Add resize grip as overlay */
    state->resize_grip = create_resize_grip(state);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), state->resize_grip);

    /* Create CPU module container */
    GtkWidget *cpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create CPU activity bar */
    state->cpu_activity_bar = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->cpu_activity_bar, state->prefs->graph_width, 20);
    g_signal_connect(state->cpu_activity_bar, "draw", G_CALLBACK(on_draw_cpu_activity), state);
    gtk_box_pack_start(GTK_BOX(cpu_box), state->cpu_activity_bar, FALSE, FALSE, 0);

    /* Create CPU graph */
    state->cpu_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->cpu_drawing_area,
                                state->prefs->graph_width,
                                state->prefs->graph_height_cpu);

    /* Enable button press events for context menu */
    gtk_widget_add_events(state->cpu_drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->cpu_drawing_area, "draw", G_CALLBACK(on_draw_cpu), state);
    g_signal_connect(state->cpu_drawing_area, "button-press-event", G_CALLBACK(on_cpu_button_press), state);
    gtk_box_pack_start(GTK_BOX(cpu_box), state->cpu_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), cpu_box, TRUE, TRUE, 0);

    /* Create Memory module container */
    GtkWidget *memory_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create Memory activity bar */
    state->memory_activity_bar = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->memory_activity_bar, state->prefs->graph_width, 20);
    g_signal_connect(state->memory_activity_bar, "draw", G_CALLBACK(on_draw_memory_activity), state);
    gtk_box_pack_start(GTK_BOX(memory_box), state->memory_activity_bar, FALSE, FALSE, 0);

    /* Create Memory graph */
    state->memory_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->memory_drawing_area,
                                state->prefs->graph_width,
                                state->prefs->graph_height_memory);

    /* Enable button press events for context menu */
    gtk_widget_add_events(state->memory_drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->memory_drawing_area, "draw", G_CALLBACK(on_draw_memory), state);
    g_signal_connect(state->memory_drawing_area, "button-press-event", G_CALLBACK(on_memory_button_press), state);
    gtk_box_pack_start(GTK_BOX(memory_box), state->memory_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), memory_box, TRUE, TRUE, 0);

    /* Create Network module container */
    GtkWidget *network_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create Network activity bar */
    state->network_activity_bar = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->network_activity_bar, state->prefs->graph_width, 20);
    g_signal_connect(state->network_activity_bar, "draw", G_CALLBACK(on_draw_network_activity), state);
    gtk_box_pack_start(GTK_BOX(network_box), state->network_activity_bar, FALSE, FALSE, 0);

    /* Create Network graph */
    state->network_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->network_drawing_area,
                                state->prefs->graph_width,
                                state->prefs->graph_height_network);

    /* Enable button press events for context menu */
    gtk_widget_add_events(state->network_drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->network_drawing_area, "draw", G_CALLBACK(on_draw_network), state);
    g_signal_connect(state->network_drawing_area, "button-press-event", G_CALLBACK(on_network_button_press), state);
    gtk_box_pack_start(GTK_BOX(network_box), state->network_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), network_box, TRUE, TRUE, 0);

    /* Create Disk module container */
    GtkWidget *disk_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create Disk activity bar */
    state->disk_activity_bar = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->disk_activity_bar, state->prefs->graph_width, 20);
    g_signal_connect(state->disk_activity_bar, "draw", G_CALLBACK(on_draw_disk_activity), state);
    gtk_box_pack_start(GTK_BOX(disk_box), state->disk_activity_bar, FALSE, FALSE, 0);

    /* Create Disk graph */
    state->disk_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->disk_drawing_area,
                                state->prefs->graph_width,
                                state->prefs->graph_height_disk);

    /* Enable button press events for context menu */
    gtk_widget_add_events(state->disk_drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->disk_drawing_area, "draw", G_CALLBACK(on_draw_disk), state);
    g_signal_connect(state->disk_drawing_area, "button-press-event", G_CALLBACK(on_disk_button_press), state);
    gtk_box_pack_start(GTK_BOX(disk_box), state->disk_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), disk_box, TRUE, TRUE, 0);

    /* Create AI Token module container */
    GtkWidget *aitoken_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create AI Token activity bar */
    state->aitoken_activity_bar = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->aitoken_activity_bar, state->prefs->graph_width, 20);
    g_signal_connect(state->aitoken_activity_bar, "draw", G_CALLBACK(on_draw_aitoken_activity), state);
    gtk_box_pack_start(GTK_BOX(aitoken_box), state->aitoken_activity_bar, FALSE, FALSE, 0);

    /* Create AI Token graph */
    state->aitoken_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(state->aitoken_drawing_area,
                                state->prefs->graph_width,
                                state->prefs->graph_height_aitoken);

    /* Enable button press events for context menu */
    gtk_widget_add_events(state->aitoken_drawing_area, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->aitoken_drawing_area, "draw", G_CALLBACK(on_draw_aitoken), state);
    g_signal_connect(state->aitoken_drawing_area, "button-press-event", G_CALLBACK(on_aitoken_button_press), state);
    gtk_box_pack_start(GTK_BOX(aitoken_box), state->aitoken_drawing_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(state->vbox), aitoken_box, TRUE, TRUE, 0);

    /* Connect keyboard events */
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_key_press), state);

    /* Connect destroy signal */
    g_signal_connect(state->window, "destroy", G_CALLBACK(on_window_destroy), state);

    /* Create preferences window */
    state->prefs_window = xrg_preferences_window_new(GTK_WINDOW(state->window), state->prefs);

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

    /* Draw write rate (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->disk_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha);

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

    /* Draw output tokens (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

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
    
    return FALSE;
}

/**
 * Draw CPU activity bar
 */
static gboolean on_draw_cpu_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
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

    /* Draw text */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);

    /* CPU usage and load */
    gdouble current_usage = xrg_cpu_collector_get_total_usage(state->cpu_collector);
    gdouble load_avg = xrg_cpu_collector_get_load_average_1min(state->cpu_collector);
    gint num_cores = xrg_cpu_collector_get_num_cpus(state->cpu_collector);

    gchar *label = g_strdup_printf("CPU: %.1f%% | Load: %.2f | %d cores", current_usage, load_avg, num_cores);
    cairo_move_to(cr, 5, 14);
    cairo_show_text(cr, label);
    g_free(label);

    return FALSE;
}

/**
 * Draw Memory activity bar
 */
static gboolean on_draw_memory_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
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

    /* Draw text */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);

    /* Memory usage */
    guint64 total_mem = xrg_memory_collector_get_total_memory(state->memory_collector);
    guint64 used_mem = xrg_memory_collector_get_used_memory(state->memory_collector);
    gdouble total_gb = total_mem / (1024.0 * 1024.0 * 1024.0);
    gdouble used_gb = used_mem / (1024.0 * 1024.0 * 1024.0);
    gdouble usage_pct = xrg_memory_collector_get_used_percentage(state->memory_collector);

    gchar *label = g_strdup_printf("Mem: %.1f/%.1f GB | %.1f%%", used_gb, total_gb, usage_pct);
    cairo_move_to(cr, 5, 14);
    cairo_show_text(cr, label);
    g_free(label);

    return FALSE;
}

/**
 * Draw Network activity bar
 */
static gboolean on_draw_network_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
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

    /* Draw text */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);

    /* Network rates */
    const gchar *iface = xrg_network_collector_get_primary_interface(state->network_collector);
    gdouble download_rate = xrg_network_collector_get_download_rate(state->network_collector);
    gdouble upload_rate = xrg_network_collector_get_upload_rate(state->network_collector);

    gchar *label = g_strdup_printf("%s | ↓ %.2f MB/s | ↑ %.2f MB/s", iface, download_rate, upload_rate);
    cairo_move_to(cr, 5, 14);
    cairo_show_text(cr, label);
    g_free(label);

    return FALSE;
}

/**
 * Draw Disk activity bar
 */
static gboolean on_draw_disk_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
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

    /* Draw text */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);

    /* Disk rates */
    const gchar *device = xrg_disk_collector_get_primary_device(state->disk_collector);
    gdouble read_rate = xrg_disk_collector_get_read_rate(state->disk_collector);
    gdouble write_rate = xrg_disk_collector_get_write_rate(state->disk_collector);

    gchar *label = g_strdup_printf("%s | Read: %.2f MB/s | Write: %.2f MB/s", device, read_rate, write_rate);
    cairo_move_to(cr, 5, 14);
    cairo_show_text(cr, label);
    g_free(label);

    return FALSE;
}

/**
 * Draw AI Token activity bar
 */
static gboolean on_draw_aitoken_activity(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
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

    /* Draw text */
    GdkRGBA *text_color = &state->prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);

    /* AI Token stats */
    const gchar *source = xrg_aitoken_collector_get_source_name(state->aitoken_collector);
    guint64 total_tokens = xrg_aitoken_collector_get_total_tokens(state->aitoken_collector);
    guint64 session_tokens = xrg_aitoken_collector_get_session_tokens(state->aitoken_collector);

    gchar *label = g_strdup_printf("%s: %lu | Session: %lu", source, total_tokens, session_tokens);
    cairo_move_to(cr, 5, 14);
    cairo_show_text(cr, label);
    g_free(label);

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

    /* Draw system CPU usage on top (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

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

    /* Draw wired memory on top (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

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

    /* Draw cached memory on top (amber - FG3) */
    GdkRGBA *fg3_color = &state->prefs->graph_fg3_color;
    cairo_set_source_rgba(cr, fg3_color->red, fg3_color->green, fg3_color->blue, fg3_color->alpha * 0.5);

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

    /* Draw upload rate (purple - FG2) */
    GdkRGBA *fg2_color = &state->prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

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
    xrg_aitoken_collector_update(state->aitoken_collector);

    /* Redraw activity bars */
    gtk_widget_queue_draw(state->cpu_activity_bar);
    gtk_widget_queue_draw(state->memory_activity_bar);
    gtk_widget_queue_draw(state->network_activity_bar);
    gtk_widget_queue_draw(state->disk_activity_bar);
    gtk_widget_queue_draw(state->aitoken_activity_bar);

    /* Redraw graphs */
    gtk_widget_queue_draw(state->cpu_drawing_area);
    gtk_widget_queue_draw(state->memory_drawing_area);
    gtk_widget_queue_draw(state->network_drawing_area);
    gtk_widget_queue_draw(state->disk_drawing_area);
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

    /* Save preferences */
    xrg_preferences_save(state->prefs);

    /* Cleanup */
    xrg_cpu_collector_free(state->cpu_collector);
    xrg_memory_collector_free(state->memory_collector);
    xrg_network_collector_free(state->network_collector);
    xrg_disk_collector_free(state->disk_collector);
    xrg_aitoken_collector_free(state->aitoken_collector);
    xrg_preferences_window_free(state->prefs_window);
    xrg_preferences_free(state->prefs);
    g_free(state);

    g_message("XRG-Linux stopped");
}
