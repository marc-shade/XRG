/**
 * XRG CPU Widget Header
 *
 * Widget for displaying CPU usage graphs and activity bars.
 * Shows user/system CPU usage with load averages.
 */

#ifndef XRG_CPU_WIDGET_H
#define XRG_CPU_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/cpu_collector.h"

/* Opaque widget type */
typedef struct _XRGCPUWidget XRGCPUWidget;

/* Constructor and destructor */
XRGCPUWidget* xrg_cpu_widget_new(XRGPreferences *prefs, XRGCPUCollector *collector);
void xrg_cpu_widget_free(XRGCPUWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_cpu_widget_get_container(XRGCPUWidget *widget);

/* Update widget (call after collector update) */
void xrg_cpu_widget_update(XRGCPUWidget *widget);

/* Visibility control */
void xrg_cpu_widget_set_visible(XRGCPUWidget *widget, gboolean visible);
gboolean xrg_cpu_widget_get_visible(XRGCPUWidget *widget);

/* Request redraw */
void xrg_cpu_widget_queue_draw(XRGCPUWidget *widget);

#endif /* XRG_CPU_WIDGET_H */
