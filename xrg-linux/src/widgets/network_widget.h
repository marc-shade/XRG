/**
 * XRG Network Widget Header
 *
 * Widget for displaying network traffic graphs and activity bars.
 * Shows download/upload rates.
 */

#ifndef XRG_NETWORK_WIDGET_H
#define XRG_NETWORK_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/network_collector.h"

/* Opaque widget type */
typedef struct _XRGNetworkWidget XRGNetworkWidget;

/* Constructor and destructor */
XRGNetworkWidget* xrg_network_widget_new(XRGPreferences *prefs, XRGNetworkCollector *collector);
void xrg_network_widget_free(XRGNetworkWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_network_widget_get_container(XRGNetworkWidget *widget);

/* Update widget (call after collector update) */
void xrg_network_widget_update(XRGNetworkWidget *widget);

/* Visibility control */
void xrg_network_widget_set_visible(XRGNetworkWidget *widget, gboolean visible);
gboolean xrg_network_widget_get_visible(XRGNetworkWidget *widget);

/* Request redraw */
void xrg_network_widget_queue_draw(XRGNetworkWidget *widget);

#endif /* XRG_NETWORK_WIDGET_H */
