#ifndef XRG_AITOKEN_PRICING_H
#define XRG_AITOKEN_PRICING_H

#include <glib.h>
#include "../core/preferences.h"

/**
 * AI Token Pricing and Subscription Tier Data
 *
 * Default pricing for various AI models (USD per 1K tokens)
 * Subscription tier caps converted to approximate token equivalents
 *
 * Prices and caps as of November 2025 - update as needed
 *
 * Sources:
 * - Claude: https://anthropic.com/pricing
 *           https://support.claude.com/en/articles/11145838-using-claude-code-with-your-pro-or-max-plan
 * - OpenAI: https://openai.com/pricing
 *           https://help.openai.com/en/articles/11369540-using-codex-with-your-chatgpt-plan
 * - Google: https://ai.google.dev/pricing
 *           https://blog.google/technology/developers/gemini-cli-code-assist-higher-limits/
 *
 * Token Cap Conversions (approximate):
 * - 1 hour of Claude Code usage ≈ 50K-100K tokens (varies by task complexity)
 * - 1 Codex message ≈ 2K-10K tokens (varies by codebase size)
 * - Gemini base daily limit ≈ 100K tokens (estimated)
 */

/* Claude Models (Anthropic) - USD per 1K tokens */
typedef struct {
    const char *model_pattern;  /* Pattern to match model name */
    double input_price;         /* $/1K input tokens */
    double output_price;        /* $/1K output tokens */
} AIModelPricing;

/* Claude pricing table */
static const AIModelPricing CLAUDE_PRICING[] = {
    /* Claude 4.5 (Opus) - Premium model */
    {"claude-opus-4-5",      0.015,  0.075},
    {"claude-4-5-opus",      0.015,  0.075},

    /* Claude 4.5 (Sonnet) - Balanced */
    {"claude-sonnet-4-5",    0.003,  0.015},
    {"claude-4-5-sonnet",    0.003,  0.015},

    /* Claude 3.5 Sonnet - Previous gen */
    {"claude-3-5-sonnet",    0.003,  0.015},
    {"claude-3.5-sonnet",    0.003,  0.015},

    /* Claude 3 Opus */
    {"claude-3-opus",        0.015,  0.075},

    /* Claude 3 Sonnet */
    {"claude-3-sonnet",      0.003,  0.015},

    /* Claude 3 Haiku - Fastest/cheapest */
    {"claude-3-haiku",       0.00025, 0.00125},
    {"claude-3-5-haiku",     0.0008,  0.004},

    /* Default Claude pricing (if model not matched) */
    {"claude",               0.003,  0.015},

    {NULL, 0, 0}  /* Sentinel */
};

/* OpenAI/Codex pricing table */
static const AIModelPricing OPENAI_PRICING[] = {
    /* GPT-4.1 series */
    {"gpt-4.1",              0.002,  0.008},
    {"gpt-4.1-mini",         0.0004, 0.0016},
    {"gpt-4.1-nano",         0.0001, 0.0004},

    /* GPT-4o series */
    {"gpt-4o",               0.0025, 0.01},
    {"gpt-4o-mini",          0.00015, 0.0006},

    /* GPT-4 Turbo */
    {"gpt-4-turbo",          0.01,   0.03},
    {"gpt-4-turbo-preview",  0.01,   0.03},

    /* GPT-4 */
    {"gpt-4-32k",            0.06,   0.12},
    {"gpt-4",                0.03,   0.06},

    /* GPT-3.5 Turbo */
    {"gpt-3.5-turbo",        0.0005, 0.0015},

    /* Codex-specific models */
    {"gpt-5.1-codex",        0.003,  0.012},
    {"codex",                0.002,  0.008},

    /* o1 reasoning models */
    {"o1-preview",           0.015,  0.06},
    {"o1-mini",              0.003,  0.012},
    {"o1",                   0.015,  0.06},

    /* Default OpenAI pricing */
    {"gpt",                  0.002,  0.008},

    {NULL, 0, 0}  /* Sentinel */
};

