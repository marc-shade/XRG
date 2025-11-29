/**
 * XRG GPU Collector Implementation
 *
 * Multi-backend GPU monitoring support:
 * - NVML (nvidia-smi) for NVIDIA proprietary driver
 * - Nouveau sysfs for NVIDIA open source driver
 * - AMDGPU sysfs for AMD GPUs
 * - Intel i915 sysfs for Intel integrated GPUs
 * - Simulated fallback for testing
 */

#include "gpu_collector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

/* Sysfs paths */
#define DRM_PATH "/sys/class/drm"
#define HWMON_PATH "/sys/class/hwmon"

struct _XRGGPUCollector {
    XRGDataset *utilization_dataset;
    XRGDataset *memory_dataset;

    /* Current values */
    gdouble current_utilization;
    gdouble memory_used_mb;
    gdouble memory_total_mb;
    gdouble temperature;
    gdouble fan_speed_rpm;
    gdouble power_watts;
    gchar *gpu_name;

    /* Backend info */
    XRGGPUBackend backend;
    gchar *drm_card_path;      /* e.g., /sys/class/drm/card0/device */
    gchar *hwmon_path;         /* e.g., /sys/class/hwmon/hwmon4 */
    gint gpu_index;            /* For nvidia-smi */

    /* For simulated backend */
    gdouble phase;
};

/* Forward declarations */
static void detect_gpu_backend(XRGGPUCollector *collector);
static void update_nvml(XRGGPUCollector *collector);
static void update_nouveau(XRGGPUCollector *collector);
static void update_amdgpu(XRGGPUCollector *collector);
static void update_intel(XRGGPUCollector *collector);
static void update_simulated(XRGGPUCollector *collector);
static gchar* read_sysfs_string(const gchar *path);
static gint64 read_sysfs_int(const gchar *path);
static gchar* find_hwmon_for_device(const gchar *device_path);
static gchar* get_pci_device_name(guint16 vendor, guint16 device);

XRGGPUCollector* xrg_gpu_collector_new(gint history_size) {
    XRGGPUCollector *collector = g_new0(XRGGPUCollector, 1);

    collector->utilization_dataset = xrg_dataset_new(history_size);
    collector->memory_dataset = xrg_dataset_new(history_size);

    collector->current_utilization = 0.0;
    collector->memory_total_mb = 0.0;
    collector->memory_used_mb = 0.0;
    collector->temperature = 0.0;
    collector->fan_speed_rpm = 0.0;
    collector->power_watts = 0.0;
    collector->gpu_name = NULL;
    collector->backend = XRG_GPU_BACKEND_NONE;
    collector->drm_card_path = NULL;
    collector->hwmon_path = NULL;
    collector->gpu_index = 0;
    collector->phase = 0.0;

    /* Detect available GPU and backend */
    detect_gpu_backend(collector);

    return collector;
}

void xrg_gpu_collector_free(XRGGPUCollector *collector) {
    if (collector == NULL)
        return;

    xrg_dataset_free(collector->utilization_dataset);
    xrg_dataset_free(collector->memory_dataset);
    g_free(collector->gpu_name);
    g_free(collector->drm_card_path);
    g_free(collector->hwmon_path);
    g_free(collector);
}

/**
 * Check if nvidia proprietary driver is in use (not nouveau)
 * Returns TRUE if nvidia driver is found for any GPU
 */
static gboolean check_nvidia_proprietary_driver(void) {
    DIR *dir = opendir(DRM_PATH);
    if (!dir) return FALSE;

    struct dirent *entry;
    gboolean found_nvidia_driver = FALSE;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) != 0)
            continue;
        if (strchr(entry->d_name, '-') != NULL)
            continue;

        gchar *card_path = g_strdup_printf("%s/%s/device", DRM_PATH, entry->d_name);

        /* Check vendor ID for NVIDIA (0x10de) */
        gchar *vendor_path = g_strdup_printf("%s/vendor", card_path);
        gchar *vendor_str = read_sysfs_string(vendor_path);
        g_free(vendor_path);

        if (vendor_str) {
            guint16 vendor_id = (guint16)strtol(vendor_str, NULL, 16);
            g_free(vendor_str);

            if (vendor_id == 0x10de) {
                /* Found NVIDIA card - check driver */
                gchar *driver_link = g_strdup_printf("%s/driver", card_path);
                gchar *driver_path = g_file_read_link(driver_link, NULL);
                g_free(driver_link);

                if (driver_path) {
                    gchar *driver_name = g_path_get_basename(driver_path);
                    g_free(driver_path);

                    /* nvidia driver (not nouveau) means proprietary */
                    if (driver_name && g_strcmp0(driver_name, "nvidia") == 0) {
                        found_nvidia_driver = TRUE;
                    }
                    g_free(driver_name);
                }
            }
        }
        g_free(card_path);

        if (found_nvidia_driver) break;
    }
    closedir(dir);
    return found_nvidia_driver;
}

