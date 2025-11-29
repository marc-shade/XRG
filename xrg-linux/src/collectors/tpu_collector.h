/**
 * XRG TPU Collector
 *
 * Monitors Google Coral Edge TPU devices:
 * - Device detection (USB/PCIe/M.2)
 * - Inference statistics from stats file
 * - Temperature (when available)
 */

#ifndef XRG_TPU_COLLECTOR_H
#define XRG_TPU_COLLECTOR_H

#include "../core/dataset.h"
#include <glib.h>

/**
 * TPU device types
 */
typedef enum {
    XRG_TPU_TYPE_NONE,        /* No TPU detected */
    XRG_TPU_TYPE_USB,         /* USB Accelerator */
    XRG_TPU_TYPE_PCIE,        /* PCIe/M.2 Accelerator */
    XRG_TPU_TYPE_DEVBOARD     /* Dev Board (on-board TPU) */
} XRGTPUType;

/**
 * TPU status
 */
typedef enum {
    XRG_TPU_STATUS_DISCONNECTED,
    XRG_TPU_STATUS_CONNECTED,
    XRG_TPU_STATUS_BUSY,
    XRG_TPU_STATUS_ERROR
} XRGTPUStatus;

typedef struct _XRGTPUCollector XRGTPUCollector;

/* Constructor and destructor */
XRGTPUCollector* xrg_tpu_collector_new(gint history_size);
void xrg_tpu_collector_free(XRGTPUCollector *collector);

/* Update TPU statistics */
void xrg_tpu_collector_update(XRGTPUCollector *collector);

/* Get datasets */
XRGDataset* xrg_tpu_collector_get_inference_rate_dataset(XRGTPUCollector *collector);
XRGDataset* xrg_tpu_collector_get_latency_dataset(XRGTPUCollector *collector);

/* Get current values */
XRGTPUStatus xrg_tpu_collector_get_status(XRGTPUCollector *collector);
XRGTPUType xrg_tpu_collector_get_type(XRGTPUCollector *collector);
const gchar* xrg_tpu_collector_get_device_path(XRGTPUCollector *collector);
const gchar* xrg_tpu_collector_get_name(XRGTPUCollector *collector);

/* Inference statistics */
guint64 xrg_tpu_collector_get_total_inferences(XRGTPUCollector *collector);
gdouble xrg_tpu_collector_get_inferences_per_second(XRGTPUCollector *collector);
gdouble xrg_tpu_collector_get_avg_latency_ms(XRGTPUCollector *collector);
gdouble xrg_tpu_collector_get_last_latency_ms(XRGTPUCollector *collector);

/* Temperature (if available) */
gdouble xrg_tpu_collector_get_temperature(XRGTPUCollector *collector);
gboolean xrg_tpu_collector_has_temperature(XRGTPUCollector *collector);

/* Stats file path (for external updates) */
const gchar* xrg_tpu_collector_get_stats_file_path(void);

#endif /* XRG_TPU_COLLECTOR_H */
