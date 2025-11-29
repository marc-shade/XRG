#include "aitoken_collector.h"
#include "aitoken_pricing.h"
#ifndef XRG_CLI_TEST
#include "../core/preferences.h"
#endif
#include <glib/gstdio.h>
#include <json-glib/json-glib.h>
#include <string.h>

/**
 * Auto-detect Claude JSONL path from common locations
 */
static gchar* auto_detect_claude_path(void) {
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
 * Auto-detect Codex sessions path
 * Location: ~/.codex/sessions/YYYY/MM/DD/rollout-*.jsonl
 */
static gchar* auto_detect_codex_path(void) {
    const gchar *home = g_get_home_dir();
    gchar *codex_path = g_build_filename(home, ".codex", "sessions", NULL);

    if (g_file_test(codex_path, G_FILE_TEST_IS_DIR)) {
        return codex_path;
    }

    g_free(codex_path);
    return NULL;
}

/**
 * Auto-detect Gemini CLI chats path
 * Location: ~/.gemini/tmp/<hash>/chats/session-*.json
 */
static gchar* auto_detect_gemini_path(void) {
    const gchar *home = g_get_home_dir();
    gchar *gemini_path = g_build_filename(home, ".gemini", "tmp", NULL);

    if (g_file_test(gemini_path, G_FILE_TEST_IS_DIR)) {
        return gemini_path;
    }

    g_free(gemini_path);
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
 * Parse Codex JSONL for token_count events
 * Format: {"type":"event_msg","payload":{"type":"token_count","info":{"total_token_usage":{"input_tokens":X,"output_tokens":Y}}}}
 */
static gboolean parse_codex_token_event(const gchar *line, guint64 *total_tokens, guint64 *input_tokens, guint64 *output_tokens) {
    if (!line || strlen(line) < 2) {
        return FALSE;
    }

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_data(parser, line, -1, &error)) {
        if (error) g_error_free(error);
        g_object_unref(parser);
        return FALSE;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return FALSE;
    }

    JsonObject *obj = json_node_get_object(root);

    /* Check for event_msg type with token_count payload */
    if (!json_object_has_member(obj, "type") ||
        g_strcmp0(json_object_get_string_member(obj, "type"), "event_msg") != 0) {
        g_object_unref(parser);
        return FALSE;
    }

    if (!json_object_has_member(obj, "payload")) {
        g_object_unref(parser);
        return FALSE;
    }

    JsonObject *payload = json_object_get_object_member(obj, "payload");
    if (!payload || !json_object_has_member(payload, "type") ||
        g_strcmp0(json_object_get_string_member(payload, "type"), "token_count") != 0) {
        g_object_unref(parser);
        return FALSE;
    }

    if (!json_object_has_member(payload, "info")) {
        g_object_unref(parser);
        return FALSE;
    }

    JsonObject *info = json_object_get_object_member(payload, "info");
    if (!info || !json_object_has_member(info, "total_token_usage")) {
        g_object_unref(parser);
        return FALSE;
    }

    JsonObject *usage = json_object_get_object_member(info, "total_token_usage");
    if (usage) {
        if (json_object_has_member(usage, "input_tokens")) {
            *input_tokens = json_object_get_int_member(usage, "input_tokens");
        }
        if (json_object_has_member(usage, "output_tokens")) {
            *output_tokens = json_object_get_int_member(usage, "output_tokens");
        }
        if (json_object_has_member(usage, "total_tokens")) {
            *total_tokens = json_object_get_int_member(usage, "total_tokens");
        } else {
            *total_tokens = *input_tokens + *output_tokens;
        }
    }

    g_object_unref(parser);
    return (*total_tokens > 0);
}

/**
 * Parse Codex JSONL for model info from turn_context
 * Format: {"type":"turn_context","payload":{"model":"gpt-5.1-codex",...}}
 */
static gchar* parse_codex_model(const gchar *line) {
    if (!line || strlen(line) < 2) {
        return NULL;
    }

    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, line, -1, NULL)) {
        g_object_unref(parser);
        return NULL;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return NULL;
    }

    JsonObject *obj = json_node_get_object(root);

    /* Check for turn_context type */
    if (!json_object_has_member(obj, "type") ||
        g_strcmp0(json_object_get_string_member(obj, "type"), "turn_context") != 0) {
        g_object_unref(parser);
        return NULL;
    }

    if (!json_object_has_member(obj, "payload")) {
        g_object_unref(parser);
        return NULL;
    }

    JsonObject *payload = json_object_get_object_member(obj, "payload");
    if (!payload || !json_object_has_member(payload, "model")) {
        g_object_unref(parser);
        return NULL;
    }

    const gchar *model = json_object_get_string_member(payload, "model");
    gchar *result = model ? g_strdup(model) : NULL;

    g_object_unref(parser);
    return result;
}

