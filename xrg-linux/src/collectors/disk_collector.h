#ifndef XRG_DISK_COLLECTOR_H
#define XRG_DISK_COLLECTOR_H

#include <glib.h>
#include "../core/dataset.h"

/**
 * XRGDiskCollector - Disk I/O data collector
 *
 * Collects disk statistics from /proc/diskstats for:
 * - Bytes read/written per device
 * - Read/write rates
 * - Total I/O activity
 */

#define MAX_DISKS 16
#define DISK_NAME_LEN 32

typedef struct {
    gchar name[DISK_NAME_LEN];
    guint64 reads_completed;
    guint64 writes_completed;
    guint64 sectors_read;
    guint64 sectors_written;
    guint64 prev_sectors_read;
    guint64 prev_sectors_written;
    gdouble read_rate;   /* Bytes per second */
    gdouble write_rate;  /* Bytes per second */
    gboolean active;
} DiskDevice;

typedef struct _XRGDiskCollector XRGDiskCollector;

struct _XRGDiskCollector {
    /* Disk devices */
    DiskDevice devices[MAX_DISKS];
    gint num_devices;
    gint primary_device_idx;  /* Index of primary device */

    /* Datasets for graphing (primary device) */
    XRGDataset *read_rate;    /* Read in MB/s */
    XRGDataset *write_rate;   /* Write in MB/s */

    /* Update tracking */
    gint64 last_update_time;
};

/* Constructor and destructor */
XRGDiskCollector* xrg_disk_collector_new(gint dataset_capacity);
void xrg_disk_collector_free(XRGDiskCollector *collector);

/* Update methods */
void xrg_disk_collector_update(XRGDiskCollector *collector);

/* Getters */
const gchar* xrg_disk_collector_get_primary_device(XRGDiskCollector *collector);
gdouble xrg_disk_collector_get_read_rate(XRGDiskCollector *collector);
gdouble xrg_disk_collector_get_write_rate(XRGDiskCollector *collector);
guint64 xrg_disk_collector_get_total_read(XRGDiskCollector *collector);
guint64 xrg_disk_collector_get_total_written(XRGDiskCollector *collector);

/* Dataset access */
XRGDataset* xrg_disk_collector_get_read_dataset(XRGDiskCollector *collector);
XRGDataset* xrg_disk_collector_get_write_dataset(XRGDiskCollector *collector);

#endif /* XRG_DISK_COLLECTOR_H */
