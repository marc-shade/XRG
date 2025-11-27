/**
 * XRG Memory Widget Header
 *
 * Widget for displaying memory usage graphs and activity bars.
 * Shows used/wired/cached memory breakdown.
 */

#ifndef XRG_MEMORY_WIDGET_H
#define XRG_MEMORY_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/memory_collector.h"

/* Opaque widget type */
typedef struct _XRGMemoryWidget XRGMemoryWidget;

/* Constructor and destructor */
XRGMemoryWidget* xrg_memory_widget_new(XRGPreferences *prefs, XRGMemoryCollector *collector);
void xrg_memory_widget_free(XRGMemoryWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_memory_widget_get_container(XRGMemoryWidget *widget);

/* Update widget (call after collector update) */
void xrg_memory_widget_update(XRGMemoryWidget *widget);

/* Visibility control */
void xrg_memory_widget_set_visible(XRGMemoryWidget *widget, gboolean visible);
gboolean xrg_memory_widget_get_visible(XRGMemoryWidget *widget);

/* Request redraw */
void xrg_memory_widget_queue_draw(XRGMemoryWidget *widget);

#endif /* XRG_MEMORY_WIDGET_H */
