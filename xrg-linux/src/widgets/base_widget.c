/**
 * XRG Base Widget Implementation
 *
 * Common structures and functions for all XRG graph widgets.
 */

#include "base_widget.h"
#include <math.h>
#include <string.h>

/* Unicode block characters for smooth graphs */
const gchar* XRG_BLOCK_CHARS[] = {
    "\u2581", "\u2582", "\u2583", "\u2584",
    "\u2585", "\u2586", "\u2587", "\u2588"
};

/*============================================================================
 * Widget Creation and Destruction
 *============================================================================*/

XRGBaseWidget* xrg_base_widget_new(const gchar *name, XRGPreferences *prefs) {
    XRGBaseWidget *widget = g_new0(XRGBaseWidget, 1);
    widget->name = name;
    widget->prefs = prefs;
    widget->min_height = 60;
    widget->preferred_height = 80;
    widget->visible = TRUE;
    widget->show_activity_bar = TRUE;
    return widget;
}

void xrg_base_widget_free(XRGBaseWidget *widget) {
    if (!widget) return;
    /* Note: GTK widgets are destroyed by their parent container */
    g_free(widget);
}

/*============================================================================
 * Widget Initialization
 *============================================================================*/

static gboolean on_draw_callback(GtkWidget *drawing_area, cairo_t *cr, gpointer data) {
    XRGBaseWidget *widget = (XRGBaseWidget *)data;
    if (widget->draw_func) {
        int width = gtk_widget_get_allocated_width(drawing_area);
        int height = gtk_widget_get_allocated_height(drawing_area);
        widget->draw_func(widget, cr, width, height);
    }
    return FALSE;
}

static gboolean on_motion_callback(GtkWidget *drawing_area, GdkEventMotion *event,
                                    gpointer data) {
    XRGBaseWidget *widget = (XRGBaseWidget *)data;
    widget->hovered = TRUE;
    widget->hover_x = (gint)event->x;
    widget->hover_y = (gint)event->y;

    if (widget->tooltip_func) {
        gchar *tooltip = widget->tooltip_func(widget, widget->hover_x, widget->hover_y);
        if (tooltip) {
            gtk_widget_set_tooltip_text(drawing_area, tooltip);
            g_free(tooltip);
        }
    }

    return FALSE;
}

static gboolean on_leave_callback(GtkWidget *drawing_area, GdkEventCrossing *event,
                                   gpointer data) {
    XRGBaseWidget *widget = (XRGBaseWidget *)data;
    widget->hovered = FALSE;
    gtk_widget_set_tooltip_text(drawing_area, NULL);
    return FALSE;
}

static gboolean on_button_press_callback(GtkWidget *drawing_area, GdkEventButton *event,
                                          gpointer data) {
    XRGBaseWidget *widget = (XRGBaseWidget *)data;
    if (event->button == 3 && widget->menu_func) {  /* Right click */
        widget->menu_func(widget, event);
        return TRUE;
    }
    return FALSE;
}

void xrg_base_widget_init_container(XRGBaseWidget *widget) {
    /* Create outer container */
    widget->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Create drawing area */
    widget->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(widget->drawing_area, -1, widget->min_height);

    /* Enable events for tooltips and context menu */
    gtk_widget_add_events(widget->drawing_area,
                          GDK_POINTER_MOTION_MASK |
                          GDK_LEAVE_NOTIFY_MASK |
                          GDK_BUTTON_PRESS_MASK);

    /* Connect signals */
    g_signal_connect(widget->drawing_area, "draw",
                     G_CALLBACK(on_draw_callback), widget);
    g_signal_connect(widget->drawing_area, "motion-notify-event",
                     G_CALLBACK(on_motion_callback), widget);
    g_signal_connect(widget->drawing_area, "leave-notify-event",
                     G_CALLBACK(on_leave_callback), widget);
    g_signal_connect(widget->drawing_area, "button-press-event",
                     G_CALLBACK(on_button_press_callback), widget);

    /* Pack drawing area into container */
    gtk_box_pack_start(GTK_BOX(widget->container), widget->drawing_area,
                       TRUE, TRUE, 0);

    /* Create activity bar if needed */
    if (widget->show_activity_bar) {
        widget->activity_bar = gtk_drawing_area_new();
        gtk_widget_set_size_request(widget->activity_bar, -1, 12);
        gtk_box_pack_start(GTK_BOX(widget->container), widget->activity_bar,
                           FALSE, FALSE, 0);
    }
}

