#ifndef XRG_UTILS_H
#define XRG_UTILS_H

#include <glib.h>

/* String formatting utilities */
gchar* xrg_format_bytes(guint64 bytes);
gchar* xrg_format_rate(gdouble rate);
gchar* xrg_format_percentage(gdouble percentage);
gchar* xrg_format_time_duration(guint seconds);

/* File utilities */
gboolean xrg_file_exists(const gchar *path);
gchar* xrg_read_file_contents(const gchar *path, gsize *length);

/* Math utilities */
gdouble xrg_clamp(gdouble value, gdouble min, gdouble max);
gint xrg_round_to_int(gdouble value);

#endif /* XRG_UTILS_H */
