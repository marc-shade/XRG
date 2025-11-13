#include "network_collector.h"
#include <stdio.h>
#include <string.h>

#define PROC_NET_DEV "/proc/net/dev"

/**
 * Find primary network interface (first non-loopback with traffic)
 */
static gint find_primary_interface(XRGNetworkCollector *collector) {
    /* Look for first active non-loopback interface with traffic */
    for (gint i = 0; i < collector->num_interfaces; i++) {
        NetworkInterface *iface = &collector->interfaces[i];

        /* Skip loopback */
        if (g_str_has_prefix(iface->name, "lo"))
            continue;

        /* Check if interface has any traffic */
        if (iface->rx_bytes > 0 || iface->tx_bytes > 0) {
            return i;
        }
    }

    /* Fallback to first non-loopback */
    for (gint i = 0; i < collector->num_interfaces; i++) {
        if (!g_str_has_prefix(collector->interfaces[i].name, "lo"))
            return i;
    }

    return 0;  /* Last resort - use first interface */
}

/**
 * Create new network collector
 */
XRGNetworkCollector* xrg_network_collector_new(gint dataset_capacity) {
    XRGNetworkCollector *collector = g_new0(XRGNetworkCollector, 1);

    /* Create datasets */
    collector->download_rate = xrg_dataset_new(dataset_capacity);
    collector->upload_rate = xrg_dataset_new(dataset_capacity);

    /* Initialize */
    collector->num_interfaces = 0;
    collector->primary_interface_idx = 0;
    collector->last_update_time = g_get_monotonic_time();

    /* Do initial read */
    xrg_network_collector_update(collector);

    return collector;
}

/**
 * Free network collector
 */
void xrg_network_collector_free(XRGNetworkCollector *collector) {
    if (collector == NULL)
        return;

    xrg_dataset_free(collector->download_rate);
    xrg_dataset_free(collector->upload_rate);

    g_free(collector);
}

/**
 * Update network statistics
 */
void xrg_network_collector_update(XRGNetworkCollector *collector) {
    g_return_if_fail(collector != NULL);

    /* Calculate time delta */
    gint64 current_time = g_get_monotonic_time();
    gdouble time_delta = (current_time - collector->last_update_time) / 1000000.0;  /* seconds */

    /* Read /proc/net/dev */
    FILE *fp = fopen(PROC_NET_DEV, "r");
    if (fp == NULL) {
        g_warning("Failed to open %s", PROC_NET_DEV);
        return;
    }

    gchar line[256];
    gint iface_idx = 0;

    /* Skip header lines */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    /* Parse interface lines */
    while (fgets(line, sizeof(line), fp) && iface_idx < MAX_INTERFACES) {
        NetworkInterface *iface = &collector->interfaces[iface_idx];

        /* Parse line: "  eth0: 12345 67890 ... 98765 43210 ..." */
        gchar iface_name[INTERFACE_NAME_LEN];
        guint64 rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
        guint64 tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;

        gint parsed = sscanf(line, " %31[^:]: %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                           iface_name,
                           &rx_bytes, &rx_packets, &rx_errs, &rx_drop, &rx_fifo, &rx_frame, &rx_compressed, &rx_multicast,
                           &tx_bytes, &tx_packets, &tx_errs, &tx_drop, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed);

        if (parsed >= 9) {  /* At least interface name + RX + TX bytes */
            /* Save previous values */
            iface->prev_rx_bytes = iface->rx_bytes;
            iface->prev_tx_bytes = iface->tx_bytes;

            /* Update current values */
            g_strlcpy(iface->name, iface_name, INTERFACE_NAME_LEN);
            iface->rx_bytes = rx_bytes;
            iface->tx_bytes = tx_bytes;
            iface->active = TRUE;

            /* Calculate rates (bytes per second) */
            if (time_delta > 0) {
                guint64 rx_delta = iface->rx_bytes - iface->prev_rx_bytes;
                guint64 tx_delta = iface->tx_bytes - iface->prev_tx_bytes;

                iface->rx_rate = (gdouble)rx_delta / time_delta;
                iface->tx_rate = (gdouble)tx_delta / time_delta;
            }

            iface_idx++;
        }
    }

    fclose(fp);
    collector->num_interfaces = iface_idx;

    /* Identify primary interface */
    collector->primary_interface_idx = find_primary_interface(collector);

    /* Store primary interface rates in datasets (convert to MB/s) */
    if (collector->num_interfaces > 0 && time_delta > 0) {
        NetworkInterface *primary = &collector->interfaces[collector->primary_interface_idx];

        gdouble download_mbps = primary->rx_rate / (1024.0 * 1024.0);
        gdouble upload_mbps = primary->tx_rate / (1024.0 * 1024.0);

        xrg_dataset_add_value(collector->download_rate, download_mbps);
        xrg_dataset_add_value(collector->upload_rate, upload_mbps);
    }

    collector->last_update_time = current_time;
}

/* Getters */

const gchar* xrg_network_collector_get_primary_interface(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, "");
    if (collector->num_interfaces == 0)
        return "";
    return collector->interfaces[collector->primary_interface_idx].name;
}

gdouble xrg_network_collector_get_download_rate(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (collector->num_interfaces == 0)
        return 0.0;
    return collector->interfaces[collector->primary_interface_idx].rx_rate / (1024.0 * 1024.0);
}

gdouble xrg_network_collector_get_upload_rate(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0.0);
    if (collector->num_interfaces == 0)
        return 0.0;
    return collector->interfaces[collector->primary_interface_idx].tx_rate / (1024.0 * 1024.0);
}

guint64 xrg_network_collector_get_total_rx(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    if (collector->num_interfaces == 0)
        return 0;
    return collector->interfaces[collector->primary_interface_idx].rx_bytes;
}

guint64 xrg_network_collector_get_total_tx(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, 0);
    if (collector->num_interfaces == 0)
        return 0;
    return collector->interfaces[collector->primary_interface_idx].tx_bytes;
}

/* Dataset access */

XRGDataset* xrg_network_collector_get_download_dataset(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->download_rate;
}

XRGDataset* xrg_network_collector_get_upload_dataset(XRGNetworkCollector *collector) {
    g_return_val_if_fail(collector != NULL, NULL);
    return collector->upload_rate;
}
