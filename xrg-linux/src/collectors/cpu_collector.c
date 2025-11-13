#include "cpu_collector.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PROC_STAT "/proc/stat"
#define PROC_LOADAVG "/proc/loadavg"

/* Helper: Get CPU count */
static gint get_cpu_count(void) {
    return (gint)sysconf(_SC_NPROCESSORS_ONLN);
}

/* Helper: Parse /proc/stat line */
static gboolean parse_cpu_stat_line(const gchar *line, CPUStats *stats) {
    if (!g_str_has_prefix(line, "cpu"))
        return FALSE;

    /* Skip "cpu" or "cpuN " prefix */
    const gchar *data = strchr(line, ' ');
    if (data == NULL)
        return FALSE;

    /* Parse up to 10 fields (Linux kernel 2.6.33+) */
    gint parsed = sscanf(data, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                         &stats->user, &stats->nice, &stats->system, &stats->idle,
                         &stats->iowait, &stats->irq, &stats->softirq, &stats->steal,
                         &stats->guest, &stats->guest_nice);

    return (parsed >= 4);  /* At least user, nice, system, idle */
}

/* Helper: Calculate CPU usage percentage */
static gdouble calculate_cpu_usage(CPUStats *current, CPUStats *previous) {
    guint64 prev_idle = previous->idle + previous->iowait;
    guint64 curr_idle = current->idle + current->iowait;

    guint64 prev_non_idle = previous->user + previous->nice + previous->system +
                            previous->irq + previous->softirq + previous->steal;
    guint64 curr_non_idle = current->user + current->nice + current->system +
                            current->irq + current->softirq + current->steal;

    guint64 prev_total = prev_idle + prev_non_idle;
    guint64 curr_total = curr_idle + curr_non_idle;

    guint64 total_diff = curr_total - prev_total;
    guint64 idle_diff = curr_idle - prev_idle;

    if (total_diff == 0)
        return 0.0;

    return (gdouble)(total_diff - idle_diff) / (gdouble)total_diff * 100.0;
}

/**
 * Create new CPU collector
 */
XRGCPUCollector* xrg_cpu_collector_new(gint dataset_capacity) {
    XRGCPUCollector *collector = g_new0(XRGCPUCollector, 1);

    /* Detect CPU count */
    collector->num_cpus = get_cpu_count();
    collector->num_cores = collector->num_cpus;  /* Simplified for now */
    collector->num_threads = collector->num_cpus;

    /* Allocate stats arrays */
    collector->current_stats = g_new0(CPUStats, collector->num_cpus);
    collector->previous_stats = g_new0(CPUStats, collector->num_cpus);

    /* Create datasets for overall usage */
    collector->system_usage = xrg_dataset_new(dataset_capacity);
    collector->user_usage = xrg_dataset_new(dataset_capacity);
    collector->nice_usage = xrg_dataset_new(dataset_capacity);

    /* Create per-core datasets */
    collector->per_core_usage = g_new0(XRGDataset*, collector->num_cpus);
    for (gint i = 0; i < collector->num_cpus; i++) {
        collector->per_core_usage[i] = xrg_dataset_new(dataset_capacity);
    }

    /* Initialize */
    collector->last_update_time = g_get_monotonic_time();

    /* Do initial read to populate previous_stats */
    xrg_cpu_collector_update(collector);

    return collector;
}

/**
 * Free CPU collector
 */
void xrg_cpu_collector_free(XRGCPUCollector *collector) {
    if (collector == NULL)
        return;

    g_free(collector->current_stats);
    g_free(collector->previous_stats);

    xrg_dataset_free(collector->system_usage);
    xrg_dataset_free(collector->user_usage);
    xrg_dataset_free(collector->nice_usage);

    for (gint i = 0; i < collector->num_cpus; i++) {
        xrg_dataset_free(collector->per_core_usage[i]);
    }
    g_free(collector->per_core_usage);

    g_free(collector);
}

/**
 * Update CPU statistics (normal update - 1 second)
 */
