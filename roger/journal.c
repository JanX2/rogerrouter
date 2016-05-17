/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This file is part of Roger Router.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>
#include <libroutermanager/call.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/filter.h>
#include <libroutermanager/appobject.h>
#include <libroutermanager/file.h>
#include <libroutermanager/osdep.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/lookup.h>
#include <libroutermanager/csv.h>
#include <libroutermanager/gstring.h>

#include <roger/main.h>
#include <roger/phone.h>
#include <roger/journal.h>
#include <roger/print.h>
#include <roger/contacts.h>
#include <roger/icons.h>
#include <roger/application.h>
#include <roger/uitools.h>
#include <roger/answeringmachine.h>

GtkWidget *journal_view = NULL;
GtkWidget *journal_win = NULL;
GtkWidget *journal_filter_box = NULL;
GSList *journal_list = NULL;
GApplication *journal_application = NULL;
static GdkPixbuf *icon_call_in = NULL;
static GdkPixbuf *icon_call_missed = NULL;
static GdkPixbuf *icon_call_out = NULL;
static GdkPixbuf *icon_fax = NULL;
static GdkPixbuf *icon_fax_report = NULL;
static GdkPixbuf *icon_voice = NULL;
static GdkPixbuf *icon_record = NULL;
static GdkPixbuf *icon_blocked = NULL;
static struct filter *journal_filter = NULL;
static struct filter *journal_search_filter = NULL;
static GtkWidget *spinner = NULL;
static GMutex journal_mutex;
static gboolean use_header = FALSE;

gboolean roger_uses_headerbar(void)
{
	//return TRUE;
	//return FALSE;
	return use_header;
}

void journal_clear(void)
{
	GtkListStore *list_store;

	if (!journal_win) {
		return;
	}

	list_store = g_object_get_data(G_OBJECT(journal_win), "list_store");

	gtk_list_store_clear(list_store);
}

GdkPixbuf *pixbuf_copy_mirror(GdkPixbuf *src)
{
	GdkPixbuf *dest;
	gint has_alpha;
	gint w, h, srs;
	gint drs;
	guchar *s_pix;
	guchar *d_pix;
	guchar *sp;
	guchar *dp;
	gint i, j;
	gint a;

	if (!src) {
		return NULL;
	}

	w = gdk_pixbuf_get_width(src);
	h = gdk_pixbuf_get_height(src);
	has_alpha = gdk_pixbuf_get_has_alpha(src);
	srs = gdk_pixbuf_get_rowstride(src);
	s_pix = gdk_pixbuf_get_pixels(src);

	dest = gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8, w, h);
	drs = gdk_pixbuf_get_rowstride(dest);
	d_pix = gdk_pixbuf_get_pixels(dest);

	a = has_alpha ? 4 : 3;

	for (i = 0; i < h; i++) {
		sp = s_pix + (i * srs);
		dp = d_pix + (i * drs);

		dp += (w - 1) * a;

		for (j = 0; j < w; j++) {
			*(dp++) = *(sp++);	/* r */
			*(dp++) = *(sp++);	/* g */
			*(dp++) = *(sp++);	/* b */

			if (has_alpha) {
				*(dp) = *(sp++);	/* a */
			}

			dp -= (a + 3);
		}
	}

	return dest;
}

void journal_init_call_icon(void)
{
	GtkIconTheme *icons;
	gint width = 18;
	gint icon_type = g_settings_get_uint(app_settings, "icon-type");

	icons = gtk_icon_theme_get_default();

	gtk_icon_theme_append_search_path(icons, get_directory(APP_DATA));
	if (icon_call_in) {
		g_object_unref(icon_call_in);
	}
	if (icon_call_missed) {
		g_object_unref(icon_call_missed);
	}
	if (icon_call_out) {
		g_object_unref(icon_call_out);
	}
	if (icon_fax) {
		g_object_unref(icon_fax);
	}
	if (icon_fax_report) {
		g_object_unref(icon_fax_report);
	}
	if (icon_voice) {
		g_object_unref(icon_voice);
	}
	if (icon_record) {
		g_object_unref(icon_record);
	}
	if (icon_blocked) {
		g_object_unref(icon_blocked);
	}

	switch (icon_type) {
	default:
	case 0:
		icon_call_in = gtk_icon_theme_load_icon(icons, "roger-call-in", width, 0, NULL);
		icon_call_missed = gtk_icon_theme_load_icon(icons, "roger-call-missed", width, 0, NULL);
		icon_call_out = gtk_icon_theme_load_icon(icons, "roger-call-out", width, 0, NULL);
		icon_fax = gtk_icon_theme_load_icon(icons, "roger-fax", width, 0, NULL);
		icon_fax_report = gtk_icon_theme_load_icon(icons, "roger-fax-report", width, 0, NULL);
		icon_voice = gtk_icon_theme_load_icon(icons, "roger-call-voice", width, 0, NULL);
		icon_record = gtk_icon_theme_load_icon(icons, "roger-record", width, 0, NULL);
		icon_blocked = gtk_icon_theme_load_icon(icons, "roger-call-blocked", width, 0, NULL);
		break;
	case 1:
		icon_call_in = gtk_icon_theme_load_icon(icons, "roger-call-in-symbolic", width, 0, NULL);
		icon_call_missed = gtk_icon_theme_load_icon(icons, "roger-call-missed-symbolic", width, 0, NULL);
		icon_call_out = gtk_icon_theme_load_icon(icons, "roger-call-out-symbolic", width, 0, NULL);
		icon_fax = gtk_icon_theme_load_icon(icons, "roger-fax-symbolic", width, 0, NULL);
		icon_fax_report = gtk_icon_theme_load_icon(icons, "roger-fax-report-symbolic", width, 0, NULL);
		icon_voice = gtk_icon_theme_load_icon(icons, "roger-call-voice-symbolic", width, 0, NULL);
		icon_record = gtk_icon_theme_load_icon(icons, "roger-record-symbolic", width, 0, NULL);
		icon_blocked = gtk_icon_theme_load_icon(icons, "roger-call-blocked-symbolic", width, 0, NULL);
		break;
	}

	g_assert(icon_call_in != NULL);
	g_assert(icon_call_missed != NULL);
	g_assert(icon_call_out != NULL);
	g_assert(icon_fax != NULL);
	g_assert(icon_fax_report != NULL);
	g_assert(icon_voice != NULL);
	g_assert(icon_record != NULL);
	g_assert(icon_blocked != NULL);
}

