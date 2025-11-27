/**
 * XRG Process Collector Implementation
 *
 * Reads process information from /proc filesystem.
 */

#include "process_collector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

struct _XRGProcessCollector {
    GList *processes;               /* List of XRGProcessInfo */
    gint max_processes;             /* Maximum processes to track */
    XRGProcessSortBy sort_by;       /* Current sort criteria */
    gboolean sort_descending;       /* Sort order */
    gboolean show_all_users;        /* Show processes from all users */
    gchar *filter;                  /* Name filter */
    uid_t current_uid;              /* Current user's UID */

    /* System info */
    gint total_processes;           /* Total process count */
    gint running_processes;         /* Running process count */
    guint64 total_memory;           /* Total system memory */
    guint64 page_size;              /* System page size */
    guint64 clock_ticks;            /* Clock ticks per second */
    gdouble uptime_seconds;         /* System uptime */

    /* Previous CPU times for delta calculation */
    guint64 prev_total_cpu;         /* Previous total CPU time */
    GHashTable *prev_cpu_times;     /* pid -> prev total time */
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

static gchar* read_file_contents(const gchar *path, gsize max_len, gsize *bytes_read) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    gchar *buffer = g_malloc(max_len + 1);
    gsize len = fread(buffer, 1, max_len, f);
    fclose(f);

    buffer[len] = '\0';
    if (bytes_read) *bytes_read = len;
    return buffer;
}

static gchar* read_cmdline(pid_t pid) {
    gchar path[64];
    g_snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    gsize bytes_read = 0;
    gchar *cmdline = read_file_contents(path, 256, &bytes_read);
    if (!cmdline || bytes_read == 0) {
        g_free(cmdline);
        return NULL;
    }

    /* Replace null bytes with spaces (cmdline has null-separated args) */
    for (gsize i = 0; i < bytes_read - 1; i++) {  /* -1 to keep final null */
        if (cmdline[i] == '\0') {
            cmdline[i] = ' ';
        }
    }

    /* Trim trailing whitespace */
    g_strchomp(cmdline);

    return cmdline;
}

static gchar* read_comm(pid_t pid) {
    gchar path[64];
    g_snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    gchar *comm = read_file_contents(path, 64, NULL);
    if (comm) {
        g_strchomp(comm);
    }
    return comm;
}

static gchar* get_username(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return g_strdup(pw->pw_name);
    }
    return g_strdup_printf("%d", uid);
}

static guint64 get_total_cpu_time(void) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;

    guint64 user, nice, system, idle, iowait, irq, softirq, steal;
    user = nice = system = idle = iowait = irq = softirq = steal = 0;

    if (fscanf(f, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 4) {
        fclose(f);
        return 0;
    }
    fclose(f);

    return user + nice + system + idle + iowait + irq + softirq + steal;
}

static gdouble get_uptime(void) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) return 0;

    gdouble uptime = 0;
    fscanf(f, "%lf", &uptime);
    fclose(f);

    return uptime;
}

static guint64 get_total_memory(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;

    gchar line[256];
    guint64 total = 0;

    while (fgets(line, sizeof(line), f)) {
        if (g_str_has_prefix(line, "MemTotal:")) {
            sscanf(line, "MemTotal: %lu kB", &total);
            total *= 1024;  /* Convert to bytes */
            break;
        }
    }
    fclose(f);

    return total;
}

/*============================================================================
 * Process Parsing
 *============================================================================*/

