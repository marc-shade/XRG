#ifndef XRG_PREFERENCES_WINDOW_H
#define XRG_PREFERENCES_WINDOW_H

#include <gtk/gtk.h>
#include "../core/preferences.h"

typedef struct _XRGPreferencesWindow XRGPreferencesWindow;

/* Create preferences window */
XRGPreferencesWindow* xrg_preferences_window_new(GtkWindow *parent, XRGPreferences *prefs);

/* Show/hide window */
void xrg_preferences_window_show(XRGPreferencesWindow *win);
void xrg_preferences_window_hide(XRGPreferencesWindow *win);

/* Cleanup */
void xrg_preferences_window_free(XRGPreferencesWindow *win);

#endif /* XRG_PREFERENCES_WINDOW_H */