/**
 * Try to initialize NVML backend via nvidia-smi
 * Returns TRUE if successful
 */
static gboolean try_nvml_backend(XRGGPUCollector *collector) {
    FILE *fp = popen("nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null", "r");
    if (!fp) return FALSE;

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        int status = pclose(fp);
        fp = NULL;

        /* Check for nvidia-smi error messages */
        if (status == 0 && strncmp(buffer, "NVIDIA-SMI", 10) != 0) {
            collector->backend = XRG_GPU_BACKEND_NVML;
            collector->gpu_name = g_strdup(g_strstrip(buffer));

            /* Get memory total */
            fp = popen("nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>/dev/null", "r");
            if (fp) {
                if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                    collector->memory_total_mb = atof(buffer);
                }
                pclose(fp);
            }
            g_message("GPU: Using NVML backend for %s", collector->gpu_name);
            return TRUE;
        }
    }
    if (fp) pclose(fp);
    return FALSE;
}

/**
 * Detect available GPU and select appropriate backend
 */
static void detect_gpu_backend(XRGGPUCollector *collector) {
    DIR *dir;
    struct dirent *entry;

    /* Only try nvidia-smi if proprietary nvidia driver is in use.
     * This avoids blocking popen() calls when nouveau driver is active,
     * as nvidia-smi hangs/times out without proprietary driver. */
    if (check_nvidia_proprietary_driver()) {
        if (try_nvml_backend(collector)) {
            return;  /* Successfully using NVML */
        }
    }

    /* Scan DRM devices for GPU hardware */
    dir = opendir(DRM_PATH);
    if (!dir) {
        collector->backend = XRG_GPU_BACKEND_SIMULATED;
        collector->gpu_name = g_strdup("Simulated GPU");
        collector->memory_total_mb = 2048.0;
        g_message("GPU: No DRM devices found, using simulated backend");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        /* Look for card devices (card0, card1, etc.) */
        if (strncmp(entry->d_name, "card", 4) != 0)
            continue;
        if (strchr(entry->d_name, '-') != NULL)  /* Skip card0-DP-1 etc */
            continue;

        gchar *card_path = g_strdup_printf("%s/%s/device", DRM_PATH, entry->d_name);

        /* Read vendor ID */
        gchar *vendor_path = g_strdup_printf("%s/vendor", card_path);
        gchar *vendor_str = read_sysfs_string(vendor_path);
        g_free(vendor_path);

        if (!vendor_str) {
            g_free(card_path);
            continue;
        }

        guint16 vendor_id = (guint16)strtol(vendor_str, NULL, 16);
        g_free(vendor_str);

        /* Read device ID */
        gchar *device_path = g_strdup_printf("%s/device", card_path);
        gchar *device_str = read_sysfs_string(device_path);
        g_free(device_path);

        guint16 device_id = 0;
        if (device_str) {
            device_id = (guint16)strtol(device_str, NULL, 16);
            g_free(device_str);
        }

        /* Check driver */
        gchar *driver_link = g_strdup_printf("%s/driver", card_path);
        gchar *driver_path = g_file_read_link(driver_link, NULL);
        g_free(driver_link);

        gchar *driver_name = NULL;
        if (driver_path) {
            driver_name = g_path_get_basename(driver_path);
            g_free(driver_path);
        }

        /* Determine backend based on vendor and driver */
        if (vendor_id == 0x10de) {  /* NVIDIA */
            if (driver_name && g_strcmp0(driver_name, "nouveau") == 0) {
                collector->backend = XRG_GPU_BACKEND_NOUVEAU;
                collector->drm_card_path = g_strdup(card_path);
                collector->hwmon_path = find_hwmon_for_device(card_path);
                collector->gpu_name = get_pci_device_name(vendor_id, device_id);
                if (!collector->gpu_name) {
                    collector->gpu_name = g_strdup_printf("NVIDIA GPU 0x%04x (nouveau)", device_id);
                }
                /* Estimate memory from device ID (GTX 680 = 2GB) */
                collector->memory_total_mb = 2048.0;
                g_message("GPU: Using nouveau backend for %s", collector->gpu_name);
            }
        } else if (vendor_id == 0x1002) {  /* AMD */
            collector->backend = XRG_GPU_BACKEND_AMDGPU;
            collector->drm_card_path = g_strdup(card_path);
            collector->hwmon_path = find_hwmon_for_device(card_path);
            collector->gpu_name = get_pci_device_name(vendor_id, device_id);
            if (!collector->gpu_name) {
                collector->gpu_name = g_strdup_printf("AMD GPU 0x%04x", device_id);
            }
            g_message("GPU: Using amdgpu backend for %s", collector->gpu_name);
        } else if (vendor_id == 0x8086) {  /* Intel */
            collector->backend = XRG_GPU_BACKEND_INTEL;
            collector->drm_card_path = g_strdup(card_path);
            collector->hwmon_path = find_hwmon_for_device(card_path);
            collector->gpu_name = get_pci_device_name(vendor_id, device_id);
            if (!collector->gpu_name) {
                collector->gpu_name = g_strdup_printf("Intel GPU 0x%04x", device_id);
            }
            g_message("GPU: Using Intel backend for %s", collector->gpu_name);
        }

        g_free(driver_name);
        g_free(card_path);

        if (collector->backend != XRG_GPU_BACKEND_NONE) {
            break;  /* Found a usable GPU */
        }
    }
    closedir(dir);

    /* Fallback to simulated if no GPU found */
    if (collector->backend == XRG_GPU_BACKEND_NONE) {
        collector->backend = XRG_GPU_BACKEND_SIMULATED;
        collector->gpu_name = g_strdup("Simulated GPU");
        collector->memory_total_mb = 2048.0;
        g_message("GPU: No supported GPU found, using simulated backend");
    }
}

