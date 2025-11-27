/**
 * XRG Battery Widget Implementation
 *
 * Widget for displaying battery status and charge/discharge rates.
 */

#include "battery_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include <math.h>

struct _XRGBatteryWidget {
    XRGBaseWidget base;
    XRGBatteryCollector *collector;
};

/* Forward declarations */
static void battery_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* battery_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void battery_widget_update(XRGBaseWidget *base);

/**
 * Create a new battery widget
 */
XRGBatteryWidget* xrg_battery_widget_new(XRGPreferences *prefs, XRGBatteryCollector *collector) {
    XRGBatteryWidget *widget = g_new0(XRGBatteryWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("Battery", prefs);
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
    widget->base.show_activity_bar = FALSE;  /* Battery has built-in charge bar */
    widget->base.user_data = widget;

    /* Set callbacks */
    xrg_base_widget_set_draw_func(&widget->base, battery_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, battery_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, battery_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free battery widget
 */
void xrg_battery_widget_free(XRGBatteryWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_battery_widget_get_container(XRGBatteryWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_battery_widget_update(XRGBatteryWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_battery_widget_set_visible(XRGBatteryWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_battery_widget_get_visible(XRGBatteryWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_battery_widget_queue_draw(XRGBatteryWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the battery graph
 */
static void battery_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGBatteryWidget *widget = (XRGBatteryWidget *)base;
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

    /* Get battery datasets */
    XRGDataset *charge_dataset = widget->collector->charge_watts;
    XRGDataset *discharge_dataset = widget->collector->discharge_watts;

    gint count = xrg_dataset_get_count(charge_dataset);
    if (count < 2) {
        /* Not enough data yet - draw "Battery" message */
        GdkRGBA *text_color = &prefs->text_color;
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha * 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10.0);
        cairo_move_to(cr, 10, height / 2);
        cairo_show_text(cr, "Battery");
        return;
    }

    /* Get battery status */
    XRGBatteryStatus status = xrg_battery_collector_get_status(widget->collector);
    gint charge_percent = xrg_battery_collector_get_charge_percent(widget->collector);
    gint minutes_remaining = xrg_battery_collector_get_minutes_remaining(widget->collector);

    XRGGraphStyle style = prefs->battery_graph_style;
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;

    /* Draw discharge watts (cyan - FG1) */
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    gdouble max_discharge = xrg_dataset_get_max(discharge_dataset);
    if (max_discharge < 10.0) max_discharge = 10.0;  /* Minimum scale */

    if (style == XRG_GRAPH_STYLE_SOLID) {
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(discharge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_discharge * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(discharge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_discharge * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(discharge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_discharge * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(discharge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_discharge * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw charge watts (green - FG2) on top */
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha);

    gdouble max_charge = xrg_dataset_get_max(charge_dataset);
    if (max_charge < 10.0) max_charge = 10.0;

    if (style == XRG_GRAPH_STYLE_SOLID) {
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(charge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_charge * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(charge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_charge * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(charge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_charge * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(charge_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_charge * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw battery icon/bar on the right */
    gint bar_x = width - 30;
    gint bar_width = 25;
    gint bar_height_total = height - 20;
    gint bar_y = 10;

    /* Draw battery outline */
    cairo_set_source_rgba(cr, border_color->red, border_color->green, border_color->blue, border_color->alpha);
    cairo_set_line_width(cr, 2.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height_total);
    cairo_stroke(cr);

    /* Draw battery terminal */
    cairo_rectangle(cr, bar_x + 7, bar_y - 3, 11, 3);
    cairo_fill(cr);

    /* Draw charge level fill */
    gdouble fill_height = (charge_percent / 100.0) * (bar_height_total - 4);
    gdouble fill_y = bar_y + bar_height_total - fill_height - 2;

    /* Color based on charge level */
    if (charge_percent > 50) {
        cairo_set_source_rgba(cr, 0.0, 1.0, 0.2, 0.8);  /* Green */
    } else if (charge_percent > 20) {
        cairo_set_source_rgba(cr, 1.0, 0.8, 0.0, 0.8);  /* Yellow */
    } else {
        cairo_set_source_rgba(cr, 1.0, 0.2, 0.0, 0.8);  /* Red */
    }

    cairo_rectangle(cr, bar_x + 2, fill_y, bar_width - 4, fill_height);
    cairo_fill(cr);

    /* Draw text labels */
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    /* Line 1: Status and charge percent */
    const gchar *status_text = "";
    switch (status) {
        case XRG_BATTERY_STATUS_CHARGING:
            status_text = "Charging";
            break;
        case XRG_BATTERY_STATUS_DISCHARGING:
            status_text = "Battery";
            break;
        case XRG_BATTERY_STATUS_FULL:
            status_text = "Fully Charged";
            break;
        case XRG_BATTERY_STATUS_NOT_CHARGING:
            status_text = "Not Charging";
            break;
        case XRG_BATTERY_STATUS_NO_BATTERY:
            status_text = "No Battery";
            break;
        default:
            status_text = "Unknown";
    }

    gchar *line1 = g_strdup_printf("%s: %d%%", status_text, charge_percent);
    cairo_move_to(cr, 5, 12);
    cairo_show_text(cr, line1);
    g_free(line1);

    /* Line 2: Time remaining */
    if (minutes_remaining > 0) {
        gint hours = minutes_remaining / 60;
        gint mins = minutes_remaining % 60;
        gchar *line2 = g_strdup_printf("Time: %dh %dm", hours, mins);
        cairo_move_to(cr, 5, 26);
        cairo_show_text(cr, line2);
        g_free(line2);
    }

    /* Line 3: Current power */
    gdouble current_discharge = xrg_dataset_get_latest(discharge_dataset);
    gdouble current_charge = xrg_dataset_get_latest(charge_dataset);
    if (current_charge > 0.1) {
        gchar *line3 = g_strdup_printf("Charging: %.1fW", current_charge);
        cairo_move_to(cr, 5, 40);
        cairo_show_text(cr, line3);
        g_free(line3);
    } else if (current_discharge > 0.1) {
        gchar *line3 = g_strdup_printf("Discharging: %.1fW", current_discharge);
        cairo_move_to(cr, 5, 40);
        cairo_show_text(cr, line3);
        g_free(line3);
    }
}

/**
 * Generate tooltip text for battery widget
 */
static gchar* battery_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    (void)x; (void)y;
    XRGBatteryWidget *widget = (XRGBatteryWidget *)base;

    XRGBatteryStatus status = xrg_battery_collector_get_status(widget->collector);
    gint charge_percent = xrg_battery_collector_get_charge_percent(widget->collector);
    gint64 charge = xrg_battery_collector_get_total_charge(widget->collector);
    gint64 capacity = xrg_battery_collector_get_total_capacity(widget->collector);
    gint minutes_remaining = xrg_battery_collector_get_minutes_remaining(widget->collector);

    const gchar *status_text = "";
    switch (status) {
        case XRG_BATTERY_STATUS_CHARGING:
            status_text = "Charging";
            break;
        case XRG_BATTERY_STATUS_DISCHARGING:
            status_text = "Discharging";
            break;
        case XRG_BATTERY_STATUS_FULL:
            status_text = "Fully Charged";
            break;
        case XRG_BATTERY_STATUS_NOT_CHARGING:
            status_text = "Not Charging";
            break;
        case XRG_BATTERY_STATUS_NO_BATTERY:
            status_text = "No Battery";
            break;
        default:
            status_text = "Unknown";
    }

    gchar *tooltip;
    if (minutes_remaining > 0) {
        gint hours = minutes_remaining / 60;
        gint mins = minutes_remaining % 60;
        tooltip = g_strdup_printf(
            "Battery: %d%% (%s)\n"
            "Charge: %ldmWh / %ldmWh\n"
            "Time Remaining: %dh %dm",
            charge_percent, status_text,
            (long)(charge / 1000), (long)(capacity / 1000),
            hours, mins
        );
    } else {
        tooltip = g_strdup_printf(
            "Battery: %d%% (%s)\n"
            "Charge: %ldmWh / %ldmWh",
            charge_percent, status_text,
            (long)(charge / 1000), (long)(capacity / 1000)
        );
    }

    return tooltip;
}

/**
 * Update callback (called by timer)
 */
static void battery_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
