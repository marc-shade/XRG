/**
 * XRG CPU Widget Implementation
 *
 * Widget for displaying CPU usage graphs and activity bars.
 * Extracts drawing code from main.c into modular widget.
 */

#include "cpu_widget.h"
#include <math.h>

/* Widget private data structure */
struct _XRGCPUWidget {
    XRGBaseWidget base;           /* Embedded base widget */
    XRGCPUCollector *collector;   /* Data source */
};

/* Forward declarations */
static void cpu_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* cpu_widget_tooltip(XRGBaseWidget *base, int x, int y);

/* Activity bar gradient color helper */
static void get_activity_gradient_color(gdouble position, XRGPreferences *prefs, GdkRGBA *out_color) {
    /* Default gradient: green -> yellow -> red */
    if (position < 0.5) {
        /* Green to Yellow */
        gdouble t = position * 2.0;
        out_color->red = t;
        out_color->green = 1.0;
        out_color->blue = 0.0;
    } else {
        /* Yellow to Red */
        gdouble t = (position - 0.5) * 2.0;
        out_color->red = 1.0;
        out_color->green = 1.0 - t;
        out_color->blue = 0.0;
    }
    out_color->alpha = 1.0;
}

/*============================================================================
 * Widget Creation and Destruction
 *============================================================================*/

XRGCPUWidget* xrg_cpu_widget_new(XRGPreferences *prefs, XRGCPUCollector *collector) {
    XRGCPUWidget *widget = g_new0(XRGCPUWidget, 1);

    /* Initialize base widget */
    widget->base.name = "CPU";
    widget->base.prefs = prefs;
    widget->base.min_height = 60;
    widget->base.preferred_height = 80;
    widget->base.visible = prefs->show_cpu;
    widget->base.show_activity_bar = prefs->show_activity_bars;

    /* Store collector reference */
    widget->collector = collector;

    /* Initialize GTK container */
    xrg_base_widget_init_container(&widget->base);

    /* Set callbacks */
    xrg_base_widget_set_draw_func(&widget->base, cpu_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, cpu_widget_tooltip);

    return widget;
}

void xrg_cpu_widget_free(XRGCPUWidget *widget) {
    if (!widget) return;
    /* Note: collector is not owned by widget, don't free it */
    g_free(widget);
}

/*============================================================================
 * Widget Public API
 *============================================================================*/

GtkWidget* xrg_cpu_widget_get_container(XRGCPUWidget *widget) {
    return xrg_base_widget_get_container(&widget->base);
}

void xrg_cpu_widget_update(XRGCPUWidget *widget) {
    if (widget->base.update_func) {
        widget->base.update_func(&widget->base);
    }
    xrg_base_widget_queue_draw(&widget->base);
}

void xrg_cpu_widget_set_visible(XRGCPUWidget *widget, gboolean visible) {
    xrg_base_widget_set_visible(&widget->base, visible);
}

gboolean xrg_cpu_widget_get_visible(XRGCPUWidget *widget) {
    return widget->base.visible;
}

void xrg_cpu_widget_queue_draw(XRGCPUWidget *widget) {
    xrg_base_widget_queue_draw(&widget->base);
}

/*============================================================================
 * Widget Drawing Implementation
 *============================================================================*/

