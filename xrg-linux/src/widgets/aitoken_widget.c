/**
 * XRG AI Token Widget Implementation
 *
 * Widget for displaying AI token usage from Claude, Codex, Gemini, etc.
 */

#include "aitoken_widget.h"
#include "base_widget.h"
#include "../core/utils.h"
#include "../collectors/aitoken_pricing.h"
#include <math.h>

struct _XRGAITokenWidget {
    XRGBaseWidget base;
    XRGAITokenCollector *collector;
};

/* Forward declarations */
static void aitoken_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height);
static gchar* aitoken_widget_tooltip(XRGBaseWidget *base, int x, int y);
static void aitoken_widget_update(XRGBaseWidget *base);

/**
 * Create a new AI token widget
 */
XRGAITokenWidget* xrg_aitoken_widget_new(XRGPreferences *prefs, XRGAITokenCollector *collector) {
    XRGAITokenWidget *widget = g_new0(XRGAITokenWidget, 1);

    /* Initialize base widget */
    XRGBaseWidget *base = xrg_base_widget_new("AI Tokens", prefs);
    if (!base) {
        g_free(widget);
        return NULL;
    }

    /* Copy base widget data */
    widget->base = *base;
    g_free(base);

    /* Set widget-specific data */
    widget->collector = collector;
    widget->base.min_height = 80;
    widget->base.preferred_height = 120;
    widget->base.show_activity_bar = TRUE;
    widget->base.user_data = widget;

    /* Set callbacks */
    xrg_base_widget_set_draw_func(&widget->base, aitoken_widget_draw);
    xrg_base_widget_set_tooltip_func(&widget->base, aitoken_widget_tooltip);
    xrg_base_widget_set_update_func(&widget->base, aitoken_widget_update);

    /* Initialize container */
    xrg_base_widget_init_container(&widget->base);

    return widget;
}

/**
 * Free AI token widget
 */
void xrg_aitoken_widget_free(XRGAITokenWidget *widget) {
    if (!widget) return;
    xrg_base_widget_free(&widget->base);
    g_free(widget);
}

/**
 * Get GTK container
 */
GtkWidget* xrg_aitoken_widget_get_container(XRGAITokenWidget *widget) {
    if (!widget) return NULL;
    return xrg_base_widget_get_container(&widget->base);
}

/**
 * Update widget
 */