GdkPixbuf *journal_get_call_icon(gint type)
{
	switch (type) {
	case CALL_TYPE_INCOMING:
		return icon_call_in;
	case CALL_TYPE_MISSED:
		return icon_call_missed;
	case CALL_TYPE_OUTGOING:
		return icon_call_out;
	case CALL_TYPE_FAX:
		return icon_fax;
	case CALL_TYPE_FAX_REPORT:
		return icon_fax_report;
	case CALL_TYPE_VOICE:
		return icon_voice;
	case CALL_TYPE_RECORD:
		return icon_record;
	case CALL_TYPE_BLOCKED:
		return icon_blocked;
	default:
		g_debug("Unknown icon type: %d", type);
		break;
	}

	return NULL;
}

void journal_redraw(void)
{
	GSList *list;
	gint duration = 0;
	GtkWidget *status;
	gchar *text = NULL;
	gint count = 0;
	struct profile *profile;
	GtkListStore *list_store;

	if (!journal_win) {
		return;
	}

	list_store = g_object_get_data(G_OBJECT(journal_win), "list_store");

	/* Update liststore */
	for (list = journal_list; list != NULL; list = list->next) {
		GtkTreeIter iter;
		struct call *call = list->data;

		g_assert(call != NULL);

		if (filter_rule_match(journal_filter, call) == FALSE) {
			continue;
		}

		if (filter_rule_match(journal_search_filter, call) == FALSE) {
			continue;
		}

		gtk_list_store_insert_with_values(list_store, &iter, -1,
			JOURNAL_COL_TYPE, journal_get_call_icon(call->type),
			JOURNAL_COL_DATETIME, call->date_time,
			JOURNAL_COL_NAME, call->remote->name,
			JOURNAL_COL_COMPANY, call->remote->company,
			JOURNAL_COL_NUMBER, call->remote->number,
			JOURNAL_COL_CITY, call->remote->city,
			JOURNAL_COL_EXTENSION, call->local->name,
			JOURNAL_COL_LINE, call->local->number,
			JOURNAL_COL_DURATION, call->duration,
			JOURNAL_COL_CALL_PTR, call,
			-1);

		if (strchr(call->duration, 's') != NULL) {
			/* Ignore voicebox duration */
		} else {
			if (call->duration != NULL && strlen(call->duration) > 0) {
				duration += (call->duration[0] - '0') * 60;
				duration += (call->duration[2] - '0') * 10;
				duration += call->duration[3] - '0';
			}
		}
		count++;
	}

	profile = profile_get_active();

	status = g_object_get_data(G_OBJECT(journal_win), "headerbar");

	if (roger_uses_headerbar()) {
		GtkWidget *grid = gtk_grid_new();
		GtkWidget *title;
		GtkWidget *subtitle;
		GtkWidget *image;
		gchar *markup;
		PangoAttrList *attributes;
		GSList *list = profile_get_list();
		gboolean selection = g_slist_length(list) > 1;

		gtk_widget_set_hexpand(grid, TRUE);
		markup = g_strdup_printf("<b>%s</b>", profile ? profile->name : _("<No profile>"));
		
		if (pango_parse_markup(markup, -1, 0, &attributes, &text, NULL, NULL)) {
			title = gtk_label_new(text);
			gtk_label_set_attributes(GTK_LABEL(title), attributes);
		} else {
			title = gtk_label_new(_("<No profile>"));
		}

		gtk_widget_set_hexpand(title, TRUE);
		gtk_grid_attach(GTK_GRID(grid), title, 0, 0, selection ? 1 : 2, 1);

		if (selection) {
			image = gtk_image_new_from_icon_name("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
			gtk_grid_attach(GTK_GRID(grid), image, 1, 0, 1, 1);
		}

		markup = g_strdup_printf(_("<small>%d calls, %d:%2.2dh</small>"), count, duration / 60, duration % 60);
		pango_parse_markup(markup, -1, 0, &attributes, &text, NULL, NULL);
		subtitle = gtk_label_new(text);
		gtk_label_set_attributes(GTK_LABEL(subtitle), attributes);
		gtk_widget_set_sensitive(subtitle, FALSE);
		gtk_grid_attach(GTK_GRID(grid), subtitle, 0, 1, 2, 1);

		gtk_widget_show_all(grid);
		gtk_header_bar_set_custom_title(GTK_HEADER_BAR(status), grid);
	} else {
		text = g_strdup_printf(_("%s - %d calls, %d:%2.2dh"), profile ? profile->name : _("<No profile>"), count, duration / 60, duration % 60);
		gtk_window_set_title(GTK_WINDOW(journal_win), text);
	}

	g_free(text);
}

static gboolean reload_journal(gpointer user_data)
{
	GtkListStore *list_store;
	GtkTreeIter iter;
	gboolean valid;
	struct call *call;

	if (!journal_win) {
		return FALSE;
	}

	list_store = g_object_get_data(G_OBJECT(journal_win), "list_store");
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	while (valid) {
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, JOURNAL_COL_CALL_PTR, &call, -1);

		if (call->remote->lookup) {
			gtk_list_store_set(list_store, &iter, JOURNAL_COL_NAME, call->remote->name, -1);
			gtk_list_store_set(list_store, &iter, JOURNAL_COL_CITY, call->remote->city, -1);
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}

	gtk_spinner_stop(GTK_SPINNER(spinner));
	gtk_widget_hide(spinner);

	g_mutex_unlock(&journal_mutex);

	return FALSE;
}

gpointer lookup_journal(gpointer user_data)
{
	GSList *journal = user_data;
	GSList *list;

	for (list = journal; list; list = list->next) {
		struct call *call = list->data;
		gchar *name = NULL;
		gchar *street = NULL;
		gchar *zip = NULL;
		gchar *city = NULL;

		if (!EMPTY_STRING(call->remote->name)) {
			continue;
		}

		if (routermanager_lookup(call->remote->number, &name, &street, &zip, &city)) {
			call->remote->name = name;
			call->remote->street = street;
			call->remote->zip = zip;
			call->remote->city = city;
			call->remote->lookup = TRUE;
		}
	}

	g_idle_add(reload_journal, journal_list);

	return NULL;
}

void journal_loaded_cb(AppObject *obj, GSList *journal, gpointer unused)
{
	GSList *old;

	if (g_mutex_trylock(&journal_mutex) == FALSE) {
		g_debug("Journal loading already in progress");
		return;
	}

	if (spinner && !gtk_widget_get_visible(spinner)) {
		gtk_spinner_start(GTK_SPINNER(spinner));
		gtk_widget_show(spinner);
	}

	/* Clear existing liststore */
	journal_clear();

	/* Set new internal list */
	old = journal_list;
	journal_list = journal;

	if (old) {
		g_slist_free_full(old, call_free);
	}

	journal_redraw();

	g_thread_new("Reverse Lookup Journal", lookup_journal, journal_list);
}

static void journal_connection_notify_cb(AppObject *obj, struct connection *connection, gpointer user_data)
{
	if (connection->type & CONNECTION_TYPE_DISCONNECT) {
		router_load_journal(profile_get_active());
	}
}

gboolean journal_button_refresh_idle(gpointer data)
{
	struct profile *profile = profile_get_active();

	if (!profile) {
		return FALSE;
	}

	if (router_load_journal(profile) == FALSE) {
		gtk_spinner_stop(GTK_SPINNER(spinner));
		gtk_widget_hide(spinner);
	}

	return FALSE;
}

void journal_button_refresh_clicked_cb(GtkWidget *button, GtkWidget *window)
{
	gtk_spinner_start(GTK_SPINNER(spinner));
	gtk_widget_show(spinner);

	g_idle_add(journal_button_refresh_idle, NULL);
}

void journal_button_print_clicked_cb(GtkWidget *button, GtkWidget *view)
{
	journal_print(view);
}

void journal_button_clear_clicked_cb(GtkWidget *button, GtkWidget *view)
{
	GtkWidget *dialog;
	gint flags = GTK_DIALOG_MODAL;

	if (roger_uses_headerbar()) {
		flags |= GTK_DIALOG_USE_HEADER_BAR;
	}

	dialog = gtk_message_dialog_new(GTK_WINDOW(journal_win), flags, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Do you want to delete the router journal?"));

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), GTK_RESPONSE_OK);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (result != GTK_RESPONSE_OK) {
		return;
	}

	router_clear_journal(profile_get_active());
}

