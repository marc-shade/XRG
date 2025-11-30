/**
 * XRG TPU Collector Implementation
 *
 * Monitors Google Coral Edge TPU devices.
 *
 * Stats file format (JSON):
 * {
 *   "total_inferences": 12345,
 *   "last_latency_ms": 18.5,
 *   "avg_latency_ms": 19.2,
 *   "model_name": "mobilenet_v2",
 *   "timestamp": 1234567890.123
 * }
 *
 * Stats file location: /tmp/xrg-coral-tpu-stats.json
 */

#include "tpu_collector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <json-glib/json-glib.h>

/* Coral USB Vendor/Product IDs */
#define CORAL_VENDOR_ID  "1a6e"
#define CORAL_PRODUCT_ID "089a"
#define GOOGLE_VENDOR_ID "18d1"  /* Alternative vendor ID */

/* Stats file path */
#define TPU_STATS_FILE "/tmp/xrg-coral-tpu-stats.json"

/* USB sysfs path */
#define USB_DEVICES_PATH "/sys/bus/usb/devices"

struct _XRGTPUCollector {
    XRGDataset *inference_rate_dataset;
    XRGDataset *latency_dataset;

    /* Category-specific rate datasets for 3-color display */
    XRGDataset *direct_rate_dataset;
    XRGDataset *hooked_rate_dataset;
    XRGDataset *logged_rate_dataset;

    /* Device info */
    XRGTPUStatus status;
    XRGTPUType type;
    gchar *device_path;
    gchar *device_name;

    /* Current stats */
    guint64 total_inferences;
    guint64 prev_total_inferences;
    gdouble inferences_per_second;
    gdouble avg_latency_ms;
    gdouble last_latency_ms;
    gdouble temperature;
    gboolean has_temperature;

    /* Category breakdown (direct/hooked/logged) */
    guint64 direct_inferences;
    guint64 hooked_inferences;
    guint64 logged_inferences;
    guint64 prev_direct;
    guint64 prev_hooked;
    guint64 prev_logged;

    /* Timing for rate calculation */
    gint64 last_update_time;

    /* Model info */
    gchar *model_name;
};

/* Forward declarations */
static void detect_tpu_device(XRGTPUCollector *collector);
static void read_stats_file(XRGTPUCollector *collector);
static gboolean check_usb_device(const gchar *vendor, const gchar *product, gchar **path);
static gchar* read_sysfs_string(const gchar *path);

XRGTPUCollector* xrg_tpu_collector_new(gint history_size) {
    XRGTPUCollector *collector = g_new0(XRGTPUCollector, 1);

    collector->inference_rate_dataset = xrg_dataset_new(history_size);
    collector->latency_dataset = xrg_dataset_new(history_size);

    /* Category-specific datasets for 3-color display */
    collector->direct_rate_dataset = xrg_dataset_new(history_size);
    collector->hooked_rate_dataset = xrg_dataset_new(history_size);
    collector->logged_rate_dataset = xrg_dataset_new(history_size);

    collector->status = XRG_TPU_STATUS_DISCONNECTED;
    collector->type = XRG_TPU_TYPE_NONE;
    collector->device_path = NULL;
    collector->device_name = NULL;
    collector->model_name = NULL;

    collector->total_inferences = 0;
    collector->prev_total_inferences = 0;
    collector->inferences_per_second = 0.0;
    collector->avg_latency_ms = 0.0;
    collector->last_latency_ms = 0.0;
    collector->temperature = 0.0;
    collector->has_temperature = FALSE;

    /* Category breakdown */
    collector->direct_inferences = 0;
    collector->hooked_inferences = 0;
    collector->logged_inferences = 0;
    collector->prev_direct = 0;
    collector->prev_hooked = 0;
    collector->prev_logged = 0;

    collector->last_update_time = g_get_monotonic_time();

    /* Initial device detection */
    detect_tpu_device(collector);

    return collector;
}

