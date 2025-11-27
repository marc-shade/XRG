/**
 * XRG Base Widget Header
 *
 * Common structures and functions for all XRG graph widgets.
 * Each widget (CPU, Memory, Network, etc.) inherits from this base.
 */

#ifndef XRG_BASE_WIDGET_H
#define XRG_BASE_WIDGET_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "../core/preferences.h"
#include "../core/dataset.h"
#include "../core/utils.h"

/* Forward declarations */
typedef struct _XRGBaseWidget XRGBaseWidget;

/* Activity bar styles (reuses XRGGraphStyle values) */
typedef XRGGraphStyle XRGActivityStyle;

/* Widget draw callback */
typedef void (*XRGWidgetDrawFunc)(XRGBaseWidget *widget, cairo_t *cr,
                                   int width, int height);

/* Widget context menu callback */
typedef void (*XRGWidgetMenuFunc)(XRGBaseWidget *widget, GdkEventButton *event);

/* Widget tooltip callback - returns allocated string or NULL */
typedef gchar* (*XRGWidgetTooltipFunc)(XRGBaseWidget *widget, int x, int y);

/* Widget update callback */
typedef void (*XRGWidgetUpdateFunc)(XRGBaseWidget *widget);

/**
 * Base widget structure
 * All XRG widgets embed this as their first member
 */
struct _XRGBaseWidget {
    /* GTK widgets */
    GtkWidget *container;       /* Outer box */
    GtkWidget *drawing_area;    /* Main graph area */
    GtkWidget *activity_bar;    /* Optional activity indicator */

    /* Configuration */
    const gchar *name;          /* Widget name ("CPU", "Memory", etc.) */
    XRGPreferences *prefs;      /* Shared preferences */
    gint min_height;            /* Minimum height */
    gint preferred_height;      /* Preferred height */
    gboolean show_activity_bar; /* Whether to show activity bar */

    /* Callbacks */
    XRGWidgetDrawFunc draw_func;        /* Main drawing function */
    XRGWidgetMenuFunc menu_func;        /* Context menu handler */
    XRGWidgetTooltipFunc tooltip_func;  /* Tooltip generator */
    XRGWidgetUpdateFunc update_func;    /* Data update function */

    /* State */
    gboolean visible;           /* Is widget currently visible */
    gboolean hovered;           /* Is mouse over widget */
    gint hover_x, hover_y;      /* Mouse position for tooltips */

    /* User data pointer for widget-specific data */
    gpointer user_data;
};

/* Widget creation and destruction */
XRGBaseWidget* xrg_base_widget_new(const gchar *name, XRGPreferences *prefs);
void xrg_base_widget_free(XRGBaseWidget *widget);

/* Widget initialization helpers */
void xrg_base_widget_init_container(XRGBaseWidget *widget);
void xrg_base_widget_set_draw_func(XRGBaseWidget *widget, XRGWidgetDrawFunc func);
void xrg_base_widget_set_menu_func(XRGBaseWidget *widget, XRGWidgetMenuFunc func);
void xrg_base_widget_set_tooltip_func(XRGBaseWidget *widget, XRGWidgetTooltipFunc func);
void xrg_base_widget_set_update_func(XRGBaseWidget *widget, XRGWidgetUpdateFunc func);

/* Widget visibility */
void xrg_base_widget_show(XRGBaseWidget *widget);
void xrg_base_widget_hide(XRGBaseWidget *widget);
void xrg_base_widget_set_visible(XRGBaseWidget *widget, gboolean visible);

/* Request redraw */
void xrg_base_widget_queue_draw(XRGBaseWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_base_widget_get_container(XRGBaseWidget *widget);

/*============================================================================
 * Common Drawing Utilities
 *============================================================================*/

/* Draw filled graph from dataset */
void xrg_draw_filled_graph(cairo_t *cr, XRGDataset *data,
                           int x, int y, int width, int height,
                           gdouble max_value, GdkRGBA *color,
                           XRGGraphStyle style);

/* Draw line graph from dataset */
void xrg_draw_line_graph(cairo_t *cr, XRGDataset *data,
                         int x, int y, int width, int height,
                         gdouble max_value, GdkRGBA *color,
                         gdouble line_width);

/* Draw stacked area graph (multiple datasets) */
void xrg_draw_stacked_graph(cairo_t *cr, XRGDataset **datasets, int count,
                            int x, int y, int width, int height,
                            gdouble max_value, GdkRGBA *colors,
                            XRGGraphStyle style);

/* Draw activity bar */
void xrg_draw_activity_bar(cairo_t *cr, gdouble value, gdouble max_value,
                           int x, int y, int width, int height,
                           XRGPreferences *prefs, XRGActivityStyle style);

/* Draw gradient activity bar */
void xrg_draw_gradient_activity_bar(cairo_t *cr, gdouble value, gdouble max_value,
                                    int x, int y, int width, int height,
                                    GdkRGBA *start_color, GdkRGBA *end_color,
                                    XRGActivityStyle style);

/* Draw background with border */
void xrg_draw_background(cairo_t *cr, int x, int y, int width, int height,
                         GdkRGBA *bg_color, GdkRGBA *border_color,
                         gdouble border_width, gdouble corner_radius);

/* Draw text label */
void xrg_draw_label(cairo_t *cr, const gchar *text,
                    int x, int y, GdkRGBA *color,
                    const gchar *font_family, gdouble font_size,
                    gboolean bold);

/* Draw right-aligned text */
void xrg_draw_label_right(cairo_t *cr, const gchar *text,
                          int x, int y, int width, GdkRGBA *color,
                          const gchar *font_family, gdouble font_size);

/* Draw centered text */
void xrg_draw_label_center(cairo_t *cr, const gchar *text,
                           int x, int y, int width, GdkRGBA *color,
                           const gchar *font_family, gdouble font_size);

/* Note: Use xrg_format_bytes, xrg_format_rate, xrg_format_percentage from utils.h */

/* Format temperature (widget-specific) */
gchar* xrg_format_temperature(gdouble celsius, gboolean use_fahrenheit);

/* Get gradient color for activity bars */
void xrg_get_gradient_color(gdouble position, XRGPreferences *prefs, GdkRGBA *out_color);

/* Unicode block characters for smooth graphs */
extern const gchar* XRG_BLOCK_CHARS[];  /* ▁▂▃▄▅▆▇█ */
#define XRG_BLOCK_CHAR_COUNT 8

/* Get block character index for value (0.0-1.0) */
int xrg_get_block_char_index(gdouble value);

/*============================================================================
 * Tooltip Support
 *============================================================================*/

/* Show tooltip at position */
void xrg_widget_show_tooltip(XRGBaseWidget *widget, const gchar *text,
                             int x, int y);

/* Hide tooltip */
void xrg_widget_hide_tooltip(XRGBaseWidget *widget);

#endif /* XRG_BASE_WIDGET_H */