void delete_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	struct call *call = NULL;
	GValue ptr = { 0 };

	gtk_tree_model_get_value(model, iter, JOURNAL_COL_CALL_PTR, &ptr);

	call = g_value_get_pointer(&ptr);

	switch (call->type) {
	case CALL_TYPE_RECORD:
		g_unlink(call->priv);
		break;
	case CALL_TYPE_VOICE:
		router_delete_voice(profile_get_active(), call->priv);
		break;
	case CALL_TYPE_FAX:
		router_delete_fax(profile_get_active(), call->priv);
		break;
	case CALL_TYPE_FAX_REPORT:
		g_unlink(call->priv);
		break;
	default:
		journal_list = g_slist_remove(journal_list, call);
		g_debug("Deleting: '%s'", call->date_time);
		csv_save_journal(journal_list);
		break;
	}
}

void journal_button_delete_clicked_cb(GtkWidget *button, GtkWidget *view)
{
	GtkWidget *dialog;
	gint flags = GTK_DIALOG_MODAL;

	if (roger_uses_headerbar()) {
		flags |= GTK_DIALOG_USE_HEADER_BAR;
	}

	dialog = gtk_message_dialog_new(GTK_WINDOW(journal_win), flags, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, _("Do you really want to delete the selected entry?"));

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), GTK_RESPONSE_OK);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (result != GTK_RESPONSE_OK) {
		return;
	}

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	gtk_tree_selection_selected_foreach(selection, delete_foreach, NULL);

	router_load_journal(profile_get_active());
}

