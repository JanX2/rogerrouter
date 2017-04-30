#include <ctype.h>

#include <rm/rmaddressbook.h>
#include <rm/rmstring.h>
#include <rm/rmmain.h>
#include <rm/rmrouter.h>
#include <rm/rmobjectemit.h>
#include <rm/rmstring.h>

#include <roger/contactsearch.h>
#include <roger/contacts.h>
#include <roger/icons.h>
#include <roger/main.h>

struct _ContactSearch {
	GtkBox parent_instance;

	GtkWidget *entry;

	GtkWidget *menu;
	GtkWidget *box;
	GtkWidget *scrolled_win;
	gchar *filter;
	gboolean discard;
};

G_DEFINE_TYPE(ContactSearch, contact_search, GTK_TYPE_BOX);

static void contact_search_destroy_contacts(GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

static gboolean contact_search_filter_cb(GtkListBoxRow *row, gpointer user_data)
{
	ContactSearch *widget = user_data;
	GtkWidget *grid = gtk_bin_get_child(GTK_BIN(row));
	RmContact *contact = g_object_get_data(G_OBJECT(grid), "contact");

	g_assert(contact != NULL);

	if (RM_EMPTY_STRING(widget->filter)) {
		return TRUE;
	}

	return rm_strcasestr(contact->name, widget->filter) != NULL;
}

static gchar *fax_number_type_to_string(enum phone_number_type type)
{
	gchar *tmp;

	switch (type) {
	case PHONE_NUMBER_HOME:
		tmp = g_strdup(_("Home"));
		break;
	case PHONE_NUMBER_WORK:
		tmp = g_strdup(_("Work"));
		break;
	case PHONE_NUMBER_MOBILE:
		tmp = g_strdup(_("Mobile"));
		break;
	case PHONE_NUMBER_FAX_HOME:
		tmp = g_strdup(_("Fax Home"));
		break;
	case PHONE_NUMBER_FAX_WORK:
		tmp = g_strdup(_("Fax Work"));
		break;
	default:
		tmp = g_strdup(_("Unknown"));
		break;
	}

	return tmp;
}

void contact_search_set_contact(ContactSearch *widget, RmContact *contact, gboolean identify)
{
	RmContact *search_contact;

	if (!widget || !contact) {
		return;
	}

	widget->discard = TRUE;

	if (identify) {
		/* Copy contact and try to identify it */
		search_contact = rm_contact_dup(contact);
		rm_object_emit_contact_process(search_contact);
	} else {
		search_contact = contact;
	}

	g_object_set_data(G_OBJECT(widget->entry), "number", g_strdup(search_contact->number));

	if (!RM_EMPTY_STRING(search_contact->name)) {
		gtk_entry_set_text(GTK_ENTRY(widget->entry), search_contact->name);
	} else {
		gtk_entry_set_text(GTK_ENTRY(widget->entry), search_contact->number);
	}

	if (identify) {
		rm_contact_free(search_contact);
	}
}

static void contact_search_set_contact_by_row(ContactSearch *widget, GtkListBoxRow *row)
{
	GtkWidget *grid = gtk_bin_get_child(GTK_BIN(row));
	RmContact *contact;
	gchar *number = g_object_get_data(G_OBJECT(grid), "number");

	contact = g_object_get_data(G_OBJECT(grid), "contact");
	contact->number = number;
	contact_search_set_contact(widget, contact, FALSE);

	gtk_widget_hide(widget->menu);
}

static void contact_search_list_box_activated_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	g_assert(row != NULL);

	/* Set phone number details */
	contact_search_set_contact_by_row(user_data, row);
}