void xrg_tpu_collector_free(XRGTPUCollector *collector) {
    if (collector == NULL)
        return;

    xrg_dataset_free(collector->inference_rate_dataset);
    xrg_dataset_free(collector->latency_dataset);
    xrg_dataset_free(collector->direct_rate_dataset);
    xrg_dataset_free(collector->hooked_rate_dataset);
    xrg_dataset_free(collector->logged_rate_dataset);
    g_free(collector->device_path);
    g_free(collector->device_name);
    g_free(collector->model_name);
    g_free(collector);
}

/**
 * Detect Coral TPU device via USB sysfs
 */
static void detect_tpu_device(XRGTPUCollector *collector) {
    gchar *path = NULL;
    gboolean was_disconnected = (collector->status == XRG_TPU_STATUS_DISCONNECTED);

    /* Check for Coral USB Accelerator (Global Unichip Corp) */
    if (check_usb_device(CORAL_VENDOR_ID, CORAL_PRODUCT_ID, &path)) {
        collector->status = XRG_TPU_STATUS_CONNECTED;
        collector->type = XRG_TPU_TYPE_USB;
        g_free(collector->device_path);
        collector->device_path = path;
        g_free(collector->device_name);
        collector->device_name = g_strdup("Coral USB Accelerator");
        /* Only log on initial detection, not every update */
        if (was_disconnected) {
            g_message("TPU: Detected Coral USB Accelerator at %s", path);
        }
        return;
    }

    /* Check for Google-branded Coral (alternative vendor ID) */
    if (check_usb_device(GOOGLE_VENDOR_ID, "9302", &path)) {
        collector->status = XRG_TPU_STATUS_CONNECTED;
        collector->type = XRG_TPU_TYPE_USB;
        g_free(collector->device_path);
        collector->device_path = path;
        g_free(collector->device_name);
        collector->device_name = g_strdup("Coral Edge TPU");
        /* Only log on initial detection, not every update */
        if (was_disconnected) {
            g_message("TPU: Detected Coral Edge TPU at %s", path);
        }
        return;
    }

    /* TODO: Check for PCIe/M.2 Coral devices in /sys/class/apex */

    /* No device found */
    collector->status = XRG_TPU_STATUS_DISCONNECTED;
    collector->type = XRG_TPU_TYPE_NONE;
    g_free(collector->device_path);
    collector->device_path = NULL;
    g_free(collector->device_name);
    collector->device_name = g_strdup("No TPU");
}

/**
 * Check if a USB device with given vendor/product exists
 */
static gboolean check_usb_device(const gchar *vendor, const gchar *product, gchar **path) {
    DIR *dir = opendir(USB_DEVICES_PATH);
    if (!dir) return FALSE;

    struct dirent *entry;
    gboolean found = FALSE;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        /* Read vendor ID */
        gchar *vendor_path = g_strdup_printf("%s/%s/idVendor", USB_DEVICES_PATH, entry->d_name);
        gchar *vendor_str = read_sysfs_string(vendor_path);
        g_free(vendor_path);

        if (!vendor_str) continue;

        if (g_ascii_strcasecmp(vendor_str, vendor) == 0) {
            /* Check product ID */
            gchar *product_path = g_strdup_printf("%s/%s/idProduct", USB_DEVICES_PATH, entry->d_name);
            gchar *product_str = read_sysfs_string(product_path);
            g_free(product_path);

            if (product_str && g_ascii_strcasecmp(product_str, product) == 0) {
                *path = g_strdup_printf("%s/%s", USB_DEVICES_PATH, entry->d_name);
                found = TRUE;
            }
            g_free(product_str);
        }
        g_free(vendor_str);

        if (found) break;
    }

    closedir(dir);
    return found;
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
 * Read stats from JSON file
 */