void xrg_cpu_collector_update(XRGCPUCollector *collector) {
    g_return_if_fail(collector != NULL);

    /* Read /proc/stat */
    FILE *fp = fopen(PROC_STAT, "r");
    if (fp == NULL) {
        g_warning("Failed to open %s", PROC_STAT);
        return;
    }

    gchar line[256];
    gint cpu_index = 0;

    /* Swap current -> previous */
    CPUStats *temp = collector->previous_stats;
    collector->previous_stats = collector->current_stats;
    collector->current_stats = temp;
    collector->previous_total = collector->current_total;

    while (fgets(line, sizeof(line), fp)) {
        if (g_str_has_prefix(line, "cpu ")) {
            /* Overall CPU stats */
            parse_cpu_stat_line(line, &collector->current_total);
        } else if (g_str_has_prefix(line, "cpu")) {
            /* Per-core stats */
            if (cpu_index < collector->num_cpus) {
                parse_cpu_stat_line(line, &collector->current_stats[cpu_index]);
                cpu_index++;
            }
        } else if (g_str_has_prefix(line, "procs_running")) {
            sscanf(line, "procs_running %d", &collector->running_processes);
        }
    }

    fclose(fp);

    /* Calculate and store CPU usage percentages */
    gdouble total_usage = calculate_cpu_usage(&collector->current_total, &collector->previous_total);

    /* System vs User breakdown (simplified) */
    gdouble system_pct = (gdouble)collector->current_total.system /
                         (collector->current_total.user + collector->current_total.system + 1) * total_usage;
    gdouble user_pct = total_usage - system_pct;

    xrg_dataset_add_value(collector->system_usage, system_pct);
    xrg_dataset_add_value(collector->user_usage, user_pct);
    xrg_dataset_add_value(collector->nice_usage, 0.0);  /* TODO: calculate nice */

    /* Per-core usage */
    for (gint i = 0; i < collector->num_cpus; i++) {
        gdouble core_usage = calculate_cpu_usage(&collector->current_stats[i], &collector->previous_stats[i]);
        xrg_dataset_add_value(collector->per_core_usage[i], core_usage);
    }

    /* Read load averages */
    fp = fopen(PROC_LOADAVG, "r");
    if (fp != NULL) {
        if (fgets(line, sizeof(line), fp)) {
            sscanf(line, "%lf %lf %lf",
                   &collector->load_average_1min,
                   &collector->load_average_5min,
                   &collector->load_average_15min);
        }
        fclose(fp);
    }

    collector->last_update_time = g_get_monotonic_time();
}

/**
 * Fast update (100ms interval - called more frequently)
 */
gboolean xrg_cpu_collector_fast_update(XRGCPUCollector *collector) {
    /* For CPU, we might want to skip fast updates and only use normal updates
     * to avoid too much overhead. Return FALSE to indicate no action taken. */
    return FALSE;
}

/* Getters */

gint xrg_cpu_collector_get_num_cpus(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    return collector->num_cpus;
}

gdouble xrg_cpu_collector_get_total_usage(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);

    gdouble system = xrg_dataset_get_latest(collector->system_usage);
    gdouble user = xrg_dataset_get_latest(collector->user_usage);
    return system + user;
}

gdouble xrg_cpu_collector_get_core_usage(XRGCPUCollector *collector, gint core) {
    g_return_val_if_fail(collector != NULL, 0.0);
    g_return_val_if_fail(core >= 0 && core < collector->num_cpus, 0.0);

    return xrg_dataset_get_latest(collector->per_core_usage[core]);
}

gdouble xrg_cpu_collector_get_load_average_1min(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->load_average_1min;
}

gdouble xrg_cpu_collector_get_load_average_5min(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->load_average_5min;
}

gdouble xrg_cpu_collector_get_load_average_15min(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    return collector->load_average_15min;
}

/* Dataset access */

XRGDataset* xrg_cpu_collector_get_system_dataset(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->system_usage;
}

XRGDataset* xrg_cpu_collector_get_user_dataset(XRGCPUCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->user_usage;
}

XRGDataset* xrg_cpu_collector_get_core_dataset(XRGCPUCollector *collector, gint core) {
    g_return_val_if_fail(collector != NULL, NULL);
    g_return_val_if_fail(core >= 0 && core < collector->num_cpus, NULL);
    return collector->per_core_usage[core];
}