void journal_add_contact(struct call *call)
{
	app_contacts(call->remote);
}

void add_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	struct call *call = NULL;
	GValue ptr = { 0 };

	gtk_tree_model_get_value(model, iter, JOURNAL_COL_CALL_PTR, &ptr);

	call = g_value_get_pointer(&ptr);

	journal_add_contact(call);
}

void journal_button_add_clicked_cb(GtkWidget *button, GtkWidget *view)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

	gtk_tree_selection_selected_foreach(selection, add_foreach, NULL);
}

static void journal_startup(GApplication *application)
{
	journal_application = application;
}

void search_entry_changed(GtkEditable *entry, GtkTreeView *view)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

	if (journal_search_filter) {
		filter_free(journal_search_filter);
		journal_search_filter = NULL;
	}

	if (strlen(text)) {
		journal_search_filter = filter_new("internal_search");

		if (g_ascii_isdigit(text[0])) {
			filter_rule_add(journal_search_filter, FILTER_REMOTE_NUMBER, FILTER_CONTAINS, (gchar *)text);
		} else {
			filter_rule_add(journal_search_filter, FILTER_REMOTE_NAME, FILTER_CONTAINS, (gchar *)text);
		}
	}

	journal_clear();
	journal_redraw();
}

void entry_icon_released(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
	switch (icon_pos) {
	case GTK_ENTRY_ICON_PRIMARY:
		break;
	case GTK_ENTRY_ICON_SECONDARY:
		gtk_entry_set_text(entry, "");
		break;
	}
}

void filter_box_changed(GtkComboBox *box, gpointer user_data)
{
	const gchar *text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box));
	GSList *filter_list = filter_get_list();

	if (!text) {
		journal_filter = NULL;
		journal_clear();
		journal_redraw();
		return;
	}

	while (filter_list) {
		struct filter *filter = filter_list->data;

		if (!strcmp(filter->name, text)) {
			journal_filter = filter;
			journal_clear();
			journal_redraw();
			break;
		}

		filter_list = filter_list->next;
	}
}

void journal_update_filter(void)
{
	GSList *filter_list;

	filter_list = filter_get_list();

	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(journal_filter_box));

	while (filter_list) {
		struct filter *filter = filter_list->data;

		gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(journal_filter_box), NULL, filter->name);
		filter_list = filter_list->next;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(journal_filter_box), 0);
}

void row_activated_foreach(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	struct call *call;

	gtk_tree_model_get(model, iter, JOURNAL_COL_CALL_PTR, &call, -1);

	switch (call->type) {
	case CALL_TYPE_FAX_REPORT:
	case CALL_TYPE_FAX: {
		gsize len = 0;
		gchar *data = router_load_fax(profile_get_active(), call->priv, &len);

		if (data && len) {
			gchar *path = g_build_filename(g_get_user_cache_dir(), G_DIR_SEPARATOR_S, "fax.pdf", NULL);
			file_save(path, data, len);

			os_execute(path);
			g_free(path);
		}
		g_free(data);
		break;
	}
	case CALL_TYPE_RECORD:
		os_execute(call->priv);
		break;
	case CALL_TYPE_VOICE:
		answeringmachine_play(call->priv);
		break;
	default:
		app_show_phone_window(call->remote, NULL);
		break;
	}
}

void journal_row_activated_cb(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(view);

	gtk_tree_selection_selected_foreach(selection, row_activated_foreach, NULL);
}

static gboolean journal_hide_on_quit = FALSE;
static gboolean journal_hide_on_start = FALSE;

void journal_set_hide_on_quit(gboolean hide)
{
	journal_hide_on_quit = hide;
	if (hide && journal_win) {
		gtk_widget_set_visible(journal_win, FALSE);
	}
}

void journal_set_hide_on_start(gboolean hide)
{
	journal_hide_on_start = hide;
}

gint journal_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	if (journal_hide_on_quit == TRUE) {
		gtk_widget_hide(widget);
		return TRUE;
	}

	//gtk_application_remove_window(GTK_APPLICATION(data), GTK_WINDOW(journal_win));
	//gtk_widget_destroy(journal_win);
	journal_win = NULL;
	spinner = NULL;

	return FALSE;
}

gint journal_sort_by_date(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	struct call *call_a;
	struct call *call_b;

	gtk_tree_model_get(model, a, JOURNAL_COL_CALL_PTR, &call_a, -1);
	gtk_tree_model_get(model, b, JOURNAL_COL_CALL_PTR, &call_b, -1);

	return call_sort_by_date(call_a, call_b);
}

