/**
 * XRG Disk Widget Implementation
 *
 * Widget for displaying disk I/O with read/write rates.
 */

#include "disk_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

struct _XRGDiskWidget {
    XRGBaseWidget base;
    XRGDiskCollector *collector;
};

/* Forward declarations */
static void disk_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* disk_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void disk_widget_update(XRGBaseWidget *base);

/**
 * Create a new disk widget
 */
XRGDiskWidget* xrg_disk_widget_new(XRGPreferences *prefs, XRGDiskCollector *collector) {
    XRGDiskWidget *widget = g_new0(XRGDiskWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("Disk", prefs);
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
    xrg_base_widget_set_draw_func(&widget->base, disk_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, disk_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, disk_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free disk widget
 */
void xrg_disk_widget_free(XRGDiskWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_disk_widget_get_container(XRGDiskWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_disk_widget_update(XRGDiskWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_disk_widget_set_visible(XRGDiskWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_disk_widget_get_visible(XRGDiskWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_disk_widget_queue_draw(XRGDiskWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the disk graph
 */
static void disk_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGDiskWidget *widget = (XRGDiskWidget *)base;
    XRGPreferences *prefs = base->prefs;

    /* Draw background - disk uses its own colors */
    GdkRGBA *bg_color = &prefs->disk_bg_color;
    cairo_set_source_rgba(cr, bg_color->red, bg_color->green, bg_color->blue, bg_color->alpha);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    /* Draw border */
    GdkRGBA *border_color = &prefs->border_color;
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, 0.5, 0.5, width - 1, height - 1);
    cairo_stroke(cr);

    /* Get disk datasets */
    XRGDataset *read_dataset = xrg_disk_collector_get_read_dataset(widget->collector);
    XRGDataset *write_dataset = xrg_disk_collector_get_write_dataset(widget->collector);

    gint count = xrg_dataset_get_count(read_dataset);
    if (count < 2) {
        return;
    }

    /* Find maximum rate for auto-scaling */
    gdouble max_rate = 0.1;  /* Minimum 0.1 MB/s */
    for (gint i = 0; i < count; i++) {
        gdouble read_val = xrg_dataset_get_value(read_dataset, i);
        gdouble write_val = xrg_dataset_get_value(write_dataset, i);
        if (read_val > max_rate) max_rate = read_val;
        if (write_val > max_rate) max_rate = write_val;
    }

    XRGGraphStyle style = prefs->disk_graph_style;

    /* Draw read rate (disk FG1) */
    GdkRGBA *fg1_color = &prefs->disk_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(read_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw write rate (disk FG2) */
    GdkRGBA *fg2_color = &prefs->disk_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(write_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble read_rate = xrg_disk_collector_get_read_rate(widget->collector);
    gdouble write_rate = xrg_disk_collector_get_write_rate(widget->collector);

    gchar *line1 = g_strdup_printf("Disk I/O");
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    gchar *line2 = g_strdup_printf("Read: %.2f MB/s", read_rate);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    gchar *line3 = g_strdup_printf("Write: %.2f MB/s", write_rate);
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

        /* Scale bar to current read rate relative to max */
        gdouble current_value = read_rate / max_rate;
        if (current_value > 1.0) current_value = 1.0;
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
 * Generate tooltip text for disk widget
 */
static gchar* disk_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    (void)x; (void)y;
    XRGDiskWidget *widget = (XRGDiskWidget *)base;

    const gchar *device = xrg_disk_collector_get_primary_device(widget->collector);
    gdouble read_rate = xrg_disk_collector_get_read_rate(widget->collector);
    gdouble write_rate = xrg_disk_collector_get_write_rate(widget->collector);
    guint64 total_read = xrg_disk_collector_get_total_read(widget->collector);
    guint64 total_written = xrg_disk_collector_get_total_written(widget->collector);

    gchar *read_str = xrg_format_bytes(total_read);
    gchar *write_str = xrg_format_bytes(total_written);

    gchar *tooltip = g_strdup_printf(
        "Device: %s\n"
        "Read Rate: %.2f MB/s\n"
        "Write Rate: %.2f MB/s\n"
        "Total Read: %s\n"
        "Total Written: %s",
        device ? device : "unknown",
        read_rate,
        write_rate,
        read_str,
        write_str
    );

    g_free(read_str);
    g_free(write_str);

    return tooltip;
}

/**
 * Update callback (called by timer)
 */
static void disk_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
