/**
 * XRG TPU Widget
 *
 * Widget for displaying Google Coral Edge TPU status and statistics.
 */

#ifndef XRG_TPU_WIDGET_H
#define XRG_TPU_WIDGET_H

#include <gtk/gtk.h>
#include "../collectors/tpu_collector.h"
#include "../core/preferences.h"

typedef struct _XRGTPUWidget XRGTPUWidget;

/* Constructor and destructor */
XRGTPUWidget* xrg_tpu_widget_new(XRGPreferences *prefs, XRGTPUCollector *collector);
void xrg_tpu_widget_free(XRGTPUWidget *widget);

/* Get GTK container */
GtkWidget* xrg_tpu_widget_get_container(XRGTPUWidget *widget);

/* Update and redraw */
void xrg_tpu_widget_update(XRGTPUWidget *widget);
void xrg_tpu_widget_queue_draw(XRGTPUWidget *widget);

/* Visibility */
void xrg_tpu_widget_set_visible(XRGTPUWidget *widget, gboolean visible);
gboolean xrg_tpu_widget_get_visible(XRGTPUWidget *widget);

#endif /* XRG_TPU_WIDGET_H */