/**
 * Find hwmon device associated with a DRM device
 */
static gchar* find_hwmon_for_device(const gchar *device_path) {
    gchar *hwmon_dir = g_strdup_printf("%s/hwmon", device_path);
    DIR *dir = opendir(hwmon_dir);

    if (!dir) {
        g_free(hwmon_dir);
        return NULL;
    }

    struct dirent *entry;
    gchar *result = NULL;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) == 0) {
            result = g_strdup_printf("%s/%s", hwmon_dir, entry->d_name);
            break;
        }
    }

    closedir(dir);
    g_free(hwmon_dir);
    return result;
}

/**
 * Get PCI device name from vendor/device ID
 */
static gchar* get_pci_device_name(guint16 vendor, guint16 device) {
    /* Common NVIDIA devices */
    if (vendor == 0x10de) {
        switch (device) {
            case 0x1180: return g_strdup("NVIDIA GeForce GTX 680");
            case 0x1187: return g_strdup("NVIDIA GeForce GTX 760");
            case 0x1189: return g_strdup("NVIDIA GeForce GTX 670");
            case 0x11c0: return g_strdup("NVIDIA GeForce GTX 660");
            case 0x1401: return g_strdup("NVIDIA GeForce GTX 960");
            case 0x1b80: return g_strdup("NVIDIA GeForce GTX 1080");
            case 0x1b81: return g_strdup("NVIDIA GeForce GTX 1070");
            case 0x1c02: return g_strdup("NVIDIA GeForce GTX 1060");
            case 0x1e04: return g_strdup("NVIDIA GeForce RTX 2080 Ti");
            case 0x2204: return g_strdup("NVIDIA GeForce RTX 3090");
            case 0x2684: return g_strdup("NVIDIA GeForce RTX 4090");
        }
    }
    return NULL;
}

/**
 * Read string from sysfs file
 */
static gchar* read_sysfs_string(const gchar *path) {
    gchar *contents = NULL;
    gsize length;

    if (!g_file_get_contents(path, &contents, &length, NULL)) {
        return NULL;
    }

    g_strstrip(contents);
    return contents;
}

