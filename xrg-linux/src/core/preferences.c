#include "preferences.h"
#include <glib/gstdio.h>

#define CONFIG_DIR "xrg-linux"
#define CONFIG_FILE "settings.conf"

/* Helper function to parse RGBA color from string */
static gboolean parse_color(const gchar *str, GdkRGBA *color) {
    gboolean result = gdk_rgba_parse(color, str);
    if (!result) {
        g_warning("Failed to parse color string: '%s'", str);
    } else {
        g_message("Parsed color '%s' -> (%.3f, %.3f, %.3f, %.3f)",
                  str, color->red, color->green, color->blue, color->alpha);
    }
    return result;
}

/* Helper function to format RGBA color to string */
static gchar* format_color(GdkRGBA *color) {
    return g_strdup_printf("rgba(%.3f,%.3f,%.3f,%.3f)",
                           color->red, color->green, color->blue, color->alpha);
}

/**
 * Create new preferences with defaults
 */
XRGPreferences* xrg_preferences_new(void) {
    XRGPreferences *prefs = g_new0(XRGPreferences, 1);

    /* Build config path */
    const gchar *config_dir = g_get_user_config_dir();
    gchar *xrg_config_dir = g_build_filename(config_dir, CONFIG_DIR, NULL);
    prefs->config_path = g_build_filename(xrg_config_dir, CONFIG_FILE, NULL);

    /* Ensure config directory exists */
    g_mkdir_with_parents(xrg_config_dir, 0755);
    g_free(xrg_config_dir);

    /* Initialize keyfile */
    prefs->keyfile = g_key_file_new();

    /* Set defaults */
    xrg_preferences_set_defaults(prefs);

    /* Set default theme */
    prefs->current_theme = g_strdup("Cyberpunk");

    return prefs;
}

/**
 * Free preferences
 */
void xrg_preferences_free(XRGPreferences *prefs) {
    if (prefs == NULL)
        return;

    if (prefs->keyfile)
        g_key_file_free(prefs->keyfile);

    g_free(prefs->config_path);
    g_free(prefs->aitoken_jsonl_path);
    g_free(prefs->aitoken_db_path);
    g_free(prefs->aitoken_otel_endpoint);
    g_free(prefs->current_theme);
    g_free(prefs);
}

/**
 * Set default preferences (matching macOS XRG defaults)
 */
