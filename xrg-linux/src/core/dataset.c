#include "dataset.h"
#include <string.h>
#include <math.h>

/**
 * Create a new dataset with specified capacity
 */
XRGDataset* xrg_dataset_new(gint capacity) {
    g_return_val_if_fail(capacity > 0, NULL);

    XRGDataset *dataset = g_new0(XRGDataset, 1);
    dataset->values = g_new0(gdouble, capacity);
    dataset->capacity = capacity;
    dataset->count = 0;
    dataset->index = 0;
    dataset->min = G_MAXDOUBLE;
    dataset->max = -G_MAXDOUBLE;
    dataset->sum = 0.0;

    return dataset;
}

/**
 * Free a dataset and all its resources
 */
void xrg_dataset_free(XRGDataset *dataset) {
    if (dataset == NULL)
        return;

    g_free(dataset->values);
    g_free(dataset);
}

/**
 * Add a value to the dataset (ring buffer)
 */
void xrg_dataset_add_value(XRGDataset *dataset, gdouble value) {
    g_return_if_fail(dataset != NULL);

    /* Remove old value from sum if buffer is full */
    if (dataset->count == dataset->capacity) {
        dataset->sum -= dataset->values[dataset->index];
    }

    /* Add new value */
    dataset->values[dataset->index] = value;
    dataset->sum += value;

    /* Update statistics */
    if (value < dataset->min) dataset->min = value;
    if (value > dataset->max) dataset->max = value;

    /* Advance ring buffer index */
    dataset->index = (dataset->index + 1) % dataset->capacity;

    /* Update count */
    if (dataset->count < dataset->capacity) {
        dataset->count++;
    }

    /* Recalculate min/max periodically to handle decaying extrema */
    if (dataset->index == 0 && dataset->count == dataset->capacity) {
        dataset->min = G_MAXDOUBLE;
        dataset->max = -G_MAXDOUBLE;
        for (gint i = 0; i < dataset->capacity; i++) {
            if (dataset->values[i] < dataset->min) dataset->min = dataset->values[i];
            if (dataset->values[i] > dataset->max) dataset->max = dataset->values[i];
        }
    }
}

/**
 * Clear all values from the dataset
 */
void xrg_dataset_clear(XRGDataset *dataset) {
    g_return_if_fail(dataset != NULL);

    memset(dataset->values, 0, sizeof(gdouble) * dataset->capacity);
    dataset->count = 0;
    dataset->index = 0;
    dataset->min = G_MAXDOUBLE;
    dataset->max = -G_MAXDOUBLE;
    dataset->sum = 0.0;
}

/**
 * Resize the dataset capacity
 */
void xrg_dataset_resize(XRGDataset *dataset, gint new_capacity) {
    g_return_if_fail(dataset != NULL);
    g_return_if_fail(new_capacity > 0);

    if (new_capacity == dataset->capacity)
        return;

    /* Create new array */
    gdouble *new_values = g_new0(gdouble, new_capacity);

    /* Copy existing values */
    gint copy_count = MIN(dataset->count, new_capacity);
    for (gint i = 0; i < copy_count; i++) {
        gint old_idx = (dataset->index - dataset->count + i + dataset->capacity) % dataset->capacity;
        new_values[i] = dataset->values[old_idx];
    }

    /* Replace old array */
    g_free(dataset->values);
    dataset->values = new_values;
    dataset->capacity = new_capacity;
    dataset->count = copy_count;
    dataset->index = copy_count % new_capacity;

    /* Recalculate statistics */
    dataset->min = G_MAXDOUBLE;
    dataset->max = -G_MAXDOUBLE;
    dataset->sum = 0.0;
    for (gint i = 0; i < dataset->count; i++) {
        gdouble val = dataset->values[i];
        dataset->sum += val;
        if (val < dataset->min) dataset->min = val;
        if (val > dataset->max) dataset->max = val;
    }
}

/**
 * Get value at specific index (0 = oldest, count-1 = newest)
 */
gdouble xrg_dataset_get_value(XRGDataset *dataset, gint index) {
    g_return_val_if_fail(dataset != NULL, 0.0);
    g_return_val_if_fail(index >= 0 && index < dataset->count, 0.0);

    gint actual_index = (dataset->index - dataset->count + index + dataset->capacity) % dataset->capacity;
    return dataset->values[actual_index];
}

/**
 * Get the most recently added value
 */
gdouble xrg_dataset_get_latest(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0.0);
    g_return_val_if_fail(dataset->count > 0, 0.0);

    gint latest_index = (dataset->index - 1 + dataset->capacity) % dataset->capacity;
    return dataset->values[latest_index];
}

/**
 * Get current count of values
 */
gint xrg_dataset_get_count(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0);
    return dataset->count;
}

/**
 * Get capacity
 */
gint xrg_dataset_get_capacity(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0);
    return dataset->capacity;
}

/**
 * Get minimum value
 */
gdouble xrg_dataset_get_min(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0.0);
    return (dataset->count > 0) ? dataset->min : 0.0;
}

/**
 * Get maximum value
 */
gdouble xrg_dataset_get_max(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0.0);
    return (dataset->count > 0) ? dataset->max : 0.0;
}

/**
 * Get average value
 */
gdouble xrg_dataset_get_average(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0.0);
    if (dataset->count == 0)
        return 0.0;
    return dataset->sum / dataset->count;
}

/**
 * Get sum of all values
 */
gdouble xrg_dataset_get_sum(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, 0.0);
    return dataset->sum;
}

/**
 * Copy values to external array (oldest to newest)
 */
void xrg_dataset_copy_values(XRGDataset *dataset, gdouble *dest, gint max_count) {
    g_return_if_fail(dataset != NULL);
    g_return_if_fail(dest != NULL);

    gint copy_count = MIN(dataset->count, max_count);
    for (gint i = 0; i < copy_count; i++) {
        dest[i] = xrg_dataset_get_value(dataset, i);
    }
}

/**
 * Check if dataset is empty
 */
gboolean xrg_dataset_is_empty(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, TRUE);
    return dataset->count == 0;
}

/**
 * Check if dataset is full
 */
gboolean xrg_dataset_is_full(XRGDataset *dataset) {
    g_return_val_if_fail(dataset != NULL, FALSE);
    return dataset->count == dataset->capacity;
}
