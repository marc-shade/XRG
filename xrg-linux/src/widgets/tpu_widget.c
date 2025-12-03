/**
 * XRG TPU Widget Implementation
 *
 * Widget for displaying Google Coral Edge TPU status and inference stats.
 * Uses coral/orange color theme to match the Coral branding.
 */

#include "tpu_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

/* Coral brand colors */
#define CORAL_ORANGE_R 1.0
#define CORAL_ORANGE_G 0.45
#define CORAL_ORANGE_B 0.2

#define CORAL_TEAL_R 0.0
#define CORAL_TEAL_G 0.7
#define CORAL_TEAL_B 0.7

/* Category colors for 4-color display */
#define DIRECT_R 1.0    /* Coral orange for direct MCP calls */
#define DIRECT_G 0.45
#define DIRECT_B 0.2

#define HOOKED_R 0.7    /* Purple for Claude Code hooks */
#define HOOKED_G 0.3
#define HOOKED_B 0.9

#define LOGGED_R 0.3    /* Blue for logged/other calls */
#define LOGGED_G 0.6
#define LOGGED_B 0.9

#define WARMING_R 1.0   /* Gold for warming service */
#define WARMING_G 0.8
#define WARMING_B 0.2

struct _XRGTPUWidget {
    XRGBaseWidget base;
    XRGTPUCollector *collector;
};

/* Forward declarations */
static void tpu_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* tpu_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void tpu_widget_update(XRGBaseWidget *base);

/**
 * Create a new TPU widget
 */