void xrg_preferences_set_defaults(XRGPreferences *prefs) {
    g_return_if_fail(prefs != NULL);

    /* Window settings */
    prefs->window_x = 100;
    prefs->window_y = 100;
    prefs->window_width = 200;
    prefs->window_height = 600;
    prefs->window_always_on_top = TRUE;
    prefs->window_transparent = TRUE;
    prefs->window_opacity = 0.9;

    /* Module visibility (all enabled by default) */
    prefs->show_cpu = TRUE;
    prefs->show_memory = TRUE;
    prefs->show_network = TRUE;
    prefs->show_disk = TRUE;
    prefs->show_gpu = TRUE;
    prefs->show_temperature = TRUE;
    prefs->show_battery = TRUE;
    prefs->show_aitoken = TRUE;
    prefs->show_weather = FALSE;  /* Disabled by default (needs API key) */
    prefs->show_stock = FALSE;    /* Disabled by default (needs API key) */

    /* Activity bars */
    prefs->show_activity_bars = TRUE;  /* Enabled by default */
    prefs->activity_bar_style = XRG_GRAPH_STYLE_SOLID;  /* Solid fill by default */

    /* Layout orientation */
    prefs->layout_orientation = XRG_LAYOUT_VERTICAL;  /* Vertical (stacked) by default */

    /* Update intervals (milliseconds) */
    prefs->fast_update_interval = 100;      /* 0.1 second */
    prefs->normal_update_interval = 1000;   /* 1 second */
    prefs->slow_update_interval = 5000;     /* 5 seconds */
    prefs->vslow_update_interval = 300000;  /* 5 minutes */

    /* Colors - Cyberpunk/Cyber theme (vibrant neon on dark) */
    prefs->background_color.red = 0.02; prefs->background_color.green = 0.05;
    prefs->background_color.blue = 0.12; prefs->background_color.alpha = 0.95;  /* Dark blue-black */

    prefs->graph_bg_color.red = 0.05; prefs->graph_bg_color.green = 0.08;
    prefs->graph_bg_color.blue = 0.15; prefs->graph_bg_color.alpha = 0.95;  /* Dark blue */

    prefs->graph_fg1_color.red = 0.0; prefs->graph_fg1_color.green = 0.95;
    prefs->graph_fg1_color.blue = 1.0; prefs->graph_fg1_color.alpha = 1.0;  /* Electric Cyan */

    prefs->graph_fg2_color.red = 1.0; prefs->graph_fg2_color.green = 0.0;
    prefs->graph_fg2_color.blue = 0.8; prefs->graph_fg2_color.alpha = 1.0;  /* Magenta */

    prefs->graph_fg3_color.red = 0.2; prefs->graph_fg3_color.green = 1.0;
    prefs->graph_fg3_color.blue = 0.3; prefs->graph_fg3_color.alpha = 1.0;  /* Electric Green */

    prefs->text_color.red = 0.9; prefs->text_color.green = 1.0;
    prefs->text_color.blue = 1.0; prefs->text_color.alpha = 1.0;  /* Bright Cyan-White */

    prefs->border_color.red = 0.0; prefs->border_color.green = 0.7;
    prefs->border_color.blue = 0.9; prefs->border_color.alpha = 0.5;  /* Cyan border */

    prefs->activity_bar_color.red = 0.2; prefs->activity_bar_color.green = 1.0;
    prefs->activity_bar_color.blue = 0.3; prefs->activity_bar_color.alpha = 1.0;  /* Electric Green */

    /* Module-specific colors (default to global colors) */
    /* Memory */
    prefs->memory_bg_color = prefs->graph_bg_color;
    prefs->memory_fg1_color = prefs->graph_fg1_color;  /* Cyan */
    prefs->memory_fg2_color = prefs->graph_fg2_color;  /* Purple */
    prefs->memory_fg3_color = prefs->graph_fg3_color;  /* Amber */

    /* Network */
    prefs->network_bg_color = prefs->graph_bg_color;
    prefs->network_fg1_color = prefs->graph_fg1_color;  /* Cyan - download */
    prefs->network_fg2_color = prefs->graph_fg2_color;  /* Purple - upload */

    /* Disk */
    prefs->disk_bg_color = prefs->graph_bg_color;
    prefs->disk_fg1_color = prefs->graph_fg1_color;  /* Cyan - read */
    prefs->disk_fg2_color = prefs->graph_fg2_color;  /* Purple - write */

    /* AI Token */
    prefs->aitoken_bg_color = prefs->graph_bg_color;
    prefs->aitoken_fg1_color = prefs->graph_fg1_color;  /* Cyan - input */
    prefs->aitoken_fg2_color = prefs->graph_fg2_color;  /* Purple - output */

    /* Graph dimensions */
    prefs->graph_width = 200;
    prefs->graph_height_cpu = 80;
    prefs->graph_height_memory = 60;
    prefs->graph_height_network = 60;
    prefs->graph_height_disk = 60;
    prefs->graph_height_gpu = 60;
    prefs->graph_height_temperature = 60;
    prefs->graph_height_battery = 40;
    prefs->graph_height_aitoken = 60;

    /* Graph visual styles (default to SOLID) */
    prefs->cpu_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->memory_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->network_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->disk_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->gpu_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->battery_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->temperature_graph_style = XRG_GRAPH_STYLE_SOLID;
    prefs->aitoken_graph_style = XRG_GRAPH_STYLE_SOLID;

    /* Temperature settings */
    prefs->temperature_units = XRG_TEMP_CELSIUS;  /* Default to Celsius */

    /* AI Token settings */
    gchar *home = g_strdup(g_get_home_dir());
    prefs->aitoken_jsonl_path = g_build_filename(home, ".claude", "projects", NULL);
    prefs->aitoken_db_path = g_build_filename(home, ".claude", "monitoring", "claude_usage.db", NULL);
    prefs->aitoken_otel_endpoint = g_strdup("http://localhost:8889/metrics");
    prefs->aitoken_auto_detect = TRUE;
    prefs->aitoken_show_model_breakdown = FALSE;  /* Default to grouped total */
    g_free(home);
}

/**
 * Load preferences from config file
 */
