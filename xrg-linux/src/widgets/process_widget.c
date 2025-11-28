/**
 * XRG Process Widget Implementation
 *
 * Displays top processes in a compact list format with bar graphs.
 */

#include "process_widget.h"
#include "base_widget.h"
#include <cairo.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

/* Forward declarations */
static void process_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* process_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void process_widget_update(XRGBaseWidget *base);
static void process_widget_menu(XRGBaseWidget *base, GdkEventButton *event);

struct _XRGProcessWidget {
    XRGBaseWidget base;             /* Must be first for casting */
    XRGProcessCollector *collector;
    gint hovered_row;               /* Which row is hovered (-1 = none) */
    gint row_height;                /* Calculated row height */
};

/*============================================================================
 * Widget Creation
 *============================================================================*/

XRGProcessWidget* xrg_process_widget_new(XRGPreferences *prefs, XRGProcessCollector *collector) {
    XRGProcessWidget *widget = g_new0(XRGProcessWidget, 1);

    /* Initialize base widget */
    widget->base.name = "Processes";
    widget->base.prefs = prefs;
    widget->base.min_height = 100;
    widget->base.preferred_height = 160;
    widget->base.show_activity_bar = FALSE;  /* No activity bar for process list */
    widget->base.visible = TRUE;

    widget->collector = collector;
    widget->hovered_row = -1;
    widget->row_height = 16;

    /* Initialize container and drawing area */
    xrg_base_widget_init_container(&widget->base);

    /* Set callbacks */
    xrg_base_widget_set_draw_func(&widget->base, process_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, process_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, process_widget_update);
    xrg_base_widget_set_menu_func(&widget->base, process_widget_menu);

    return widget;
}

void xrg_process_widget_free(XRGProcessWidget *widget) {
    if (!widget) return;
    /* Note: collector is owned by main app, not freed here */
    xrg_base_widget_free(&widget->base);
}

GtkWidget* xrg_process_widget_get_container(XRGProcessWidget *widget) {
    return widget ? xrg_base_widget_get_container(&widget->base) : NULL;
}

void xrg_process_widget_update(XRGProcessWidget *widget) {
    if (widget && widget->base.update_func) {
        widget->base.update_func(&widget->base);
    }
}

void xrg_process_widget_set_visible(XRGProcessWidget *widget, gboolean visible) {
    if (widget) {
        xrg_base_widget_set_visible(&widget->base, visible);
    }
}

gboolean xrg_process_widget_get_visible(XRGProcessWidget *widget) {
    return widget ? widget->base.visible : FALSE;
}

void xrg_process_widget_queue_draw(XRGProcessWidget *widget) {
    if (widget) {
        xrg_base_widget_queue_draw(&widget->base);
    }
}

XRGProcessCollector* xrg_process_widget_get_collector(XRGProcessWidget *widget) {
    return widget ? widget->collector : NULL;
}

/*============================================================================
 * Drawing Implementation
 *============================================================================*/

static void draw_process_bar(cairo_t *cr, gdouble value, gdouble max_value,
                             int x, int y, int width, int height,
                             GdkRGBA *color) {
    gdouble ratio = CLAMP(value / max_value, 0.0, 1.0);
    gint bar_width = (gint)(ratio * width);

    if (bar_width > 0) {
        /* Draw bar background */
        cairo_set_source_rgba(cr, color->red, color->green, color->blue, 0.3);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);

        /* Draw filled portion */
        cairo_set_source_rgba(cr, color->red, color->green, color->blue, 0.8);
        cairo_rectangle(cr, x, y, bar_width, height);
        cairo_fill(cr);
    }
}