gint journal_sort_by_type(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	GdkPixbuf *call_a;
	GdkPixbuf *call_b;

	gtk_tree_model_get(model, a, JOURNAL_COL_TYPE, &call_a, -1);
	gtk_tree_model_get(model, b, JOURNAL_COL_TYPE, &call_b, -1);

	return call_a > call_b ? -1 : call_a < call_b ? 1 : 0;
}

void name_column_cell_data_func(GtkTreeViewColumn *column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
	struct call *call;

	gtk_tree_model_get(model, iter, JOURNAL_COL_CALL_PTR, &call, -1);

	if (call && call->remote->lookup) {
		g_object_set(renderer, "foreground", "darkgrey", "foreground-set", TRUE, NULL);
	} else {
		g_object_set(renderer, "foreground-set", FALSE, NULL);
	}
}

static gboolean journal_configure_event_cb(GtkWidget *window, GdkEvent *event, gpointer user_data)
{
	if (!g_settings_get_boolean(app_settings, "maximized")) {
		gint width, height;

		gtk_window_get_size(GTK_WINDOW(window), &width, &height);
		g_settings_set_uint(app_settings, "width", width);
		g_settings_set_uint(app_settings, "height", height);
	}

	return FALSE;
}

static gboolean journal_window_state_event_cb(GtkWidget *window, GdkEventWindowState *event)
{
	gboolean maximized = event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED;

	g_settings_set_boolean(app_settings, "maximized", maximized);

	return FALSE;
}

static guint timeout_id = 0;

static gboolean save_column_state(gpointer data)
{
	gint column_id = gtk_tree_view_column_get_sort_column_id(data);
	gint width = gtk_tree_view_column_get_width(data);
	gboolean visible = gtk_tree_view_column_get_visible(data);
	gchar *key;

	key = g_strdup_printf("col-%d-width", column_id);
	g_settings_set_uint(app_settings, key, width);
	g_free(key);

	key = g_strdup_printf("col-%d-visible", column_id);
	g_settings_set_boolean(app_settings, key, visible);
	g_free(key);

	timeout_id = 0;

	return FALSE;
}

static void journal_column_fixed_width_cb(GtkWidget *widget, gpointer user_data)
{
	GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN(widget);

	if (timeout_id) {
		g_source_remove(timeout_id);
	}

	timeout_id = g_timeout_add(250, save_column_state, column);
}

static gboolean journal_column_header_button_pressed_cb(GtkTreeViewColumn *column, GdkEventButton *event, gpointer user_data)
{
	GtkMenu *menu = GTK_MENU(user_data);

	if (event->button == GDK_BUTTON_SECONDARY) {
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

void journal_popup_copy_number(GtkWidget *widget, struct call *call)
{
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_NONE), call->remote->number, -1);
}

void journal_popup_call_number(GtkWidget *widget, struct call *call)
{
	app_show_phone_window(call->remote, NULL);
}

void journal_popup_add_contact(GtkWidget *widget, struct call *call)
{
	journal_add_contact(call);
}

void journal_popup_delete_entry(GtkWidget *widget, struct call *call)
{
	journal_button_delete_clicked_cb(NULL, journal_view);
}

void journal_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer user_data)
{
	GtkWidget *menu, *menuitem;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	GtkTreeModel *model;
	GList *list;
	GtkTreeIter iter;
	struct call *call = NULL;

	if (gtk_tree_selection_count_selected_rows(selection) != 1) {
		return;
	}
#if 0
	GMenu *popover_menu = g_menu_new();
	GtkWidget *popover;

	g_menu_append(popover_menu, _("Copy number"), "app.copy-number");
	g_menu_append(popover_menu, _("Call number"), "app.call-number");

	popover = gtk_popover_new_from_model(treeview, popover_menu);
	gtk_widget_show(popover);
#else

	list = gtk_tree_selection_get_selected_rows(selection, &model);
	gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)list->data);
	gtk_tree_model_get(model, &iter, JOURNAL_COL_CALL_PTR, &call, -1);

	menu = gtk_menu_new();

	/* Copy phone number */
	menuitem = gtk_menu_item_new_with_label(_("Copy number"));
	g_signal_connect(menuitem, "activate", (GCallback) journal_popup_copy_number, call);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	/* Call phone number */
	menuitem = gtk_menu_item_new_with_label(_("Call number"));
	g_signal_connect(menuitem, "activate", (GCallback) journal_popup_call_number, call);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	/* Separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	/* Add contact */
	menuitem = gtk_menu_item_new_with_label(_("Add contact"));
	g_signal_connect(menuitem, "activate", (GCallback) journal_popup_add_contact, call);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	/* Separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	/* Delete entry */
	menuitem = gtk_menu_item_new_with_label(_("Delete entry"));
	g_signal_connect(menuitem, "activate", (GCallback) journal_popup_delete_entry, call);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
#endif
}

gboolean journal_button_press_event_cb(GtkWidget *treeview, GdkEventButton *event, gpointer user_data)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
		GtkTreeSelection *selection;
		GtkTreePath *path;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all(selection);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_path_free(path);
		}

		journal_popup_menu(treeview, event, user_data);

		return TRUE;
	}

	return FALSE;
}

static gboolean search_active = FALSE;