XRGTPUWidget* xrg_tpu_widget_new(XRGPreferences *prefs, XRGTPUCollector *collector) {
    XRGTPUWidget *widget = g_new0(XRGTPUWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("TPU", prefs);
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
    widget->base.preferred_height = 70;
    widget->base.show_activity_bar = TRUE;
    widget->base.user_data = widget;

    /* Set callbacks */
    xrg_base_widget_set_draw_func(&widget->base, tpu_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, tpu_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, tpu_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free TPU widget
 */
void xrg_tpu_widget_free(XRGTPUWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_tpu_widget_get_container(XRGTPUWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_tpu_widget_update(XRGTPUWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_tpu_widget_set_visible(XRGTPUWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_tpu_widget_get_visible(XRGTPUWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_tpu_widget_queue_draw(XRGTPUWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Get status color based on TPU status
 */
static void get_status_color(XRGTPUStatus status, gdouble *r, gdouble *g, gdouble *b) {
    switch (status) {
        case XRG_TPU_STATUS_CONNECTED:
            /* Green - connected and idle */
            *r = 0.2; *g = 0.8; *b = 0.3;
            break;
        case XRG_TPU_STATUS_BUSY:
            /* Coral orange - actively inferencing */
            *r = CORAL_ORANGE_R; *g = CORAL_ORANGE_G; *b = CORAL_ORANGE_B;
            break;
        case XRG_TPU_STATUS_ERROR:
            /* Red - error state */
            *r = 0.9; *g = 0.2; *b = 0.2;
            break;
        case XRG_TPU_STATUS_DISCONNECTED:
        default:
            /* Gray - disconnected */
            *r = 0.5; *g = 0.5; *b = 0.5;
            break;
    }
}

/**
 * Get status text
 */
static const gchar* get_status_text(XRGTPUStatus status) {
    switch (status) {
        case XRG_TPU_STATUS_CONNECTED:  return "Connected";
        case XRG_TPU_STATUS_BUSY:       return "Inferencing";
        case XRG_TPU_STATUS_ERROR:      return "Error";
        case XRG_TPU_STATUS_DISCONNECTED:
        default:                        return "Disconnected";
    }
}

/**
 * Draw the TPU graph
 */
static void tpu_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGTPUWidget *widget = (XRGTPUWidget *)base;
    XRGPreferences *prefs = base->prefs;
    XRGTPUCollector *collector = widget->collector;

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

    /* Get TPU data */
    XRGTPUStatus status = xrg_tpu_collector_get_status(collector);
    const gchar *name = xrg_tpu_collector_get_name(collector);
    gdouble inf_per_sec = xrg_tpu_collector_get_inferences_per_second(collector);
    gdouble latency = xrg_tpu_collector_get_last_latency_ms(collector);
    guint64 total_inf = xrg_tpu_collector_get_total_inferences(collector);

    /* Get datasets */
    XRGDataset *rate_dataset = xrg_tpu_collector_get_inference_rate_dataset(collector);
    XRGDataset *latency_dataset = xrg_tpu_collector_get_latency_dataset(collector);

    gint count = xrg_dataset_get_count(rate_dataset);

    /* Get category-specific datasets for 4-color display */
    XRGDataset *direct_dataset = xrg_tpu_collector_get_direct_dataset(collector);
    XRGDataset *hooked_dataset = xrg_tpu_collector_get_hooked_dataset(collector);
    XRGDataset *logged_dataset = xrg_tpu_collector_get_logged_dataset(collector);
    XRGDataset *warming_dataset = xrg_tpu_collector_get_warming_dataset(collector);

    /* Draw 4-color stacked inference rate graph */
    if (count >= 2) {
        /* Find max value for scaling across all categories */
        gdouble max_rate = 1.0;
        for (gint i = 0; i < count; i++) {
            gdouble total = xrg_dataset_get_value(direct_dataset, i) +
                           xrg_dataset_get_value(hooked_dataset, i) +
                           xrg_dataset_get_value(logged_dataset, i) +
                           xrg_dataset_get_value(warming_dataset, i);
            if (total > max_rate) max_rate = total;
        }
        max_rate = max_rate * 1.2;  /* Add 20% headroom */

        XRGGraphStyle style = prefs->gpu_graph_style;  /* Reuse GPU style setting */

        /* Draw stacked areas (bottom to top: direct, hooked, logged, warming) */
        if (style == XRG_GRAPH_STYLE_SOLID) {
            /* Layer 1: Direct (coral orange) at bottom */
            cairo_set_source_rgba(cr, DIRECT_R, DIRECT_G, DIRECT_B, 0.8);
            cairo_move_to(cr, 0, height);
            for (gint i = 0; i < count; i++) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y = height - (direct / max_rate * height);
                cairo_line_to(cr, x, y);
            }
            cairo_line_to(cr, width, height);
            cairo_close_path(cr);
            cairo_fill(cr);

            /* Layer 2: Hooked (purple) stacked on direct */
            cairo_set_source_rgba(cr, HOOKED_R, HOOKED_G, HOOKED_B, 0.8);
            cairo_move_to(cr, 0, height);
            gdouble prev_y = height;
            for (gint i = 0; i < count; i++) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble base_y = height - (direct / max_rate * height);
                gdouble y = base_y - (hooked / max_rate * height);
                if (i == 0) {
                    cairo_move_to(cr, x, base_y);
                } else {
                    cairo_line_to(cr, x, prev_y);  /* baseline */
                }
                prev_y = base_y;
            }
            /* Draw top line backwards */
            for (gint i = count - 1; i >= 0; i--) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y = height - ((direct + hooked) / max_rate * height);
                cairo_line_to(cr, x, y);
            }
            cairo_close_path(cr);
            cairo_fill(cr);

            /* Layer 3: Logged (blue) stacked on hooked */
            cairo_set_source_rgba(cr, LOGGED_R, LOGGED_G, LOGGED_B, 0.8);
            for (gint i = 0; i < count; i++) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                gdouble logged = xrg_dataset_get_value(logged_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble base_y = height - ((direct + hooked) / max_rate * height);
                gdouble top_y = height - ((direct + hooked + logged) / max_rate * height);
                if (i == 0) cairo_move_to(cr, x, base_y);
                else cairo_line_to(cr, x, base_y);
            }
            for (gint i = count - 1; i >= 0; i--) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                gdouble logged = xrg_dataset_get_value(logged_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y = height - ((direct + hooked + logged) / max_rate * height);
                cairo_line_to(cr, x, y);
            }
            cairo_close_path(cr);
            cairo_fill(cr);

            /* Layer 4: Warming (gold) at top */
            cairo_set_source_rgba(cr, WARMING_R, WARMING_G, WARMING_B, 0.8);
            for (gint i = 0; i < count; i++) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                gdouble logged = xrg_dataset_get_value(logged_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble base_y = height - ((direct + hooked + logged) / max_rate * height);
                if (i == 0) cairo_move_to(cr, x, base_y);
                else cairo_line_to(cr, x, base_y);
            }
            for (gint i = count - 1; i >= 0; i--) {
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                gdouble logged = xrg_dataset_get_value(logged_dataset, i);
                gdouble warming = xrg_dataset_get_value(warming_dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y = height - ((direct + hooked + logged + warming) / max_rate * height);
                cairo_line_to(cr, x, y);
            }
            cairo_close_path(cr);
            cairo_fill(cr);
        } else {
            /* Dots mode - draw each category with different colors */
            for (gint i = 0; i < count; i++) {
                gdouble x = (gdouble)i / count * width;

                /* Direct dots (coral orange) */
                gdouble direct = xrg_dataset_get_value(direct_dataset, i);
                if (direct > 0) {
                    cairo_set_source_rgba(cr, DIRECT_R, DIRECT_G, DIRECT_B, 0.8);
                    gdouble y = height - (direct / max_rate * height);
                    cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }

                /* Hooked dots (purple) */
                gdouble hooked = xrg_dataset_get_value(hooked_dataset, i);
                if (hooked > 0) {
                    cairo_set_source_rgba(cr, HOOKED_R, HOOKED_G, HOOKED_B, 0.8);
                    gdouble y = height - ((direct + hooked) / max_rate * height);
                    cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }

                /* Logged dots (blue) */
                gdouble logged = xrg_dataset_get_value(logged_dataset, i);
                if (logged > 0) {
                    cairo_set_source_rgba(cr, LOGGED_R, LOGGED_G, LOGGED_B, 0.8);
                    gdouble y = height - ((direct + hooked + logged) / max_rate * height);
                    cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }

                /* Warming dots (gold) */
                gdouble warming = xrg_dataset_get_value(warming_dataset, i);
                if (warming > 0) {
                    cairo_set_source_rgba(cr, WARMING_R, WARMING_G, WARMING_B, 0.8);
                    gdouble y = height - ((direct + hooked + logged + warming) / max_rate * height);
                    cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        }

        /* Draw latency as secondary line (teal) */
        gdouble max_latency = 100.0;  /* 100ms max scale */
        cairo_set_source_rgba(cr, CORAL_TEAL_R, CORAL_TEAL_G, CORAL_TEAL_B, 0.6);

        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(latency_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_latency * height);
            if (y < 0) y = 0;
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw status indicator (top-left circle) */
    gdouble sr, sg, sb;
    get_status_color(status, &sr, &sg, &sb);
    cairo_set_source_rgba(cr, sr, sg, sb, 1.0);
    cairo_arc(cr, 8, 8, 4, 0, 2 * G_PI);
    cairo_fill(cr);

    /* Overlay text labels */
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    /* Line 1: Device name with status */
    gchar *name_text = g_strdup_printf("%s", name);
    cairo_move_to(cr, 16, 12);
    cairo_show_text(cr, name_text);
    g_free(name_text);

    /* Line 2: Inference rate */
    gchar *rate_text;
    if (inf_per_sec > 0) {
        rate_text = g_strdup_printf("Rate: %.1f inf/s", inf_per_sec);
    } else {
        rate_text = g_strdup_printf("Status: %s", get_status_text(status));
    }
    cairo_move_to(cr, 5, 26);
    cairo_show_text(cr, rate_text);
    g_free(rate_text);

    /* Line 3: Latency */
    gchar *latency_text;
    if (latency > 0) {
        latency_text = g_strdup_printf("Latency: %.1f ms", latency);
    } else {
        latency_text = g_strdup_printf("Latency: --");
    }
    cairo_move_to(cr, 5, 38);
    cairo_show_text(cr, latency_text);
    g_free(latency_text);

    /* Line 4: Total inferences */
    gchar *total_text;
    if (total_inf > 1000000) {
        total_text = g_strdup_printf("Total: %.2fM inf", total_inf / 1000000.0);
    } else if (total_inf > 1000) {
        total_text = g_strdup_printf("Total: %.1fK inf", total_inf / 1000.0);
    } else {
        total_text = g_strdup_printf("Total: %lu inf", (unsigned long)total_inf);
    }
    cairo_move_to(cr, 5, 50);
    cairo_show_text(cr, total_text);
    g_free(total_text);

    /* Draw activity bar on the right (if enabled) */
    if (prefs->show_activity_bars && status != XRG_TPU_STATUS_DISCONNECTED) {
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

        /* Normalize inference rate (0-100 scale, where 100 = ~50 inf/s) */
        gdouble normalized_rate = fmin(inf_per_sec / 50.0, 1.0);
        gdouble fill_height = normalized_rate * height;
        gdouble bar_y = height - fill_height;

        /* Draw filled bar with coral gradient */
        for (gdouble y = height; y >= bar_y; y -= 1.0) {
            gdouble position = (height - y) / height;
            gdouble intensity = 0.5 + 0.5 * position;
            cairo_set_source_rgba(cr,
                CORAL_ORANGE_R * intensity,
                CORAL_ORANGE_G * intensity,
                CORAL_ORANGE_B * intensity,
                0.9);
            cairo_rectangle(cr, bar_x, y, bar_width, 1.0);
            cairo_fill(cr);
        }
    }
}

/**
 * Generate tooltip text for TPU widget
 */
static gchar* tpu_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    XRGTPUWidget *widget = (XRGTPUWidget *)base;
    XRGTPUCollector *collector = widget->collector;

    const gchar *name = xrg_tpu_collector_get_name(collector);
    XRGTPUStatus status = xrg_tpu_collector_get_status(collector);
    XRGTPUType type = xrg_tpu_collector_get_type(collector);
    const gchar *device_path = xrg_tpu_collector_get_device_path(collector);
    gdouble inf_per_sec = xrg_tpu_collector_get_inferences_per_second(collector);
    gdouble avg_latency = xrg_tpu_collector_get_avg_latency_ms(collector);
    gdouble last_latency = xrg_tpu_collector_get_last_latency_ms(collector);
    guint64 total_inf = xrg_tpu_collector_get_total_inferences(collector);

    (void)x; (void)y;  /* Unused for now */

    GString *tooltip = g_string_new("");
    g_string_append_printf(tooltip, "%s\n─────────────\n", name);

    /* Status */
    g_string_append_printf(tooltip, "Status: %s\n", get_status_text(status));

    /* Type */
    const gchar *type_str = "Unknown";
    switch (type) {
        case XRG_TPU_TYPE_USB:      type_str = "USB Accelerator"; break;
        case XRG_TPU_TYPE_PCIE:     type_str = "PCIe/M.2"; break;
        case XRG_TPU_TYPE_DEVBOARD: type_str = "Dev Board"; break;
        default: break;
    }
    g_string_append_printf(tooltip, "Type: %s\n", type_str);

    /* Path */
    if (device_path) {
        g_string_append_printf(tooltip, "Path: %s\n", device_path);
    }

    g_string_append(tooltip, "\nStatistics\n");

    /* Inference rate */
    g_string_append_printf(tooltip, "   Rate: %.2f inf/s\n", inf_per_sec);

    /* Latency */
    g_string_append_printf(tooltip, "   Last latency: %.2f ms\n", last_latency);
    g_string_append_printf(tooltip, "   Avg latency: %.2f ms\n", avg_latency);

    /* Total inferences */
    g_string_append_printf(tooltip, "   Total: %lu inferences\n", (unsigned long)total_inf);

    /* Category breakdown */
    guint64 direct_inf = xrg_tpu_collector_get_direct_inferences(collector);
    guint64 hooked_inf = xrg_tpu_collector_get_hooked_inferences(collector);
    guint64 logged_inf = xrg_tpu_collector_get_logged_inferences(collector);
    guint64 warming_inf = xrg_tpu_collector_get_warming_inferences(collector);

    if (direct_inf + hooked_inf + logged_inf + warming_inf > 0) {
        g_string_append(tooltip, "\nBy Category\n");
        g_string_append_printf(tooltip, "   Direct: %lu\n", (unsigned long)direct_inf);
        g_string_append_printf(tooltip, "   Hooked: %lu\n", (unsigned long)hooked_inf);
        g_string_append_printf(tooltip, "   Logged: %lu\n", (unsigned long)logged_inf);
        g_string_append_printf(tooltip, "   Warming: %lu\n", (unsigned long)warming_inf);
    }

    /* Temperature if available */
    if (xrg_tpu_collector_has_temperature(collector)) {
        gdouble temp = xrg_tpu_collector_get_temperature(collector);
        g_string_append_printf(tooltip, "   Temp: %.1f°C\n", temp);
    }

    /* Stats file info */
    g_string_append_printf(tooltip, "\nStats file: %s", xrg_tpu_collector_get_stats_file_path());

    return g_string_free(tooltip, FALSE);
}

/**
 * Update callback (called by timer)
 */
static void tpu_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