/**
 * Read Codex tokens from sessions directory
 * Path: ~/.codex/sessions/YYYY/MM/DD/rollout-*.jsonl
 */
static void read_codex_tokens(const gchar *sessions_path, guint64 *total_tokens, GHashTable *model_tokens) {
    *total_tokens = 0;

    GDir *sessions_dir = g_dir_open(sessions_path, 0, NULL);
    if (!sessions_dir) return;

    /* Traverse YYYY/MM/DD directory structure */
    const gchar *year_name;
    while ((year_name = g_dir_read_name(sessions_dir)) != NULL) {
        gchar *year_path = g_build_filename(sessions_path, year_name, NULL);
        if (!g_file_test(year_path, G_FILE_TEST_IS_DIR)) {
            g_free(year_path);
            continue;
        }

        GDir *year_dir = g_dir_open(year_path, 0, NULL);
        if (!year_dir) {
            g_free(year_path);
            continue;
        }

        const gchar *month_name;
        while ((month_name = g_dir_read_name(year_dir)) != NULL) {
            gchar *month_path = g_build_filename(year_path, month_name, NULL);
            if (!g_file_test(month_path, G_FILE_TEST_IS_DIR)) {
                g_free(month_path);
                continue;
            }

            GDir *month_dir = g_dir_open(month_path, 0, NULL);
            if (!month_dir) {
                g_free(month_path);
                continue;
            }

            const gchar *day_name;
            while ((day_name = g_dir_read_name(month_dir)) != NULL) {
                gchar *day_path = g_build_filename(month_path, day_name, NULL);
                if (!g_file_test(day_path, G_FILE_TEST_IS_DIR)) {
                    g_free(day_path);
                    continue;
                }

                GDir *day_dir = g_dir_open(day_path, 0, NULL);
                if (!day_dir) {
                    g_free(day_path);
                    continue;
                }

                /* Find rollout-*.jsonl files */
                const gchar *file_name;
                while ((file_name = g_dir_read_name(day_dir)) != NULL) {
                    if (g_str_has_prefix(file_name, "rollout-") &&
                        g_str_has_suffix(file_name, ".jsonl")) {
                        gchar *file_path = g_build_filename(day_path, file_name, NULL);

                        FILE *fp = fopen(file_path, "r");
                        if (fp) {
                            gchar line[16384];
                            guint64 last_total = 0;
                            guint64 last_input = 0;
                            guint64 last_output = 0;
                            gchar *session_model = NULL;

                            /* Find the last token_count event and model (most recent total) */
                            while (fgets(line, sizeof(line), fp)) {
                                guint64 tokens = 0, input = 0, output = 0;
                                if (parse_codex_token_event(line, &tokens, &input, &output)) {
                                    last_total = tokens;
                                    last_input = input;
                                    last_output = output;
                                }
                                /* Also look for model in turn_context */
                                gchar *model = parse_codex_model(line);
                                if (model) {
                                    g_free(session_model);
                                    session_model = model;
                                }
                            }
                            fclose(fp);

                            /* Add the last total from this session */
                            *total_tokens += last_total;

                            /* Track per-model tokens if we found a model */
                            if (session_model && model_tokens && last_total > 0) {
                                ModelTokens *mt = g_hash_table_lookup(model_tokens, session_model);
                                if (!mt) {
                                    mt = g_new0(ModelTokens, 1);
                                    g_hash_table_insert(model_tokens, g_strdup(session_model), mt);
                                }
                                mt->input_tokens += last_input;
                                mt->output_tokens += last_output;
                            }
                            g_free(session_model);
                        }
                        g_free(file_path);
                    }
                }
                g_dir_close(day_dir);
                g_free(day_path);
            }
            g_dir_close(month_dir);
            g_free(month_path);
        }
        g_dir_close(year_dir);
        g_free(year_path);
    }
    g_dir_close(sessions_dir);
}

