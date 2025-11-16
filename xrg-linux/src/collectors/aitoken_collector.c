#include "aitoken_collector.h"
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>
#include <string.h>

/**
 * Auto-detect JSONL path from common locations
 */
static gchar* auto_detect_jsonl_path(void) {
    const gchar *home = g_get_home_dir();

    /* Check common locations */
    const gchar *paths[] = {
        ".claude/projects",
        ".config/claude/projects",
        ".anthropic/projects",
        NULL
    };

    for (gint i = 0; paths[i] != NULL; i++) {
        gchar *full_path = g_build_filename(home, paths[i], NULL);

        if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
            return full_path;
        }

        g_free(full_path);
    }

    return NULL;
}

/**
 * Parse tokens, session ID, and model from a single JSONL line
 */
static gboolean parse_jsonl_tokens(const gchar *line, guint64 *input_tokens, guint64 *output_tokens, gchar **session_id, gchar **model) {
    /* Skip empty lines */
    if (!line || strlen(line) < 2) {
        return FALSE;
    }

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_data(parser, line, -1, &error)) {
        if (error) {
            g_error_free(error);
        }
        g_object_unref(parser);
        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return FALSE;
    }

    JsonObject *obj = json_node_get_object(root);
    if (!obj) {
        g_object_unref(parser);
        return FALSE;
    }

    /* Extract session ID if requested */
    if (session_id && json_object_has_member(obj, "sessionId")) {
        const gchar *sid = json_object_get_string_member(obj, "sessionId");
        if (sid && *session_id == NULL) {
            *session_id = g_strdup(sid);
        }
    }

    /* Extract model name if requested - can be at root level or inside message */
    if (model) {
        const gchar *mdl = NULL;

        if (json_object_has_member(obj, "model")) {
            mdl = json_object_get_string_member(obj, "model");
        } else if (json_object_has_member(obj, "message")) {
            JsonObject *message = json_object_get_object_member(obj, "message");
            if (message && json_object_has_member(message, "model")) {
                mdl = json_object_get_string_member(message, "model");
            }
        }

        if (mdl) {
            g_free(*model);
            *model = g_strdup(mdl);
        }
    }

    /* Look for usage object - can be at root level or inside message */
    JsonObject *usage = NULL;

    if (json_object_has_member(obj, "usage")) {
        usage = json_object_get_object_member(obj, "usage");
    } else if (json_object_has_member(obj, "message")) {
        JsonObject *message = json_object_get_object_member(obj, "message");
        if (message && json_object_has_member(message, "usage")) {
            usage = json_object_get_object_member(message, "usage");
        }
    }

    if (usage) {
        if (json_object_has_member(usage, "input_tokens")) {
            *input_tokens = json_object_get_int_member(usage, "input_tokens");
        }

        if (json_object_has_member(usage, "output_tokens")) {
            *output_tokens = json_object_get_int_member(usage, "output_tokens");
        }
    }

    g_object_unref(parser);
    return (*input_tokens > 0 || *output_tokens > 0);
}

/**
 * Find most recent JSONL file and read tokens for current session
 */