static void process_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGProcessWidget *widget = (XRGProcessWidget *)base;
    XRGPreferences *prefs = base->prefs;

    /* Draw background */
    GdkRGBA *bg_color = &prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_paint(cr);

    /* Get colors */
    GdkRGBA *text_color = &prefs->text_color;
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    GdkRGBA *fg3_color = &prefs->graph_fg3_color;
    (void)fg3_color;  /* Unused for now */

    /* Font setup */
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);

    /* Calculate layout */
    int margin = 4;
    int header_height = 14;
    int row_height = widget->row_height;
    int y_offset = margin;

    /* Draw header */
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.7);

    XRGProcessSortBy sort_by = xrg_process_collector_get_sort_by(widget->collector);

    /* Column headers */
    int col_name = margin;
    int col_cpu = width - 100;
    int col_mem = width - 50;

    cairo_move_to(cr, col_name, y_offset + 10);
    cairo_show_text(cr, "Process");

    cairo_move_to(cr, col_cpu, y_offset + 10);
    if (sort_by == XRG_PROCESS_SORT_CPU) {
        cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, 1.0);
    }
    cairo_show_text(cr, "CPU%");

    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.7);
    cairo_move_to(cr, col_mem, y_offset + 10);
    if (sort_by == XRG_PROCESS_SORT_MEMORY) {
        cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, 1.0);
    }
    cairo_show_text(cr, "Mem%");

    y_offset += header_height + 2;

    /* Draw separator line */
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.3);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, margin, y_offset);
    cairo_line_to(cr, width - margin, y_offset);
    cairo_stroke(cr);

    y_offset += 4;

    /* Draw process list */
    GList *processes = xrg_process_collector_get_processes(widget->collector);

    /* Calculate how many processes fit */
    int available_height = height - y_offset - margin;
    int max_rows = available_height / row_height;

    int row = 0;
    for (GList *l = processes; l != NULL && row < max_rows; l = l->next, row++) {
        XRGProcessInfo *proc = l->data;
        int row_y = y_offset + row * row_height;

        /* Highlight hovered row */
        if (row == widget->hovered_row) {
            cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.1);
            cairo_rectangle(cr, margin, row_y, width - 2 * margin, row_height);
            cairo_fill(cr);
        }

        /* Process name (truncate if needed) */
        gchar name_buf[32];
        if (proc->name) {
            g_snprintf(name_buf, sizeof(name_buf), "%.20s", proc->name);
        } else {
            g_snprintf(name_buf, sizeof(name_buf), "%d", proc->pid);
        }

        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.9);
        cairo_move_to(cr, col_name, row_y + row_height - 4);
        cairo_show_text(cr, name_buf);

        /* CPU bar and percentage */
        int bar_width = 35;
        int bar_x = col_cpu - 5;
        draw_process_bar(cr, proc->cpu_percent, 100.0,
                        bar_x, row_y + 2, bar_width, row_height - 4, fg1_color);

        gchar cpu_str[16];
        g_snprintf(cpu_str, sizeof(cpu_str), "%5.1f", proc->cpu_percent);
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.9);
        cairo_move_to(cr, col_cpu, row_y + row_height - 4);
        cairo_show_text(cr, cpu_str);

        /* Memory bar and percentage */
        bar_x = col_mem - 5;
        draw_process_bar(cr, proc->mem_percent, 100.0,
                        bar_x, row_y + 2, bar_width, row_height - 4, fg2_color);

        gchar mem_str[16];
        g_snprintf(mem_str, sizeof(mem_str), "%5.1f", proc->mem_percent);
        cairo_move_to(cr, col_mem, row_y + row_height - 4);
        cairo_show_text(cr, mem_str);
    }

    /* Draw summary at bottom if space permits */
    int summary_y = height - margin - 2;
    if (summary_y > y_offset + row * row_height + 10) {
        gint total = xrg_process_collector_get_total_processes(widget->collector);
        gint running = xrg_process_collector_get_running_processes(widget->collector);

        gchar summary[64];
        g_snprintf(summary, sizeof(summary), "Processes: %d (%d running)", total, running);

        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, 0.5);
        cairo_set_font_size(cr, 9);
        cairo_move_to(cr, margin, summary_y);
        cairo_show_text(cr, summary);
    }

    /* Store row info for hover detection */
    widget->row_height = row_height;

    /* Update hovered row based on mouse position */
    if (base->hovered) {
        int rel_y = base->hover_y - y_offset + header_height + 6;
        if (rel_y >= 0) {
            widget->hovered_row = rel_y / row_height;
            if (widget->hovered_row >= row) {
                widget->hovered_row = -1;
            }
        } else {
            widget->hovered_row = -1;
        }
    } else {
        widget->hovered_row = -1;
    }
}

/*============================================================================
 * Tooltip Implementation
 *============================================================================*/

