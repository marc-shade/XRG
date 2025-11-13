#ifndef XRG_CPU_COLLECTOR_H
#define XRG_CPU_COLLECTOR_H

#include <glib.h>
#include "../core/dataset.h"

/**
 * XRGCPUCollector - CPU usage data collector
 *
 * Collects CPU usage statistics from /proc/stat for:
 * - Per-core utilization
 * - Overall system load
 * - Load averages (from /proc/loadavg)
 * - Process counts
 */

typedef struct _XRGCPUCollector XRGCPUCollector;

typedef struct {
    guint64 user;
    guint64 nice;
    guint64 system;
    guint64 idle;
    guint64 iowait;
    guint64 irq;
    guint64 softirq;
    guint64 steal;
    guint64 guest;
    guint64 guest_nice;
} CPUStats;

struct _XRGCPUCollector {
    /* CPU count */
    gint num_cpus;
    gint num_cores;
    gint num_threads;

    /* Current and previous CPU statistics */
    CPUStats *current_stats;  /* Array of per-core stats */
    CPUStats *previous_stats; /* Array of per-core stats */
    CPUStats current_total;   /* Total across all cores */
    CPUStats previous_total;  /* Previous total */

    /* Datasets for graphing */
    XRGDataset *system_usage;   /* System CPU % */
    XRGDataset *user_usage;     /* User CPU % */
    XRGDataset *nice_usage;     /* Nice CPU % */
    XRGDataset **per_core_usage; /* Per-core CPU % array */

    /* Load averages */
    gdouble load_average_1min;
    gdouble load_average_5min;
    gdouble load_average_15min;

    /* Process counts */
    gint running_processes;
    gint total_processes;

    /* Update tracking */
    gint64 last_update_time;
};

/* Constructor and destructor */
XRGCPUCollector* xrg_cpu_collector_new(gint dataset_capacity);
void xrg_cpu_collector_free(XRGCPUCollector *collector);

/* Update methods */
void xrg_cpu_collector_update(XRGCPUCollector *collector);
gboolean xrg_cpu_collector_fast_update(XRGCPUCollector *collector);

/* Getters */
gint xrg_cpu_collector_get_num_cpus(XRGCPUCollector *collector);
gdouble xrg_cpu_collector_get_total_usage(XRGCPUCollector *collector);
gdouble xrg_cpu_collector_get_core_usage(XRGCPUCollector *collector, gint core);
gdouble xrg_cpu_collector_get_load_average_1min(XRGCPUCollector *collector);
gdouble xrg_cpu_collector_get_load_average_5min(XRGCPUCollector *collector);
gdouble xrg_cpu_collector_get_load_average_15min(XRGCPUCollector *collector);

/* Dataset access */
XRGDataset* xrg_cpu_collector_get_system_dataset(XRGCPUCollector *collector);
XRGDataset* xrg_cpu_collector_get_user_dataset(XRGCPUCollector *collector);
XRGDataset* xrg_cpu_collector_get_core_dataset(XRGCPUCollector *collector, gint core);

#endif /* XRG_CPU_COLLECTOR_H */
