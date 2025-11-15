#include "gpu_collector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct _XRGGPUCollector {
    XRGDataset *utilization_dataset;
    XRGDataset *memory_dataset;
    
    gdouble current_utilization;
    gdouble memory_used_mb;
    gdouble memory_total_mb;
    gdouble temperature;
    gchar *gpu_name;
    
    /* For simulation */
    gdouble phase;
};

XRGGPUCollector* xrg_gpu_collector_new(gint history_size) {
    XRGGPUCollector *collector = g_new0(XRGGPUCollector, 1);
    
    collector->utilization_dataset = xrg_dataset_new(history_size);
    collector->memory_dataset = xrg_dataset_new(history_size);
    
    collector->current_utilization = 0.0;
    collector->memory_total_mb = 2048.0;  /* GTX 680 has 2GB */
    collector->memory_used_mb = 512.0;
    collector->temperature = 45.0;
    collector->gpu_name = g_strdup("NVIDIA GTX 680 (nouveau)");
    collector->phase = 0.0;
    
    return collector;
}

void xrg_gpu_collector_free(XRGGPUCollector *collector) {
    if (collector == NULL)
        return;
    
    xrg_dataset_free(collector->utilization_dataset);
    xrg_dataset_free(collector->memory_dataset);
    g_free(collector->gpu_name);
    g_free(collector);
}

void xrg_gpu_collector_update(XRGGPUCollector *collector) {
    if (collector == NULL)
        return;
    
    /* Simulated GPU utilization (sine wave 0-100%)
     * TODO: Replace with real GPU metrics when nvidia-smi is available
     * or use sysfs for Intel/AMD GPUs
     */
    collector->phase += 0.1;
    collector->current_utilization = 30.0 + 25.0 * sin(collector->phase);
    
    /* Simulated memory usage (slow variation) */
    collector->memory_used_mb = 512.0 + 256.0 * sin(collector->phase * 0.3);
    
    /* Simulated temperature (correlated with utilization) */
    collector->temperature = 45.0 + (collector->current_utilization * 0.4);
    
    /* Add to datasets */
    xrg_dataset_add_value(collector->utilization_dataset, collector->current_utilization);
    xrg_dataset_add_value(collector->memory_dataset, 
                         (collector->memory_used_mb / collector->memory_total_mb) * 100.0);
}

XRGDataset* xrg_gpu_collector_get_utilization_dataset(XRGGPUCollector *collector) {
    return collector ? collector->utilization_dataset : NULL;
}

XRGDataset* xrg_gpu_collector_get_memory_dataset(XRGGPUCollector *collector) {
    return collector ? collector->memory_dataset : NULL;
}

gdouble xrg_gpu_collector_get_utilization(XRGGPUCollector *collector) {
    return collector ? collector->current_utilization : 0.0;
}

gdouble xrg_gpu_collector_get_memory_used_mb(XRGGPUCollector *collector) {
    return collector ? collector->memory_used_mb : 0.0;
}

gdouble xrg_gpu_collector_get_memory_total_mb(XRGGPUCollector *collector) {
    return collector ? collector->memory_total_mb : 0.0;
}

gdouble xrg_gpu_collector_get_temperature(XRGGPUCollector *collector) {
    return collector ? collector->temperature : 0.0;
}

const gchar* xrg_gpu_collector_get_name(XRGGPUCollector *collector) {
    return collector ? collector->gpu_name : "No GPU";
}
