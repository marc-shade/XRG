#include "disk_collector.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define PROC_DISKSTATS "/proc/diskstats"
#define SECTOR_SIZE 512  /* Standard sector size in bytes */

/**
 * Check if device name is a whole disk (not a partition)
 * Examples: sda, nvme0n1, vda are whole disks
 *           sda1, nvme0n1p1, vda1 are partitions
 */
static gboolean is_whole_disk(const gchar *name) {
    gint len = strlen(name);
    if (len == 0)
        return FALSE;

    /* Check if last character is a digit - if so, likely a partition */
    gchar last_char = name[len - 1];

    /* Special handling for nvme devices (nvme0n1 is whole, nvme0n1p1 is partition) */
    if (g_str_has_prefix(name, "nvme")) {
        /* nvme0n1 is whole disk, nvme0n1p1 is partition */
        return !strstr(name, "p");  /* Has 'p' means partition */
    }

    /* md RAID arrays (md0, md127) are whole disks; md0p1 is a partition */
    if (g_str_has_prefix(name, "md") && isdigit(name[2])) {
        return !strstr(name + 2, "p");
    }

    /* For regular disks (sd*, vd*, hd*), check if ends with digit */
    if (isdigit(last_char))
        return FALSE;  /* Partition */

    return TRUE;  /* Whole disk */
}

/**
 * Find primary disk device (first whole disk with activity)
 */
static gint find_primary_disk(XRGDiskCollector *collector) {
    /* Prefer the whole disk with the highest current I/O rate, so the graph
     * follows the active device (e.g. an md RAID array under load) instead
     * of being pinned to whichever disk appears first in /proc/diskstats */
    gint busiest = -1;
    gdouble busiest_rate = 0.0;
    for (gint i = 0; i < collector->num_devices; i++) {
        DiskDevice *disk = &collector->devices[i];

        if (!is_whole_disk(disk->name))
            continue;

        gdouble rate = disk->read_rate + disk->write_rate;
        if (rate > busiest_rate) {
            busiest_rate = rate;
            busiest = i;
        }
    }
    if (busiest >= 0)
        return busiest;

    /* Idle system: keep the previous primary to avoid flapping */
    if (collector->primary_device_idx >= 0 &&
        collector->primary_device_idx < collector->num_devices &&
        is_whole_disk(collector->devices[collector->primary_device_idx].name))
        return collector->primary_device_idx;

    /* Look for first whole disk with I/O activity */
    for (gint i = 0; i < collector->num_devices; i++) {
        DiskDevice *disk = &collector->devices[i];

        if (!is_whole_disk(disk->name))
            continue;

        /* Check if disk has any activity */
        if (disk->sectors_read > 0 || disk->sectors_written > 0) {
            return i;
        }
    }

    /* Fallback to first whole disk */
    for (gint i = 0; i < collector->num_devices; i++) {
        if (is_whole_disk(collector->devices[i].name))
            return i;
    }

    return 0;  /* Last resort */
}

/**
 * Create new disk collector
 */
XRGDiskCollector* xrg_disk_collector_new(gint dataset_capacity) {
    XRGDiskCollector *collector = g_new0(XRGDiskCollector, 1);

    /* Create datasets */
    collector->read_rate = xrg_dataset_new(dataset_capacity);
    collector->write_rate = xrg_dataset_new(dataset_capacity);

    /* Initialize */
    collector->num_devices = 0;
    collector->primary_device_idx = 0;
    collector->last_update_time = g_get_monotonic_time();

    /* Do initial read */
    xrg_disk_collector_update(collector);

    return collector;
}

/**
 * Free disk collector
 */
void xrg_disk_collector_free(XRGDiskCollector *collector) {
    if (collector == NULL)
        return;

    xrg_dataset_free(collector->read_rate);
    xrg_dataset_free(collector->write_rate);

    g_free(collector);
}

/**
 * Update disk statistics
 */
