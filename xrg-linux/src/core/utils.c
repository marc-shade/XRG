#include "utils.h"
#include <math.h>

/**
 * Format bytes with appropriate unit (B, KB, MB, GB)
 */
gchar* xrg_format_bytes(guint64 bytes) {
    if (bytes < 1024)
        return g_strdup_printf("%lu B", bytes);
    else if (bytes < 1024 * 1024)
        return g_strdup_printf("%.1f KB", bytes / 1024.0);
    else if (bytes < 1024 * 1024 * 1024)
        return g_strdup_printf("%.1f MB", bytes / (1024.0 * 1024.0));
    else
        return g_strdup_printf("%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
}

/**
 * Format rate (bytes/sec) with appropriate unit
 */
gchar* xrg_format_rate(gdouble rate) {
    if (rate < 1024)
        return g_strdup_printf("%.0f B/s", rate);
    else if (rate < 1024 * 1024)
        return g_strdup_printf("%.1f KB/s", rate / 1024.0);
    else
        return g_strdup_printf("%.2f MB/s", rate / (1024.0 * 1024.0));
}

/**
 * Format percentage with one decimal place
 */
gchar* xrg_format_percentage(gdouble percentage) {
    return g_strdup_printf("%.1f%%", percentage);
}

/**
 * Format time duration in human-readable format
 */
gchar* xrg_format_time_duration(guint seconds) {
    if (seconds < 60)
        return g_strdup_printf("%us", seconds);
    else if (seconds < 3600)
        return g_strdup_printf("%um %us", seconds / 60, seconds % 60);
    else
        return g_strdup_printf("%uh %um", seconds / 3600, (seconds % 3600) / 60);
}

/**
 * Check if file exists
 */
gboolean xrg_file_exists(const gchar *path) {
    return g_file_test(path, G_FILE_TEST_EXISTS);
}

/**
 * Read entire file contents
 */
gchar* xrg_read_file_contents(const gchar *path, gsize *length) {
    GError *error = NULL;
    gchar *contents = NULL;

    if (!g_file_get_contents(path, &contents, length, &error)) {
        g_warning("Failed to read file %s: %s", path, error->message);
        g_error_free(error);
        return NULL;
    }

    return contents;
}

/**
 * Clamp value between min and max
 */
gdouble xrg_clamp(gdouble value, gdouble min, gdouble max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * Round double to integer
 */
gint xrg_round_to_int(gdouble value) {
    return (gint)round(value);
}
