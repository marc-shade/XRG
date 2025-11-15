#ifndef XRG_GPU_COLLECTOR_H
#define XRG_GPU_COLLECTOR_H

#include "../core/dataset.h"
#include <glib.h>

typedef struct _XRGGPUCollector XRGGPUCollector;

/* Constructor and destructor */
XRGGPUCollector* xrg_gpu_collector_new(gint history_size);
void xrg_gpu_collector_free(XRGGPUCollector *collector);

/* Update GPU statistics */
void xrg_gpu_collector_update(XRGGPUCollector *collector);

/* Get datasets */
XRGDataset* xrg_gpu_collector_get_utilization_dataset(XRGGPUCollector *collector);
XRGDataset* xrg_gpu_collector_get_memory_dataset(XRGGPUCollector *collector);

/* Get current values */
gdouble xrg_gpu_collector_get_utilization(XRGGPUCollector *collector);
gdouble xrg_gpu_collector_get_memory_used_mb(XRGGPUCollector *collector);
gdouble xrg_gpu_collector_get_memory_total_mb(XRGGPUCollector *collector);
gdouble xrg_gpu_collector_get_temperature(XRGGPUCollector *collector);
const gchar* xrg_gpu_collector_get_name(XRGGPUCollector *collector);

#endif /* XRG_GPU_COLLECTOR_H */
