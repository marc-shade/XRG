/**
 * XRG GPU Widget Header
 *
 * Widget for displaying GPU utilization and memory graphs.
 * Shows utilization, memory, and temperature.
 */

#ifndef XRG_GPU_WIDGET_H
#define XRG_GPU_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/gpu_collector.h"

/* Opaque widget type */
typedef struct _XRGGPUWidget XRGGPUWidget;

/* Constructor and destructor */
XRGGPUWidget* xrg_gpu_widget_new(XRGPreferences *prefs, XRGGPUCollector *collector);
void xrg_gpu_widget_free(XRGGPUWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_gpu_widget_get_container(XRGGPUWidget *widget);

/* Update widget (call after collector update) */
void xrg_gpu_widget_update(XRGGPUWidget *widget);

/* Visibility control */
void xrg_gpu_widget_set_visible(XRGGPUWidget *widget, gboolean visible);
gboolean xrg_gpu_widget_get_visible(XRGGPUWidget *widget);

/* Request redraw */
void xrg_gpu_widget_queue_draw(XRGGPUWidget *widget);

#endif /* XRG_GPU_WIDGET_H */
