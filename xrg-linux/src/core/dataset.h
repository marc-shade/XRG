#ifndef XRG_DATASET_H
#define XRG_DATASET_H

#include <glib.h>
#include <stdint.h>

/**
 * XRGDataset - Ring buffer for time-series data
 *
 * Equivalent to XRGDataSet in the macOS version.
 * Stores a fixed-size ring buffer of double values for efficient
 * time-series graph rendering.
 */

typedef struct _XRGDataset XRGDataset;

struct _XRGDataset {
    gdouble *values;        /* Array of values */
    gint capacity;          /* Maximum number of values */
    gint count;             /* Current number of values */
    gint index;             /* Current insertion index */
    gdouble min;            /* Minimum value in dataset */
    gdouble max;            /* Maximum value in dataset */
    gdouble sum;            /* Sum of all values */
};

/* Constructor and destructor */
XRGDataset* xrg_dataset_new(gint capacity);
void xrg_dataset_free(XRGDataset *dataset);

/* Data manipulation */
void xrg_dataset_add_value(XRGDataset *dataset, gdouble value);
void xrg_dataset_clear(XRGDataset *dataset);
void xrg_dataset_resize(XRGDataset *dataset, gint new_capacity);

/* Data access */
gdouble xrg_dataset_get_value(XRGDataset *dataset, gint index);
gdouble xrg_dataset_get_latest(XRGDataset *dataset);
gint xrg_dataset_get_count(XRGDataset *dataset);
gint xrg_dataset_get_capacity(XRGDataset *dataset);

/* Statistics */
gdouble xrg_dataset_get_min(XRGDataset *dataset);
gdouble xrg_dataset_get_max(XRGDataset *dataset);
gdouble xrg_dataset_get_average(XRGDataset *dataset);
gdouble xrg_dataset_get_sum(XRGDataset *dataset);

/* Utility */
void xrg_dataset_copy_values(XRGDataset *dataset, gdouble *dest, gint max_count);
gboolean xrg_dataset_is_empty(XRGDataset *dataset);
gboolean xrg_dataset_is_full(XRGDataset *dataset);

#endif /* XRG_DATASET_H */
