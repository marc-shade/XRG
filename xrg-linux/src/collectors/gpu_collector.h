#ifndef XRG_GPU_COLLECTOR_H
#define XRG_GPU_COLLECTOR_H

#include "../core/dataset.h"
#include <glib.h>

/**
 * GPU Driver/Backend types
 */
typedef enum {
    XRG_GPU_BACKEND_NONE,       /* No GPU detected */
    XRG_GPU_BACKEND_NVML,       /* NVIDIA proprietary (nvidia-smi) */
    XRG_GPU_BACKEND_NOUVEAU,    /* NVIDIA open source (nouveau sysfs) */
    XRG_GPU_BACKEND_AMDGPU,     /* AMD GPU (amdgpu sysfs) */
    XRG_GPU_BACKEND_INTEL,      /* Intel integrated (i915 sysfs) */
    XRG_GPU_BACKEND_SIMULATED   /* Fallback simulated data */
} XRGGPUBackend;

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

/* Extended getters */
gdouble xrg_gpu_collector_get_fan_speed_rpm(XRGGPUCollector *collector);
gdouble xrg_gpu_collector_get_power_watts(XRGGPUCollector *collector);
XRGGPUBackend xrg_gpu_collector_get_backend(XRGGPUCollector *collector);
const gchar* xrg_gpu_collector_get_backend_name(XRGGPUCollector *collector);

#endif /* XRG_GPU_COLLECTOR_H */
