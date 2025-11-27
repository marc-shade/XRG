/**
 * XRG GPU Widget Implementation
 *
 * Widget for displaying GPU utilization and memory usage.
 */

#include "gpu_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

struct _XRGGPUWidget {
    XRGBaseWidget base;
    XRGGPUCollector *collector;
};

/* Forward declarations */
static void gpu_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* gpu_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void gpu_widget_update(XRGBaseWidget *base);

/**
 * Create a new GPU widget
 */
XRGGPUWidget* xrg_gpu_widget_new(XRGPreferences *prefs, XRGGPUCollector *collector) {
    XRGGPUWidget *widget = g_new0(XRGGPUWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("GPU", prefs);
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
    xrg_base_widget_set_draw_func(&widget->base, gpu_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, gpu_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, gpu_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free GPU widget
 */
void xrg_gpu_widget_free(XRGGPUWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_gpu_widget_get_container(XRGGPUWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_gpu_widget_update(XRGGPUWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_gpu_widget_set_visible(XRGGPUWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_gpu_widget_get_visible(XRGGPUWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_gpu_widget_queue_draw(XRGGPUWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the GPU graph
 */
static void gpu_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGGPUWidget *widget = (XRGGPUWidget *)base;
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

    /* Get GPU datasets */
    XRGDataset *util_dataset = xrg_gpu_collector_get_utilization_dataset(widget->collector);
    XRGDataset *mem_dataset = xrg_gpu_collector_get_memory_dataset(widget->collector);

    gint count = xrg_dataset_get_count(util_dataset);
    if (count < 2) {
        /* Not enough data yet - draw label */
        GdkRGBA *text_color = &prefs->text_color;
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha * 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10);
        cairo_move_to(cr, 5, 12);
        cairo_show_text(cr, "GPU: Waiting for data...");
        return;
    }

    XRGGraphStyle style = prefs->gpu_graph_style;

    /* Draw GPU utilization (cyan - FG1) - bottom layer */
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(util_dataset, i);
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
            gdouble value = xrg_dataset_get_value(util_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(util_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw GPU memory usage on top (purple - FG2) */
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
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
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
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
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / 100.0 * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(mem_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / 100.0 * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    const gchar *gpu_name = xrg_gpu_collector_get_name(widget->collector);
    gdouble utilization = xrg_gpu_collector_get_utilization(widget->collector);
    gdouble mem_used = xrg_gpu_collector_get_memory_used_mb(widget->collector);
    gdouble mem_total = xrg_gpu_collector_get_memory_total_mb(widget->collector);
    gdouble temp = xrg_gpu_collector_get_temperature(widget->collector);

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

    /* Line 4: Temperature with unit conversion */
    gdouble display_temp = temp;
    const gchar *unit_symbol = "\302\260C";  /* °C in UTF-8 */

    if (prefs->temperature_units == XRG_TEMP_FAHRENHEIT) {
        display_temp = (temp * 9.0 / 5.0) + 32.0;
        unit_symbol = "\302\260F";  /* °F in UTF-8 */
    }

    gchar *temp_text = g_strdup_printf("Temp: %.0f%s", display_temp, unit_symbol);
    cairo_move_to(cr, 5, 48);
    cairo_show_text(cr, temp_text);
    g_free(temp_text);

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

        /* Draw filled bar representing current GPU utilization */
        gdouble current_value = utilization / 100.0;
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
 * Generate tooltip text for GPU widget
 */
static gchar* gpu_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    XRGGPUWidget *widget = (XRGGPUWidget *)base;
    XRGPreferences *prefs = base->prefs;

    const gchar *gpu_name = xrg_gpu_collector_get_name(widget->collector);
    const gchar *backend_name = xrg_gpu_collector_get_backend_name(widget->collector);
    gdouble utilization = xrg_gpu_collector_get_utilization(widget->collector);
    gdouble mem_used = xrg_gpu_collector_get_memory_used_mb(widget->collector);
    gdouble mem_total = xrg_gpu_collector_get_memory_total_mb(widget->collector);
    gdouble temp = xrg_gpu_collector_get_temperature(widget->collector);
    gdouble fan_rpm = xrg_gpu_collector_get_fan_speed_rpm(widget->collector);
    gdouble power_watts = xrg_gpu_collector_get_power_watts(widget->collector);

    XRGDataset *util_dataset = xrg_gpu_collector_get_utilization_dataset(widget->collector);
    XRGDataset *mem_dataset = xrg_gpu_collector_get_memory_dataset(widget->collector);

    /* Get widget width for position mapping */
    gint width = gtk_widget_get_allocated_width(base->drawing_area);

    /* Temperature unit conversion */
    gdouble display_temp = temp;
    const gchar *unit_symbol = "°C";

    if (prefs->temperature_units == XRG_TEMP_FAHRENHEIT) {
        display_temp = (temp * 9.0 / 5.0) + 32.0;
        unit_symbol = "°F";
    }

    GString *tooltip = g_string_new("");
    g_string_append_printf(tooltip, "%s\n─────────────\n", gpu_name);

    /* Check for position-aware historical value */
    gdouble hist_util = 0, hist_mem = 0;
    gint time_offset = xrg_get_time_offset_at_position(x, width, util_dataset);

    gboolean has_position_data =
        xrg_get_value_at_position(x, width, util_dataset, &hist_util) &&
        xrg_get_value_at_position(x, width, mem_dataset, &hist_mem);

    if (has_position_data && time_offset >= 0) {
        gchar *time_str = xrg_format_time_offset(time_offset);

        g_string_append_printf(tooltip,
            "📍 At %s:\n"
            "   Util: %.1f%%\n"
            "   Mem:  %.1f%%\n"
            "\n",
            time_str, hist_util, hist_mem);

        g_free(time_str);
    }

    /* Current values */
    gdouble mem_percent = mem_total > 0 ? (mem_used / mem_total * 100.0) : 0.0;

    g_string_append_printf(tooltip,
        "Current\n"
        "   Util: %.1f%%\n"
        "   Mem:  %.0f / %.0f MB (%.1f%%)\n"
        "   Temp: %.0f%s\n",
        utilization,
        mem_used, mem_total, mem_percent,
        display_temp, unit_symbol);

    /* Optional power and fan info */
    if (power_watts > 0) {
        g_string_append_printf(tooltip, "   Power: %.1f W\n", power_watts);
    }
    if (fan_rpm > 0) {
        g_string_append_printf(tooltip, "   Fan: %.0f RPM\n", fan_rpm);
    }

    /* Backend info */
    g_string_append_printf(tooltip, "\nDriver: %s", backend_name);

    return g_string_free(tooltip, FALSE);
}

/**
 * Update callback (called by timer)
 */
static void gpu_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
