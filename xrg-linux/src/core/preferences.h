#ifndef XRG_PREFERENCES_H
#define XRG_PREFERENCES_H

#include <glib.h>
#include <gtk/gtk.h>

/**
 * Graph visual styles
 */
typedef enum {
    XRG_GRAPH_STYLE_SOLID  = 0,  /* Filled area (default) */
    XRG_GRAPH_STYLE_PIXEL  = 1,  /* Chunky dithered fill */
    XRG_GRAPH_STYLE_DOT    = 2,  /* Fine dithered fill */
    XRG_GRAPH_STYLE_HOLLOW = 3   /* Outline only (no fill) */
} XRGGraphStyle;

/**
 * Window layout orientation
 */
typedef enum {
    XRG_LAYOUT_VERTICAL   = 0,  /* Graphs stacked vertically (default) */
    XRG_LAYOUT_HORIZONTAL = 1   /* Graphs arranged horizontally */
} XRGLayoutOrientation;

/**
 * XRGPreferences - Application settings and preferences
 *
 * Manages all user preferences including window position, module visibility,
 * colors, update intervals, and feature flags.
 */

typedef struct _XRGPreferences XRGPreferences;

struct _XRGPreferences {
    GKeyFile *keyfile;
    gchar *config_path;

    /* Window settings */
    gint window_x;
    gint window_y;
    gint window_width;
    gint window_height;
    gboolean window_always_on_top;
    gboolean window_transparent;
    gdouble window_opacity;

    /* Module visibility */
    gboolean show_cpu;
    gboolean show_memory;
    gboolean show_network;
    gboolean show_disk;
    gboolean show_gpu;
    gboolean show_temperature;
    gboolean show_battery;
    gboolean show_aitoken;
    gboolean show_weather;
    gboolean show_stock;

    /* Activity bars */
    gboolean show_activity_bars;
    XRGGraphStyle activity_bar_style;

    /* Layout orientation */
    XRGLayoutOrientation layout_orientation;

    /* Update intervals (milliseconds) */
    guint fast_update_interval;     /* 100ms for CPU, Network */
    guint normal_update_interval;   /* 1000ms for Memory, Disk, GPU */
    guint slow_update_interval;     /* 5000ms for Temperature, Battery */
    guint vslow_update_interval;    /* 300000ms (5min) for Weather, Stocks */

    /* Colors (RGBA) */
    GdkRGBA background_color;
    GdkRGBA graph_bg_color;
    GdkRGBA graph_fg1_color;  /* Primary data series (cyan) */
    GdkRGBA graph_fg2_color;  /* Secondary data series (purple) */
    GdkRGBA graph_fg3_color;  /* Tertiary data series (amber) */
    GdkRGBA text_color;
    GdkRGBA border_color;
    GdkRGBA activity_bar_color;

    /* Module-specific colors */
    GdkRGBA memory_bg_color;
    GdkRGBA memory_fg1_color;
    GdkRGBA memory_fg2_color;
    GdkRGBA memory_fg3_color;
    GdkRGBA network_bg_color;
    GdkRGBA network_fg1_color;
    GdkRGBA network_fg2_color;
    GdkRGBA disk_bg_color;
    GdkRGBA disk_fg1_color;
    GdkRGBA disk_fg2_color;
    GdkRGBA aitoken_bg_color;
    GdkRGBA aitoken_fg1_color;
    GdkRGBA aitoken_fg2_color;

    /* Graph settings */
    gint graph_width;
    gint graph_height_cpu;
    gint graph_height_memory;
    gint graph_height_network;
    gint graph_height_disk;
    gint graph_height_gpu;
    gint graph_height_temperature;
    gint graph_height_battery;
    gint graph_height_aitoken;

    /* Graph visual styles per module */
    XRGGraphStyle cpu_graph_style;
    XRGGraphStyle memory_graph_style;
    XRGGraphStyle network_graph_style;
    XRGGraphStyle disk_graph_style;
    XRGGraphStyle gpu_graph_style;
    XRGGraphStyle battery_graph_style;
    XRGGraphStyle temperature_graph_style;
    XRGGraphStyle aitoken_graph_style;

    /* AI Token settings */
    gchar *aitoken_jsonl_path;
    gchar *aitoken_db_path;
    gchar *aitoken_otel_endpoint;
    gboolean aitoken_auto_detect;
    gboolean aitoken_show_model_breakdown;

    /* Theme settings */
    gchar *current_theme;  /* Name of current theme */
};

/* Constructor and destructor */
XRGPreferences* xrg_preferences_new(void);
void xrg_preferences_free(XRGPreferences *prefs);

/* Load and save */
gboolean xrg_preferences_load(XRGPreferences *prefs);
gboolean xrg_preferences_save(XRGPreferences *prefs);
void xrg_preferences_set_defaults(XRGPreferences *prefs);

/* Getters and setters (examples - implement as needed) */
gboolean xrg_preferences_get_show_cpu(XRGPreferences *prefs);
void xrg_preferences_set_show_cpu(XRGPreferences *prefs, gboolean show);

GdkRGBA xrg_preferences_get_graph_fg1_color(XRGPreferences *prefs);
void xrg_preferences_set_graph_fg1_color(XRGPreferences *prefs, GdkRGBA *color);

/* Theme functions */
void xrg_preferences_apply_theme(XRGPreferences *prefs, const gchar *theme_name);
const gchar* xrg_preferences_get_current_theme(XRGPreferences *prefs);
gint xrg_preferences_get_theme_count(void);
const gchar* xrg_preferences_get_theme_name(gint index);

#endif /* XRG_PREFERENCES_H */
