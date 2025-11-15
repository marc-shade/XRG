#ifndef XRG_PREFERENCES_WINDOW_H
#define XRG_PREFERENCES_WINDOW_H

#include <gtk/gtk.h>
#include "../core/preferences.h"

typedef struct _XRGPreferencesWindow XRGPreferencesWindow;

/* Callback type for when preferences are applied */
typedef void (*XRGPreferencesAppliedCallback)(gpointer user_data);

/* Create preferences window */
XRGPreferencesWindow* xrg_preferences_window_new(GtkWindow *parent, XRGPreferences *prefs);

/* Set callback for when preferences are applied */
void xrg_preferences_window_set_applied_callback(XRGPreferencesWindow *win,
                                                   XRGPreferencesAppliedCallback callback,
                                                   gpointer user_data);

/* Show/hide window */
void xrg_preferences_window_show(XRGPreferencesWindow *win);
void xrg_preferences_window_hide(XRGPreferencesWindow *win);

/* Cleanup */
void xrg_preferences_window_free(XRGPreferencesWindow *win);

#endif /* XRG_PREFERENCES_WINDOW_H */