static void read_jsonl_tokens(const gchar *dir_path, AITokenStats *stats, gchar **current_session_id) {
    GDir *dir = g_dir_open(dir_path, 0, NULL);
    if (!dir) return;

    /* Find the most recently modified .jsonl file across all projects */
    gchar *most_recent_file = NULL;
    time_t most_recent_time = 0;

    const gchar *filename;
    while ((filename = g_dir_read_name(dir)) != NULL) {
        gchar *full_path = g_build_filename(dir_path, filename, NULL);

        if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
            GDir *project_dir = g_dir_open(full_path, 0, NULL);
            if (project_dir) {
                const gchar *project_file;
                while ((project_file = g_dir_read_name(project_dir)) != NULL) {
                    if (g_str_has_suffix(project_file, ".jsonl")) {
                        gchar *jsonl_file = g_build_filename(full_path, project_file, NULL);

                        if (g_file_test(jsonl_file, G_FILE_TEST_IS_REGULAR)) {
                            GStatBuf stat_buf;
                            if (g_stat(jsonl_file, &stat_buf) == 0) {
                                if (stat_buf.st_mtime > most_recent_time) {
                                    most_recent_time = stat_buf.st_mtime;
                                    g_free(most_recent_file);
                                    most_recent_file = g_strdup(jsonl_file);
                                }
                            }
                        }

                        g_free(jsonl_file);
                    }
                }
                g_dir_close(project_dir);
            }
        }

        g_free(full_path);
    }
    g_dir_close(dir);

    /* If we found a recent file, read it and determine current session */
    if (most_recent_file) {
        FILE *fp = fopen(most_recent_file, "r");
        if (fp) {
            gchar line[8192];
            gchar *detected_session = NULL;

            /* First, scan the last few lines to find the current session ID */
            while (fgets(line, sizeof(line), fp)) {
                guint64 input = 0, output = 0;
                gchar *model = NULL;
                parse_jsonl_tokens(line, &input, &output, &detected_session, &model);

                /* Track the most recent model */
                if (model) {
                    g_free(stats->current_model);
                    stats->current_model = model;  /* Transfer ownership */
                }
            }

            /* If we found a session ID and it's different from current, we have a new session */
            if (detected_session) {
                if (*current_session_id == NULL || strcmp(*current_session_id, detected_session) != 0) {
                    /* New session detected */
                    g_free(*current_session_id);
                    *current_session_id = detected_session;
                    /* Session tokens will be recalculated */
                } else {
                    g_free(detected_session);
                }

                /* Now read the entire file and sum tokens for current session */
                rewind(fp);
                while (fgets(line, sizeof(line), fp)) {
                    guint64 input = 0, output = 0;
                    gchar *line_session = NULL;
                    gchar *model = NULL;
                    if (parse_jsonl_tokens(line, &input, &output, &line_session, &model)) {
                        /* Only count tokens from the current session */
                        if (line_session && *current_session_id &&
                            strcmp(line_session, *current_session_id) == 0) {
                            stats->total_input_tokens += input;
                            stats->total_output_tokens += output;

                            /* Track per-model tokens */
                            if (model && stats->model_tokens) {
                                ModelTokens *mt = g_hash_table_lookup(stats->model_tokens, model);
                                if (!mt) {
                                    /* Create new entry for this model */
                                    mt = g_new0(ModelTokens, 1);
                                    g_hash_table_insert(stats->model_tokens, g_strdup(model), mt);
                                }
                                mt->input_tokens += input;
                                mt->output_tokens += output;
                            }
                        }
                        g_free(line_session);
                        g_free(model);
                    }
                }
            }
            fclose(fp);
        }
        g_free(most_recent_file);
    }

    stats->total_tokens = stats->total_input_tokens + stats->total_output_tokens;
}

/**
 * Create new AI Token collector
 */
XRGAITokenCollector* xrg_aitoken_collector_new(gint dataset_capacity) {
    XRGAITokenCollector *collector = g_new0(XRGAITokenCollector, 1);

    /* Initialize datasets */
    collector->input_tokens_rate = xrg_dataset_new(dataset_capacity);
    collector->output_tokens_rate = xrg_dataset_new(dataset_capacity);
    collector->total_tokens_rate = xrg_dataset_new(dataset_capacity);

    /* Initialize stats */
    collector->stats.source_type = AITOKEN_SOURCE_NONE;
    collector->stats.source_path = NULL;
    collector->stats.total_input_tokens = 0;
    collector->stats.total_output_tokens = 0;
    collector->stats.total_tokens = 0;
    collector->stats.session_input_tokens = 0;
    collector->stats.session_output_tokens = 0;
    collector->stats.tokens_last_hour = 0;

    /* Initialize per-model tracking */
    collector->stats.model_tokens = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    collector->stats.current_model = NULL;

    /* Auto-detect by default */
    collector->auto_detect = TRUE;
    collector->jsonl_path = NULL;
    collector->db_path = NULL;
    collector->otel_endpoint = NULL;

    /* Session tracking */
    collector->current_session_id = NULL;
    collector->session_baseline_tokens = 0;

    collector->last_update_time = g_get_monotonic_time();
    collector->prev_total_tokens = 0;

    return collector;
}