/* Google Gemini pricing table */
static const AIModelPricing GEMINI_PRICING[] = {
    /* Gemini 2.0 */
    {"gemini-2.0-flash",     0.0,    0.0},      /* Free tier */
    {"gemini-2.0-flash-exp", 0.0,    0.0},

    /* Gemini 1.5 Pro */
    {"gemini-1.5-pro",       0.00125, 0.005},   /* Up to 128K context */
    {"gemini-1.5-pro-128k",  0.00125, 0.005},
    {"gemini-1.5-pro-1m",    0.0025,  0.01},    /* 1M+ context */

    /* Gemini 1.5 Flash */
    {"gemini-1.5-flash",     0.000075, 0.0003}, /* Up to 128K context */
    {"gemini-1.5-flash-128k", 0.000075, 0.0003},
    {"gemini-1.5-flash-1m",  0.00015, 0.0006},  /* 1M+ context */

    /* Gemini 1.0 Pro */
    {"gemini-1.0-pro",       0.0005,  0.0015},
    {"gemini-pro",           0.0005,  0.0015},

    /* Default Gemini pricing */
    {"gemini",               0.0005,  0.002},

    {NULL, 0, 0}  /* Sentinel */
};

/**
 * Get pricing for a model by name
 * Returns TRUE if found, FALSE if using defaults
 */
static inline gboolean get_model_pricing(const char *model_name,
                                         double *input_price,
                                         double *output_price) {
    if (!model_name || !input_price || !output_price) {
        return FALSE;
    }

    /* Check Claude models */
    for (int i = 0; CLAUDE_PRICING[i].model_pattern != NULL; i++) {
        if (g_str_has_prefix(model_name, CLAUDE_PRICING[i].model_pattern) ||
            strstr(model_name, CLAUDE_PRICING[i].model_pattern) != NULL) {
            *input_price = CLAUDE_PRICING[i].input_price;
            *output_price = CLAUDE_PRICING[i].output_price;
            return TRUE;
        }
    }

    /* Check OpenAI/Codex models */
    for (int i = 0; OPENAI_PRICING[i].model_pattern != NULL; i++) {
        if (g_str_has_prefix(model_name, OPENAI_PRICING[i].model_pattern) ||
            strstr(model_name, OPENAI_PRICING[i].model_pattern) != NULL) {
            *input_price = OPENAI_PRICING[i].input_price;
            *output_price = OPENAI_PRICING[i].output_price;
            return TRUE;
        }
    }

    /* Check Gemini models */
    for (int i = 0; GEMINI_PRICING[i].model_pattern != NULL; i++) {
        if (g_str_has_prefix(model_name, GEMINI_PRICING[i].model_pattern) ||
            strstr(model_name, GEMINI_PRICING[i].model_pattern) != NULL) {
            *input_price = GEMINI_PRICING[i].input_price;
            *output_price = GEMINI_PRICING[i].output_price;
            return TRUE;
        }
    }

    /* Default fallback pricing */
    *input_price = 0.003;   /* $3/1M input */
    *output_price = 0.015;  /* $15/1M output */
    return FALSE;
}

/**
 * Calculate cost for token usage
 * Returns cost in USD
 */
static inline double calculate_token_cost(guint64 input_tokens,
                                          guint64 output_tokens,
                                          double input_price_per_1k,
                                          double output_price_per_1k) {
    double input_cost = (input_tokens / 1000.0) * input_price_per_1k;
    double output_cost = (output_tokens / 1000.0) * output_price_per_1k;
    return input_cost + output_cost;
}

/**
 * Format cost as human-readable string
 * Caller must free the returned string
 */
static inline gchar* format_cost(double cost_usd) {
    if (cost_usd < 0.01) {
        /* Show in cents for small amounts */
        return g_strdup_printf("%.2f¢", cost_usd * 100.0);
    } else if (cost_usd < 1.0) {
        return g_strdup_printf("$%.2f", cost_usd);
    } else if (cost_usd < 100.0) {
        return g_strdup_printf("$%.2f", cost_usd);
    } else {
        return g_strdup_printf("$%.0f", cost_usd);
    }
}

/**
 * Format cap usage as percentage string
 * Caller must free the returned string
 */
static inline gchar* format_cap_usage(guint64 used, guint64 cap) {
    if (cap == 0) {
        return g_strdup("∞");
    }
    double pct = (double)used / cap * 100.0;
    if (pct > 100.0) {
        return g_strdup_printf("%.0f%% OVER", pct);
    }
    return g_strdup_printf("%.1f%%", pct);
}

/*
 * ============================================================================
 * Subscription Tier Token Caps
 *
 * These are WEEKLY token caps (approximate) based on subscription tier.
 * Conversions are estimates since providers measure in hours/messages, not tokens.
 *
 * Update these values when providers change their limits!
 * Last updated: November 2025
 * ============================================================================
 */

