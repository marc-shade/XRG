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

/* Show window and navigate to specific module tab */
typedef enum {
    XRG_PREFS_TAB_WINDOW = 0,
    XRG_PREFS_TAB_CPU = 1,
    XRG_PREFS_TAB_MEMORY = 2,
    XRG_PREFS_TAB_NETWORK = 3,
    XRG_PREFS_TAB_DISK = 4,
    XRG_PREFS_TAB_GPU = 5,
    XRG_PREFS_TAB_BATTERY = 6,
    XRG_PREFS_TAB_TEMPERATURE = 7,
    XRG_PREFS_TAB_AITOKEN = 8,
    XRG_PREFS_TAB_TPU = 9,
    XRG_PREFS_TAB_PROCESS = 10,
    XRG_PREFS_TAB_COLORS = 11
} XRGPrefsTab;

void xrg_preferences_window_show_tab(XRGPreferencesWindow *win, XRGPrefsTab tab);

/* Cleanup */
void xrg_preferences_window_free(XRGPreferencesWindow *win);

#endif /* XRG_PREFERENCES_WINDOW_H */