/**
 * Free AI Token collector
 */
void xrg_aitoken_collector_free(XRGAITokenCollector *collector) {
    if (collector == NULL)
        return;

    xrg_dataset_free(collector->input_tokens_rate);
    xrg_dataset_free(collector->output_tokens_rate);
    xrg_dataset_free(collector->total_tokens_rate);

    g_free(collector->stats.source_path);
    g_free(collector->stats.current_model);
    if (collector->stats.model_tokens) {
        g_hash_table_destroy(collector->stats.model_tokens);
    }

    g_free(collector->jsonl_path);
    g_free(collector->db_path);
    g_free(collector->otel_endpoint);
    g_free(collector->current_session_id);
    g_free(collector);
}

/**
 * Set JSONL path
 */
void xrg_aitoken_collector_set_jsonl_path(XRGAITokenCollector *collector, const gchar *path) {
    g_return_if_fail(collector != NULL);

    g_free(collector->jsonl_path);
    collector->jsonl_path = g_strdup(path);
}

/**
 * Set database path
 */
void xrg_aitoken_collector_set_db_path(XRGAITokenCollector *collector, const gchar *path) {
    g_return_if_fail(collector != NULL);

    g_free(collector->db_path);
    collector->db_path = g_strdup(path);
}

/**
 * Set OpenTelemetry endpoint
 */
void xrg_aitoken_collector_set_otel_endpoint(XRGAITokenCollector *collector, const gchar *endpoint) {
    g_return_if_fail(collector != NULL);

    g_free(collector->otel_endpoint);
    collector->otel_endpoint = g_strdup(endpoint);
}

/**
 * Set auto-detect mode
 */
void xrg_aitoken_collector_set_auto_detect(XRGAITokenCollector *collector, gboolean auto_detect) {
    g_return_if_fail(collector != NULL);
    collector->auto_detect = auto_detect;
}

/**
 * Update token statistics
 */
void xrg_aitoken_collector_update(XRGAITokenCollector *collector) {
    g_return_if_fail(collector != NULL);

    gint64 current_time = g_get_monotonic_time();
    gdouble time_delta = (current_time - collector->last_update_time) / 1000000.0;  /* seconds */

    /* Update every 5 seconds instead of every call (reduces CPU load) */
    if (time_delta < 5.0)
        return;

    guint64 prev_input = collector->stats.total_input_tokens;
    guint64 prev_output = collector->stats.total_output_tokens;
    gchar *prev_session_id = collector->current_session_id ? g_strdup(collector->current_session_id) : NULL;

    /* Reset counters for fresh read */
    collector->stats.total_input_tokens = 0;
    collector->stats.total_output_tokens = 0;

    /* Clear per-model tracking for fresh read */
    if (collector->stats.model_tokens) {
        g_hash_table_remove_all(collector->stats.model_tokens);
    }

    /* Determine source */
    gchar *source_path = NULL;

    if (collector->auto_detect) {
        source_path = auto_detect_jsonl_path();
        if (source_path) {
            collector->stats.source_type = AITOKEN_SOURCE_JSONL;
        }
    } else if (collector->jsonl_path) {
        source_path = g_strdup(collector->jsonl_path);
        collector->stats.source_type = AITOKEN_SOURCE_JSONL;
    }

    /* Read tokens from source (this may update current_session_id) */
    if (source_path && collector->stats.source_type == AITOKEN_SOURCE_JSONL) {
        read_jsonl_tokens(source_path, &collector->stats, &collector->current_session_id);

        g_free(collector->stats.source_path);
        collector->stats.source_path = source_path;
    }

    /* Check if session changed */
    gboolean session_changed = FALSE;
    if (prev_session_id == NULL && collector->current_session_id != NULL) {
        /* First session detected */
        session_changed = TRUE;
        collector->session_baseline_tokens = collector->stats.total_input_tokens + collector->stats.total_output_tokens;
    } else if (prev_session_id && collector->current_session_id &&
               strcmp(prev_session_id, collector->current_session_id) != 0) {
        /* New session detected */
        session_changed = TRUE;
        collector->session_baseline_tokens = collector->stats.total_input_tokens + collector->stats.total_output_tokens;
    }

    g_free(prev_session_id);

    /* Calculate session tokens (tokens in current session only) */
    guint64 current_total = collector->stats.total_input_tokens + collector->stats.total_output_tokens;
    if (current_total >= collector->session_baseline_tokens) {
        guint64 session_total = current_total - collector->session_baseline_tokens;

        /* Estimate input/output split based on current ratio */
        if (collector->stats.total_tokens > 0 && session_total > 0) {
            gdouble input_ratio = (gdouble)collector->stats.total_input_tokens / (collector->stats.total_input_tokens + collector->stats.total_output_tokens);
            collector->stats.session_input_tokens = (guint64)(session_total * input_ratio);
            collector->stats.session_output_tokens = session_total - collector->stats.session_input_tokens;
        } else {
            collector->stats.session_input_tokens = 0;
            collector->stats.session_output_tokens = 0;
        }
    } else {
        collector->stats.session_input_tokens = 0;
        collector->stats.session_output_tokens = 0;
    }

    collector->stats.total_tokens = collector->stats.total_input_tokens + collector->stats.total_output_tokens;

    /* Calculate rates (tokens per minute) */
    gdouble input_rate = 0.0;
    gdouble output_rate = 0.0;

    if (time_delta > 0 && !session_changed) {
        guint64 input_delta = collector->stats.total_input_tokens - prev_input;
        guint64 output_delta = collector->stats.total_output_tokens - prev_output;

        input_rate = (gdouble)input_delta / time_delta * 60.0;  /* per minute */
        output_rate = (gdouble)output_delta / time_delta * 60.0;  /* per minute */
    }

    /* Add to datasets */
    xrg_dataset_add_value(collector->input_tokens_rate, input_rate);
    xrg_dataset_add_value(collector->output_tokens_rate, output_rate);
    xrg_dataset_add_value(collector->total_tokens_rate, input_rate + output_rate);

    collector->last_update_time = current_time;
}