static gchar* process_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    (void)x;
    XRGProcessWidget *widget = (XRGProcessWidget *)base;

    /* Calculate which row is hovered */
    int margin = 4;
    int header_height = 14;
    int y_offset = margin + header_height + 6;

    int rel_y = y - y_offset;
    if (rel_y < 0) {
        /* Hovering over header - show summary */
        gint total = xrg_process_collector_get_total_processes(widget->collector);
        gint running = xrg_process_collector_get_running_processes(widget->collector);
        gdouble uptime = xrg_process_collector_get_uptime_seconds(widget->collector);

        gint days = (gint)(uptime / 86400);
        gint hours = (gint)((uptime - days * 86400) / 3600);
        gint mins = (gint)((uptime - days * 86400 - hours * 3600) / 60);

        gchar uptime_str[64];
        if (days > 0) {
            g_snprintf(uptime_str, sizeof(uptime_str), "%dd %dh %dm", days, hours, mins);
        } else if (hours > 0) {
            g_snprintf(uptime_str, sizeof(uptime_str), "%dh %dm", hours, mins);
        } else {
            g_snprintf(uptime_str, sizeof(uptime_str), "%dm", mins);
        }

        return g_strdup_printf(
            "Process Monitor\n"
            "─────────────\n"
            "Total: %d processes\n"
            "Running: %d\n"
            "Uptime: %s\n"
            "\n"
            "Click column header to sort",
            total, running, uptime_str);
    }

    int row = rel_y / widget->row_height;

    /* Find the process at this row */
    GList *processes = xrg_process_collector_get_processes(widget->collector);
    XRGProcessInfo *proc = g_list_nth_data(processes, row);

    if (!proc) {
        return NULL;
    }

    /* Format memory */
    gchar *mem_rss_str = xrg_format_bytes(proc->mem_rss);
    gchar *mem_vsize_str = xrg_format_bytes(proc->mem_vsize);

    GString *tooltip = g_string_new("");
    g_string_append_printf(tooltip, "%s (PID %d)\n", proc->name, proc->pid);
    g_string_append(tooltip, "─────────────\n");

    if (proc->cmdline && strlen(proc->cmdline) > 0) {
        /* Truncate long command lines */
        gchar cmd_truncated[80];
        g_snprintf(cmd_truncated, sizeof(cmd_truncated), "%.75s%s",
                   proc->cmdline, strlen(proc->cmdline) > 75 ? "..." : "");
        g_string_append_printf(tooltip, "Cmd: %s\n", cmd_truncated);
    }

    g_string_append_printf(tooltip,
        "\n"
        "CPU:    %.1f%%\n"
        "Memory: %.1f%% (%s)\n"
        "Virtual: %s\n"
        "Threads: %d\n"
        "Nice:   %d\n"
        "State:  %s\n"
        "User:   %s",
        proc->cpu_percent,
        proc->mem_percent, mem_rss_str,
        mem_vsize_str,
        proc->threads,
        proc->nice,
        xrg_process_state_name(proc->state),
        proc->username);

    g_free(mem_rss_str);
    g_free(mem_vsize_str);

    return g_string_free(tooltip, FALSE);
}

/*============================================================================
 * Update Implementation
 *============================================================================*/

static void process_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}

/*============================================================================
 * Context Menu Implementation
 *============================================================================*/

/* Data passed to menu item callbacks */
typedef struct {
    XRGProcessWidget *widget;
    pid_t pid;
    gchar *name;
} ProcessMenuData;

static void free_menu_data(gpointer data, GClosure *closure) {
    (void)closure;
    ProcessMenuData *menu_data = data;
    if (menu_data) {
        g_free(menu_data->name);
        g_free(menu_data);
    }
}