static void cpu_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGCPUWidget *widget = (XRGCPUWidget *)base;
    XRGPreferences *prefs = base->prefs;

    /* Draw background */
    GdkRGBA *bg_color = &prefs->graph_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get CPU datasets */
    XRGDataset *user_dataset = xrg_cpu_collector_get_user_dataset(widget->collector);
    XRGDataset *system_dataset = xrg_cpu_collector_get_system_dataset(widget->collector);

    gint count = xrg_dataset_get_count(user_dataset);
    if (count < 2) {
        /* Not enough data yet */
        return;
    }

    /* Draw user CPU usage (cyan - FG1) */
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    XRGGraphStyle style = prefs->cpu_graph_style;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        /* Solid filled area */
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
        /* Chunky pixels */
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        /* Fine dots */
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);

            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        /* Outline only */
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(user_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw system CPU usage on top (purple - FG2) */
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble user_val = xrg_dataset_get_value(user_dataset, i);
            gdouble system_val = xrg_dataset_get_value(system_dataset, i);
            gdouble total_val = user_val + system_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (user_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble user_val = xrg_dataset_get_value(user_dataset, i);
            gdouble system_val = xrg_dataset_get_value(system_dataset, i);
            gdouble total_val = user_val + system_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (user_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);

            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
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
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble total_usage = xrg_cpu_collector_get_total_usage(widget->collector);
    gdouble load_avg = xrg_cpu_collector_get_load_average_1min(widget->collector);

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
    if (prefs->show_activity_bars) {
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

        XRGGraphStyle bar_style = prefs->activity_bar_style;
        GdkRGBA gradient_color;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            /* Draw gradient by slicing horizontally */
            for (gdouble y = height; y >= bar_y; y -= 1.0) {
                gdouble position = (height - y) / height;
                get_activity_gradient_color(position, prefs, &gradient_color);
                cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
                cairo_rectangle(cr, bar_x, y, bar_width, 1.0);
                cairo_fill(cr);
            }
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            /* Chunky pixels with gradient colors */
            for (gdouble y = height; y >= bar_y; y -= 4) {
                gdouble position = (height - y) / height;
                get_activity_gradient_color(position, prefs, &gradient_color);
                cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            /* Fine dots with gradient colors */
            for (gdouble y = height; y >= bar_y; y -= 2) {
                gdouble position = (height - y) / height;
                get_activity_gradient_color(position, prefs, &gradient_color);
                cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            /* Outline only */
            gdouble position = (height - bar_y) / height;
            get_activity_gradient_color(position, prefs, &gradient_color);
            cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }
}

/*============================================================================
 * Widget Tooltip Implementation
 *============================================================================*/

static gchar* cpu_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    XRGCPUWidget *widget = (XRGCPUWidget *)base;

    gdouble total_usage = xrg_cpu_collector_get_total_usage(widget->collector);
    gdouble load_1 = xrg_cpu_collector_get_load_average_1min(widget->collector);
    gdouble load_5 = xrg_cpu_collector_get_load_average_5min(widget->collector);
    gdouble load_15 = xrg_cpu_collector_get_load_average_15min(widget->collector);

    XRGDataset *user_dataset = xrg_cpu_collector_get_user_dataset(widget->collector);
    XRGDataset *system_dataset = xrg_cpu_collector_get_system_dataset(widget->collector);

    gdouble user_usage = xrg_dataset_get_latest(user_dataset);
    gdouble system_usage = xrg_dataset_get_latest(system_dataset);

    gint num_cpus = xrg_cpu_collector_get_num_cpus(widget->collector);

    /* Get widget width for position mapping */
    gint width = gtk_widget_get_allocated_width(base->drawing_area);

    /* Check for position-aware historical value */
    gdouble hist_user = 0, hist_system = 0;
    gint time_offset = xrg_get_time_offset_at_position(x, width, user_dataset);

    gboolean has_position_data =
        xrg_get_value_at_position(x, width, user_dataset, &hist_user) &&
        xrg_get_value_at_position(x, width, system_dataset, &hist_system);

    GString *tooltip = g_string_new("CPU Usage\n─────────────\n");

    /* Show position-specific value if hovering over valid graph area */
    if (has_position_data && time_offset >= 0) {
        gchar *time_str = xrg_format_time_offset(time_offset);
        gdouble hist_total = hist_user + hist_system;

        g_string_append_printf(tooltip,
            "📍 At %s:\n"
            "   Total: %.1f%%\n"
            "   User:  %.1f%%\n"
            "   System: %.1f%%\n"
            "\n",
            time_str, hist_total, hist_user, hist_system);
        g_free(time_str);
    }

    /* Current values */
    g_string_append_printf(tooltip,
        "Current\n"
        "   Total: %.1f%%\n"
        "   User:  %.1f%%\n"
        "   System: %.1f%%\n"
        "\n"
        "Load Average\n"
        "─────────────\n"
        "1min:  %.2f\n"
        "5min:  %.2f\n"
        "15min: %.2f\n"
        "\n"
        "CPUs: %d",
        total_usage, user_usage, system_usage,
        load_1, load_5, load_15,
        num_cpus);

    return g_string_free(tooltip, FALSE);
}