static void contact_search_search_changed_cb(ContactSearch *widget, gpointer user_data)
{
	GtkWidget *label;
	GSList *contacts = NULL;
	GSList *list;
	RmAddressBook *book;

	/* Get current filter text */
	widget->filter = (gchar*) gtk_entry_get_text(GTK_ENTRY(widget->entry));

	/* If it is an invalid filter, abort and close menu if needed */
	if (RM_EMPTY_STRING(widget->filter) || isdigit(widget->filter[0]) || widget->discard || widget->filter[0] == '*' || widget->filter[0] == '#') {
		widget->discard = FALSE;

		if (widget->menu) {
			gtk_widget_hide(widget->menu);
		}
		return;
	}

	gtk_widget_hide(widget->menu);

	/* Clean previous contacts */
	gtk_container_foreach(GTK_CONTAINER(widget->box), contact_search_destroy_contacts, NULL);

	widget->filter = (gchar*)gtk_entry_get_text(GTK_ENTRY(widget->entry));

	/* Add contacts to entry completion */
	book = rm_profile_get_addressbook(rm_profile_get_active());
	if (!book) {
		GSList *book_plugins = rm_addressbook_get_plugins();

		if (book_plugins) {
			book = book_plugins->data;
		}
	}

	if (book) {
		g_debug("%s(): book '%s'", __FUNCTION__, rm_addressbook_get_name(book));
	}
	contacts = rm_addressbook_get_contacts(book);

	for (list = contacts; list; list = list->next) {
		RmContact *contact = list->data;
		GSList *numbers;
		GtkWidget *num;
		GtkWidget *grid;
		gchar *name;

		if (!rm_strcasestr(contact->name, widget->filter) || !contact->numbers) {
			continue;
		}

		name = g_strdup_printf("<b>%s</b>", contact->name);

		for (numbers = contact->numbers; numbers != NULL; numbers = numbers->next) {
			RmPhoneNumber *number = numbers->data;
			GtkWidget *image;
			gchar *num_str;
			gchar *type;

			grid = gtk_grid_new();

			/* Set standard spacing */
			gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
			gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

			g_object_set_data(G_OBJECT(grid), "contact", contact);
			g_object_set_data(G_OBJECT(grid), "number", number->number);

			if (contact && contact->image) {
				GdkPixbuf *buf = image_get_scaled(contact->image, 32, 32);

				image = gtk_image_new_from_pixbuf(buf);
				g_object_unref(buf);
			} else {
				image = gtk_image_new_from_icon_name("avatar-default-symbolic", GTK_ICON_SIZE_DND);
			}

			gtk_grid_attach(GTK_GRID(grid), image, 0, 0, 1, 2);

			label = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(label), name);
			gtk_widget_set_halign(label, GTK_ALIGN_START);
			gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);

			type = fax_number_type_to_string(number->type);
			num_str = g_strdup_printf("%s: %s", type, number->number);
			num = gtk_label_new(num_str);
			g_free(num_str);
			g_free(type);
			gtk_widget_set_halign(num, GTK_ALIGN_START);
			gtk_widget_set_sensitive(num, FALSE);
			gtk_grid_attach(GTK_GRID(grid), num, 1, 1, 1, 1);
			gtk_widget_show_all(grid);

			gtk_list_box_insert(GTK_LIST_BOX(widget->box), grid, -1);
		}

		g_free(name);
	}

	gint width = gtk_widget_get_allocated_width(widget->entry);
	gint height = width;// gtk_widget_get_allocated_height(widget->frame);

	gtk_widget_set_size_request(widget->menu, width, height);

	gtk_widget_show_all(widget->menu);

	gtk_entry_grab_focus_without_selecting(GTK_ENTRY(widget->entry));
}

static void contact_search_icon_press_cb(ContactSearch *widget, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	if (icon_pos == GTK_ENTRY_ICON_PRIMARY) {
		/* If primary icon has been pressed, toggle menu */
		if (!widget->menu) {
			contact_search_search_changed_cb(CONTACT_SEARCH(widget->entry), NULL);
		} else {
			gtk_widget_hide(widget->menu);
		}
	} else if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
		g_object_set_data(G_OBJECT(widget->entry), "number", NULL);
	}
}

static void contact_list_box_set_focus(GtkWidget *scrolled_win, GtkWidget *box, GtkListBoxRow *row)
{
	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_win));
	gint wleft, wtop, wbottom;
	gdouble top, bottom, value;

	/* Get coordinated */
	gtk_widget_translate_coordinates(GTK_WIDGET(row), box, 0, 0, &wleft, &wtop);
	wbottom = wtop + gtk_widget_get_allocated_height(GTK_WIDGET(row));

	/* Compute new vertial adjustment value */
	top = gtk_adjustment_get_value(vadj);
	bottom = top + gtk_adjustment_get_page_size(vadj);

	if (wtop < top) {
		value = wtop;
	} else if (wbottom > bottom) {
		value = wbottom - gtk_adjustment_get_page_size(vadj);
	} else {
		value = top;
	}

	/* Set value */
	gtk_adjustment_set_value(vadj, value);
}