/**
 * Read integer from sysfs file
 */
static gint64 read_sysfs_int(const gchar *path) {
    gchar *contents = read_sysfs_string(path);
    if (!contents) {
        return -1;
    }

    gint64 value = g_ascii_strtoll(contents, NULL, 10);
    g_free(contents);
    return value;
}

void xrg_gpu_collector_update(XRGGPUCollector *collector) {
    if (collector == NULL)
        return;

    switch (collector->backend) {
        case XRG_GPU_BACKEND_NVML:
            update_nvml(collector);
            break;
        case XRG_GPU_BACKEND_NOUVEAU:
            update_nouveau(collector);
            break;
        case XRG_GPU_BACKEND_AMDGPU:
            update_amdgpu(collector);
            break;
        case XRG_GPU_BACKEND_INTEL:
            update_intel(collector);
            break;
        case XRG_GPU_BACKEND_SIMULATED:
        case XRG_GPU_BACKEND_NONE:
        default:
            update_simulated(collector);
            break;
    }

    /* Add to datasets */
    xrg_dataset_add_value(collector->utilization_dataset, collector->current_utilization);

    gdouble memory_pct = 0.0;
    if (collector->memory_total_mb > 0) {
        memory_pct = (collector->memory_used_mb / collector->memory_total_mb) * 100.0;
    }
    xrg_dataset_add_value(collector->memory_dataset, memory_pct);
}

/**
 * Update using NVML (nvidia-smi)
 */
static void update_nvml(XRGGPUCollector *collector) {
    FILE *fp;
    char buffer[256];

    /* Query utilization, memory, temperature */
    fp = popen("nvidia-smi --query-gpu=utilization.gpu,memory.used,temperature.gpu,power.draw,fan.speed --format=csv,noheader,nounits 2>/dev/null", "r");
    if (!fp) return;

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        gdouble util, mem_used, temp, power, fan;
        if (sscanf(buffer, "%lf, %lf, %lf, %lf, %lf", &util, &mem_used, &temp, &power, &fan) >= 3) {
            collector->current_utilization = util;
            collector->memory_used_mb = mem_used;
            collector->temperature = temp;
            collector->power_watts = power;
            collector->fan_speed_rpm = fan;  /* nvidia-smi reports percentage, not RPM */
        }
    }
    pclose(fp);
}

/**
 * Update using nouveau sysfs
 */
static void update_nouveau(XRGGPUCollector *collector) {
    if (!collector->hwmon_path) return;

    gchar *path;

    /* Temperature (millidegrees -> degrees) */
    path = g_strdup_printf("%s/temp1_input", collector->hwmon_path);
    gint64 temp_milli = read_sysfs_int(path);
    g_free(path);
    if (temp_milli >= 0) {
        collector->temperature = temp_milli / 1000.0;
    }

    /* Fan speed (RPM) */
    path = g_strdup_printf("%s/fan1_input", collector->hwmon_path);
    gint64 fan = read_sysfs_int(path);
    g_free(path);
    if (fan >= 0) {
        collector->fan_speed_rpm = fan;
    }

    /* Power (microwatts -> watts) */
    path = g_strdup_printf("%s/power1_input", collector->hwmon_path);
    gint64 power_micro = read_sysfs_int(path);
    g_free(path);
    if (power_micro >= 0) {
        collector->power_watts = power_micro / 1000000.0;
    }

    /* Nouveau doesn't provide utilization directly - estimate from power */
    /* Typical GTX 680 TDP is 195W, idle around 15W */
    if (collector->power_watts > 0) {
        gdouble max_power = 195.0;
        gdouble idle_power = 15.0;
        collector->current_utilization = ((collector->power_watts - idle_power) / (max_power - idle_power)) * 100.0;
        if (collector->current_utilization < 0) collector->current_utilization = 0;
        if (collector->current_utilization > 100) collector->current_utilization = 100;
    }

    /* Memory usage not available from nouveau sysfs - keep previous value or estimate */
    /* For now, just show minimal usage */
    if (collector->memory_used_mb == 0) {
        collector->memory_used_mb = 256.0;  /* Baseline VRAM usage */
    }
}

/**
 * Update using AMD sysfs
 */