void xrg_aitoken_widget_update(XRGAITokenWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Set visibility
 */
void xrg_aitoken_widget_set_visible(XRGAITokenWidget *widget, gboolean visible) {
    if (!widget) return;
    xrg_base_widget_set_visible(&widget->base, visible);
}

/**
 * Get visibility
 */
gboolean xrg_aitoken_widget_get_visible(XRGAITokenWidget *widget) {
    if (!widget) return FALSE;
    return widget->base.visible;
}

/**
 * Request redraw
 */
void xrg_aitoken_widget_queue_draw(XRGAITokenWidget *widget) {
    if (!widget) return;
    xrg_base_widget_queue_draw(&widget->base);
}

/**
 * Draw the AI token graph
 */
static void aitoken_widget_draw(XRGBaseWidget *base, cairo_t *cr, int width, int height) {
    XRGAITokenWidget *widget = (XRGAITokenWidget *)base;
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

    /* Get AI Token datasets */
    XRGDataset *input_dataset = xrg_aitoken_collector_get_input_dataset(widget->collector);
    XRGDataset *output_dataset = xrg_aitoken_collector_get_output_dataset(widget->collector);
    XRGDataset *gemini_dataset = xrg_aitoken_collector_get_gemini_dataset(widget->collector);

    gint count = xrg_dataset_get_count(input_dataset);
    if (count < 2) {
        /* Not enough data yet */
        GdkRGBA *text_color = &prefs->text_color;
        cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha * 0.5);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 10);
        cairo_move_to(cr, 5, 12);
        cairo_show_text(cr, "AI Tokens: Waiting for data...");
        return;
    }

    /* Find maximum rate for scaling (minimum 100 tokens/min) */
    gdouble max_rate = 100.0;
    for (gint i = 0; i < count; i++) {
        gdouble input = xrg_dataset_get_value(input_dataset, i);
        gdouble output = xrg_dataset_get_value(output_dataset, i);
        gdouble gemini = xrg_dataset_get_value(gemini_dataset, i);
        if (input > max_rate) max_rate = input;
        if (output > max_rate) max_rate = output;
        if (gemini > max_rate) max_rate = gemini;
    }

    XRGGraphStyle style = prefs->aitoken_graph_style;
    GdkRGBA *fg1_color = &prefs->graph_fg1_color;
    GdkRGBA *fg2_color = &prefs->graph_fg2_color;
    GdkRGBA *fg3_color = &prefs->graph_fg3_color;

    /* Draw input tokens (cyan - FG1) */
    cairo_set_source_rgba(cr, fg1_color->red, fg1_color->green, fg1_color->blue, fg1_color->alpha);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(input_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw output tokens (purple - FG2) */
    cairo_set_source_rgba(cr, fg2_color->red, fg2_color->green, fg2_color->blue, fg2_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(output_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Draw Gemini tokens (green - FG3) */
    cairo_set_source_rgba(cr, fg3_color->red, fg3_color->green, fg3_color->blue, fg3_color->alpha * 0.7);

    if (style == XRG_GRAPH_STYLE_SOLID) {
        cairo_move_to(cr, 0, height);
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(gemini_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, width, height);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (style == XRG_GRAPH_STYLE_PIXEL) {
        gint dot_spacing = 4;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(gemini_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 1.5, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_DOT) {
        gint dot_spacing = 2;
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(gemini_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y_top = height - (value / max_rate * height);
            for (gdouble y = height; y >= y_top; y -= dot_spacing) {
                cairo_arc(cr, x, y, 0.6, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    } else if (style == XRG_GRAPH_STYLE_HOLLOW) {
        for (gint i = 0; i < count; i++) {
            gdouble value = xrg_dataset_get_value(gemini_dataset, i);
            gdouble x = (gdouble)i / count * width;
            gdouble y = height - (value / max_rate * height);
            cairo_arc(cr, x, y, 1.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    /* Overlay text labels */
    GdkRGBA *text_color = &prefs->text_color;
    cairo_set_source_rgba(cr, text_color->red, text_color->green, text_color->blue, text_color->alpha);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);

    gdouble tokens_per_min = xrg_aitoken_collector_get_tokens_per_minute(widget->collector);
    guint64 total_tokens = xrg_aitoken_collector_get_total_tokens(widget->collector);

    /* Convert tokens per minute to tokens per second */
    gdouble token_rate = tokens_per_min / 60.0;

    /* Line 1: AI Tokens label */
    cairo_move_to(cr, 5, 15);
    cairo_show_text(cr, "AI Tokens");

    /* Line 2: Token rate */
    gchar *line2 = g_strdup_printf("Rate: %.0f/s", token_rate);
    cairo_move_to(cr, 5, 27);
    cairo_show_text(cr, line2);
    g_free(line2);

    /* Line 3: Total tokens */
    gchar *line3 = g_strdup_printf("Total: %lu", (unsigned long)total_tokens);
    cairo_move_to(cr, 5, 39);
    cairo_show_text(cr, line3);
    g_free(line3);

    /* Line 4: Cost or Cap display based on billing mode */
    gint next_y = 51;

    /* Update cost calculations */
    xrg_aitoken_collector_update_costs(widget->collector, prefs);

    /* Get Claude tokens for cap display */
    guint64 claude_tokens = xrg_aitoken_collector_get_claude_tokens(widget->collector);

    if (prefs->aitoken_claude_billing_mode == XRG_AITOKEN_BILLING_API) {
        /* API billing - show cost in USD */
        gdouble total_cost = xrg_aitoken_collector_get_total_cost(widget->collector);
        gchar *cost_str;
        if (total_cost < 0.01) {
            cost_str = g_strdup_printf("Cost: %.2f\302\242", total_cost * 100.0);
        } else {
            cost_str = g_strdup_printf("Cost: $%.2f", total_cost);
        }
        cairo_move_to(cr, 5, next_y);
        cairo_show_text(cr, cost_str);
        g_free(cost_str);
        next_y += 12;

        /* Show budget if set */
        if (prefs->aitoken_budget_daily > 0) {
            gdouble pct = (total_cost / prefs->aitoken_budget_daily) * 100.0;
            gchar *budget_str = g_strdup_printf("Budget: %.0f%%", pct);
            cairo_move_to(cr, 5, next_y);
            cairo_show_text(cr, budget_str);
            g_free(budget_str);
            next_y += 12;
        }
    } else {
        /* Cap billing - show usage percentage */
        /* Get effective cap - use tier default if manual cap is 0 */
        guint64 effective_cap = prefs->aitoken_claude_cap > 0 ?
            prefs->aitoken_claude_cap :
            get_claude_tier_weekly_cap(prefs->aitoken_claude_tier);

        if (effective_cap > 0) {
            gdouble usage = xrg_aitoken_collector_get_claude_cap_usage(
                widget->collector, effective_cap);

            /* Show tier name and usage */
            gchar *cap_str = g_strdup_printf("%s: %.1f%%",
                get_claude_tier_name(prefs->aitoken_claude_tier), usage * 100.0);
            cairo_move_to(cr, 5, next_y);
            cairo_show_text(cr, cap_str);
            g_free(cap_str);
            next_y += 12;

            /* Show tokens remaining */
            guint64 remaining = (claude_tokens < effective_cap) ?
                                (effective_cap - claude_tokens) : 0;
            gchar *remain_str = format_tokens(remaining);
            gchar *left_str = g_strdup_printf("Left: %s", remain_str);
            cairo_move_to(cr, 5, next_y);
            cairo_show_text(cr, left_str);
            g_free(remain_str);
            g_free(left_str);
            next_y += 12;
        }
    }


    /* Show per-model breakdown if enabled */
    if (prefs->aitoken_show_model_breakdown) {
        GHashTable *model_tokens = xrg_aitoken_collector_get_model_tokens(widget->collector);
        const gchar *current_model = xrg_aitoken_collector_get_current_model(widget->collector);

        if (model_tokens && g_hash_table_size(model_tokens) > 0) {

            /* Display each model */
            GHashTableIter iter;
            gpointer key, value;
            g_hash_table_iter_init(&iter, model_tokens);

            while (g_hash_table_iter_next(&iter, &key, &value)) {
                const gchar *model_name = (const gchar *)key;
                ModelTokens *tokens = (ModelTokens *)value;
                guint64 model_total = tokens->input_tokens + tokens->output_tokens;

                /* Truncate long model names */
                gchar *display_name = g_strdup(model_name);
                if (strlen(display_name) > 20) {
                    display_name[17] = '.';
                    display_name[18] = '.';
                    display_name[19] = '.';
                    display_name[20] = '\0';
                }

                /* Mark current model with asterisk */
                gchar *model_line;
                if (current_model && g_strcmp0(model_name, current_model) == 0) {
                    model_line = g_strdup_printf("* %s: %lu", display_name, (unsigned long)model_total);
                } else {
                    model_line = g_strdup_printf("  %s: %lu", display_name, (unsigned long)model_total);
                }

                cairo_move_to(cr, 5, next_y);
                cairo_show_text(cr, model_line);
                g_free(model_line);
                g_free(display_name);
                next_y += 12;
            }
        }
    }

    /* Draw activity bar on the right (if enabled) */
    if (prefs->show_activity_bars) {
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

        /* Find max token rate in dataset to scale the bar */
        gdouble bar_max_rate = 1.0;  /* Minimum to avoid division by zero */
        for (gint i = 0; i < count; i++) {
            gdouble input_val = xrg_dataset_get_value(input_dataset, i);
            gdouble output_val = xrg_dataset_get_value(output_dataset, i);
            gdouble gemini_val = xrg_dataset_get_value(gemini_dataset, i);
            gdouble total_val = input_val + output_val + gemini_val;
            if (total_val > bar_max_rate) {
                bar_max_rate = total_val;
            }
        }

        /* Draw filled bar representing current token rate */
        gdouble current_value = token_rate / bar_max_rate;
        if (current_value > 1.0) current_value = 1.0;  /* Cap at 100% */
        gdouble fill_height = current_value * height;
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
}

/**
 * Generate tooltip text for AI token widget
 */
static gchar* aitoken_widget_tooltip(XRGBaseWidget *base, int x, int y) {
    (void)x; (void)y;
    XRGAITokenWidget *widget = (XRGAITokenWidget *)base;
    XRGPreferences *prefs = base->prefs;

    guint64 total_tokens = xrg_aitoken_collector_get_total_tokens(widget->collector);
    guint64 input_tokens = xrg_aitoken_collector_get_input_tokens(widget->collector);
    guint64 output_tokens = xrg_aitoken_collector_get_output_tokens(widget->collector);
    guint64 session_tokens = xrg_aitoken_collector_get_session_tokens(widget->collector);
    gdouble tokens_per_min = xrg_aitoken_collector_get_tokens_per_minute(widget->collector);

    GString *tooltip_str = g_string_new("AI Token Usage\n\n");

    g_string_append_printf(tooltip_str, "Total: %lu tokens\n", (unsigned long)total_tokens);
    g_string_append_printf(tooltip_str, "Input: %lu tokens\n", (unsigned long)input_tokens);
    g_string_append_printf(tooltip_str, "Output: %lu tokens\n", (unsigned long)output_tokens);
    g_string_append_printf(tooltip_str, "Session: %lu tokens\n", (unsigned long)session_tokens);
    g_string_append_printf(tooltip_str, "Rate: %.1f tokens/min\n", tokens_per_min);

    /* Provider breakdown */
    guint64 claude_tokens = xrg_aitoken_collector_get_claude_tokens(widget->collector);
    guint64 codex_tokens = xrg_aitoken_collector_get_codex_tokens(widget->collector);
    guint64 gemini_tokens = xrg_aitoken_collector_get_gemini_tokens(widget->collector);

    g_string_append(tooltip_str, "\n--- By Provider ---\n");
    if (claude_tokens > 0)
        g_string_append_printf(tooltip_str, "Claude: %lu\n", (unsigned long)claude_tokens);
    if (codex_tokens > 0)
        g_string_append_printf(tooltip_str, "Codex: %lu\n", (unsigned long)codex_tokens);
    if (gemini_tokens > 0)
        g_string_append_printf(tooltip_str, "Gemini: %lu\n", (unsigned long)gemini_tokens);

    /* Cost info */
    xrg_aitoken_collector_update_costs(widget->collector, prefs);
    gdouble total_cost = xrg_aitoken_collector_get_total_cost(widget->collector);

    if (total_cost > 0) {
        g_string_append(tooltip_str, "\n--- Cost ---\n");
        if (total_cost < 0.01) {
            g_string_append_printf(tooltip_str, "Total: %.2f\302\242\n", total_cost * 100.0);
        } else {
            g_string_append_printf(tooltip_str, "Total: $%.2f\n", total_cost);
        }
    }

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
static void aitoken_widget_update(XRGBaseWidget *base) {
    (void)base;
    /* Data is updated by collector, just trigger redraw */
}