void xrg_disk_collector_update(XRGDiskCollector *collector) {
    g_return_if_fail(collector != NULL);

    /* Calculate time delta */
    gint64 current_time = g_get_monotonic_time();
    gdouble time_delta = (current_time - collector->last_update_time) / 1000000.0;  /* seconds */

    /* Read /proc/diskstats */
    FILE *fp = fopen(PROC_DISKSTATS, "r");
    if (fp == NULL) {
        g_warning("Failed to open %s", PROC_DISKSTATS);
        return;
    }

    gchar line[256];
    gint disk_idx = 0;

    /* Parse disk lines */
    while (fgets(line, sizeof(line), fp) && disk_idx < MAX_DISKS) {
        DiskDevice *disk = &collector->devices[disk_idx];

        /* Format: major minor device reads_completed reads_merged sectors_read time_reading
                   writes_completed writes_merged sectors_written time_writing ... */
        gint major, minor;
        gchar device_name[DISK_NAME_LEN];
        guint64 reads_completed, reads_merged, sectors_read, time_reading;
        guint64 writes_completed, writes_merged, sectors_written, time_writing;

        gint parsed = sscanf(line, "%d %d %31s %lu %lu %lu %lu %lu %lu %lu %lu",
                           &major, &minor, device_name,
                           &reads_completed, &reads_merged, &sectors_read, &time_reading,
                           &writes_completed, &writes_merged, &sectors_written, &time_writing);

        if (parsed >= 11) {
            /* Only track whole disks, not partitions */
            if (!is_whole_disk(device_name))
                continue;

            /* First sighting: no previous sample, so a rate would wrongly
             * count all I/O since boot as one interval */
            gboolean first_sight = !disk->active;

            /* Save previous values */
            disk->prev_sectors_read = disk->sectors_read;
            disk->prev_sectors_written = disk->sectors_written;

            /* Update current values */
            g_strlcpy(disk->name, device_name, DISK_NAME_LEN);
            disk->reads_completed = reads_completed;
            disk->writes_completed = writes_completed;
            disk->sectors_read = sectors_read;
            disk->sectors_written = sectors_written;
            disk->active = TRUE;

            /* Calculate rates (bytes per second) */
            if (time_delta > 0 && !first_sight) {
                guint64 read_delta = disk->sectors_read - disk->prev_sectors_read;
                guint64 write_delta = disk->sectors_written - disk->prev_sectors_written;

                disk->read_rate = (gdouble)(read_delta * SECTOR_SIZE) / time_delta;
                disk->write_rate = (gdouble)(write_delta * SECTOR_SIZE) / time_delta;
            } else {
                disk->read_rate = 0.0;
                disk->write_rate = 0.0;
            }

            disk_idx++;
        }
    }

    fclose(fp);
    collector->num_devices = disk_idx;

    /* Identify primary disk */
    collector->primary_device_idx = find_primary_disk(collector);

    /* Store primary disk rates in datasets (convert to MB/s) */
    if (collector->num_devices > 0 && time_delta > 0) {
        DiskDevice *primary = &collector->devices[collector->primary_device_idx];

        gdouble read_mbps = primary->read_rate / (1024.0 * 1024.0);
        gdouble write_mbps = primary->write_rate / (1024.0 * 1024.0);

        xrg_dataset_add_value(collector->read_rate, read_mbps);
        xrg_dataset_add_value(collector->write_rate, write_mbps);
    }

    collector->last_update_time = current_time;
}

/* Getters */

const gchar* xrg_disk_collector_get_primary_device(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, "");
    if (collector->num_devices == 0)
        return "";
    return collector->devices[collector->primary_device_idx].name;
}

gdouble xrg_disk_collector_get_read_rate(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (collector->num_devices == 0)
        return 0.0;
    return collector->devices[collector->primary_device_idx].read_rate / (1024.0 * 1024.0);
}

gdouble xrg_disk_collector_get_write_rate(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (collector->num_devices == 0)
        return 0.0;
    return collector->devices[collector->primary_device_idx].write_rate / (1024.0 * 1024.0);
}

guint64 xrg_disk_collector_get_total_read(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    if (collector->num_devices == 0)
        return 0;
    return collector->devices[collector->primary_device_idx].sectors_read * SECTOR_SIZE;
}

guint64 xrg_disk_collector_get_total_written(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    if (collector->num_devices == 0)
        return 0;
    return collector->devices[collector->primary_device_idx].sectors_written * SECTOR_SIZE;
}

/* Dataset access */

XRGDataset* xrg_disk_collector_get_read_dataset(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->read_rate;
}

XRGDataset* xrg_disk_collector_get_write_dataset(XRGDiskCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->write_rate;
}