/**
 * Get total tokens
 */
guint64 xrg_aitoken_collector_get_total_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.total_tokens;
}

/**
 * Get session tokens
 */
guint64 xrg_aitoken_collector_get_session_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.session_input_tokens + collector->stats.session_output_tokens;
}

/**
 * Get input tokens
 */
guint64 xrg_aitoken_collector_get_input_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.total_input_tokens;
}

/**
 * Get output tokens
 */
guint64 xrg_aitoken_collector_get_output_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.total_output_tokens;
}

/**
 * Get tokens per minute
 */
gdouble xrg_aitoken_collector_get_tokens_per_minute(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);

    gint count = xrg_dataset_get_count(collector->total_tokens_rate);
    if (count == 0)
        return 0.0;

    return xrg_dataset_get_value(collector->total_tokens_rate, count - 1);
}

/**
 * Get source name
 */
const gchar* xrg_aitoken_collector_get_source_name(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, "None");

    switch (collector->stats.source_type) {
        case AITOKEN_SOURCE_JSONL:
            return "JSONL";
        case AITOKEN_SOURCE_SQLITE:
            return "SQLite";
        case AITOKEN_SOURCE_OTEL:
            return "OpenTelemetry";
        default:
            return "None";
    }
}

/**
 * Get input dataset
 */
XRGDataset* xrg_aitoken_collector_get_input_dataset(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->input_tokens_rate;
}

/**
 * Get output dataset
 */
XRGDataset* xrg_aitoken_collector_get_output_dataset(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->output_tokens_rate;
}

/**
 * Get total dataset
 */
XRGDataset* xrg_aitoken_collector_get_total_dataset(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->total_tokens_rate;
}

/**
 * Get current model name
 */
const gchar* xrg_aitoken_collector_get_current_model(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->stats.current_model;
}

/**
 * Get model tokens hash table
 * Returns: (transfer none): Hash table mapping model names to ModelTokens structs
 */
GHashTable* xrg_aitoken_collector_get_model_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->stats.model_tokens;
}