void xrg_base_widget_set_draw_func(XRGBaseWidget *widget, XRGWidgetDrawFunc func) {
    widget->draw_func = func;
}

void xrg_base_widget_set_menu_func(XRGBaseWidget *widget, XRGWidgetMenuFunc func) {
    widget->menu_func = func;
}

void xrg_base_widget_set_tooltip_func(XRGBaseWidget *widget, XRGWidgetTooltipFunc func) {
    widget->tooltip_func = func;
}

void xrg_base_widget_set_update_func(XRGBaseWidget *widget, XRGWidgetUpdateFunc func) {
    widget->update_func = func;
}

/*============================================================================
 * Widget Visibility
 *============================================================================*/

void xrg_base_widget_show(XRGBaseWidget *widget) {
    widget->visible = TRUE;
    gtk_widget_show_all(widget->container);
}

void xrg_base_widget_hide(XRGBaseWidget *widget) {
    widget->visible = FALSE;
    gtk_widget_hide(widget->container);
}

void xrg_base_widget_set_visible(XRGBaseWidget *widget, gboolean visible) {
    if (visible) {
        xrg_base_widget_show(widget);
    } else {
        xrg_base_widget_hide(widget);
    }
}

void xrg_base_widget_queue_draw(XRGBaseWidget *widget) {
    if (widget->drawing_area) {
        gtk_widget_queue_draw(widget->drawing_area);
    }
    if (widget->activity_bar) {
        gtk_widget_queue_draw(widget->activity_bar);
    }
}

GtkWidget* xrg_base_widget_get_container(XRGBaseWidget *widget) {
    return widget->container;
}

/*============================================================================
 * Common Drawing Utilities
 *============================================================================*/

void xrg_draw_filled_graph(cairo_t *cr, XRGDataset *data,
                           int x, int y, int width, int height,
                           gdouble max_value, GdkRGBA *color,
                           XRGGraphStyle style) {
    if (!data || max_value <= 0 || width <= 0 || height <= 0) return;

    gint count = xrg_dataset_get_count(data);
    if (count == 0) return;

    cairo_save(cr);
    cairo_rectangle(cr, x, y, width, height);
    cairo_clip(cr);

    switch (style) {
        case XRG_GRAPH_STYLE_SOLID: {
            /* Filled area graph */
            cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
            cairo_move_to(cr, x + width, y + height);

            for (gint i = 0; i < count && i < width; i++) {
                gdouble value = xrg_dataset_get_value(data, count - 1 - i);
                gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
                gdouble bar_height = normalized * height;
                cairo_line_to(cr, x + width - i, y + height - bar_height);
            }

            cairo_line_to(cr, x + width - count + 1, y + height);
            cairo_close_path(cr);
            cairo_fill(cr);
            break;
        }

        case XRG_GRAPH_STYLE_PIXEL: {
            /* Chunky pixel style */
            gint pixel_size = 3;
            cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);

            for (gint i = 0; i < count && i < width; i += pixel_size) {
                gdouble value = xrg_dataset_get_value(data, count - 1 - i);
                gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
                gint bar_height = (gint)(normalized * height);

                for (gint py = 0; py < bar_height; py += pixel_size) {
                    cairo_rectangle(cr,
                                    x + width - i - pixel_size,
                                    y + height - py - pixel_size,
                                    pixel_size - 1, pixel_size - 1);
                }
            }
            cairo_fill(cr);
            break;
        }

        case XRG_GRAPH_STYLE_DOT: {
            /* Fine dot style */
            cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);

            for (gint i = 0; i < count && i < width; i += 2) {
                gdouble value = xrg_dataset_get_value(data, count - 1 - i);
                gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
                gint bar_height = (gint)(normalized * height);

                for (gint py = 0; py < bar_height; py += 2) {
                    cairo_rectangle(cr,
                                    x + width - i - 1,
                                    y + height - py - 1,
                                    1, 1);
                }
            }
            cairo_fill(cr);
            break;
        }

        case XRG_GRAPH_STYLE_HOLLOW: {
            /* Outline only */
            cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
            cairo_set_line_width(cr, 1.0);

            gboolean first = TRUE;
            for (gint i = 0; i < count && i < width; i++) {
                gdouble value = xrg_dataset_get_value(data, count - 1 - i);
                gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
                gdouble bar_height = normalized * height;

                if (first) {
                    cairo_move_to(cr, x + width - i, y + height - bar_height);
                    first = FALSE;
                } else {
                    cairo_line_to(cr, x + width - i, y + height - bar_height);
                }
            }
            cairo_stroke(cr);
            break;
        }
    }

    cairo_restore(cr);
}