/**
 * Parse Gemini session JSON for tokens
 * Format: messages array with "tokens" objects containing input/output/total
 */
static void read_gemini_session_file(const gchar *file_path, guint64 *total_tokens, GHashTable *model_tokens) {
    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_file(parser, file_path, &error)) {
        if (error) g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return;
    }

    JsonObject *obj = json_node_get_object(root);

    /* Look for messages array */
    if (!json_object_has_member(obj, "messages")) {
        g_object_unref(parser);
        return;
    }

    JsonArray *messages = json_object_get_array_member(obj, "messages");
    if (!messages) {
        g_object_unref(parser);
        return;
    }

    /* Sum tokens from all messages */
    guint len = json_array_get_length(messages);
    for (guint i = 0; i < len; i++) {
        JsonNode *msg_node = json_array_get_element(messages, i);
        if (!msg_node || !JSON_NODE_HOLDS_OBJECT(msg_node)) continue;

        JsonObject *msg = json_node_get_object(msg_node);
        if (!msg || !json_object_has_member(msg, "tokens")) continue;

        JsonObject *tokens = json_object_get_object_member(msg, "tokens");
        if (!tokens) continue;

        guint64 input = 0, output = 0;
        if (json_object_has_member(tokens, "input")) {
            input = json_object_get_int_member(tokens, "input");
        }
        if (json_object_has_member(tokens, "output")) {
            output = json_object_get_int_member(tokens, "output");
        }

        /* Use "total" if available, otherwise sum input + output */
        if (json_object_has_member(tokens, "total")) {
            *total_tokens += json_object_get_int_member(tokens, "total");
        } else {
            *total_tokens += input + output;
        }

        /* Track per-model tokens */
        if (model_tokens && json_object_has_member(msg, "model")) {
            const gchar *model_name = json_object_get_string_member(msg, "model");
            if (model_name) {
                ModelTokens *mt = g_hash_table_lookup(model_tokens, model_name);
                if (!mt) {
                    mt = g_new0(ModelTokens, 1);
                    g_hash_table_insert(model_tokens, g_strdup(model_name), mt);
                }
                mt->input_tokens += input;
                mt->output_tokens += output;
            }
        }
    }

    g_object_unref(parser);
}

/**
 * Read Gemini tokens from tmp directory
 * Path: ~/.gemini/tmp/<hash>/chats/session-*.json
 */