gboolean window_key_press_event_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	GtkWidget *button = g_object_get_data(G_OBJECT(widget), "button");
	GdkEventKey *key = (GdkEventKey*)event;
	gint ret;

	ret = gtk_search_bar_handle_event(GTK_SEARCH_BAR(user_data), event);

	if (ret != GDK_EVENT_STOP) {
		if (search_active && key->keyval == GDK_KEY_Escape) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
		}
	} else if (!search_active) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
	}

	return ret;
}

void find_button_pressed_cb(GtkWidget *widget, gpointer user_data)
{
	gtk_search_bar_set_search_mode(GTK_SEARCH_BAR(user_data), !search_active);
	search_active = !search_active;
}

void refresh_journal_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	journal_button_refresh_clicked_cb(NULL, NULL);
}

void print_journal_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	journal_button_print_clicked_cb(NULL, journal_view);
}

void clear_journal_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	journal_button_clear_clicked_cb(NULL, NULL);
}

static void journal_export_cb(GtkWidget *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_ACCEPT) {
		gchar *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		g_debug("file: %s", file);
		csv_save_journal_as(journal_list, file);
	}

	gtk_widget_destroy(dialog);
}

void export_journal_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Export journal"), GTK_WINDOW(journal_win), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "journal.csv");
	g_signal_connect(dialog, "response", G_CALLBACK(journal_export_cb), NULL);
	gtk_widget_show_all(dialog);
}

void delete_entry_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	journal_button_delete_clicked_cb(NULL, journal_view);
}

void add_contact_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	journal_button_add_clicked_cb(NULL, journal_view);
}

static void journal_contacts_changed_cb(AppObject *app, gpointer user_data)
{
	journal_button_refresh_clicked_cb(NULL, NULL);
}

void journal_quit(void)
{
	gtk_widget_destroy(GTK_WIDGET(journal_win));
}

GtkWidget *journal_get_window(void)
{
	return journal_win;
}

void journal_set_visible(gboolean state)
{
	gtk_widget_set_visible(journal_win, state);
}

void contacts_add_detail_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	gchar *name;

	g_object_get(action, "name", &name, NULL);
	g_debug("%s(): action %s", __FUNCTION__, name);
	contacts_add_detail(name + 14);
}

void journal_column_restore_default(GtkMenuItem *item, gpointer user_data)
{
	gint index;

	for (index = JOURNAL_COL_DATETIME; index <= JOURNAL_COL_DURATION; index++) {
		GtkTreeViewColumn *column;
		gchar *key;

		key = g_strdup_printf("col-%d-width", index);
		g_settings_set_uint(app_settings, key, 0);
		g_free(key);

		key = g_strdup_printf("col-%d-visible", index);
		g_settings_set_boolean(app_settings, key, TRUE);
		g_free(key);

		column = gtk_tree_view_get_column(GTK_TREE_VIEW(user_data), index);
		gtk_tree_view_column_set_visible(column, TRUE);
		gtk_tree_view_column_set_fixed_width(column, -1);
	}
}

void journal_window(GApplication *app)
{
	GtkWidget *window, *grid, *scrolled;
	GtkWidget *button;
	GtkListStore *list_store;
	GtkTreeModel *tree_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSortable *sortable;
	gint index;
	gint y = 0;
	gchar *column_name[10] = {
		_("Type"),
		_("Date/Time"),
		_("Name"),
		_("Company"),
		_("Number"),
		_("City"),
		_("Extension"),
		_("Line"),
		_("Duration")
	};
	const GActionEntry journal_actions[] = {
		{"refresh-journal", refresh_journal_activated},
		{"print-journal", print_journal_activated},
		{"clear-journal", clear_journal_activated},
		{"export-journal", export_journal_activated},
		{"add-contact", add_contact_activated},
		{"delete-entry", delete_entry_activated},
		{"contacts-edit-phone-home", contacts_add_detail_activated},
		{"contacts-edit-phone-work", contacts_add_detail_activated},
		{"contacts-edit-phone-mobile", contacts_add_detail_activated},
		{"contacts-edit-phone-home-fax", contacts_add_detail_activated},
		{"contacts-edit-phone-work-fax", contacts_add_detail_activated},
		{"contacts-edit-phone-pager", contacts_add_detail_activated},
		{"contacts-edit-address-home", contacts_add_detail_activated},
		{"contacts-edit-address-work", contacts_add_detail_activated},
	};

	//int i = 0;
	//g_printf("%d ", journal_hide_on_start / i);

	journal_startup(app);

#if GTK_CHECK_VERSION(3,12,0)
	use_header = gtk_application_prefers_app_menu(GTK_APPLICATION(app));
#else
	g_object_get(gtk_settings_get_default(), "gtk-dialogs-use-header", &use_header, NULL);
#endif
	g_debug("Use headerbar: %d", use_header);

	journal_init_call_icon();

	window = gtk_application_window_new(GTK_APPLICATION(app));

	journal_win = window;
	gtk_window_set_default_size((GtkWindow *)window, g_settings_get_uint(app_settings, "width"), g_settings_get_uint(app_settings, "height"));
	if (g_settings_get_boolean(app_settings, "maximized")) {
		gtk_window_maximize((GtkWindow *)(window));
	}

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	grid = gtk_grid_new();

	journal_view = gtk_tree_view_new();

	/* Show horizontal grid lines */
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(journal_view), GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

	/* Set fixed height mode (improves rendering speed) */
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(journal_view), TRUE);

	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
	gtk_container_add(GTK_CONTAINER(window), grid);

	GtkWidget *header;
	GtkWidget *box;
	GtkWidget *search;
	GtkWidget *icon;
	GtkWidget *menu_button;
	GtkWidget *entry;
	GMenu *menu;

	if (roger_uses_headerbar()) {
		/* Create header bar and set it to window */
		header = gtk_header_bar_new();
		gtk_widget_set_hexpand(header, TRUE);
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
		gtk_header_bar_set_title(GTK_HEADER_BAR (header), "Journal");
		gtk_window_set_titlebar(GTK_WINDOW(window), header);
		g_object_set_data(G_OBJECT(window), "headerbar", header);
	} else {
		header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
		gtk_widget_set_hexpand(header, TRUE);
		gtk_grid_attach(GTK_GRID(grid), header, 0, y++, 1, 1);
	}

	/* Create button box as raised and linked */
	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(header), box);
	gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_RAISED);
	gtk_style_context_add_class(gtk_widget_get_style_context(box), GTK_STYLE_CLASS_LINKED);

	/* Create menu button and menu */
	menu_button = gtk_menu_button_new();
	gtk_menu_button_set_direction(GTK_MENU_BUTTON(menu_button), GTK_ARROW_NONE);

	menu = g_menu_new();

	g_action_map_add_action_entries(G_ACTION_MAP(app), journal_actions, G_N_ELEMENTS(journal_actions), app);

	g_menu_append(menu, _("Refresh journal"), "app.refresh-journal");
	g_menu_append(menu, _("Print journal"), "app.print-journal");
	g_menu_append(menu, _("Clear journal"), "app.clear-journal");
	g_menu_append(menu, _("Export journal"), "app.export-journal");