gboolean xrg_preferences_load(XRGPreferences *prefs) {
    g_return_val_if_fail(prefs != NULL, FALSE);

    GError *error = NULL;
    if (!g_key_file_load_from_file(prefs->keyfile, prefs->config_path, G_KEY_FILE_NONE, &error)) {
        if (error->code != G_FILE_ERROR_NOENT) {
            g_warning("Failed to load preferences: %s", error->message);
        }
        g_error_free(error);
        return FALSE;
    }

    /* Load window settings */
    prefs->window_x = g_key_file_get_integer(prefs->keyfile, "Window", "x", NULL);
    prefs->window_y = g_key_file_get_integer(prefs->keyfile, "Window", "y", NULL);
    prefs->window_width = g_key_file_get_integer(prefs->keyfile, "Window", "width", NULL);
    prefs->window_height = g_key_file_get_integer(prefs->keyfile, "Window", "height", NULL);
    prefs->window_always_on_top = g_key_file_get_boolean(prefs->keyfile, "Window", "always_on_top", NULL);
    prefs->window_opacity = g_key_file_get_double(prefs->keyfile, "Window", "opacity", NULL);

    /* Load module visibility */
    prefs->show_cpu = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_cpu", NULL);
    prefs->show_memory = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_memory", NULL);
    prefs->show_network = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_network", NULL);
    prefs->show_disk = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_disk", NULL);
    prefs->show_gpu = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_gpu", NULL);
    prefs->show_temperature = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_temperature", NULL);
    prefs->show_battery = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_battery", NULL);
    prefs->show_aitoken = g_key_file_get_boolean(prefs->keyfile, "Modules", "show_aitoken", NULL);

    /* Load activity bars setting */
    prefs->show_activity_bars = g_key_file_get_boolean(prefs->keyfile, "Display", "show_activity_bars", NULL);

    /* Load layout orientation */
    prefs->layout_orientation = g_key_file_get_integer(prefs->keyfile, "Display", "layout_orientation", NULL);

    /* Load colors */
    gchar *color_str;
    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "background", NULL);
    if (color_str) { parse_color(color_str, &prefs->background_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "graph_bg", NULL);
    if (color_str) { parse_color(color_str, &prefs->graph_bg_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "graph_fg1", NULL);
    if (color_str) { parse_color(color_str, &prefs->graph_fg1_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "graph_fg2", NULL);
    if (color_str) { parse_color(color_str, &prefs->graph_fg2_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "graph_fg3", NULL);
    if (color_str) { parse_color(color_str, &prefs->graph_fg3_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "text", NULL);
    if (color_str) { parse_color(color_str, &prefs->text_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "border", NULL);
    if (color_str) { parse_color(color_str, &prefs->border_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "activity_bar", NULL);
    if (color_str) { parse_color(color_str, &prefs->activity_bar_color); g_free(color_str); }

    /* Load activity bar style */
    prefs->activity_bar_style = g_key_file_get_integer(prefs->keyfile, "Display", "activity_bar_style", NULL);

    /* Load module-specific colors */
    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "memory_bg", NULL);
    if (color_str) { parse_color(color_str, &prefs->memory_bg_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "memory_fg1", NULL);
    if (color_str) { parse_color(color_str, &prefs->memory_fg1_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "memory_fg2", NULL);
    if (color_str) { parse_color(color_str, &prefs->memory_fg2_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "memory_fg3", NULL);
    if (color_str) { parse_color(color_str, &prefs->memory_fg3_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "network_bg", NULL);
    if (color_str) { parse_color(color_str, &prefs->network_bg_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "network_fg1", NULL);
    if (color_str) { parse_color(color_str, &prefs->network_fg1_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "network_fg2", NULL);
    if (color_str) { parse_color(color_str, &prefs->network_fg2_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "disk_bg", NULL);
    if (color_str) { parse_color(color_str, &prefs->disk_bg_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "disk_fg1", NULL);
    if (color_str) { parse_color(color_str, &prefs->disk_fg1_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "disk_fg2", NULL);
    if (color_str) { parse_color(color_str, &prefs->disk_fg2_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "aitoken_bg", NULL);
    if (color_str) { parse_color(color_str, &prefs->aitoken_bg_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "aitoken_fg1", NULL);
    if (color_str) { parse_color(color_str, &prefs->aitoken_fg1_color); g_free(color_str); }

    color_str = g_key_file_get_string(prefs->keyfile, "Colors", "aitoken_fg2", NULL);
    if (color_str) { parse_color(color_str, &prefs->aitoken_fg2_color); g_free(color_str); }

    /* Load theme */
    gchar *theme_name = g_key_file_get_string(prefs->keyfile, "Appearance", "theme", NULL);
    if (theme_name) {
        g_free(prefs->current_theme);
        prefs->current_theme = theme_name;
    }

    /* Load graph styles */
    prefs->cpu_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "cpu_style", NULL);
    prefs->memory_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "memory_style", NULL);
    prefs->network_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "network_style", NULL);
    prefs->disk_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "disk_style", NULL);
    prefs->gpu_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "gpu_style", NULL);
    prefs->battery_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "battery_style", NULL);
    prefs->temperature_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "temperature_style", NULL);
    prefs->aitoken_graph_style = g_key_file_get_integer(prefs->keyfile, "GraphStyles", "aitoken_style", NULL);

    /* Load Temperature settings */
    prefs->temperature_units = g_key_file_get_integer(prefs->keyfile, "Temperature", "units", NULL);

    /* Load AI Token settings */
    prefs->aitoken_show_model_breakdown = g_key_file_get_boolean(prefs->keyfile, "AIToken", "show_model_breakdown", NULL);

    return TRUE;
}

/**
 * Save preferences to config file
 */
gboolean xrg_preferences_save(XRGPreferences *prefs) {
    g_return_val_if_fail(prefs != NULL, FALSE);

    /* Save window settings */
    g_key_file_set_integer(prefs->keyfile, "Window", "x", prefs->window_x);
    g_key_file_set_integer(prefs->keyfile, "Window", "y", prefs->window_y);
    g_key_file_set_integer(prefs->keyfile, "Window", "width", prefs->window_width);
    g_key_file_set_integer(prefs->keyfile, "Window", "height", prefs->window_height);
    g_key_file_set_boolean(prefs->keyfile, "Window", "always_on_top", prefs->window_always_on_top);
    g_key_file_set_double(prefs->keyfile, "Window", "opacity", prefs->window_opacity);

    /* Save module visibility */
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_cpu", prefs->show_cpu);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_memory", prefs->show_memory);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_network", prefs->show_network);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_disk", prefs->show_disk);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_gpu", prefs->show_gpu);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_temperature", prefs->show_temperature);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_battery", prefs->show_battery);
    g_key_file_set_boolean(prefs->keyfile, "Modules", "show_aitoken", prefs->show_aitoken);

    /* Save activity bars setting */
    g_key_file_set_boolean(prefs->keyfile, "Display", "show_activity_bars", prefs->show_activity_bars);

    /* Save layout orientation */
    g_key_file_set_integer(prefs->keyfile, "Display", "layout_orientation", prefs->layout_orientation);

    /* Save colors */
    gchar *color_str;
    color_str = format_color(&prefs->background_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "background", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->graph_bg_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "graph_bg", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->graph_fg1_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "graph_fg1", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->graph_fg2_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "graph_fg2", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->graph_fg3_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "graph_fg3", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->text_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "text", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->border_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "border", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->activity_bar_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "activity_bar", color_str);
    g_free(color_str);

    /* Save activity bar style */
    g_key_file_set_integer(prefs->keyfile, "Display", "activity_bar_style", prefs->activity_bar_style);

    /* Save module-specific colors */
    color_str = format_color(&prefs->memory_bg_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "memory_bg", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->memory_fg1_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "memory_fg1", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->memory_fg2_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "memory_fg2", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->memory_fg3_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "memory_fg3", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->network_bg_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "network_bg", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->network_fg1_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "network_fg1", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->network_fg2_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "network_fg2", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->disk_bg_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "disk_bg", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->disk_fg1_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "disk_fg1", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->disk_fg2_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "disk_fg2", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->aitoken_bg_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "aitoken_bg", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->aitoken_fg1_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "aitoken_fg1", color_str);
    g_free(color_str);

    color_str = format_color(&prefs->aitoken_fg2_color);
    g_key_file_set_string(prefs->keyfile, "Colors", "aitoken_fg2", color_str);
    g_free(color_str);

    /* Save theme */
    if (prefs->current_theme) {
        g_key_file_set_string(prefs->keyfile, "Appearance", "theme", prefs->current_theme);
    }

    /* Save graph styles */
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "cpu_style", prefs->cpu_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "memory_style", prefs->memory_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "network_style", prefs->network_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "disk_style", prefs->disk_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "gpu_style", prefs->gpu_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "battery_style", prefs->battery_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "temperature_style", prefs->temperature_graph_style);
    g_key_file_set_integer(prefs->keyfile, "GraphStyles", "aitoken_style", prefs->aitoken_graph_style);

    /* Save temperature settings */
    g_key_file_set_integer(prefs->keyfile, "Temperature", "units", prefs->temperature_units);

    /* Save AI Token settings */
    g_key_file_set_boolean(prefs->keyfile, "AIToken", "show_model_breakdown", prefs->aitoken_show_model_breakdown);

    /* Write to file */
    GError *error = NULL;
    if (!g_key_file_save_to_file(prefs->keyfile, prefs->config_path, &error)) {
        g_warning("Failed to save preferences: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    return TRUE;
}

/* Example getters and setters */

gboolean xrg_preferences_get_show_cpu(XRGPreferences *prefs) {
    g_return_val_if_fail(prefs != NULL, FALSE);
    return prefs->show_cpu;
}

void xrg_preferences_set_show_cpu(XRGPreferences *prefs, gboolean show) {
    g_return_if_fail(prefs != NULL);
    prefs->show_cpu = show;
}

GdkRGBA xrg_preferences_get_graph_fg1_color(XRGPreferences *prefs) {
    static const GdkRGBA black = {0, 0, 0, 1};
    g_return_val_if_fail(prefs != NULL, black);
    return prefs->graph_fg1_color;
}

void xrg_preferences_set_graph_fg1_color(XRGPreferences *prefs, GdkRGBA *color) {
    g_return_if_fail(prefs != NULL);
    g_return_if_fail(color != NULL);
    prefs->graph_fg1_color = *color;
}

/* ============================================================================
 * Theme Management Functions
 * ============================================================================ */

#include "themes.h"

/**
 * Apply a theme by name
 */
void xrg_preferences_apply_theme(XRGPreferences *prefs, const gchar *theme_name) {
    g_return_if_fail(prefs != NULL);
    g_return_if_fail(theme_name != NULL);

    /* Find the theme by name */
    const XRGTheme *theme = NULL;
    for (gsize i = 0; i < XRG_THEME_COUNT; i++) {
        if (g_strcmp0(XRG_THEMES[i].name, theme_name) == 0) {
            theme = &XRG_THEMES[i];
            break;
        }
    }

    if (theme == NULL) {
        g_warning("Theme '%s' not found, using default (Cyberpunk)", theme_name);
        theme = &XRG_THEMES[0];  /* Fallback to Cyberpunk */
    }

    /* Apply theme colors */
    prefs->background_color = theme->background_color;
    prefs->graph_bg_color = theme->graph_bg_color;
    prefs->graph_fg1_color = theme->graph_fg1_color;
    prefs->graph_fg2_color = theme->graph_fg2_color;
    prefs->graph_fg3_color = theme->graph_fg3_color;
    prefs->text_color = theme->text_color;
    prefs->border_color = theme->border_color;

    /* Apply to module-specific colors */
    prefs->memory_bg_color = theme->graph_bg_color;
    prefs->memory_fg1_color = theme->graph_fg1_color;
    prefs->memory_fg2_color = theme->graph_fg2_color;
    prefs->memory_fg3_color = theme->graph_fg3_color;

    prefs->network_bg_color = theme->graph_bg_color;
    prefs->network_fg1_color = theme->graph_fg1_color;
    prefs->network_fg2_color = theme->graph_fg2_color;

    prefs->disk_bg_color = theme->graph_bg_color;
    prefs->disk_fg1_color = theme->graph_fg1_color;
    prefs->disk_fg2_color = theme->graph_fg2_color;

    prefs->aitoken_bg_color = theme->graph_bg_color;
    prefs->aitoken_fg1_color = theme->graph_fg1_color;
    prefs->aitoken_fg2_color = theme->graph_fg2_color;

    /* Update current theme name */
    g_free(prefs->current_theme);
    prefs->current_theme = g_strdup(theme->name);

    g_message("Applied theme: %s", theme->name);
}

/**
 * Get current theme name
 */
const gchar* xrg_preferences_get_current_theme(XRGPreferences *prefs) {
    g_return_val_if_fail(prefs != NULL, "Cyberpunk");
    
    if (prefs->current_theme == NULL) {
        return "Cyberpunk";  /* Default */
    }
    
    return prefs->current_theme;
}

/**
 * Get number of available themes
 */
gint xrg_preferences_get_theme_count(void) {
    return (gint)XRG_THEME_COUNT;
}

/**
 * Get theme name by index
 */
const gchar* xrg_preferences_get_theme_name(gint index) {
    if (index < 0 || index >= (gint)XRG_THEME_COUNT) {
        return NULL;
    }
    
    return XRG_THEMES[index].name;
}