static void read_gemini_tokens(const gchar *tmp_path, guint64 *total_tokens, GHashTable *model_tokens) {
    *total_tokens = 0;

    GDir *tmp_dir = g_dir_open(tmp_path, 0, NULL);
    if (!tmp_dir) return;

    /* Traverse <hash>/chats/ directories */
    const gchar *hash_name;
    while ((hash_name = g_dir_read_name(tmp_dir)) != NULL) {
        gchar *hash_path = g_build_filename(tmp_path, hash_name, NULL);
        if (!g_file_test(hash_path, G_FILE_TEST_IS_DIR)) {
            g_free(hash_path);
            continue;
        }

        gchar *chats_path = g_build_filename(hash_path, "chats", NULL);
        if (!g_file_test(chats_path, G_FILE_TEST_IS_DIR)) {
            g_free(chats_path);
            g_free(hash_path);
            continue;
        }

        GDir *chats_dir = g_dir_open(chats_path, 0, NULL);
        if (!chats_dir) {
            g_free(chats_path);
            g_free(hash_path);
            continue;
        }

        /* Find session-*.json files */
        const gchar *file_name;
        while ((file_name = g_dir_read_name(chats_dir)) != NULL) {
            if (g_str_has_prefix(file_name, "session-") &&
                g_str_has_suffix(file_name, ".json")) {
                gchar *file_path = g_build_filename(chats_path, file_name, NULL);

                guint64 session_tokens = 0;
                read_gemini_session_file(file_path, &session_tokens, model_tokens);
                *total_tokens += session_tokens;

                g_free(file_path);
            }
        }
        g_dir_close(chats_dir);
        g_free(chats_path);
        g_free(hash_path);
    }
    g_dir_close(tmp_dir);
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

    /* Initialize datasets - total */
    collector->input_tokens_rate = xrg_dataset_new(dataset_capacity);
    collector->output_tokens_rate = xrg_dataset_new(dataset_capacity);
    collector->total_tokens_rate = xrg_dataset_new(dataset_capacity);

    /* Initialize datasets - per provider */
    collector->claude_tokens_rate = xrg_dataset_new(dataset_capacity);
    collector->codex_tokens_rate = xrg_dataset_new(dataset_capacity);
    collector->gemini_tokens_rate = xrg_dataset_new(dataset_capacity);

    /* Initialize per-provider previous values */
    collector->prev_claude_tokens = 0;
    collector->prev_codex_tokens = 0;
    collector->prev_gemini_tokens = 0;

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

    /* Initialize cost tracking */
    memset(&collector->claude_cost, 0, sizeof(ProviderCostStats));
    memset(&collector->codex_cost, 0, sizeof(ProviderCostStats));
    memset(&collector->gemini_cost, 0, sizeof(ProviderCostStats));
    collector->total_cost_usd = 0.0;
    collector->session_cost_usd = 0.0;
    collector->cost_rate_per_minute = 0.0;
    collector->alert_triggered = FALSE;
    collector->alert_message = NULL;

    return collector;
}

/**
 * Free AI Token collector
 */