void xrg_draw_line_graph(cairo_t *cr, XRGDataset *data,
                         int x, int y, int width, int height,
                         gdouble max_value, GdkRGBA *color,
                         gdouble line_width) {
    if (!data || max_value <= 0 || width <= 0 || height <= 0) return;

    gint count = xrg_dataset_get_count(data);
    if (count == 0) return;

    cairo_save(cr);
    cairo_rectangle(cr, x, y, width, height);
    cairo_clip(cr);

    cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
    cairo_set_line_width(cr, line_width);

    gboolean first = TRUE;
    for (gint i = 0; i < count && i < width; i++) {
        gdouble value = xrg_dataset_get_value(data, count - 1 - i);
        gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
        gdouble bar_height = normalized * height;

        if (first) {
            cairo_move_to(cr, x + width - i, y + height - bar_height);
            first = FALSE;
        } else {
            cairo_line_to(cr, x + width - i, y + height - bar_height);
        }
    }
    cairo_stroke(cr);

    cairo_restore(cr);
}

void xrg_draw_stacked_graph(cairo_t *cr, XRGDataset **datasets, int count,
                            int x, int y, int width, int height,
                            gdouble max_value, GdkRGBA *colors,
                            XRGGraphStyle style) {
    if (!datasets || count <= 0 || max_value <= 0) return;

    /* For stacked graphs, we draw from bottom to top */
    /* Each layer adds on top of the previous */
    gdouble *accumulated = g_new0(gdouble, width);

    for (int layer = 0; layer < count; layer++) {
        XRGDataset *data = datasets[layer];
        if (!data) continue;

        gint data_count = xrg_dataset_get_count(data);
        if (data_count == 0) continue;

        GdkRGBA *color = &colors[layer];
        cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);

        /* Draw filled area from accumulated baseline */
        cairo_move_to(cr, x + width, y + height);

        for (gint i = 0; i < width; i++) {
            gint data_idx = data_count - 1 - i;
            gdouble value = (data_idx >= 0) ? xrg_dataset_get_value(data, data_idx) : 0;
            accumulated[i] += value;

            gdouble normalized = CLAMP(accumulated[i] / max_value, 0.0, 1.0);
            gdouble bar_height = normalized * height;
            cairo_line_to(cr, x + width - i, y + height - bar_height);
        }

        /* Close path along previous baseline */
        if (layer > 0) {
            for (gint i = width - 1; i >= 0; i--) {
                gint data_idx = xrg_dataset_get_count(datasets[layer]) - 1 - i;
                gdouble value = (data_idx >= 0) ?
                    xrg_dataset_get_value(datasets[layer], data_idx) : 0;
                gdouble prev_accumulated = accumulated[i] - value;
                gdouble normalized = CLAMP(prev_accumulated / max_value, 0.0, 1.0);
                gdouble bar_height = normalized * height;
                cairo_line_to(cr, x + width - i, y + height - bar_height);
            }
        } else {
            cairo_line_to(cr, x, y + height);
        }

        cairo_close_path(cr);
        cairo_fill(cr);
    }

    g_free(accumulated);
}

