/**
 * XRG Battery Widget Header
 *
 * Widget for displaying battery status and charge/discharge rates.
 */

#ifndef XRG_BATTERY_WIDGET_H
#define XRG_BATTERY_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/battery_collector.h"

/* Opaque widget type */
typedef struct _XRGBatteryWidget XRGBatteryWidget;

/* Constructor and destructor */
XRGBatteryWidget* xrg_battery_widget_new(XRGPreferences *prefs, XRGBatteryCollector *collector);
void xrg_battery_widget_free(XRGBatteryWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_battery_widget_get_container(XRGBatteryWidget *widget);

/* Update widget (call after collector update) */
void xrg_battery_widget_update(XRGBatteryWidget *widget);

/* Visibility control */
void xrg_battery_widget_set_visible(XRGBatteryWidget *widget, gboolean visible);
gboolean xrg_battery_widget_get_visible(XRGBatteryWidget *widget);

/* Request redraw */
void xrg_battery_widget_queue_draw(XRGBatteryWidget *widget);

#endif /* XRG_BATTERY_WIDGET_H */
