/**
 * XRG Sensors Widget Header
 *
 * Widget for displaying temperature sensors from lm-sensors.
 */

#ifndef XRG_SENSORS_WIDGET_H
#define XRG_SENSORS_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/sensors_collector.h"

/* Opaque widget type */
typedef struct _XRGSensorsWidget XRGSensorsWidget;

/* Constructor and destructor */
XRGSensorsWidget* xrg_sensors_widget_new(XRGPreferences *prefs, XRGSensorsCollector *collector);
void xrg_sensors_widget_free(XRGSensorsWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_sensors_widget_get_container(XRGSensorsWidget *widget);

/* Update widget (call after collector update) */
void xrg_sensors_widget_update(XRGSensorsWidget *widget);

/* Visibility control */
void xrg_sensors_widget_set_visible(XRGSensorsWidget *widget, gboolean visible);
gboolean xrg_sensors_widget_get_visible(XRGSensorsWidget *widget);

/* Request redraw */
void xrg_sensors_widget_queue_draw(XRGSensorsWidget *widget);

#endif /* XRG_SENSORS_WIDGET_H */
