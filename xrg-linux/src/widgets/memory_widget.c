/**
 * XRG Memory Widget Implementation
 *
 * Widget for displaying memory usage graphs with stacked areas
 * for used, wired, and cached memory.
 */

#include "memory_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

struct _XRGMemoryWidget {
    XRGBaseWidget base;
    XRGMemoryCollector *collector;
};

/* Forward declarations */
static void memory_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* memory_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void memory_widget_update(XRGBaseWidget *base);

/**
 * Create a new memory widget
 */
XRGMemoryWidget* xrg_memory_widget_new(XRGPreferences *prefs, XRGMemoryCollector *collector) {
    XRGMemoryWidget *widget = g_new0(XRGMemoryWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("Memory", prefs);
    if (!base) {
        g_free(widget);
        return NULL;
    }

    /* Copy base widget data */
    widget->base = *base;
    g_free(base);

    /* Set widget-specific data */
    widget->collector = collector;
    widget->base.min_height = 60;
    widget->base.preferred_height = 80;
    widget->base.show_activity_bar = TRUE;
    widget->base.user_data = widget;

    /* Set callbacks */
    xrg_base_widget_set_draw_func(&widget->base, memory_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, memory_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, memory_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free memory widget
 */
void xrg_memory_widget_free(XRGMemoryWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_memory_widget_get_container(XRGMemoryWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_memory_widget_update(XRGMemoryWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_memory_widget_set_visible(XRGMemoryWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_memory_widget_get_visible(XRGMemoryWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_memory_widget_queue_draw(XRGMemoryWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the memory graph
 */
static void memory_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGMemoryWidget *widget = (XRGMemoryWidget *)base;
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

    /* Get memory datasets */
    XRGDataset *used_dataset = xrg_memory_collector_get_used_dataset(widget->collector);
    XRGDataset *wired_dataset = xrg_memory_collector_get_wired_dataset(widget->collector);
    XRGDataset *cached_dataset = xrg_memory_collector_get_cached_dataset(widget->collector);

    gint count = xrg_dataset_get_count(used_dataset);
    if (count < 2) {
        return;
    }

    XRGGraphStyle style = prefs->memory_graph_style;

    /* Draw used memory (cyan - FG1) - bottom layer */
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(used_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw wired memory on top (purple - FG2) */
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble total_val = used_val + wired_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (used_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble total_val = used_val + wired_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - (used_val / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
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
    GdkRGBA *fg3_color = &prefs->graph_fg3_color;
    cairo_set_source_rgba(cr, fg3_color->red, fg3_color->green, fg3_color->blue, fg3_color->alpha * 0.5);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble cached_val = xrg_dataset_get_value(cached_dataset, i);
            gdouble total_val = used_val + wired_val + cached_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - ((used_val + wired_val) / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble used_val = xrg_dataset_get_value(used_dataset, i);
            gdouble wired_val = xrg_dataset_get_value(wired_dataset, i);
            gdouble cached_val = xrg_dataset_get_value(cached_dataset, i);
            gdouble total_val = used_val + wired_val + cached_val;
            gdouble x = (gdouble)i / count * width;
            gdouble y_bottom = height - ((used_val + wired_val) / 100.0 * height);
            gdouble y_top = height - (total_val / 100.0 * height);
            for (gdouble y = y_bottom; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
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
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    guint64 total_memory = xrg_memory_collector_get_total_memory(widget->collector);
    guint64 used_memory = xrg_memory_collector_get_used_memory(widget->collector);
    guint64 swap_used = xrg_memory_collector_get_swap_used(widget->collector);

    gdouble cached_percent = xrg_dataset_get_latest(cached_dataset);
    guint64 cached_memory = (guint64)(total_memory * cached_percent / 100.0);

    gdouble total_mb = total_memory / (1024.0 * 1024.0);
    gdouble used_mb = used_memory / (1024.0 * 1024.0);
    gdouble cached_mb = cached_memory / (1024.0 * 1024.0);
    gdouble swap_mb = swap_used / (1024.0 * 1024.0);

    gchar *line1 = g_strdup_printf("Memory: %.0f/%.0f MB", used_mb, total_mb);
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    gchar *line2 = g_strdup_printf("Cached: %.0f MB", cached_mb);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    gchar *line3 = g_strdup_printf("Swap: %.0f MB", swap_mb);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Draw activity bar on the right (if enabled) */
    if (prefs->show_activity_bars) {
        gint bar_x = width - 20;
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

        /* Calculate total memory usage */
        gdouble used_val = xrg_dataset_get_latest(used_dataset);
        gdouble wired_val = xrg_dataset_get_latest(wired_dataset);
        gdouble cached_val = xrg_dataset_get_latest(cached_dataset);
        gdouble current_value = (used_val + wired_val + cached_val) / 100.0;
        gdouble fill_height = current_value * height;
        gdouble bar_y = height - fill_height;

        XRGGraphStyle bar_style = prefs->activity_bar_style;
        GdkRGBA gradient_color;

        if (bar_style == XRG_GRAPH_STYLE_SOLID) {
            for (gdouble y = height; y >= bar_y; y -= 1.0) {
                gdouble position = (height - y) / height;
                xrg_get_gradient_color(position, prefs, &gradient_color);
                cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
                cairo_rectangle(cr, bar_x, y, bar_width, 1.0);
                cairo_fill(cr);
            }
        } else if (bar_style == XRG_GRAPH_STYLE_PIXEL) {
            for (gdouble y = height; y >= bar_y; y -= 4) {
                gdouble position = (height - y) / height;
                xrg_get_gradient_color(position, prefs, &gradient_color);
                cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 4) {
                    cairo_arc(cr, x + 2, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_DOT) {
            for (gdouble y = height; y >= bar_y; y -= 2) {
                gdouble position = (height - y) / height;
                xrg_get_gradient_color(position, prefs, &gradient_color);
                cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
                for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                    cairo_arc(cr, x + 1, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (bar_style == XRG_GRAPH_STYLE_HOLLOW) {
            gdouble position = (height - bar_y) / height;
            xrg_get_gradient_color(position, prefs, &gradient_color);
            cairo_set_source_rgba(cr, gradient_color.red, gradient_color.green, gradient_color.blue, gradient_color.alpha);
            for (gdouble x = bar_x; x < bar_x + bar_width; x += 2) {
                cairo_arc(cr, x, bar_y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }
}

/**
 * Generate tooltip text for memory widget
 */
static gchar* memory_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    XRGMemoryWidget *widget = (XRGMemoryWidget *)base;

    guint64 total_memory = xrg_memory_collector_get_total_memory(widget->collector);
    guint64 used_memory = xrg_memory_collector_get_used_memory(widget->collector);
    guint64 free_memory = xrg_memory_collector_get_free_memory(widget->collector);
    guint64 swap_used = xrg_memory_collector_get_swap_used(widget->collector);
    gdouble used_percent = xrg_memory_collector_get_used_percentage(widget->collector);

    XRGDataset *used_dataset = xrg_memory_collector_get_used_dataset(widget->collector);
    XRGDataset *cached_dataset = xrg_memory_collector_get_cached_dataset(widget->collector);
    XRGDataset *wired_dataset = xrg_memory_collector_get_wired_dataset(widget->collector);

    gdouble cached_percent = xrg_dataset_get_latest(cached_dataset);
    gdouble wired_percent = xrg_dataset_get_latest(wired_dataset);
    guint64 cached_bytes = (guint64)(total_memory * cached_percent / 100.0);
    guint64 wired_bytes = (guint64)(total_memory * wired_percent / 100.0);

    /* Get widget width for position mapping */
    gint width = gtk_widget_get_allocated_width(base->drawing_area);

    GString *tooltip = g_string_new("Memory Usage\n─────────────\n");

    /* Check for position-aware historical value */
    gdouble hist_used = 0, hist_cached = 0, hist_wired = 0;
    gint time_offset = xrg_get_time_offset_at_position(x, width, used_dataset);

    gboolean has_position_data =
        xrg_get_value_at_position(x, width, used_dataset, &hist_used) &&
        xrg_get_value_at_position(x, width, cached_dataset, &hist_cached) &&
        xrg_get_value_at_position(x, width, wired_dataset, &hist_wired);

    if (has_position_data && time_offset >= 0) {
        gchar *time_str = xrg_format_time_offset(time_offset);
        guint64 hist_used_bytes = (guint64)(total_memory * hist_used / 100.0);
        guint64 hist_cached_bytes = (guint64)(total_memory * hist_cached / 100.0);
        gchar *hist_used_str = xrg_format_bytes(hist_used_bytes);
        gchar *hist_cached_str = xrg_format_bytes(hist_cached_bytes);

        g_string_append_printf(tooltip,
            "📍 At %s:\n"
            "   Used: %.1f%% (%s)\n"
            "   Cached: %.1f%% (%s)\n"
            "\n",
            time_str, hist_used, hist_used_str, hist_cached, hist_cached_str);

        g_free(time_str);
        g_free(hist_used_str);
        g_free(hist_cached_str);
    }

    /* Current values */
    gchar *total_str = xrg_format_bytes(total_memory);
    gchar *used_str = xrg_format_bytes(used_memory);
    gchar *free_str = xrg_format_bytes(free_memory);
    gchar *cached_str = xrg_format_bytes(cached_bytes);
    gchar *wired_str = xrg_format_bytes(wired_bytes);
    gchar *swap_str = xrg_format_bytes(swap_used);

    g_string_append_printf(tooltip,
        "Current: %.1f%%\n"
        "─────────────\n"
        "Total:  %s\n"
        "Used:   %s\n"
        "Cached: %s\n"
        "Wired:  %s\n"
        "Free:   %s\n"
        "Swap:   %s",
        used_percent, total_str, used_str, cached_str, wired_str, free_str, swap_str);

    g_free(total_str);
    g_free(used_str);
    g_free(free_str);
    g_free(cached_str);
    g_free(wired_str);
    g_free(swap_str);

    return g_string_free(tooltip, FALSE);
}

/**
 * Update callback (called by timer)
 */
static void memory_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