void xrg_aitoken_collector_free(XRGAITokenCollector *collector) {
    if (collector == NULL)
        return;

    /* Free total datasets */
    xrg_dataset_free(collector->input_tokens_rate);
    xrg_dataset_free(collector->output_tokens_rate);
    xrg_dataset_free(collector->total_tokens_rate);

    /* Free per-provider datasets */
    xrg_dataset_free(collector->claude_tokens_rate);
    xrg_dataset_free(collector->codex_tokens_rate);
    xrg_dataset_free(collector->gemini_tokens_rate);

    g_free(collector->stats.source_path);
    g_free(collector->stats.current_model);
    if (collector->stats.model_tokens) {
        g_hash_table_destroy(collector->stats.model_tokens);
    }

    g_free(collector->jsonl_path);
    g_free(collector->db_path);
    g_free(collector->otel_endpoint);
    g_free(collector->current_session_id);
    g_free(collector->alert_message);
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
    collector->stats.claude_tokens = 0;
    collector->stats.codex_tokens = 0;
    collector->stats.gemini_tokens = 0;
    collector->stats.other_tokens = 0;

    /* Clear per-model tracking for fresh read */
    if (collector->stats.model_tokens) {
        g_hash_table_remove_all(collector->stats.model_tokens);
    }

    /* === Read from Claude Code === */
    gchar *claude_path = NULL;
    if (collector->auto_detect) {
        claude_path = auto_detect_claude_path();
    } else if (collector->jsonl_path) {
        claude_path = g_strdup(collector->jsonl_path);
    }

    if (claude_path) {
        collector->stats.source_type = AITOKEN_SOURCE_JSONL;
        read_jsonl_tokens(claude_path, &collector->stats, &collector->current_session_id);

        /* Claude tokens = total from Claude JSONL */
        collector->stats.claude_tokens = collector->stats.total_input_tokens + collector->stats.total_output_tokens;

        g_free(collector->stats.source_path);
        collector->stats.source_path = claude_path;
    }

    /* === Read from Codex CLI === */
    gchar *codex_path = auto_detect_codex_path();
    if (codex_path) {
        guint64 codex_total = 0;
        read_codex_tokens(codex_path, &codex_total, collector->stats.model_tokens);
        collector->stats.codex_tokens = codex_total;

        /* Add to totals (Codex doesn't provide input/output split) */
        collector->stats.total_tokens += codex_total;

        g_free(codex_path);
    }

    /* === Read from Gemini CLI === */
    gchar *gemini_path = auto_detect_gemini_path();
    if (gemini_path) {
        guint64 gemini_total = 0;
        read_gemini_tokens(gemini_path, &gemini_total, collector->stats.model_tokens);
        collector->stats.gemini_tokens = gemini_total;

        /* Add to totals */
        collector->stats.total_tokens += gemini_total;

        g_free(gemini_path);
    }

    /* Check if session changed (based on Claude session) */
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

    /* Recalculate total tokens (Claude input/output + Codex + Gemini) */
    collector->stats.total_tokens = collector->stats.total_input_tokens + collector->stats.total_output_tokens +
                                    collector->stats.codex_tokens + collector->stats.gemini_tokens;

    /* Calculate rates (tokens per minute) */
    gdouble input_rate = 0.0;
    gdouble output_rate = 0.0;
    gdouble claude_rate = 0.0;
    gdouble codex_rate = 0.0;
    gdouble gemini_rate = 0.0;

    if (time_delta > 0 && !session_changed) {
        guint64 input_delta = collector->stats.total_input_tokens - prev_input;
        guint64 output_delta = collector->stats.total_output_tokens - prev_output;

        input_rate = (gdouble)input_delta / time_delta * 60.0;  /* per minute */
        output_rate = (gdouble)output_delta / time_delta * 60.0;  /* per minute */

        /* Per-provider rates */
        if (collector->stats.claude_tokens > collector->prev_claude_tokens) {
            claude_rate = (gdouble)(collector->stats.claude_tokens - collector->prev_claude_tokens) / time_delta * 60.0;
        }
        if (collector->stats.codex_tokens > collector->prev_codex_tokens) {
            codex_rate = (gdouble)(collector->stats.codex_tokens - collector->prev_codex_tokens) / time_delta * 60.0;
        }
        if (collector->stats.gemini_tokens > collector->prev_gemini_tokens) {
            gemini_rate = (gdouble)(collector->stats.gemini_tokens - collector->prev_gemini_tokens) / time_delta * 60.0;
        }
    }

    /* Update previous values for next rate calculation */
    collector->prev_claude_tokens = collector->stats.claude_tokens;
    collector->prev_codex_tokens = collector->stats.codex_tokens;
    collector->prev_gemini_tokens = collector->stats.gemini_tokens;

    /* Add to datasets - total */
    xrg_dataset_add_value(collector->input_tokens_rate, input_rate);
    xrg_dataset_add_value(collector->output_tokens_rate, output_rate);
    xrg_dataset_add_value(collector->total_tokens_rate, input_rate + output_rate + codex_rate + gemini_rate);

    /* Add to datasets - per provider */
    xrg_dataset_add_value(collector->claude_tokens_rate, claude_rate);
    xrg_dataset_add_value(collector->codex_tokens_rate, codex_rate);
    xrg_dataset_add_value(collector->gemini_tokens_rate, gemini_rate);

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

/**
 * Get Claude tokens total
 */
guint64 xrg_aitoken_collector_get_claude_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.claude_tokens;
}

