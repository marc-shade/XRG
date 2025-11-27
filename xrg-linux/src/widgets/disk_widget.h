/**
 * XRG Disk Widget Header
 *
 * Widget for displaying disk I/O graphs and activity bars.
 * Shows read/write rates.
 */

#ifndef XRG_DISK_WIDGET_H
#define XRG_DISK_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/disk_collector.h"

/* Opaque widget type */
typedef struct _XRGDiskWidget XRGDiskWidget;

/* Constructor and destructor */
XRGDiskWidget* xrg_disk_widget_new(XRGPreferences *prefs, XRGDiskCollector *collector);
void xrg_disk_widget_free(XRGDiskWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_disk_widget_get_container(XRGDiskWidget *widget);

/* Update widget (call after collector update) */
void xrg_disk_widget_update(XRGDiskWidget *widget);

/* Visibility control */
void xrg_disk_widget_set_visible(XRGDiskWidget *widget, gboolean visible);
gboolean xrg_disk_widget_get_visible(XRGDiskWidget *widget);

/* Request redraw */
void xrg_disk_widget_queue_draw(XRGDiskWidget *widget);

#endif /* XRG_DISK_WIDGET_H */
