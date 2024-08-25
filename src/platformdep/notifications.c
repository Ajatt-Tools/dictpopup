#include <libnotify/notify.h>

#include "platformdep/notifications.h"

void notify(_Bool urgent, char const *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    g_autofree char *txt = g_strdup_vprintf(fmt, argp);
    va_end(argp);
    if (!txt)
        return;

    notify_init("dictpopup");
    NotifyNotification *n = notify_notification_new("dictpopup", txt, NULL);
    notify_notification_set_urgency(n, urgent ? NOTIFY_URGENCY_CRITICAL : NOTIFY_URGENCY_NORMAL);
    notify_notification_set_timeout(n, 10000);

    GError *err = NULL;
    if (!notify_notification_show(n, &err)) {
        fprintf(stderr, "Failed to send notification: %s\n", err->message);
        g_error_free(err);
    }
    g_object_unref(G_OBJECT(n));
}