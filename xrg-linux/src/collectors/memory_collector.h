#ifndef XRG_MEMORY_COLLECTOR_H
#define XRG_MEMORY_COLLECTOR_H

#include <glib.h>
#include "../core/dataset.h"

/**
 * XRGMemoryCollector - Memory usage data collector
 *
 * Collects memory statistics from /proc/meminfo and /proc/vmstat for:
 * - Total/Free/Available memory
 * - Used memory breakdown (apps, wired, compressed)
 * - Swap usage
 * - Page in/out activity
 */

typedef struct _XRGMemoryCollector XRGMemoryCollector;

struct _XRGMemoryCollector {
    /* Memory totals (bytes) */
    guint64 mem_total;
    guint64 mem_free;
    guint64 mem_available;
    guint64 mem_buffers;
    guint64 mem_cached;
    guint64 mem_slab;
    guint64 mem_used;  /* Calculated */

    /* Swap (bytes) */
    guint64 swap_total;
    guint64 swap_free;
    guint64 swap_used;  /* Calculated */

    /* Page activity */
    guint64 page_in;
    guint64 page_out;
    guint64 prev_page_in;
    guint64 prev_page_out;

    /* Datasets for graphing */
    XRGDataset *used_memory;      /* App memory */
    XRGDataset *wired_memory;     /* Kernel/buffers */
    XRGDataset *cached_memory;    /* Cache */
    XRGDataset *swap_memory;      /* Swap usage */
    XRGDataset *page_activity;    /* Page in/out rate */

    /* Update tracking */
    gint64 last_update_time;
};

/* Constructor and destructor */
XRGMemoryCollector* xrg_memory_collector_new(gint dataset_capacity);
void xrg_memory_collector_free(XRGMemoryCollector *collector);

/* Update methods */
void xrg_memory_collector_update(XRGMemoryCollector *collector);

/* Getters */
guint64 xrg_memory_collector_get_total_memory(XRGMemoryCollector *collector);
guint64 xrg_memory_collector_get_used_memory(XRGMemoryCollector *collector);
guint64 xrg_memory_collector_get_free_memory(XRGMemoryCollector *collector);
gdouble xrg_memory_collector_get_used_percentage(XRGMemoryCollector *collector);
guint64 xrg_memory_collector_get_swap_used(XRGMemoryCollector *collector);

/* Dataset access */
XRGDataset* xrg_memory_collector_get_used_dataset(XRGMemoryCollector *collector);
XRGDataset* xrg_memory_collector_get_wired_dataset(XRGMemoryCollector *collector);
XRGDataset* xrg_memory_collector_get_cached_dataset(XRGMemoryCollector *collector);
XRGDataset* xrg_memory_collector_get_swap_dataset(XRGMemoryCollector *collector);

#endif /* XRG_MEMORY_COLLECTOR_H */