/**
 * Get Codex tokens total
 */
guint64 xrg_aitoken_collector_get_codex_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.codex_tokens;
}

/**
 * Get Gemini tokens total
 */
guint64 xrg_aitoken_collector_get_gemini_tokens(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->stats.gemini_tokens;
}

/**
 * Get Claude tokens rate dataset
 */
XRGDataset* xrg_aitoken_collector_get_claude_dataset(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->claude_tokens_rate;
}

/**
 * Get Codex tokens rate dataset
 */
XRGDataset* xrg_aitoken_collector_get_codex_dataset(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->codex_tokens_rate;
}

/**
 * Get Gemini tokens rate dataset
 */
XRGDataset* xrg_aitoken_collector_get_gemini_dataset(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->gemini_tokens_rate;
}

/* ============================================================================
 * Cost Tracking Functions
 * ============================================================================ */

/**
 * Get total estimated cost
 */
gdouble xrg_aitoken_collector_get_total_cost(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->total_cost_usd;
}

/**
 * Get session cost
 */
gdouble xrg_aitoken_collector_get_session_cost(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->session_cost_usd;
}

/**
 * Get current spending rate ($/minute)
 */
gdouble xrg_aitoken_collector_get_cost_rate(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->cost_rate_per_minute;
}

/**
 * Get Claude cost
 */
gdouble xrg_aitoken_collector_get_claude_cost(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->claude_cost.total_cost;
}

/**
 * Get Codex cost
 */
gdouble xrg_aitoken_collector_get_codex_cost(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->codex_cost.total_cost;
}

/**
 * Get Gemini cost
 */
gdouble xrg_aitoken_collector_get_gemini_cost(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->gemini_cost.total_cost;
}

/**
 * Get Claude cap usage as percentage (0.0-1.0+)
 */
gdouble xrg_aitoken_collector_get_claude_cap_usage(XRGAITokenCollector *collector, guint64 cap) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (cap == 0) return 0.0;
    return (gdouble)collector->stats.claude_tokens / cap;
}

/**
 * Get Codex cap usage as percentage (0.0-1.0+)
 */
gdouble xrg_aitoken_collector_get_codex_cap_usage(XRGAITokenCollector *collector, guint64 cap) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (cap == 0) return 0.0;
    return (gdouble)collector->stats.codex_tokens / cap;
}

/**
 * Get Gemini cap usage as percentage (0.0-1.0+)
 */
gdouble xrg_aitoken_collector_get_gemini_cap_usage(XRGAITokenCollector *collector, guint64 cap) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (cap == 0) return 0.0;
    return (gdouble)collector->stats.gemini_tokens / cap;
}

/**
 * Check if an alert has been triggered
 */
gboolean xrg_aitoken_collector_has_alert(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, FALSE);
    return collector->alert_triggered;
}

/**
 * Get alert message
 */
const gchar* xrg_aitoken_collector_get_alert_message(XRGAITokenCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->alert_message;
}

/**
 * Clear alert state
 */
void xrg_aitoken_collector_clear_alert(XRGAITokenCollector *collector) {
    g_return_if_fail(collector != NULL);
    collector->alert_triggered = FALSE;
    g_free(collector->alert_message);
    collector->alert_message = NULL;
}

#ifndef XRG_CLI_TEST
/**
 * Update cost calculations based on preferences
 */
