/**
 * XRG AI Token Widget Header
 *
 * Widget for displaying AI token usage from Claude, Codex, Gemini, etc.
 */

#ifndef XRG_AITOKEN_WIDGET_H
#define XRG_AITOKEN_WIDGET_H

#include <gtk/gtk.h>
#include "base_widget.h"
#include "../collectors/aitoken_collector.h"

/* Opaque widget type */
typedef struct _XRGAITokenWidget XRGAITokenWidget;

/* Constructor and destructor */
XRGAITokenWidget* xrg_aitoken_widget_new(XRGPreferences *prefs, XRGAITokenCollector *collector);
void xrg_aitoken_widget_free(XRGAITokenWidget *widget);

/* Get GTK container for adding to parent */
GtkWidget* xrg_aitoken_widget_get_container(XRGAITokenWidget *widget);

/* Update widget (call after collector update) */
void xrg_aitoken_widget_update(XRGAITokenWidget *widget);

/* Visibility control */
void xrg_aitoken_widget_set_visible(XRGAITokenWidget *widget, gboolean visible);
gboolean xrg_aitoken_widget_get_visible(XRGAITokenWidget *widget);

/* Request redraw */
void xrg_aitoken_widget_queue_draw(XRGAITokenWidget *widget);

#endif /* XRG_AITOKEN_WIDGET_H */