void xrg_draw_activity_bar(cairo_t *cr, gdouble value, gdouble max_value,
                           int x, int y, int width, int height,
                           XRGPreferences *prefs, XRGActivityStyle style) {
    if (max_value <= 0) return;

    gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
    gint bar_width = (gint)(normalized * width);

    /* Get gradient color based on value */
    GdkRGBA bar_color;
    xrg_get_gradient_color(normalized, prefs, &bar_color);

    cairo_save(cr);

    switch (style) {
        case XRG_GRAPH_STYLE_SOLID:
            cairo_set_source_rgba(cr, bar_color.red, bar_color.green,
                                  bar_color.blue, bar_color.alpha);
            cairo_rectangle(cr, x, y, bar_width, height);
            cairo_fill(cr);
            break;

        case XRG_GRAPH_STYLE_PIXEL: {
            gint pixel_size = 3;
            cairo_set_source_rgba(cr, bar_color.red, bar_color.green,
                                  bar_color.blue, bar_color.alpha);
            for (gint px = 0; px < bar_width; px += pixel_size) {
                cairo_rectangle(cr, x + px, y, pixel_size - 1, height);
            }
            cairo_fill(cr);
            break;
        }

        case XRG_GRAPH_STYLE_DOT: {
            cairo_set_source_rgba(cr, bar_color.red, bar_color.green,
                                  bar_color.blue, bar_color.alpha);
            for (gint px = 0; px < bar_width; px += 2) {
                for (gint py = 0; py < height; py += 2) {
                    cairo_rectangle(cr, x + px, y + py, 1, 1);
                }
            }
            cairo_fill(cr);
            break;
        }

        case XRG_GRAPH_STYLE_HOLLOW:
            cairo_set_source_rgba(cr, bar_color.red, bar_color.green,
                                  bar_color.blue, bar_color.alpha);
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, x + 0.5, y + 0.5, bar_width - 1, height - 1);
            cairo_stroke(cr);
            break;
    }

    cairo_restore(cr);
}

void xrg_draw_gradient_activity_bar(cairo_t *cr, gdouble value, gdouble max_value,
                                    int x, int y, int width, int height,
                                    GdkRGBA *start_color, GdkRGBA *end_color,
                                    XRGActivityStyle style) {
    if (max_value <= 0) return;

    gdouble normalized = CLAMP(value / max_value, 0.0, 1.0);
    gint bar_width = (gint)(normalized * width);

    cairo_save(cr);

    /* Create horizontal gradient */
    cairo_pattern_t *gradient = cairo_pattern_create_linear(x, y, x + bar_width, y);
    cairo_pattern_add_color_stop_rgba(gradient, 0.0,
                                       start_color->red, start_color->green,
                                       start_color->blue, start_color->alpha);
    cairo_pattern_add_color_stop_rgba(gradient, 1.0,
                                       end_color->red, end_color->green,
                                       end_color->blue, end_color->alpha);

    cairo_set_source(cr, gradient);
    cairo_rectangle(cr, x, y, bar_width, height);
    cairo_fill(cr);

    cairo_pattern_destroy(gradient);
    cairo_restore(cr);
}

void xrg_draw_background(cairo_t *cr, int x, int y, int width, int height,
                         GdkRGBA *bg_color, GdkRGBA *border_color,
                         gdouble border_width, gdouble corner_radius) {
    cairo_save(cr);

    if (corner_radius > 0) {
        /* Rounded rectangle */
        gdouble r = corner_radius;
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + width - r, y + r, r, -G_PI_2, 0);
        cairo_arc(cr, x + width - r, y + height - r, r, 0, G_PI_2);
        cairo_arc(cr, x + r, y + height - r, r, G_PI_2, G_PI);
        cairo_arc(cr, x + r, y + r, r, G_PI, 3 * G_PI_2);
        cairo_close_path(cr);
    } else {
        cairo_rectangle(cr, x, y, width, height);
    }

    /* Fill background */
    if (bg_color) {
        cairo_set_source_rgba(cr, bg_color->red, bg_color->green,
                              bg_color->blue, bg_color->alpha);
        cairo_fill_preserve(cr);
    }

    /* Draw border */
    if (border_color && border_width > 0) {
        cairo_set_source_rgba(cr, border_color->red, border_color->green,
                              border_color->blue, border_color->alpha);
        cairo_set_line_width(cr, border_width);
        cairo_stroke(cr);
    } else {
        cairo_new_path(cr);
    }

    cairo_restore(cr);
}

