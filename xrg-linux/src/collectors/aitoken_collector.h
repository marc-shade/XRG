#ifndef XRG_AITOKEN_COLLECTOR_H
#define XRG_AITOKEN_COLLECTOR_H

#include <glib.h>
#include "../core/dataset.h"

/**
 * XRGAITokenCollector - AI Token usage data collector
 *
 * Collects AI token usage statistics from:
 * - JSONL log files (Claude, OpenAI, etc.)
 * - SQLite databases
 * - OpenTelemetry endpoints
 *
 * Tracks tokens per time window and cumulative usage
 */

typedef enum {
    AITOKEN_SOURCE_NONE,
    AITOKEN_SOURCE_JSONL,
    AITOKEN_SOURCE_SQLITE,
    AITOKEN_SOURCE_OTEL
} AITokenSource;

typedef enum {
    AITOKEN_PROVIDER_CLAUDE,
    AITOKEN_PROVIDER_CODEX,
    AITOKEN_PROVIDER_GEMINI,
    AITOKEN_PROVIDER_OTHER
} AITokenProvider;

typedef struct {
    guint64 input_tokens;
    guint64 output_tokens;
} ModelTokens;

typedef struct {
    gchar *source_path;
    AITokenSource source_type;
    guint64 total_input_tokens;
    guint64 total_output_tokens;
    guint64 total_tokens;
    guint64 session_input_tokens;
    guint64 session_output_tokens;
    guint64 tokens_last_hour;
    gint64 last_check_time;

    /* Per-model tracking */
    GHashTable *model_tokens;  /* key: model name (gchar*), value: ModelTokens* */
    gchar *current_model;      /* Most recent model used */

    /* Per-provider tracking */
    guint64 claude_tokens;     /* Claude Code tokens */
    guint64 codex_tokens;      /* OpenAI Codex tokens */
    guint64 gemini_tokens;     /* Google Gemini CLI tokens */
    guint64 other_tokens;      /* Other AI providers */
} AITokenStats;

typedef struct _XRGAITokenCollector XRGAITokenCollector;

struct _XRGAITokenCollector {
    /* Token statistics */
    AITokenStats stats;

    /* Datasets for graphing - total */
    XRGDataset *input_tokens_rate;   /* Input tokens per minute */
    XRGDataset *output_tokens_rate;  /* Output tokens per minute */
    XRGDataset *total_tokens_rate;   /* Total tokens per minute */

    /* Datasets for graphing - per provider */
    XRGDataset *claude_tokens_rate;  /* Claude Code tokens per minute */
    XRGDataset *codex_tokens_rate;   /* OpenAI Codex tokens per minute */
    XRGDataset *gemini_tokens_rate;  /* Google Gemini tokens per minute */

    /* Configuration */
    gboolean auto_detect;
    gchar *jsonl_path;
    gchar *db_path;
    gchar *otel_endpoint;

    /* Session tracking */
    gchar *current_session_id;       /* Current Claude Code session ID */
    guint64 session_baseline_tokens; /* Token count at session start */

    /* Previous values for rate calculation (per provider) */
    guint64 prev_claude_tokens;
    guint64 prev_codex_tokens;
    guint64 prev_gemini_tokens;

    /* Update tracking */
    gint64 last_update_time;
    guint64 prev_total_tokens;
};

/* Constructor and destructor */
XRGAITokenCollector* xrg_aitoken_collector_new(gint dataset_capacity);
void xrg_aitoken_collector_free(XRGAITokenCollector *collector);

/* Configuration */
void xrg_aitoken_collector_set_jsonl_path(XRGAITokenCollector *collector, const gchar *path);
void xrg_aitoken_collector_set_db_path(XRGAITokenCollector *collector, const gchar *path);
void xrg_aitoken_collector_set_otel_endpoint(XRGAITokenCollector *collector, const gchar *endpoint);
void xrg_aitoken_collector_set_auto_detect(XRGAITokenCollector *collector, gboolean auto_detect);

/* Update methods */
void xrg_aitoken_collector_update(XRGAITokenCollector *collector);

/* Getters */
guint64 xrg_aitoken_collector_get_total_tokens(XRGAITokenCollector *collector);
guint64 xrg_aitoken_collector_get_session_tokens(XRGAITokenCollector *collector);
guint64 xrg_aitoken_collector_get_input_tokens(XRGAITokenCollector *collector);
guint64 xrg_aitoken_collector_get_output_tokens(XRGAITokenCollector *collector);
gdouble xrg_aitoken_collector_get_tokens_per_minute(XRGAITokenCollector *collector);
const gchar* xrg_aitoken_collector_get_source_name(XRGAITokenCollector *collector);

/* Dataset access */
XRGDataset* xrg_aitoken_collector_get_input_dataset(XRGAITokenCollector *collector);
XRGDataset* xrg_aitoken_collector_get_output_dataset(XRGAITokenCollector *collector);
XRGDataset* xrg_aitoken_collector_get_total_dataset(XRGAITokenCollector *collector);

/* Model tracking */
const gchar* xrg_aitoken_collector_get_current_model(XRGAITokenCollector *collector);
GHashTable* xrg_aitoken_collector_get_model_tokens(XRGAITokenCollector *collector);

/* Provider-specific getters */
guint64 xrg_aitoken_collector_get_claude_tokens(XRGAITokenCollector *collector);
guint64 xrg_aitoken_collector_get_codex_tokens(XRGAITokenCollector *collector);
guint64 xrg_aitoken_collector_get_gemini_tokens(XRGAITokenCollector *collector);
XRGDataset* xrg_aitoken_collector_get_claude_dataset(XRGAITokenCollector *collector);
XRGDataset* xrg_aitoken_collector_get_codex_dataset(XRGAITokenCollector *collector);
XRGDataset* xrg_aitoken_collector_get_gemini_dataset(XRGAITokenCollector *collector);

#endif /* XRG_AITOKEN_COLLECTOR_H */
