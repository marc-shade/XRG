#ifndef XRG_NETWORK_COLLECTOR_H
#define XRG_NETWORK_COLLECTOR_H

#include <glib.h>
#include "../core/dataset.h"

/**
 * XRGNetworkCollector - Network traffic data collector
 *
 * Collects network statistics from /proc/net/dev for:
 * - Bytes received/transmitted per interface
 * - Upload/download rates
 * - Total bandwidth usage
 */

#define MAX_INTERFACES 16
#define INTERFACE_NAME_LEN 32

typedef struct {
    gchar name[INTERFACE_NAME_LEN];
    guint64 rx_bytes;
    guint64 tx_bytes;
    guint64 prev_rx_bytes;
    guint64 prev_tx_bytes;
    gdouble rx_rate;  /* Bytes per second */
    gdouble tx_rate;  /* Bytes per second */
    gboolean active;
} NetworkInterface;

typedef struct _XRGNetworkCollector XRGNetworkCollector;

struct _XRGNetworkCollector {
    /* Network interfaces */
    NetworkInterface interfaces[MAX_INTERFACES];
    gint num_interfaces;
    gint primary_interface_idx;  /* Index of primary interface */

    /* Datasets for graphing (primary interface) */
    XRGDataset *download_rate;   /* Download in MB/s */
    XRGDataset *upload_rate;     /* Upload in MB/s */

    /* Update tracking */
    gint64 last_update_time;
};

/* Constructor and destructor */
XRGNetworkCollector* xrg_network_collector_new(gint dataset_capacity);
void xrg_network_collector_free(XRGNetworkCollector *collector);

/* Update methods */
void xrg_network_collector_update(XRGNetworkCollector *collector);

/* Getters */
const gchar* xrg_network_collector_get_primary_interface(XRGNetworkCollector *collector);
gdouble xrg_network_collector_get_download_rate(XRGNetworkCollector *collector);
gdouble xrg_network_collector_get_upload_rate(XRGNetworkCollector *collector);
guint64 xrg_network_collector_get_total_rx(XRGNetworkCollector *collector);
guint64 xrg_network_collector_get_total_tx(XRGNetworkCollector *collector);

/* Dataset access */
XRGDataset* xrg_network_collector_get_download_dataset(XRGNetworkCollector *collector);
XRGDataset* xrg_network_collector_get_upload_dataset(XRGNetworkCollector *collector);

#endif /* XRG_NETWORK_COLLECTOR_H */
