/**
 * XRG Network Widget Implementation
 *
 * Widget for displaying network traffic with download/upload rates.
 */

#include "network_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

struct _XRGNetworkWidget {
    XRGBaseWidget base;
    XRGNetworkCollector *collector;
};

/* Forward declarations */
static void network_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* network_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void network_widget_update(XRGBaseWidget *base);

/**
 * Create a new network widget
 */
XRGNetworkWidget* xrg_network_widget_new(XRGPreferences *prefs, XRGNetworkCollector *collector) {
    XRGNetworkWidget *widget = g_new0(XRGNetworkWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("Network", prefs);
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
    xrg_base_widget_set_draw_func(&widget->base, network_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, network_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, network_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free network widget
 */
void xrg_network_widget_free(XRGNetworkWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_network_widget_get_container(XRGNetworkWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_network_widget_update(XRGNetworkWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_network_widget_set_visible(XRGNetworkWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_network_widget_get_visible(XRGNetworkWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_network_widget_queue_draw(XRGNetworkWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the network graph
 */
static void network_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGNetworkWidget *widget = (XRGNetworkWidget *)base;
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

    /* Get network datasets */
    XRGDataset *download_dataset = xrg_network_collector_get_download_dataset(widget->collector);
    XRGDataset *upload_dataset = xrg_network_collector_get_upload_dataset(widget->collector);

    gint count = xrg_dataset_get_count(download_dataset);
    if (count < 2) {
        return;
    }

    /* Find maximum rate for scaling */
    gdouble max_rate = 0.1;  /* Minimum 0.1 MB/s */
    for (gint i = 0; i < count; i++) {
        gdouble download = xrg_dataset_get_value(download_dataset, i);
        gdouble upload = xrg_dataset_get_value(upload_dataset, i);
        if (download > max_rate) max_rate = download;
        if (upload > max_rate) max_rate = upload;
    }

    XRGGraphStyle style = prefs->network_graph_style;

    /* Draw download rate (cyan - FG1) */
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(download_dataset, i);
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
            gdouble value = xrg_dataset_get_value(download_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(download_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw upload rate (purple - FG2) */
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
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
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(upload_dataset, i);
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

    gdouble download_rate = xrg_network_collector_get_download_rate(widget->collector);
    gdouble upload_rate = xrg_network_collector_get_upload_rate(widget->collector);

    gchar *line1 = g_strdup_printf("Network");
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, line1);
    g_free(line1);

    gchar *line2 = g_strdup_printf("\342\206\223 %.2f MB/s", download_rate);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    gchar *line3 = g_strdup_printf("\342\206\221 %.2f MB/s", upload_rate);
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

        /* Scale bar to current download rate relative to max */
        gdouble current_value = download_rate / max_rate;
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
 * Generate tooltip text for network widget
 */
static gchar* network_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    (void)x; (void)y;
    XRGNetworkWidget *widget = (XRGNetworkWidget *)base;

    const gchar *iface = xrg_network_collector_get_primary_interface(widget->collector);
    gdouble download_rate = xrg_network_collector_get_download_rate(widget->collector);
    gdouble upload_rate = xrg_network_collector_get_upload_rate(widget->collector);
    guint64 total_rx = xrg_network_collector_get_total_rx(widget->collector);
    guint64 total_tx = xrg_network_collector_get_total_tx(widget->collector);

    gchar *rx_str = xrg_format_bytes(total_rx);
    gchar *tx_str = xrg_format_bytes(total_tx);

    gchar *tooltip = g_strdup_printf(
        "Interface: %s\n"
        "Download: %.2f MB/s\n"
        "Upload: %.2f MB/s\n"
        "Total Received: %s\n"
        "Total Sent: %s",
        iface ? iface : "unknown",
        download_rate,
        upload_rate,
        rx_str,
        tx_str
    );

    g_free(rx_str);
    g_free(tx_str);

    return tooltip;
}

/**
 * Update callback (called by timer)
 */
static void network_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