static gboolean contact_search_key_press_event_cb(ContactSearch *widget, GdkEvent *event, gpointer user_data)
{
	GtkListBoxRow *row = NULL;
	GList *childs;
	guint keyval = ((GdkEventKey *)event)->keyval;
	gint length;

	if (!widget->box) {
		return FALSE;
	}

	/* Escape pressed, unselect all and close menu */
	if (keyval == GDK_KEY_Escape) {
		gtk_list_box_unselect_all(GTK_LIST_BOX(widget->box));
		gtk_widget_hide(widget->menu);
		return TRUE;
	}

	/* Get selected row */
	row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widget->box));
	if (!row) {
		row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(widget->box), 0);

		if (!row) {
			return FALSE;
		}
	}

	/* If we have a selected row and return/enter is pressed, set number and exit */
	if (gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row)) && (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter)) {
		contact_search_set_contact_by_row(widget, row);

		return TRUE;
	}

	/* Get length of listbox childs */
	childs = gtk_container_get_children(GTK_CONTAINER(widget->box));
	g_assert(childs != NULL);

	length = g_list_length(childs);

	if (keyval == GDK_KEY_Up) {
		/* Handle key up */
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(widget->box), length - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(widget->box), GTK_LIST_BOX_ROW(row));
			contact_list_box_set_focus(widget->scrolled_win, widget->box, row);
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) > 0) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(widget->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) - 1);
			gtk_list_box_select_row(GTK_LIST_BOX(widget->box), GTK_LIST_BOX_ROW(row));
		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(widget->box));
		}
	} else if (keyval == GDK_KEY_Down) {
		/* Handle key down */
		if (!gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row))) {
			gtk_list_box_select_row(GTK_LIST_BOX(widget->box), GTK_LIST_BOX_ROW(row));
			contact_list_box_set_focus(widget->scrolled_win, widget->box, row);
			return FALSE;
		}

		if (gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) < (length - 1)) {
			row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(widget->box), gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row)) + 1);
			gtk_list_box_select_row(GTK_LIST_BOX(widget->box), GTK_LIST_BOX_ROW(row));

		} else {
			gtk_list_box_unselect_all(GTK_LIST_BOX(widget->box));
		}
	}

	contact_list_box_set_focus(widget->scrolled_win, widget->box, row);

	return FALSE;
}

static void contact_search_class_init(ContactSearchClass *klass)
{
	//GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	//gobject_class = G_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);

	gtk_widget_class_set_template_from_resource(widget_class, "/org/tabos/roger/contactsearch.glade");

	gtk_widget_class_bind_template_child(widget_class, ContactSearch, entry);

	gtk_widget_class_bind_template_callback(widget_class, contact_search_search_changed_cb);
	gtk_widget_class_bind_template_callback(widget_class, contact_search_key_press_event_cb);
	gtk_widget_class_bind_template_callback(widget_class, contact_search_icon_press_cb);
}

static void contact_search_init(ContactSearch *widget)
{
	GtkWidget *placeholder;
	GtkWidget *image;
	GtkWidget *text;

	gtk_widget_init_template(GTK_WIDGET(widget));
	/* Create popover */
	widget->menu = gtk_popover_new(NULL);
	gtk_popover_set_modal(GTK_POPOVER(widget->menu), FALSE);
	gtk_popover_set_position(GTK_POPOVER(widget->menu), GTK_POS_BOTTOM);
	gtk_popover_set_relative_to(GTK_POPOVER(widget->menu), GTK_WIDGET(widget->entry));

	widget->scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(widget->menu), widget->scrolled_win);

	widget->box = gtk_list_box_new();

	placeholder = gtk_grid_new();
	gtk_widget_set_sensitive(placeholder, FALSE);
	gtk_widget_set_halign(placeholder, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(placeholder, GTK_ALIGN_CENTER);

	image = gtk_image_new_from_icon_name(AVATAR_DEFAULT, GTK_ICON_SIZE_DIALOG);
	gtk_grid_attach(GTK_GRID(placeholder), image, 0, 0, 1, 1);

	text = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(text), _("<b>No contact found</b>"));
	gtk_widget_set_sensitive(text, FALSE);
	gtk_grid_attach(GTK_GRID(placeholder), text, 0, 1, 1, 1);

 	gtk_list_box_set_placeholder(GTK_LIST_BOX(widget->box), placeholder);
	gtk_list_box_set_filter_func(GTK_LIST_BOX(widget->box), contact_search_filter_cb, widget, NULL);
	gtk_container_add(GTK_CONTAINER(widget->scrolled_win), widget->box);
	g_signal_connect(G_OBJECT(widget->box), "row-activated", G_CALLBACK(contact_search_list_box_activated_cb), widget);
}

GtkWidget *contact_search_new(void)
{
	return g_object_new(CONTACT_TYPE_SEARCH, NULL);
}


gchar *contact_search_get_number(ContactSearch *widget)
{
	gchar *number = g_object_get_data(G_OBJECT(widget->entry), "number");

	if (RM_EMPTY_STRING(number)) {
		number = (gchar*) gtk_entry_get_text(GTK_ENTRY(widget->entry));
	}

	return number;
}

void contact_search_clear(ContactSearch *widget)
{
	gtk_entry_set_text(GTK_ENTRY(widget->entry), "");
}

void contact_search_set_text(ContactSearch *widget,
                             gchar         *text)
{
	gtk_entry_set_text(GTK_ENTRY(widget->entry), text);
}

const gchar *contact_search_get_text(ContactSearch *widget)
{
	return gtk_entry_get_text(GTK_ENTRY(widget->entry));
}
