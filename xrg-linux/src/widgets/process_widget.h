/**
 * XRG Process Widget Header
 *
 * Displays top processes by CPU or memory usage in a compact list format.
 */

#ifndef XRG_PROCESS_WIDGET_H
#define XRG_PROCESS_WIDGET_H

#include <gtk/gtk.h>
#include "../core/preferences.h"
#include "../collectors/process_collector.h"

typedef struct _XRGProcessWidget XRGProcessWidget;

/* Constructor and destructor */
XRGProcessWidget* xrg_process_widget_new(XRGPreferences *prefs, XRGProcessCollector *collector);
void xrg_process_widget_free(XRGProcessWidget *widget);

/* Get GTK container */
GtkWidget* xrg_process_widget_get_container(XRGProcessWidget *widget);

/* Update display */
void xrg_process_widget_update(XRGProcessWidget *widget);

/* Visibility control */
void xrg_process_widget_set_visible(XRGProcessWidget *widget, gboolean visible);
gboolean xrg_process_widget_get_visible(XRGProcessWidget *widget);

/* Request redraw */
void xrg_process_widget_queue_draw(XRGProcessWidget *widget);

/* Get collector */
XRGProcessCollector* xrg_process_widget_get_collector(XRGProcessWidget *widget);

#endif /* XRG_PROCESS_WIDGET_H */