/**
 * Get weekly token cap for Claude subscription tier
 *
 * Based on:
 * - Pro: 40-80 hours Sonnet/week → ~3M-6M tokens (using 75K tokens/hour avg)
 * - Max 5x: 140-280 hours Sonnet/week → ~10M-21M tokens
 * - Max 20x: 240-480 hours Sonnet/week → ~18M-36M tokens
 *
 * We use conservative midpoint estimates.
 */
static inline guint64 get_claude_tier_weekly_cap(XRGClaudeTier tier) {
    switch (tier) {
        case XRG_CLAUDE_TIER_PRO:
            return 4500000;    /* ~4.5M tokens/week (60 hrs × 75K) */
        case XRG_CLAUDE_TIER_MAX_5X:
            return 15750000;   /* ~15.75M tokens/week (210 hrs × 75K) */
        case XRG_CLAUDE_TIER_MAX_20X:
            return 27000000;   /* ~27M tokens/week (360 hrs × 75K) */
        case XRG_CLAUDE_TIER_API:
        default:
            return 0;          /* Unlimited for API (pay-per-use) */
    }
}

/**
 * Get weekly token cap for Codex subscription tier
 *
 * Based on:
 * - Plus: 30-150 msg/5hrs, ~3K thinking req/week → ~1.5M-3M tokens
 * - Pro: 300-1500 msg/5hrs → ~6M-15M tokens
 *
 * Using 5K tokens/message average estimate.
 */
static inline guint64 get_codex_tier_weekly_cap(XRGCodexTier tier) {
    switch (tier) {
        case XRG_CODEX_TIER_PLUS:
            return 2250000;    /* ~2.25M tokens/week */
        case XRG_CODEX_TIER_PRO:
            return 10500000;   /* ~10.5M tokens/week */
        case XRG_CODEX_TIER_API:
        default:
            return 0;          /* Unlimited for API (pay-per-use) */
    }
}

/**
 * Get daily token cap for Gemini subscription tier
 *
 * Based on multipliers (exact base not documented):
 * - Free: ~100K tokens/day (estimated base)
 * - Pro (5x): ~500K tokens/day
 * - Ultra (20x): ~2M tokens/day
 *
 * Note: Gemini uses DAILY limits, not weekly!
 */
static inline guint64 get_gemini_tier_daily_cap(XRGGeminiTier tier) {
    switch (tier) {
        case XRG_GEMINI_TIER_FREE:
            return 100000;     /* ~100K tokens/day (estimated) */
        case XRG_GEMINI_TIER_PRO:
            return 500000;     /* ~500K tokens/day (5x free) */
        case XRG_GEMINI_TIER_ULTRA:
            return 2000000;    /* ~2M tokens/day (20x free) */
        case XRG_GEMINI_TIER_API:
        default:
            return 0;          /* Unlimited for API (pay-per-use) */
    }
}

/**
 * Get tier name as human-readable string
 */
static inline const gchar* get_claude_tier_name(XRGClaudeTier tier) {
    switch (tier) {
        case XRG_CLAUDE_TIER_PRO:     return "Pro";
        case XRG_CLAUDE_TIER_MAX_5X:  return "Max 5x";
        case XRG_CLAUDE_TIER_MAX_20X: return "Max 20x";
        case XRG_CLAUDE_TIER_API:     return "API";
        default:                       return "Unknown";
    }
}

static inline const gchar* get_codex_tier_name(XRGCodexTier tier) {
    switch (tier) {
        case XRG_CODEX_TIER_PLUS: return "Plus";
        case XRG_CODEX_TIER_PRO:  return "Pro";
        case XRG_CODEX_TIER_API:  return "API";
        default:                   return "Unknown";
    }
}

static inline const gchar* get_gemini_tier_name(XRGGeminiTier tier) {
    switch (tier) {
        case XRG_GEMINI_TIER_FREE:  return "Free";
        case XRG_GEMINI_TIER_PRO:   return "Pro";
        case XRG_GEMINI_TIER_ULTRA: return "Ultra";
        case XRG_GEMINI_TIER_API:   return "API";
        default:                     return "Unknown";
    }
}

/**
 * Format tokens as human-readable string (K/M suffix)
 * Caller must free the returned string
 */
static inline gchar* format_tokens(guint64 tokens) {
    if (tokens >= 1000000) {
        return g_strdup_printf("%.1fM", tokens / 1000000.0);
    } else if (tokens >= 1000) {
        return g_strdup_printf("%.0fK", tokens / 1000.0);
    } else {
        return g_strdup_printf("%lu", (unsigned long)tokens);
    }
}

#endif /* XRG_AITOKEN_PRICING_H */