static XRGProcessInfo* parse_process(XRGProcessCollector *collector, pid_t pid) {
    gchar path[64];

    /* Read /proc/[pid]/stat */
    g_snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    gchar *stat_contents = read_file_contents(path, 1024, NULL);
    if (!stat_contents) return NULL;

    /* Parse stat file - format is complex due to comm field possibly containing spaces/parens */
    /* Find the last ')' to locate end of comm field */
    gchar *comm_end = strrchr(stat_contents, ')');
    if (!comm_end) {
        g_free(stat_contents);
        return NULL;
    }

    /* Fields after comm: state, ppid, pgrp, session, tty_nr, tpgid, flags, minflt, ... */
    gchar state;
    gint ppid, pgrp, session, tty_nr, tpgid;
    guint flags;
    guint64 minflt, cminflt, majflt, cmajflt, utime, stime, cutime, cstime;
    gint priority, nice_val, num_threads;
    guint64 itrealvalue, starttime, vsize;
    guint64 rss;

    gint fields = sscanf(comm_end + 2,
        "%c %d %d %d %d %d %u "
        "%lu %lu %lu %lu %lu %lu %lu %lu "
        "%d %d %d "
        "%lu %lu %lu "
        "%lu",
        &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags,
        &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime,
        &priority, &nice_val, &num_threads,
        &itrealvalue, &starttime, &vsize,
        &rss);

    g_free(stat_contents);

    if (fields < 22) return NULL;

    /* Read UID from /proc/[pid]/status */
    g_snprintf(path, sizeof(path), "/proc/%d/status", pid);
    gchar *status_contents = read_file_contents(path, 2048, NULL);
    uid_t uid = 0;

    if (status_contents) {
        gchar *uid_line = strstr(status_contents, "\nUid:");
        if (uid_line) {
            sscanf(uid_line + 5, "%u", &uid);
        }
        g_free(status_contents);
    }

    /* Filter by user if needed */
    if (!collector->show_all_users && uid != collector->current_uid) {
        return NULL;
    }

    /* Create process info */
    XRGProcessInfo *info = g_new0(XRGProcessInfo, 1);
    info->pid = pid;
    info->name = read_comm(pid);
    info->cmdline = read_cmdline(pid);
    info->state = state;
    info->uid = uid;
    info->username = get_username(uid);
    info->utime = utime;
    info->stime = stime;
    info->start_time = starttime;
    info->nice = nice_val;
    info->threads = num_threads;
    info->mem_vsize = vsize;
    info->mem_rss = rss * collector->page_size;

    /* Calculate memory percentage */
    if (collector->total_memory > 0) {
        info->mem_percent = (gdouble)info->mem_rss / collector->total_memory * 100.0;
    }

    /* CPU percentage will be calculated after we have delta times */
    info->cpu_percent = 0.0;

    /* Apply name filter if set */
    if (collector->filter && collector->filter[0] != '\0') {
        gboolean matches = FALSE;
        if (info->name && strcasestr(info->name, collector->filter)) {
            matches = TRUE;
        } else if (info->cmdline && strcasestr(info->cmdline, collector->filter)) {
            matches = TRUE;
        }
        if (!matches) {
            xrg_process_info_free(info);
            return NULL;
        }
    }

    return info;
}

/*============================================================================
 * Sorting
 *============================================================================*/

static gint compare_processes(gconstpointer a, gconstpointer b, gpointer user_data) {
    const XRGProcessInfo *pa = a;
    const XRGProcessInfo *pb = b;
    XRGProcessCollector *collector = user_data;

    gint result = 0;

    switch (collector->sort_by) {
        case XRG_PROCESS_SORT_CPU:
            if (pa->cpu_percent > pb->cpu_percent) result = -1;
            else if (pa->cpu_percent < pb->cpu_percent) result = 1;
            break;

        case XRG_PROCESS_SORT_MEMORY:
            if (pa->mem_percent > pb->mem_percent) result = -1;
            else if (pa->mem_percent < pb->mem_percent) result = 1;
            break;

        case XRG_PROCESS_SORT_PID:
            result = pa->pid - pb->pid;
            break;

        case XRG_PROCESS_SORT_NAME:
            if (pa->name && pb->name) {
                result = g_ascii_strcasecmp(pa->name, pb->name);
            }
            break;
    }

    if (!collector->sort_descending) {
        result = -result;
    }

    return result;
}

/*============================================================================
 * Public API
 *============================================================================*/

XRGProcessCollector* xrg_process_collector_new(gint max_processes) {
    XRGProcessCollector *collector = g_new0(XRGProcessCollector, 1);

    collector->max_processes = max_processes > 0 ? max_processes : 10;
    collector->sort_by = XRG_PROCESS_SORT_CPU;
    collector->sort_descending = TRUE;
    collector->show_all_users = TRUE;
    collector->current_uid = getuid();
    collector->page_size = sysconf(_SC_PAGESIZE);
    collector->clock_ticks = sysconf(_SC_CLK_TCK);
    collector->total_memory = get_total_memory();
    collector->prev_cpu_times = g_hash_table_new(g_direct_hash, g_direct_equal);

    return collector;
}

void xrg_process_collector_free(XRGProcessCollector *collector) {
    if (!collector) return;

    /* Free process list */
    g_list_free_full(collector->processes, (GDestroyNotify)xrg_process_info_free);

    /* Free other resources */
    g_free(collector->filter);
    g_hash_table_destroy(collector->prev_cpu_times);

    g_free(collector);
}