void xrg_aitoken_collector_update_costs(XRGAITokenCollector *collector,
                                        XRGPreferences *prefs) {
    g_return_if_fail(collector != NULL);
    g_return_if_fail(prefs != NULL);

    gdouble prev_total_cost = collector->total_cost_usd;

    /* Calculate Claude costs */
    if (prefs->aitoken_claude_billing_mode == XRG_AITOKEN_BILLING_API) {
        /* API billing - calculate actual cost */
        gdouble input_price = prefs->aitoken_use_custom_pricing ?
            prefs->aitoken_claude_input_price : 0.003;
        gdouble output_price = prefs->aitoken_use_custom_pricing ?
            prefs->aitoken_claude_output_price : 0.015;

        /* If we have per-model tokens, use model-specific pricing */
        if (collector->stats.model_tokens && g_hash_table_size(collector->stats.model_tokens) > 0) {
            collector->claude_cost.total_cost = 0.0;
            GHashTableIter iter;
            gpointer key, value;
            g_hash_table_iter_init(&iter, collector->stats.model_tokens);
            while (g_hash_table_iter_next(&iter, &key, &value)) {
                const gchar *model_name = (const gchar *)key;
                ModelTokens *mt = (ModelTokens *)value;

                /* Get model-specific pricing */
                gdouble model_input_price, model_output_price;
                if (get_model_pricing(model_name, &model_input_price, &model_output_price)) {
                    collector->claude_cost.total_cost += calculate_token_cost(
                        mt->input_tokens, mt->output_tokens,
                        model_input_price, model_output_price);
                } else {
                    /* Use default pricing */
                    collector->claude_cost.total_cost += calculate_token_cost(
                        mt->input_tokens, mt->output_tokens,
                        input_price, output_price);
                }
            }
        } else {
            /* Use aggregate tokens with default pricing */
            collector->claude_cost.total_cost = calculate_token_cost(
                collector->stats.total_input_tokens,
                collector->stats.total_output_tokens,
                input_price, output_price);
        }
    }
    /* For cap billing, cost is 0 (subscription) */

    /* Calculate Codex costs */
    if (prefs->aitoken_codex_billing_mode == XRG_AITOKEN_BILLING_API) {
        gdouble input_price = prefs->aitoken_use_custom_pricing ?
            prefs->aitoken_codex_input_price : 0.002;
        gdouble output_price = prefs->aitoken_use_custom_pricing ?
            prefs->aitoken_codex_output_price : 0.008;

        /* Codex doesn't provide input/output split, so estimate 50/50 */
        guint64 estimated_input = collector->stats.codex_tokens / 2;
        guint64 estimated_output = collector->stats.codex_tokens - estimated_input;

        collector->codex_cost.total_cost = calculate_token_cost(
            estimated_input, estimated_output,
            input_price, output_price);
    }

    /* Calculate Gemini costs */
    if (prefs->aitoken_gemini_billing_mode == XRG_AITOKEN_BILLING_API) {
        gdouble input_price = prefs->aitoken_use_custom_pricing ?
            prefs->aitoken_gemini_input_price : 0.000075;
        gdouble output_price = prefs->aitoken_use_custom_pricing ?
            prefs->aitoken_gemini_output_price : 0.0003;

        /* Gemini doesn't provide input/output split, so estimate 50/50 */
        guint64 estimated_input = collector->stats.gemini_tokens / 2;
        guint64 estimated_output = collector->stats.gemini_tokens - estimated_input;

        collector->gemini_cost.total_cost = calculate_token_cost(
            estimated_input, estimated_output,
            input_price, output_price);
    }

    /* Sum up total cost */
    collector->total_cost_usd = collector->claude_cost.total_cost +
                                collector->codex_cost.total_cost +
                                collector->gemini_cost.total_cost;

    /* Calculate cost rate ($/minute) */
    gdouble cost_delta = collector->total_cost_usd - prev_total_cost;
    if (cost_delta > 0) {
        /* Smooth the rate over time */
        collector->cost_rate_per_minute = collector->cost_rate_per_minute * 0.8 +
                                          cost_delta * 12.0 * 0.2;  /* 5-sec updates = 12/min */
    } else {
        collector->cost_rate_per_minute *= 0.9;  /* Decay when no new cost */
    }

    /* Check for alerts */
    gboolean should_alert = FALSE;
    gchar *new_alert = NULL;

    /* Get effective caps - use tier defaults if manual cap is 0 */
    guint64 effective_claude_cap = prefs->aitoken_claude_cap > 0 ?
        prefs->aitoken_claude_cap : get_claude_tier_weekly_cap(prefs->aitoken_claude_tier);
    guint64 effective_codex_cap = prefs->aitoken_codex_cap > 0 ?
        prefs->aitoken_codex_cap : get_codex_tier_weekly_cap(prefs->aitoken_codex_tier);
    guint64 effective_gemini_cap = prefs->aitoken_gemini_cap > 0 ?
        prefs->aitoken_gemini_cap : get_gemini_tier_daily_cap(prefs->aitoken_gemini_tier) * 7; /* Weekly */

    /* Check cap-based alerts */
    if (prefs->aitoken_claude_billing_mode == XRG_AITOKEN_BILLING_CAP &&
        effective_claude_cap > 0) {
        gdouble usage = xrg_aitoken_collector_get_claude_cap_usage(collector, effective_claude_cap);
        if (usage >= prefs->aitoken_alert_threshold && !collector->alert_triggered) {
            should_alert = TRUE;
            new_alert = g_strdup_printf("Claude %s: %.0f%% of cap",
                get_claude_tier_name(prefs->aitoken_claude_tier), usage * 100.0);
        }
    }

    if (prefs->aitoken_codex_billing_mode == XRG_AITOKEN_BILLING_CAP &&
        effective_codex_cap > 0) {
        gdouble usage = xrg_aitoken_collector_get_codex_cap_usage(collector, effective_codex_cap);
        if (usage >= prefs->aitoken_alert_threshold && !collector->alert_triggered) {
            should_alert = TRUE;
            gchar *codex_alert = g_strdup_printf("Codex %s: %.0f%% of cap",
                get_codex_tier_name(prefs->aitoken_codex_tier), usage * 100.0);
            if (new_alert) {
                gchar *combined = g_strdup_printf("%s | %s", new_alert, codex_alert);
                g_free(new_alert);
                g_free(codex_alert);
                new_alert = combined;
            } else {
                new_alert = codex_alert;
            }
        }
    }

    if (prefs->aitoken_gemini_billing_mode == XRG_AITOKEN_BILLING_CAP &&
        effective_gemini_cap > 0) {
        gdouble usage = xrg_aitoken_collector_get_gemini_cap_usage(collector, effective_gemini_cap);
        if (usage >= prefs->aitoken_alert_threshold && !collector->alert_triggered) {
            should_alert = TRUE;
            gchar *gemini_alert = g_strdup_printf("Gemini %s: %.0f%% of cap",
                get_gemini_tier_name(prefs->aitoken_gemini_tier), usage * 100.0);
            if (new_alert) {
                gchar *combined = g_strdup_printf("%s | %s", new_alert, gemini_alert);
                g_free(new_alert);
                g_free(gemini_alert);
                new_alert = combined;
            } else {
                new_alert = gemini_alert;
            }
        }
    }

    /* Check budget-based alerts (API mode) */
    if (prefs->aitoken_budget_daily > 0 && collector->total_cost_usd >= prefs->aitoken_budget_daily) {
        should_alert = TRUE;
        gchar *budget_alert = g_strdup_printf("Daily budget ($%.2f) exceeded!", prefs->aitoken_budget_daily);
        if (new_alert) {
            gchar *combined = g_strdup_printf("%s | %s", new_alert, budget_alert);
            g_free(new_alert);
            g_free(budget_alert);
            new_alert = combined;
        } else {
            new_alert = budget_alert;
        }
    }

    if (should_alert) {
        collector->alert_triggered = TRUE;
        g_free(collector->alert_message);
        collector->alert_message = new_alert;
    } else {
        g_free(new_alert);
    }
}
#endif /* XRG_CLI_TEST */
