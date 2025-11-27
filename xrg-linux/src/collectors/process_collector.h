/**
 * XRG Process Collector Header
 *
 * Collects information about running processes for display in the
 * process monitor widget. Shows top processes by CPU and memory usage.
 */

#ifndef XRG_PROCESS_COLLECTOR_H
#define XRG_PROCESS_COLLECTOR_H

#include <glib.h>
#include <stdint.h>

/**
 * Process information structure
 */
typedef struct {
    pid_t pid;              /* Process ID */
    gchar *name;            /* Process name (from comm) */
    gchar *cmdline;         /* Command line (truncated) */
    gchar state;            /* Process state (R, S, D, Z, etc.) */
    gdouble cpu_percent;    /* CPU usage percentage */
    gdouble mem_percent;    /* Memory usage percentage */
    guint64 mem_rss;        /* Resident set size in bytes */
    guint64 mem_vsize;      /* Virtual memory size in bytes */
    uid_t uid;              /* User ID */
    gchar *username;        /* Username */
    guint64 utime;          /* User time (jiffies) */
    guint64 stime;          /* System time (jiffies) */
    guint64 start_time;     /* Start time (jiffies since boot) */
    gint nice;              /* Nice value */
    gint threads;           /* Number of threads */
} XRGProcessInfo;

/**
 * Sort criteria for process list
 */
typedef enum {
    XRG_PROCESS_SORT_CPU,       /* Sort by CPU usage (default) */
    XRG_PROCESS_SORT_MEMORY,    /* Sort by memory usage */
    XRG_PROCESS_SORT_PID,       /* Sort by PID */
    XRG_PROCESS_SORT_NAME       /* Sort by name */
} XRGProcessSortBy;

typedef struct _XRGProcessCollector XRGProcessCollector;

/* Constructor and destructor */
XRGProcessCollector* xrg_process_collector_new(gint max_processes);
void xrg_process_collector_free(XRGProcessCollector *collector);

/* Update process list */
void xrg_process_collector_update(XRGProcessCollector *collector);

/* Get process list */
GList* xrg_process_collector_get_processes(XRGProcessCollector *collector);
gint xrg_process_collector_get_process_count(XRGProcessCollector *collector);

/* Get specific process info (returns NULL if not found) */
XRGProcessInfo* xrg_process_collector_get_process(XRGProcessCollector *collector, pid_t pid);

/* Sorting */
void xrg_process_collector_set_sort_by(XRGProcessCollector *collector, XRGProcessSortBy sort_by);
XRGProcessSortBy xrg_process_collector_get_sort_by(XRGProcessCollector *collector);
void xrg_process_collector_set_sort_descending(XRGProcessCollector *collector, gboolean descending);
gboolean xrg_process_collector_get_sort_descending(XRGProcessCollector *collector);

/* Filtering */
void xrg_process_collector_set_show_all_users(XRGProcessCollector *collector, gboolean show_all);
gboolean xrg_process_collector_get_show_all_users(XRGProcessCollector *collector);
void xrg_process_collector_set_filter(XRGProcessCollector *collector, const gchar *filter);
const gchar* xrg_process_collector_get_filter(XRGProcessCollector *collector);

/* System totals */
gint xrg_process_collector_get_total_processes(XRGProcessCollector *collector);
gint xrg_process_collector_get_running_processes(XRGProcessCollector *collector);
guint64 xrg_process_collector_get_total_memory(XRGProcessCollector *collector);
gdouble xrg_process_collector_get_uptime_seconds(XRGProcessCollector *collector);

/* Process info helpers */
void xrg_process_info_free(XRGProcessInfo *info);
XRGProcessInfo* xrg_process_info_copy(const XRGProcessInfo *info);

/* State name lookup */
const gchar* xrg_process_state_name(gchar state);

#endif /* XRG_PROCESS_COLLECTOR_H */