#if GTK_CHECK_VERSION(3,12,0)
	gtk_menu_button_set_use_popover(GTK_MENU_BUTTON(menu_button), TRUE);
#else
	gtk_container_add(GTK_CONTAINER(menu_button), gtk_image_new_from_icon_name("view-list-symbolic", GTK_ICON_SIZE_MENU));
#endif

	g_menu_freeze(menu);
	gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));

	gtk_box_pack_start(GTK_BOX(box), menu_button, FALSE, TRUE, 0);

	/* Create search bar */
	search = gtk_search_bar_new();

	button = gtk_toggle_button_new();
	icon = gtk_image_new_from_icon_name("edit-find-symbolic", GTK_ICON_SIZE_BUTTON);
	g_signal_connect(button, "clicked", G_CALLBACK(find_button_pressed_cb), search);
	gtk_button_set_image(GTK_BUTTON(button), icon);

	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);

	entry = gtk_search_entry_new();
	gtk_widget_set_size_request(entry, 500, -1);
	gtk_container_add(GTK_CONTAINER(search), entry);

	g_object_set_data(G_OBJECT(window), "button", button);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(search_entry_changed), journal_view);
	gtk_widget_set_hexpand(entry, TRUE);
	gtk_widget_set_hexpand(search, TRUE);

	gtk_widget_show_all(search);

	gtk_grid_attach(GTK_GRID(grid), search, 0, y++, 5, 1);

	gtk_search_bar_connect_entry(GTK_SEARCH_BAR(search), GTK_ENTRY(entry));

	/* Create filter box */
	journal_filter_box = gtk_combo_box_text_new();
	journal_update_filter();
	g_signal_connect(G_OBJECT(journal_filter_box), "changed", G_CALLBACK(filter_box_changed), NULL);
	gtk_box_pack_start(GTK_BOX(box), journal_filter_box, FALSE, TRUE, 0);

	/* Create spinner */
	spinner = gtk_spinner_new();
	gtk_widget_set_no_show_all(spinner, TRUE);
	gtk_container_add(GTK_CONTAINER(header), spinner);

	gtk_widget_show_all(header);

	g_signal_connect(window, "key-press-event", G_CALLBACK(window_key_press_event_cb), search);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand(scrolled, TRUE);
	gtk_widget_set_vexpand(scrolled, TRUE);

	list_store = gtk_list_store_new(10,
	                                GDK_TYPE_PIXBUF,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_STRING,
	                                G_TYPE_POINTER);

	g_object_set_data(G_OBJECT(window), "list_store", list_store);

	tree_model = GTK_TREE_MODEL(list_store);
	sortable = GTK_TREE_SORTABLE(list_store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(journal_view), GTK_TREE_MODEL(tree_model));

	GtkWidget *header_menu = gtk_menu_new();
	GtkWidget *column_item;
	gchar *title;

	/* Add pixbuf renderer, it's always the first in row */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(column_name[0], renderer, "pixbuf", JOURNAL_COL_TYPE, NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_sortable_set_sort_func(sortable, JOURNAL_COL_TYPE, journal_sort_by_type, 0, NULL);

	if (g_settings_get_uint(app_settings, "col-0-width")) {
		gtk_tree_view_column_set_fixed_width(column, g_settings_get_uint(app_settings, "col-0-width"));
	}

	gtk_tree_view_column_set_visible(column, g_settings_get_boolean(app_settings, "col-0-visible"));
	g_signal_connect(column, "notify::fixed-width", G_CALLBACK(journal_column_fixed_width_cb), NULL);
	g_signal_connect(column, "notify::visible", G_CALLBACK(journal_column_fixed_width_cb), NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(journal_view), column);

	title = column_name[0];
	button = gtk_tree_view_column_get_button(column);
	g_signal_connect(button, "button-press-event", G_CALLBACK(journal_column_header_button_pressed_cb), header_menu);

	column_item = gtk_check_menu_item_new_with_label(title);
	gtk_widget_set_sensitive(column_item, FALSE);
	g_object_bind_property(column, "visible", column_item, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	gtk_menu_shell_append(GTK_MENU_SHELL(header_menu), column_item);

	/* Add all the other columns renderer */
	for (index = JOURNAL_COL_DATETIME; index <= JOURNAL_COL_DURATION; index++) {
		gchar *tmp;

		renderer = gtk_cell_renderer_text_new();

		/* Usually we want GTK to autosize the columns for the optimal width. Unfortunately this is not
		 * true for versions < 3.10.0. Disable ellipsize text to show optimal column widths..... *grml*
		 */
		if (index == JOURNAL_COL_NAME) {
			g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "width", 280, NULL);
		}
		column = gtk_tree_view_column_new_with_attributes(column_name[index], renderer, "text", index, NULL);
		gtk_tree_view_column_set_sort_column_id(column, index);

		tmp = g_strdup_printf("col-%d-width", index);
		if (g_settings_get_uint(app_settings, tmp)) {
			gtk_tree_view_column_set_fixed_width(column, g_settings_get_uint(app_settings, tmp));
		}
		g_free(tmp);

		tmp = g_strdup_printf("col-%d-visible", index);
		gtk_tree_view_column_set_visible(column, g_settings_get_boolean(app_settings, tmp));
		g_free(tmp);

		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);

		g_signal_connect(column, "notify::fixed-width", G_CALLBACK(journal_column_fixed_width_cb), NULL);
		g_signal_connect(column, "notify::visible", G_CALLBACK(journal_column_fixed_width_cb), NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);

		if (index == JOURNAL_COL_DATETIME) {
			gtk_tree_sortable_set_sort_func(sortable, JOURNAL_COL_DATETIME, journal_sort_by_date, 0, NULL);
		} else if (index == JOURNAL_COL_NAME) {
			gtk_tree_view_column_set_cell_data_func(column, renderer, name_column_cell_data_func, NULL, NULL);
		}

		gtk_tree_view_append_column(GTK_TREE_VIEW(journal_view), column);

		title = column_name[index];
		button = gtk_tree_view_column_get_button(column);
		g_signal_connect(button, "button-press-event", G_CALLBACK(journal_column_header_button_pressed_cb), header_menu);

		column_item = gtk_check_menu_item_new_with_label(title);
		g_object_bind_property(column, "visible", column_item, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
		gtk_menu_shell_append(GTK_MENU_SHELL(header_menu), column_item);
	}

	column_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(header_menu), column_item);

	column_item = gtk_menu_item_new_with_label(_("Restore default"));
	g_signal_connect(G_OBJECT(column_item), "activate", G_CALLBACK(journal_column_restore_default), journal_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(header_menu), column_item);

	gtk_widget_show_all(header_menu);

	gtk_container_add(GTK_CONTAINER(scrolled), journal_view);

	gtk_grid_attach(GTK_GRID(grid), scrolled, 0, y++, 5, 1);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(journal_view)), GTK_SELECTION_MULTIPLE);

	gtk_window_set_title(GTK_WINDOW(window), _("Journal"));

	g_signal_connect(G_OBJECT(journal_view), "row-activated", G_CALLBACK(journal_row_activated_cb), list_store);
	g_signal_connect(G_OBJECT(journal_view), "button-press-event", G_CALLBACK(journal_button_press_event_cb), list_store);

	g_signal_connect(journal_win, "delete-event", G_CALLBACK(journal_delete_event_cb), app);
	g_signal_connect(G_OBJECT(app_object), "journal-loaded", G_CALLBACK(journal_loaded_cb), NULL);

	g_signal_connect(G_OBJECT(app_object), "connection-notify", G_CALLBACK(journal_connection_notify_cb), NULL);

	g_signal_connect(G_OBJECT(journal_win), "configure-event", G_CALLBACK(journal_configure_event_cb), NULL);
	g_signal_connect(G_OBJECT(journal_win), "window-state-event", G_CALLBACK(journal_window_state_event_cb), NULL);

	g_signal_connect(app_object, "contacts-changed", G_CALLBACK(journal_contacts_changed_cb), NULL);

	gtk_widget_hide_on_delete(journal_win);

	filter_box_changed(GTK_COMBO_BOX(journal_filter_box), NULL);

	gtk_widget_show_all(GTK_WIDGET(grid));

	if (!journal_hide_on_start) {
		gtk_widget_show(GTK_WIDGET(window));
	}

#ifdef FAX_DBEUG
	extern gboolean app_show_fax_window_idle(gpointer data);
	app_show_fax_window_idle(NULL);
#endif
}