void xrg_process_collector_update(XRGProcessCollector *collector) {
    if (!collector) return;

    /* Get current system state */
    guint64 total_cpu = get_total_cpu_time();
    guint64 cpu_delta = total_cpu - collector->prev_total_cpu;
    collector->prev_total_cpu = total_cpu;
    collector->uptime_seconds = get_uptime();

    /* Free old process list */
    g_list_free_full(collector->processes, (GDestroyNotify)xrg_process_info_free);
    collector->processes = NULL;

    /* Read /proc directory */
    DIR *dir = opendir("/proc");
    if (!dir) return;

    collector->total_processes = 0;
    collector->running_processes = 0;

    GList *all_processes = NULL;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip non-numeric entries */
        if (!isdigit(entry->d_name[0])) continue;

        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        XRGProcessInfo *info = parse_process(collector, pid);
        if (!info) continue;

        collector->total_processes++;
        if (info->state == 'R') {
            collector->running_processes++;
        }

        /* Calculate CPU percentage using delta from previous update */
        guint64 proc_total = info->utime + info->stime;
        guint64 prev_total = GPOINTER_TO_SIZE(
            g_hash_table_lookup(collector->prev_cpu_times, GINT_TO_POINTER(pid)));

        if (cpu_delta > 0 && prev_total > 0) {
            guint64 proc_delta = proc_total - prev_total;
            info->cpu_percent = (gdouble)proc_delta / cpu_delta * 100.0;
        }

        /* Store for next update */
        g_hash_table_insert(collector->prev_cpu_times,
                           GINT_TO_POINTER(pid),
                           GSIZE_TO_POINTER(proc_total));

        all_processes = g_list_prepend(all_processes, info);
    }
    closedir(dir);

    /* Sort processes */
    all_processes = g_list_sort_with_data(all_processes, compare_processes, collector);

    /* Keep only top N processes, free the rest */
    gint count = 0;
    GList *l = all_processes;
    while (l != NULL) {
        GList *next = l->next;  /* Save next pointer before any modifications */
        XRGProcessInfo *info = l->data;

        if (count < collector->max_processes) {
            /* Keep this process */
            collector->processes = g_list_append(collector->processes, info);
            count++;
        } else {
            /* Free this process (over the limit) */
            xrg_process_info_free(info);
        }
        l = next;
    }

    /* Free the GList nodes (data already handled above) */
    g_list_free(all_processes);

    /* Clean up stale entries from prev_cpu_times */
    /* (This is a simplification - could be optimized) */
}

GList* xrg_process_collector_get_processes(XRGProcessCollector *collector) {
    return collector ? collector->processes : NULL;
}

gint xrg_process_collector_get_process_count(XRGProcessCollector *collector) {
    return collector ? g_list_length(collector->processes) : 0;
}

XRGProcessInfo* xrg_process_collector_get_process(XRGProcessCollector *collector, pid_t pid) {
    if (!collector) return NULL;

    for (GList *l = collector->processes; l != NULL; l = l->next) {
        XRGProcessInfo *info = l->data;
        if (info->pid == pid) {
            return info;
        }
    }
    return NULL;
}

void xrg_process_collector_set_sort_by(XRGProcessCollector *collector, XRGProcessSortBy sort_by) {
    if (collector) {
        collector->sort_by = sort_by;
    }
}

XRGProcessSortBy xrg_process_collector_get_sort_by(XRGProcessCollector *collector) {
    return collector ? collector->sort_by : XRG_PROCESS_SORT_CPU;
}

void xrg_process_collector_set_sort_descending(XRGProcessCollector *collector, gboolean descending) {
    if (collector) {
        collector->sort_descending = descending;
    }
}

gboolean xrg_process_collector_get_sort_descending(XRGProcessCollector *collector) {
    return collector ? collector->sort_descending : TRUE;
}

void xrg_process_collector_set_show_all_users(XRGProcessCollector *collector, gboolean show_all) {
    if (collector) {
        collector->show_all_users = show_all;
    }
}

gboolean xrg_process_collector_get_show_all_users(XRGProcessCollector *collector) {
    return collector ? collector->show_all_users : TRUE;
}

void xrg_process_collector_set_filter(XRGProcessCollector *collector, const gchar *filter) {
    if (collector) {
        g_free(collector->filter);
        collector->filter = filter ? g_strdup(filter) : NULL;
    }
}

const gchar* xrg_process_collector_get_filter(XRGProcessCollector *collector) {
    return collector ? collector->filter : NULL;
}

gint xrg_process_collector_get_total_processes(XRGProcessCollector *collector) {
    return collector ? collector->total_processes : 0;
}

gint xrg_process_collector_get_running_processes(XRGProcessCollector *collector) {
    return collector ? collector->running_processes : 0;
}

guint64 xrg_process_collector_get_total_memory(XRGProcessCollector *collector) {
    return collector ? collector->total_memory : 0;
}

gdouble xrg_process_collector_get_uptime_seconds(XRGProcessCollector *collector) {
    return collector ? collector->uptime_seconds : 0;
}

void xrg_process_info_free(XRGProcessInfo *info) {
    if (!info) return;
    g_free(info->name);
    g_free(info->cmdline);
    g_free(info->username);
    g_free(info);
}

XRGProcessInfo* xrg_process_info_copy(const XRGProcessInfo *info) {
    if (!info) return NULL;

    XRGProcessInfo *copy = g_new0(XRGProcessInfo, 1);
    *copy = *info;
    copy->name = g_strdup(info->name);
    copy->cmdline = g_strdup(info->cmdline);
    copy->username = g_strdup(info->username);

    return copy;
}

const gchar* xrg_process_state_name(gchar state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk Sleep";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing";
        case 'X': return "Dead";
        case 'x': return "Dead";
        case 'K': return "Wakekill";
        case 'W': return "Waking";
        case 'P': return "Parked";
        case 'I': return "Idle";
        default: return "Unknown";
    }
}
