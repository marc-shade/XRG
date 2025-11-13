#include "memory_collector.h"
#include <stdio.h>
#include <string.h>

#define PROC_MEMINFO "/proc/meminfo"
#define PROC_VMSTAT "/proc/vmstat"

/* Helper: Parse meminfo value in kB */
static guint64 parse_meminfo_kb(const gchar *line) {
    guint64 value = 0;
    gchar unit[16] = {0};

    /* Format: "MemTotal:       131886844 kB" */
    if (sscanf(line, "%*s %lu %15s", &value, unit) >= 1) {
        return value * 1024;  /* Convert kB to bytes */
    }
    return 0;
}

/**
 * Create new memory collector
 */
XRGMemoryCollector* xrg_memory_collector_new(gint dataset_capacity) {
    XRGMemoryCollector *collector = g_new0(XRGMemoryCollector, 1);

    /* Create datasets */
    collector->used_memory = xrg_dataset_new(dataset_capacity);
    collector->wired_memory = xrg_dataset_new(dataset_capacity);
    collector->cached_memory = xrg_dataset_new(dataset_capacity);
    collector->swap_memory = xrg_dataset_new(dataset_capacity);
    collector->page_activity = xrg_dataset_new(dataset_capacity);

    /* Initialize */
    collector->last_update_time = g_get_monotonic_time();

    /* Do initial read */
    xrg_memory_collector_update(collector);

    return collector;
}

/**
 * Free memory collector
 */
void xrg_memory_collector_free(XRGMemoryCollector *collector) {
    if (collector == NULL)
        return;

    xrg_dataset_free(collector->used_memory);
    xrg_dataset_free(collector->wired_memory);
    xrg_dataset_free(collector->cached_memory);
    xrg_dataset_free(collector->swap_memory);
    xrg_dataset_free(collector->page_activity);

    g_free(collector);
}

/**
 * Update memory statistics
 */
void xrg_memory_collector_update(XRGMemoryCollector *collector) {
    g_return_if_fail(collector != NULL);

    /* Read /proc/meminfo */
    FILE *fp = fopen(PROC_MEMINFO, "r");
    if (fp == NULL) {
        g_warning("Failed to open %s", PROC_MEMINFO);
        return;
    }

    gchar line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (g_str_has_prefix(line, "MemTotal:")) {
            collector->mem_total = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "MemFree:")) {
            collector->mem_free = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "MemAvailable:")) {
            collector->mem_available = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "Buffers:")) {
            collector->mem_buffers = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "Cached:")) {
            collector->mem_cached = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "Slab:")) {
            collector->mem_slab = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "SwapTotal:")) {
            collector->swap_total = parse_meminfo_kb(line);
        } else if (g_str_has_prefix(line, "SwapFree:")) {
            collector->swap_free = parse_meminfo_kb(line);
        }
    }

    fclose(fp);

    /* Calculate used memory */
    collector->mem_used = collector->mem_total - collector->mem_available;
    collector->swap_used = collector->swap_total - collector->swap_free;

    /* Read /proc/vmstat for page activity */
    fp = fopen(PROC_VMSTAT, "r");
    if (fp != NULL) {
        collector->prev_page_in = collector->page_in;
        collector->prev_page_out = collector->page_out;

        while (fgets(line, sizeof(line), fp)) {
            if (g_str_has_prefix(line, "pgpgin ")) {
                sscanf(line, "pgpgin %lu", &collector->page_in);
            } else if (g_str_has_prefix(line, "pgpgout ")) {
                sscanf(line, "pgpgout %lu", &collector->page_out);
            }
        }

        fclose(fp);
    }

    /* Calculate percentages and store in datasets */
    gdouble total_gb = collector->mem_total / (1024.0 * 1024.0 * 1024.0);

    /* Used memory (apps) - simplified as total - available */
    gdouble used_gb = collector->mem_used / (1024.0 * 1024.0 * 1024.0);
    xrg_dataset_add_value(collector->used_memory, used_gb / total_gb * 100.0);

    /* Wired memory (buffers + slab) */
    gdouble wired_gb = (collector->mem_buffers + collector->mem_slab) / (1024.0 * 1024.0 * 1024.0);
    xrg_dataset_add_value(collector->wired_memory, wired_gb / total_gb * 100.0);

    /* Cached memory */
    gdouble cached_gb = collector->mem_cached / (1024.0 * 1024.0 * 1024.0);
    xrg_dataset_add_value(collector->cached_memory, cached_gb / total_gb * 100.0);

    /* Swap usage */
    if (collector->swap_total > 0) {
        gdouble swap_pct = (gdouble)collector->swap_used / collector->swap_total * 100.0;
        xrg_dataset_add_value(collector->swap_memory, swap_pct);
    } else {
        xrg_dataset_add_value(collector->swap_memory, 0.0);
    }

    /* Page activity (delta) */
    guint64 page_delta = (collector->page_in - collector->prev_page_in) +
                         (collector->page_out - collector->prev_page_out);
    xrg_dataset_add_value(collector->page_activity, (gdouble)page_delta);

    collector->last_update_time = g_get_monotonic_time();
}

/* Getters */

guint64 xrg_memory_collector_get_total_memory(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->mem_total;
}

guint64 xrg_memory_collector_get_used_memory(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->mem_used;
}

guint64 xrg_memory_collector_get_free_memory(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->mem_available;
}

gdouble xrg_memory_collector_get_used_percentage(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (collector->mem_total == 0)
        return 0.0;
    return (gdouble)collector->mem_used / collector->mem_total * 100.0;
}

guint64 xrg_memory_collector_get_swap_used(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->swap_used;
}

/* Dataset access */

XRGDataset* xrg_memory_collector_get_used_dataset(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->used_memory;
}

XRGDataset* xrg_memory_collector_get_wired_dataset(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->wired_memory;
}

XRGDataset* xrg_memory_collector_get_cached_dataset(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->cached_memory;
}

XRGDataset* xrg_memory_collector_get_swap_dataset(XRGMemoryCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->swap_memory;
}