static void read_stats_file(XRGTPUCollector *collector) {
    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_file(parser, TPU_STATS_FILE, &error)) {
        /* Stats file doesn't exist or can't be read - that's OK */
        g_clear_error(&error);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return;
    }

    JsonObject *obj = json_node_get_object(root);

    /* Read stats */
    if (json_object_has_member(obj, "total_inferences")) {
        collector->total_inferences = json_object_get_int_member(obj, "total_inferences");
    }

    if (json_object_has_member(obj, "last_latency_ms")) {
        collector->last_latency_ms = json_object_get_double_member(obj, "last_latency_ms");
    }

    if (json_object_has_member(obj, "avg_latency_ms")) {
        collector->avg_latency_ms = json_object_get_double_member(obj, "avg_latency_ms");
    }

    if (json_object_has_member(obj, "model_name")) {
        g_free(collector->model_name);
        collector->model_name = g_strdup(json_object_get_string_member(obj, "model_name"));
    }

    if (json_object_has_member(obj, "temperature")) {
        collector->temperature = json_object_get_double_member(obj, "temperature");
        collector->has_temperature = TRUE;
    }

    /* Parse by_category breakdown for 3-color display */
    if (json_object_has_member(obj, "by_category")) {
        JsonObject *by_category = json_object_get_object_member(obj, "by_category");
        if (by_category) {
            if (json_object_has_member(by_category, "direct")) {
                collector->direct_inferences = json_object_get_int_member(by_category, "direct");
            }
            if (json_object_has_member(by_category, "hooked")) {
                collector->hooked_inferences = json_object_get_int_member(by_category, "hooked");
            }
            if (json_object_has_member(by_category, "logged")) {
                collector->logged_inferences = json_object_get_int_member(by_category, "logged");
            }
        }
    } else {
        /* Fallback: If no by_category, all are "direct" */
        collector->direct_inferences = collector->total_inferences;
        collector->hooked_inferences = 0;
        collector->logged_inferences = 0;
    }

    g_object_unref(parser);
}

void xrg_tpu_collector_update(XRGTPUCollector *collector) {
    if (collector == NULL)
        return;

    /* Re-check device connection (only check transitions to disconnected) */
    XRGTPUType prev_type = collector->type;
    detect_tpu_device(collector);

    /* Only log when device is removed (detection logging is in detect_tpu_device) */
    if (prev_type != XRG_TPU_TYPE_NONE && collector->type == XRG_TPU_TYPE_NONE) {
        g_message("TPU: Device disconnected");
    }

    /* Read stats file for inference metrics */
    guint64 prev_inferences = collector->total_inferences;
    read_stats_file(collector);

    /* Calculate inference rate */
    gint64 current_time = g_get_monotonic_time();
    gdouble elapsed_seconds = (current_time - collector->last_update_time) / 1000000.0;

    if (elapsed_seconds > 0 && collector->total_inferences > prev_inferences) {
        guint64 new_inferences = collector->total_inferences - prev_inferences;
        collector->inferences_per_second = new_inferences / elapsed_seconds;
        collector->status = XRG_TPU_STATUS_BUSY;
    } else {
        collector->inferences_per_second = 0.0;
        if (collector->type != XRG_TPU_TYPE_NONE) {
            collector->status = XRG_TPU_STATUS_CONNECTED;
        }
    }

    /* Calculate category-specific rates (per second) */
    gdouble direct_rate = 0.0;
    gdouble hooked_rate = 0.0;
    gdouble logged_rate = 0.0;

    if (elapsed_seconds > 0) {
        guint64 direct_delta = collector->direct_inferences > collector->prev_direct ?
                               collector->direct_inferences - collector->prev_direct : 0;
        guint64 hooked_delta = collector->hooked_inferences > collector->prev_hooked ?
                               collector->hooked_inferences - collector->prev_hooked : 0;
        guint64 logged_delta = collector->logged_inferences > collector->prev_logged ?
                               collector->logged_inferences - collector->prev_logged : 0;

        direct_rate = (gdouble)direct_delta / elapsed_seconds;
        hooked_rate = (gdouble)hooked_delta / elapsed_seconds;
        logged_rate = (gdouble)logged_delta / elapsed_seconds;
    }

    /* Update previous values for next rate calculation */
    collector->prev_direct = collector->direct_inferences;
    collector->prev_hooked = collector->hooked_inferences;
    collector->prev_logged = collector->logged_inferences;

    collector->last_update_time = current_time;

    /* Add to datasets */
    xrg_dataset_add_value(collector->inference_rate_dataset, collector->inferences_per_second);
    xrg_dataset_add_value(collector->latency_dataset, collector->last_latency_ms);

    /* Category-specific datasets for 3-color display */
    xrg_dataset_add_value(collector->direct_rate_dataset, direct_rate);
    xrg_dataset_add_value(collector->hooked_rate_dataset, hooked_rate);
    xrg_dataset_add_value(collector->logged_rate_dataset, logged_rate);
}