static void on_kill_process(GtkMenuItem *item, gpointer data) {
    (void)item;
    ProcessMenuData *menu_data = data;

    if (kill(menu_data->pid, SIGTERM) == 0) {
        g_message("Sent SIGTERM to process %d (%s)", menu_data->pid, menu_data->name);
    } else {
        g_warning("Failed to kill process %d (%s): %s",
                  menu_data->pid, menu_data->name, g_strerror(errno));

        /* Show error dialog */
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Failed to terminate process %d (%s):\n%s\n\nYou may need root privileges.",
            menu_data->pid, menu_data->name, g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

static void on_force_kill_process(GtkMenuItem *item, gpointer data) {
    (void)item;
    ProcessMenuData *menu_data = data;

    if (kill(menu_data->pid, SIGKILL) == 0) {
        g_message("Sent SIGKILL to process %d (%s)", menu_data->pid, menu_data->name);
    } else {
        g_warning("Failed to force kill process %d (%s): %s",
                  menu_data->pid, menu_data->name, g_strerror(errno));

        /* Show error dialog */
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Failed to force kill process %d (%s):\n%s\n\nYou may need root privileges.",
            menu_data->pid, menu_data->name, g_strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

static void on_send_sigstop(GtkMenuItem *item, gpointer data) {
    (void)item;
    ProcessMenuData *menu_data = data;

    if (kill(menu_data->pid, SIGSTOP) == 0) {
        g_message("Sent SIGSTOP to process %d (%s)", menu_data->pid, menu_data->name);
    } else {
        g_warning("Failed to stop process %d (%s): %s",
                  menu_data->pid, menu_data->name, g_strerror(errno));
    }
}

static void on_send_sigcont(GtkMenuItem *item, gpointer data) {
    (void)item;
    ProcessMenuData *menu_data = data;

    if (kill(menu_data->pid, SIGCONT) == 0) {
        g_message("Sent SIGCONT to process %d (%s)", menu_data->pid, menu_data->name);
    } else {
        g_warning("Failed to continue process %d (%s): %s",
                  menu_data->pid, menu_data->name, g_strerror(errno));
    }
}

static gint get_row_at_position(XRGProcessWidget *widget, gint y) {
    int margin = 4;
    int header_height = 14;
    int y_offset = margin + header_height + 6;

    int rel_y = y - y_offset;
    if (rel_y < 0) {
        return -1;  /* Click in header area */
    }

    return rel_y / widget->row_height;
}

static void process_widget_menu(XRGBaseWidget *base, GdkEventButton *event) {
    XRGProcessWidget *widget = (XRGProcessWidget *)base;

    /* Calculate which row was clicked */
    gint row = get_row_at_position(widget, (gint)event->y);
    if (row < 0) {
        return;  /* Clicked on header, no menu */
    }

    /* Find the process at this row */
    GList *processes = xrg_process_collector_get_processes(widget->collector);
    XRGProcessInfo *proc = g_list_nth_data(processes, row);

    if (!proc) {
        return;  /* No process at this row */
    }

    /* Create context menu */
    GtkWidget *menu = gtk_menu_new();

    /* Process name header (non-clickable) */
    GtkWidget *header = gtk_menu_item_new_with_label(
        g_strdup_printf("%s (PID %d)", proc->name, proc->pid));
    gtk_widget_set_sensitive(header, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), header);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                          gtk_separator_menu_item_new());

    /* Create menu data for callbacks */
    ProcessMenuData *menu_data = g_new0(ProcessMenuData, 1);
    menu_data->widget = widget;
    menu_data->pid = proc->pid;
    menu_data->name = g_strdup(proc->name);

    /* Kill Process (SIGTERM) */
    GtkWidget *kill_item = gtk_menu_item_new_with_label("Terminate (SIGTERM)");
    g_signal_connect_data(kill_item, "activate",
                          G_CALLBACK(on_kill_process), menu_data,
                          NULL, 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), kill_item);

    /* Force Kill Process (SIGKILL) */
    ProcessMenuData *kill_data = g_new0(ProcessMenuData, 1);
    kill_data->widget = widget;
    kill_data->pid = proc->pid;
    kill_data->name = g_strdup(proc->name);

    GtkWidget *force_kill_item = gtk_menu_item_new_with_label("Force Kill (SIGKILL)");
    g_signal_connect_data(force_kill_item, "activate",
                          G_CALLBACK(on_force_kill_process), kill_data,
                          free_menu_data, 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), force_kill_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                          gtk_separator_menu_item_new());

    /* Stop/Continue submenu */
    ProcessMenuData *stop_data = g_new0(ProcessMenuData, 1);
    stop_data->widget = widget;
    stop_data->pid = proc->pid;
    stop_data->name = g_strdup(proc->name);

    GtkWidget *stop_item = gtk_menu_item_new_with_label("Pause (SIGSTOP)");
    g_signal_connect_data(stop_item, "activate",
                          G_CALLBACK(on_send_sigstop), stop_data,
                          free_menu_data, 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), stop_item);

    ProcessMenuData *cont_data = g_new0(ProcessMenuData, 1);
    cont_data->widget = widget;
    cont_data->pid = proc->pid;
    cont_data->name = g_strdup(proc->name);

    GtkWidget *cont_item = gtk_menu_item_new_with_label("Resume (SIGCONT)");
    g_signal_connect_data(cont_item, "activate",
                          G_CALLBACK(on_send_sigcont), cont_data,
                          free_menu_data, 0);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), cont_item);

    /* Free menu_data when menu is destroyed */
    g_signal_connect_swapped(menu, "destroy",
                             G_CALLBACK(free_menu_data), menu_data);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
}