static void update_amdgpu(XRGGPUCollector *collector) {
    if (!collector->drm_card_path) return;

    gchar *path;

    /* GPU utilization */
    path = g_strdup_printf("%s/gpu_busy_percent", collector->drm_card_path);
    gint64 util = read_sysfs_int(path);
    g_free(path);
    if (util >= 0) {
        collector->current_utilization = util;
    }

    /* Memory info */
    path = g_strdup_printf("%s/mem_info_vram_used", collector->drm_card_path);
    gint64 mem_used = read_sysfs_int(path);
    g_free(path);
    if (mem_used >= 0) {
        collector->memory_used_mb = mem_used / (1024.0 * 1024.0);
    }

    path = g_strdup_printf("%s/mem_info_vram_total", collector->drm_card_path);
    gint64 mem_total = read_sysfs_int(path);
    g_free(path);
    if (mem_total >= 0) {
        collector->memory_total_mb = mem_total / (1024.0 * 1024.0);
    }

    /* Temperature from hwmon */
    if (collector->hwmon_path) {
        path = g_strdup_printf("%s/temp1_input", collector->hwmon_path);
        gint64 temp_milli = read_sysfs_int(path);
        g_free(path);
        if (temp_milli >= 0) {
            collector->temperature = temp_milli / 1000.0;
        }

        /* Fan speed */
        path = g_strdup_printf("%s/fan1_input", collector->hwmon_path);
        gint64 fan = read_sysfs_int(path);
        g_free(path);
        if (fan >= 0) {
            collector->fan_speed_rpm = fan;
        }

        /* Power */
        path = g_strdup_printf("%s/power1_average", collector->hwmon_path);
        gint64 power_micro = read_sysfs_int(path);
        g_free(path);
        if (power_micro >= 0) {
            collector->power_watts = power_micro / 1000000.0;
        }
    }
}

/**
 * Update using Intel i915 sysfs
 */
static void update_intel(XRGGPUCollector *collector) {
    /* Intel integrated GPUs have limited sysfs exposure */
    /* Try to read from i915 perf or debugfs */

    if (collector->hwmon_path) {
        gchar *path = g_strdup_printf("%s/temp1_input", collector->hwmon_path);
        gint64 temp_milli = read_sysfs_int(path);
        g_free(path);
        if (temp_milli >= 0) {
            collector->temperature = temp_milli / 1000.0;
        }
    }

    /* Intel GPUs share system memory, so VRAM tracking isn't applicable */
    collector->memory_total_mb = 0;
    collector->memory_used_mb = 0;

    /* Utilization would require i915_pmu or debugfs access */
    /* For now, report 0 */
    collector->current_utilization = 0;
}

/**
 * Update with simulated data (fallback)
 */
static void update_simulated(XRGGPUCollector *collector) {
    collector->phase += 0.1;
    collector->current_utilization = 30.0 + 25.0 * sin(collector->phase);
    collector->memory_used_mb = 512.0 + 256.0 * sin(collector->phase * 0.3);
    collector->temperature = 45.0 + (collector->current_utilization * 0.4);
    collector->fan_speed_rpm = 1200 + 600 * (collector->current_utilization / 100.0);
    collector->power_watts = 15.0 + 180.0 * (collector->current_utilization / 100.0);
}

/* Getter implementations */

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

gdouble xrg_gpu_collector_get_fan_speed_rpm(XRGGPUCollector *collector) {
    return collector ? collector->fan_speed_rpm : 0.0;
}

gdouble xrg_gpu_collector_get_power_watts(XRGGPUCollector *collector) {
    return collector ? collector->power_watts : 0.0;
}

XRGGPUBackend xrg_gpu_collector_get_backend(XRGGPUCollector *collector) {
    return collector ? collector->backend : XRG_GPU_BACKEND_NONE;
}

const gchar* xrg_gpu_collector_get_backend_name(XRGGPUCollector *collector) {
    if (!collector) return "None";

    switch (collector->backend) {
        case XRG_GPU_BACKEND_NVML:     return "NVML";
        case XRG_GPU_BACKEND_NOUVEAU:  return "nouveau";
        case XRG_GPU_BACKEND_AMDGPU:   return "amdgpu";
        case XRG_GPU_BACKEND_INTEL:    return "i915";
        case XRG_GPU_BACKEND_SIMULATED: return "Simulated";
        default: return "None";
    }
}
