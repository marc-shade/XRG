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
 * Temperature units
 */
typedef enum {
    XRG_TEMP_CELSIUS    = 0,  /* Celsius (°C) */
    XRG_TEMP_FAHRENHEIT = 1   /* Fahrenheit (°F) */
} XRGTemperatureUnits;

/**
 * AI Token billing mode
 * Different providers use different billing models:
 * - Subscription: Fixed weekly/monthly caps (Claude Pro, Codex subscription)
 * - API: Pay-per-token based on actual usage
 */
typedef enum {
    XRG_AITOKEN_BILLING_CAP = 0,  /* Subscription with token cap */
    XRG_AITOKEN_BILLING_API = 1   /* API pay-per-token */
} XRGAITokenBillingMode;

/**
 * AI Token billing period for cap-based billing
 */
typedef enum {
    XRG_AITOKEN_PERIOD_DAILY   = 0,
    XRG_AITOKEN_PERIOD_WEEKLY  = 1,
    XRG_AITOKEN_PERIOD_MONTHLY = 2
} XRGAITokenBillingPeriod;

/**
 * Claude subscription tiers
 * Limits are weekly, measured in "hours" but we convert to approximate tokens
 * Source: https://support.claude.com/en/articles/11145838-using-claude-code-with-your-pro-or-max-plan
 */
typedef enum {
    XRG_CLAUDE_TIER_PRO     = 0,  /* $20/mo: 40-80 hrs Sonnet 4/week */
    XRG_CLAUDE_TIER_MAX_5X  = 1,  /* $100/mo: 140-280 hrs Sonnet, 15-35 hrs Opus/week */
    XRG_CLAUDE_TIER_MAX_20X = 2,  /* $200/mo: 240-480 hrs Sonnet, 24-40 hrs Opus/week */
    XRG_CLAUDE_TIER_API     = 3   /* Pay-per-token API usage */
} XRGClaudeTier;

/**
 * OpenAI Codex subscription tiers
 * Limits are per 5-hour window + weekly caps
 * Source: https://help.openai.com/en/articles/11369540-using-codex-with-your-chatgpt-plan
 */
typedef enum {
    XRG_CODEX_TIER_PLUS = 0,  /* $20/mo: 30-150 msg/5hrs, ~3K thinking req/week */
    XRG_CODEX_TIER_PRO  = 1,  /* $200/mo: 300-1500 msg/5hrs */
    XRG_CODEX_TIER_API  = 2   /* Pay-per-token API usage */
} XRGCodexTier;

/**
 * Google Gemini CLI subscription tiers
 * Limits are daily, measured as multipliers of base
 * Source: https://blog.google/technology/developers/gemini-cli-code-assist-higher-limits/
 */
typedef enum {
    XRG_GEMINI_TIER_FREE  = 0,  /* Free tier: base limits */
    XRG_GEMINI_TIER_PRO   = 1,  /* $20/mo: 5x base limits */
    XRG_GEMINI_TIER_ULTRA = 2,  /* 20x base limits */
    XRG_GEMINI_TIER_API   = 3   /* Pay-per-token API usage */
} XRGGeminiTier;

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

    /* Temperature settings */
    XRGTemperatureUnits temperature_units;  /* Celsius or Fahrenheit */

    /* AI Token settings */
    gchar *aitoken_jsonl_path;
    gchar *aitoken_db_path;
    gchar *aitoken_otel_endpoint;
    gboolean aitoken_auto_detect;
    gboolean aitoken_show_model_breakdown;

    /* AI Token billing settings - per provider */
    XRGAITokenBillingMode aitoken_claude_billing_mode;   /* Claude: cap or API */
    XRGAITokenBillingMode aitoken_codex_billing_mode;    /* Codex: cap or API */
    XRGAITokenBillingMode aitoken_gemini_billing_mode;   /* Gemini: cap or API */

    /* Subscription tiers - determines token caps */
    XRGClaudeTier aitoken_claude_tier;    /* Pro, Max 5x, Max 20x, or API */
    XRGCodexTier aitoken_codex_tier;      /* Plus, Pro, or API */
    XRGGeminiTier aitoken_gemini_tier;    /* Free, Pro, Ultra, or API */

    /* Cap-based billing settings (subscription accounts) */
    XRGAITokenBillingPeriod aitoken_billing_period;      /* Daily/Weekly/Monthly */
    guint64 aitoken_claude_cap;      /* Token cap for Claude (e.g., 500K/week) */
    guint64 aitoken_codex_cap;       /* Token cap for Codex */
    guint64 aitoken_gemini_cap;      /* Token cap for Gemini */
    gdouble aitoken_alert_threshold; /* Alert at this % of cap (e.g., 0.8 = 80%) */

    /* API-based billing settings (pay-per-token) */
    gdouble aitoken_budget_daily;    /* Daily budget in USD (0 = unlimited) */
    gdouble aitoken_budget_weekly;   /* Weekly budget in USD */
    gdouble aitoken_budget_monthly;  /* Monthly budget in USD */

    /* Custom model pricing (USD per 1K tokens) - user can override defaults */
    gboolean aitoken_use_custom_pricing;
    gdouble aitoken_claude_input_price;   /* $/1K input tokens */
    gdouble aitoken_claude_output_price;  /* $/1K output tokens */
    gdouble aitoken_codex_input_price;
    gdouble aitoken_codex_output_price;
    gdouble aitoken_gemini_input_price;
    gdouble aitoken_gemini_output_price;

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