void xrg_draw_label(cairo_t *cr, const gchar *text,
                    int x, int y, GdkRGBA *color,
                    const gchar *font_family, gdouble font_size,
                    gboolean bold) {
    if (!text) return;

    cairo_save(cr);

    cairo_select_font_face(cr, font_family ? font_family : "Sans",
                           CAIRO_FONT_SLANT_NORMAL,
                           bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size > 0 ? font_size : 10.0);

    if (color) {
        cairo_set_source_rgba(cr, color->red, color->green,
                              color->blue, color->alpha);
    }

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text);

    cairo_restore(cr);
}

void xrg_draw_label_right(cairo_t *cr, const gchar *text,
                          int x, int y, int width, GdkRGBA *color,
                          const gchar *font_family, gdouble font_size) {
    if (!text) return;

    cairo_save(cr);

    cairo_select_font_face(cr, font_family ? font_family : "Sans",
                           CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size > 0 ? font_size : 10.0);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);

    if (color) {
        cairo_set_source_rgba(cr, color->red, color->green,
                              color->blue, color->alpha);
    }

    cairo_move_to(cr, x + width - extents.width - extents.x_bearing, y);
    cairo_show_text(cr, text);

    cairo_restore(cr);
}

void xrg_draw_label_center(cairo_t *cr, const gchar *text,
                           int x, int y, int width, GdkRGBA *color,
                           const gchar *font_family, gdouble font_size) {
    if (!text) return;

    cairo_save(cr);

    cairo_select_font_face(cr, font_family ? font_family : "Sans",
                           CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size > 0 ? font_size : 10.0);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text, &extents);

    if (color) {
        cairo_set_source_rgba(cr, color->red, color->green,
                              color->blue, color->alpha);
    }

    gdouble text_x = x + (width - extents.width) / 2.0 - extents.x_bearing;
    cairo_move_to(cr, text_x, y);
    cairo_show_text(cr, text);

    cairo_restore(cr);
}

/*============================================================================
 * Widget-Specific Formatting Utilities
 * Note: Use xrg_format_bytes, xrg_format_rate, xrg_format_percentage from utils.h
 *============================================================================*/

gchar* xrg_format_temperature(gdouble celsius, gboolean use_fahrenheit) {
    if (use_fahrenheit) {
        return g_strdup_printf("%.0f\u00B0F", celsius * 9.0 / 5.0 + 32.0);
    } else {
        return g_strdup_printf("%.0f\u00B0C", celsius);
    }
}

void xrg_get_gradient_color(gdouble position, XRGPreferences *prefs, GdkRGBA *out_color) {
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

int xrg_get_block_char_index(gdouble value) {
    value = CLAMP(value, 0.0, 1.0);
    int index = (int)(value * (XRG_BLOCK_CHAR_COUNT - 1) + 0.5);
    return CLAMP(index, 0, XRG_BLOCK_CHAR_COUNT - 1);
}

/*============================================================================
 * Tooltip Support
 *============================================================================*/

void xrg_widget_show_tooltip(XRGBaseWidget *widget, const gchar *text,
                             int x, int y) {
    if (widget->drawing_area && text) {
        gtk_widget_set_tooltip_text(widget->drawing_area, text);
    }
}

void xrg_widget_hide_tooltip(XRGBaseWidget *widget) {
    if (widget->drawing_area) {
        gtk_widget_set_tooltip_text(widget->drawing_area, NULL);
    }
}
