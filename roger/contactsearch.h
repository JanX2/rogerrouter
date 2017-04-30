#ifndef CONTACT_SEARCH_H
#define CONTACT_SEARCH_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CONTACT_TYPE_SEARCH (contact_search_get_type())

G_DECLARE_FINAL_TYPE(ContactSearch, contact_search, CONTACT, SEARCH, GtkBox)

GtkWidget *contact_search_new(void);
gchar *contact_search_get_number(ContactSearch *widget);
void contact_search_clear(ContactSearch *widget);
void contact_search_set_text(ContactSearch *widget,
                             gchar         *text);
const gchar *contact_search_get_text(ContactSearch *widget);
void contact_search_set_contact(ContactSearch *widget, RmContact *contact, gboolean identify);

G_END_DECLS

#endif