/* Getter implementations */

XRGDataset* xrg_tpu_collector_get_inference_rate_dataset(XRGTPUCollector *collector) {
    return collector ? collector->inference_rate_dataset : NULL;
}

XRGDataset* xrg_tpu_collector_get_latency_dataset(XRGTPUCollector *collector) {
    return collector ? collector->latency_dataset : NULL;
}

XRGTPUStatus xrg_tpu_collector_get_status(XRGTPUCollector *collector) {
    return collector ? collector->status : XRG_TPU_STATUS_DISCONNECTED;
}

XRGTPUType xrg_tpu_collector_get_type(XRGTPUCollector *collector) {
    return collector ? collector->type : XRG_TPU_TYPE_NONE;
}

const gchar* xrg_tpu_collector_get_device_path(XRGTPUCollector *collector) {
    return collector ? collector->device_path : NULL;
}

const gchar* xrg_tpu_collector_get_name(XRGTPUCollector *collector) {
    return collector ? collector->device_name : "No TPU";
}

guint64 xrg_tpu_collector_get_total_inferences(XRGTPUCollector *collector) {
    return collector ? collector->total_inferences : 0;
}

gdouble xrg_tpu_collector_get_inferences_per_second(XRGTPUCollector *collector) {
    return collector ? collector->inferences_per_second : 0.0;
}

gdouble xrg_tpu_collector_get_avg_latency_ms(XRGTPUCollector *collector) {
    return collector ? collector->avg_latency_ms : 0.0;
}

gdouble xrg_tpu_collector_get_last_latency_ms(XRGTPUCollector *collector) {
    return collector ? collector->last_latency_ms : 0.0;
}

gdouble xrg_tpu_collector_get_temperature(XRGTPUCollector *collector) {
    return collector ? collector->temperature : 0.0;
}

gboolean xrg_tpu_collector_has_temperature(XRGTPUCollector *collector) {
    return collector ? collector->has_temperature : FALSE;
}

const gchar* xrg_tpu_collector_get_stats_file_path(void) {
    return TPU_STATS_FILE;
}

/* Category breakdown getters for 3-color display */

guint64 xrg_tpu_collector_get_direct_inferences(XRGTPUCollector *collector) {
    return collector ? collector->direct_inferences : 0;
}

guint64 xrg_tpu_collector_get_hooked_inferences(XRGTPUCollector *collector) {
    return collector ? collector->hooked_inferences : 0;
}

guint64 xrg_tpu_collector_get_logged_inferences(XRGTPUCollector *collector) {
    return collector ? collector->logged_inferences : 0;
}

XRGDataset* xrg_tpu_collector_get_direct_dataset(XRGTPUCollector *collector) {
    return collector ? collector->direct_rate_dataset : NULL;
}

XRGDataset* xrg_tpu_collector_get_hooked_dataset(XRGTPUCollector *collector) {
    return collector ? collector->hooked_rate_dataset : NULL;
}

XRGDataset* xrg_tpu_collector_get_logged_dataset(XRGTPUCollector *collector) {
    return collector ? collector->logged_rate_dataset : NULL;
}
