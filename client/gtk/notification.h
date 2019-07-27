#ifndef _notification_h
#define _notification_h

#include <glib.h>

void notification_init(void);
void notification_cleanup(void);
void notification_send(const gchar * text, const gchar * icon);
void notification_close(void);
gboolean get_show_notifications(void);
void set_show_notifications(gboolean notify);

#endif
