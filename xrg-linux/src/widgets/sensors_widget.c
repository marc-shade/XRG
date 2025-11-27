/**
 * XRG Sensors Widget Implementation
 *
 * Widget for displaying temperature sensors from lm-sensors.
 */

#include "sensors_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

struct _XRGSensorsWidget {
    XRGBaseWidget base;
    XRGSensorsCollector *collector;
};

/* Forward declarations */
static void sensors_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* sensors_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void sensors_widget_update(XRGBaseWidget *base);

/**
 * Create a new sensors widget
 */
XRGSensorsWidget* xrg_sensors_widget_new(XRGPreferences *prefs, XRGSensorsCollector *collector) {
    XRGSensorsWidget *widget = g_new0(XRGSensorsWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("Sensors", prefs);
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
    xrg_base_widget_set_draw_func(&widget->base, sensors_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, sensors_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, sensors_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free sensors widget
 */
void xrg_sensors_widget_free(XRGSensorsWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_sensors_widget_get_container(XRGSensorsWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_sensors_widget_update(XRGSensorsWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_sensors_widget_set_visible(XRGSensorsWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_sensors_widget_get_visible(XRGSensorsWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_sensors_widget_queue_draw(XRGSensorsWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the sensors graph
 */
static void sensors_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGSensorsWidget *widget = (XRGSensorsWidget *)base;
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

    /* Get temperature sensors */
    GSList *temp_sensors = xrg_sensors_collector_get_temp_sensors(widget->collector);

    if (temp_sensors == NULL || g_slist_length(temp_sensors) == 0) {
        /* No sensors found */
        GdkRGBA *text_color = &prefs->text_color;
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha * 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10.0);
        cairo_move_to(cr, 10, height / 2);
        cairo_show_text(cr, "No Sensors");
        if (temp_sensors) g_slist_free(temp_sensors);
        return;
    }

    XRGGraphStyle style = prefs->temperature_graph_style;
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    GdkRGBA *fg3_color = &prefs->graph_fg3_color;

    /* Draw up to 3 temperature sensors */
    gint sensor_count = 0;
    GdkRGBA *colors[] = {fg1_color, fg2_color, fg3_color};

    for (GSList *l = temp_sensors; l != NULL && sensor_count < 3; l = l->next) {
        XRGSensorData *sensor = (XRGSensorData *)l->data;
        if (!sensor->is_enabled) continue;

        gint count = xrg_dataset_get_count(sensor->dataset);
        if (count < 2) continue;

        GdkRGBA *color = colors[sensor_count];
        cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);

        gdouble max_temp = 100.0;  /* Scale to 100°C */

        if (style == XRG_GRAPH_STYLE_SOLID) {
            cairo_move_to(cr, 0, height);
            for (gint i = 0; i < count; i++) {
                gdouble temp = xrg_dataset_get_value(sensor->dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y = height - (temp / max_temp * height);
                if (y < 0) y = 0;
                if (y > height) y = height;
                cairo_line_to(cr, x, y);
            }
            cairo_line_to(cr, width, height);
            cairo_close_path(cr);
            cairo_fill(cr);
        } else if (style == XRG_GRAPH_STYLE_PIXEL) {
            gint dot_spacing = 4;
            for (gint i = 0; i < count; i++) {
                gdouble temp = xrg_dataset_get_value(sensor->dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y_top = height - (temp / max_temp * height);
                if (y_top < 0) y_top = 0;
                if (y_top > height) y_top = height;
                for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                    cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (style == XRG_GRAPH_STYLE_DOT) {
            gint dot_spacing = 2;
            for (gint i = 0; i < count; i++) {
                gdouble temp = xrg_dataset_get_value(sensor->dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y_top = height - (temp / max_temp * height);
                if (y_top < 0) y_top = 0;
                if (y_top > height) y_top = height;
                for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                    cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                    cairo_fill(cr);
                }
            }
        } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
            for (gint i = 0; i < count; i++) {
                gdouble temp = xrg_dataset_get_value(sensor->dataset, i);
                gdouble x = (gdouble)i / count * width;
                gdouble y = height - (temp / max_temp * height);
                if (y < 0) y = 0;
                if (y > height) y = height;
                cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }

        sensor_count++;
    }

    /* Draw temperature bar on the right */
    if (sensor_count > 0 && prefs->show_activity_bars) {
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

        /* Get current temperature from first sensor */
        XRGSensorData *first_sensor = (XRGSensorData *)temp_sensors->data;
        gdouble current_temp = first_sensor->current_value;
        gdouble temp_ratio = current_temp / 100.0;
        if (temp_ratio > 1.0) temp_ratio = 1.0;

        gdouble fill_height = temp_ratio * height;
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

    /* Draw text labels */
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    cairo_move_to(cr, 5, 12);
    cairo_show_text(cr, "Temperature");

    /* Show temperatures of up to 3 sensors */
    sensor_count = 0;
    gint y_offset = 26;
    for (GSList *l = temp_sensors; l != NULL && sensor_count < 3; l = l->next) {
        XRGSensorData *sensor = (XRGSensorData *)l->data;
        if (!sensor->is_enabled) continue;

        /* Convert temperature based on user preference */
        gdouble display_temp = sensor->current_value;
        const gchar *unit_symbol = "\302\260C";  /* °C in UTF-8 */

        if (prefs->temperature_units == XRG_TEMP_FAHRENHEIT) {
            display_temp = (sensor->current_value * 9.0 / 5.0) + 32.0;
            unit_symbol = "\302\260F";  /* °F in UTF-8 */
        }

        gchar *line = g_strdup_printf("%s: %.1f%s", sensor->name, display_temp, unit_symbol);
        cairo_move_to(cr, 5, y_offset);
        cairo_show_text(cr, line);
        g_free(line);

        y_offset += 14;
        sensor_count++;
    }

    g_slist_free(temp_sensors);
}

/**
 * Generate tooltip text for sensors widget
 */
static gchar* sensors_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    (void)x; (void)y;
    XRGSensorsWidget *widget = (XRGSensorsWidget *)base;
    XRGPreferences *prefs = base->prefs;

    GSList *temp_sensors = xrg_sensors_collector_get_temp_sensors(widget->collector);

    if (temp_sensors == NULL || g_slist_length(temp_sensors) == 0) {
        if (temp_sensors) g_slist_free(temp_sensors);
        return g_strdup("No temperature sensors found");
    }

    GString *tooltip_str = g_string_new("Sensors:\n");
    for (GSList *l = temp_sensors; l != NULL; l = l->next) {
        XRGSensorData *sensor = (XRGSensorData *)l->data;

        /* Convert temperature based on user preference */
        gdouble display_temp = sensor->current_value;
        const gchar *unit_symbol = "\302\260C";

        if (prefs->temperature_units == XRG_TEMP_FAHRENHEIT) {
            display_temp = (sensor->current_value * 9.0 / 5.0) + 32.0;
            unit_symbol = "\302\260F";
        }

        g_string_append_printf(tooltip_str, "%s: %.1f%s\n",
                              sensor->name, display_temp, unit_symbol);
    }

    g_slist_free(temp_sensors);

    gchar *tooltip = g_string_free(tooltip_str, FALSE);
    /* Remove trailing newline */
    gsize len = strlen(tooltip);
    if (len > 0 && tooltip[len - 1] == '\n') {
        tooltip[len - 1] = '\0';
    }
    return tooltip;
}

/**
 * Update callback (called by timer)
 */
static void sensors_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
