/**
 * The libroutermanager project
 * Copyright (c) 2012-2014 Jan-Michael Brummer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <libroutermanager/appobject.h>
#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/file.h>
#include <libroutermanager/routermanager.h>
#include <libroutermanager/settings.h>
#include <libroutermanager/gstring.h>

#include <roger/settings.h>
#include <roger/uitools.h>

#include <webjournal.h>

#define ROUTERMANAGER_TYPE_WEBJOURNAL_PLUGIN (routermanager_webjournal_plugin_get_type ())
#define ROUTERMANAGER_WEBJOURNAL_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_WEBJOURNAL_PLUGIN, RouterManagerWebJournalPlugin))

typedef struct {
	guint signal_id;
	gchar *header;
	gchar *entry;
	gchar *footer;
	gchar *dragtable;
	gchar *sortable;
	gchar *styling;
} RouterManagerWebJournalPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER_CONFIGURABLE(ROUTERMANAGER_TYPE_WEBJOURNAL_PLUGIN, RouterManagerWebJournalPlugin, routermanager_webjournal_plugin)

static GSettings *webjournal_settings = NULL;

gchar *get_call_type_string(gint type)
{
	switch (type) {
	case CALL_TYPE_INCOMING:
		return "in";
	case CALL_TYPE_OUTGOING:
		return "out";
	case CALL_TYPE_MISSED:
		return "missed";
	case CALL_TYPE_FAX:
	case CALL_TYPE_FAX_REPORT:
		return "fax";
	case CALL_TYPE_VOICE:
	case CALL_TYPE_RECORD:
		return "voice";
	case CALL_TYPE_BLOCKED:
		return "blocked";
	}

	return "unknown";
}

static gchar *convert_date_time(gchar *date_time)
{
	gint year;
	gint month;
	gint day;
	gint hour;
	gint min;

	sscanf(date_time, "%d.%d.%d %d:%d", &day, &month, &year, &hour, &min );

	return g_strdup_printf("%4.4d%2.2d%2.2d%2.2d%2.2d00", 2000 + year, month, day, hour, min);
}

void webjournal_journal_loaded_cb(AppObject *obj, GSList *journal, gpointer user_data)
{
	RouterManagerWebJournalPlugin *webjournal_plugin = user_data;
	gchar *file;
	GString *string;
	GSList *list;
	gchar *out;
	gchar *dirname;

	file = g_settings_get_string(webjournal_settings, "filename");

	string = g_string_new(webjournal_plugin->priv->header);

	for (list = journal; list != NULL; list = list->next) {
		struct call *call = list->data;
		GRegex *type = g_regex_new("%TYPE%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *date_time = g_regex_new("%DATETIME%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *name = g_regex_new("%NAME%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *company = g_regex_new("%COMPANY%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *number = g_regex_new("%NUMBER%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *city = g_regex_new("%CITY%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *extension = g_regex_new("%EXTENSION%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *line = g_regex_new("%LINE%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *duration = g_regex_new("%DURATION%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		GRegex *sortable_customkey = g_regex_new("%CUSTOMKEY%", G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		gchar *out1;
		gchar *out2;
		gchar *customkey = convert_date_time(call->date_time);

		out1 = g_regex_replace_literal(type, webjournal_plugin->priv->entry, -1, 0, get_call_type_string(call->type), 0, NULL);

		out2 = g_regex_replace_literal(date_time, out1, -1, 0, call->date_time, 0, NULL);
		g_free(out1);
		out1 = g_regex_replace_literal(name, out2, -1, 0, call->remote->name, 0, NULL);
		g_free(out2);
		out2 = g_regex_replace_literal(company, out1, -1, 0, call->remote->company ? call->remote->company : "", 0, NULL);
		g_free(out1);
		out1 = g_regex_replace_literal(number, out2, -1, 0, call->remote->number, 0, NULL);
		g_free(out2);
		out2 = g_regex_replace_literal(city, out1, -1, 0, call->remote->city, 0, NULL);
		g_free(out1);
		out1 = g_regex_replace_literal(extension, out2, -1, 0, call->local->name, 0, NULL);
		g_free(out2);
		out2 = g_regex_replace_literal(line, out1, -1, 0, call->local->number, 0, NULL);
		g_free(out1);
		out1 = g_regex_replace_literal(duration, out2, -1, 0, call->duration, 0, NULL);
		g_free(out2);
		out2 = g_regex_replace_literal(sortable_customkey, out1, -1, 0, customkey, 0, NULL);
		g_free(out1);

		string = g_string_append(string, out2);

		g_free(out2);
		g_regex_unref(duration);
		g_regex_unref(line);
		g_regex_unref(extension);
		g_regex_unref(city);
		g_regex_unref(number);
		g_regex_unref(company);
		g_regex_unref(name);
		g_regex_unref(date_time);
		g_regex_unref(type);
	}

	string = g_string_append(string, webjournal_plugin->priv->footer);

	out = g_string_free(string, FALSE);

	file_save(file, out, -1);

	dirname = g_path_get_dirname(file);

	file = g_strdup_printf("%s/dragtable.js", dirname);
	file_save(file, webjournal_plugin->priv->dragtable, -1);
	g_free(file);

	file = g_strdup_printf("%s/sortable.js", dirname);
	file_save(file, webjournal_plugin->priv->sortable, -1);
	g_free(file);

	file = g_strdup_printf("%s/styling.css", dirname);
	file_save(file, webjournal_plugin->priv->styling, -1);
	g_free(file);

	g_free(dirname);

	g_free(out);
}

/**
 * \brief Activate plugin
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	RouterManagerWebJournalPlugin *webjournal_plugin = ROUTERMANAGER_WEBJOURNAL_PLUGIN(plugin);
	gchar *file;

	file = g_strconcat(get_directory(ROUTERMANAGER_PLUGINS), G_DIR_SEPARATOR_S, "webjournal", G_DIR_SEPARATOR_S, "header.html", NULL);
	webjournal_plugin->priv->header = file_load(file, NULL);
	g_free(file);

	file = g_strconcat(get_directory(ROUTERMANAGER_PLUGINS), G_DIR_SEPARATOR_S, "webjournal", G_DIR_SEPARATOR_S, "entry.html", NULL);
	webjournal_plugin->priv->entry = file_load(file, NULL);
	g_free(file);

	file = g_strconcat(get_directory(ROUTERMANAGER_PLUGINS), G_DIR_SEPARATOR_S, "webjournal", G_DIR_SEPARATOR_S, "footer.html", NULL);
	webjournal_plugin->priv->footer = file_load(file, NULL);
	g_free(file);

	file = g_strconcat(get_directory(ROUTERMANAGER_PLUGINS), G_DIR_SEPARATOR_S, "webjournal", G_DIR_SEPARATOR_S, "dragtable.js", NULL);
	webjournal_plugin->priv->dragtable = file_load(file, NULL);
	g_free(file);

	file = g_strconcat(get_directory(ROUTERMANAGER_PLUGINS), G_DIR_SEPARATOR_S, "webjournal", G_DIR_SEPARATOR_S, "sortable.js", NULL);
	webjournal_plugin->priv->sortable = file_load(file, NULL);
	g_free(file);

	file = g_strconcat(get_directory(ROUTERMANAGER_PLUGINS), G_DIR_SEPARATOR_S, "webjournal", G_DIR_SEPARATOR_S, "styling.css", NULL);
	webjournal_plugin->priv->styling = file_load(file, NULL);
	g_free(file);

	webjournal_settings = rm_settings_plugin_new("org.tabos.roger.plugins.webjournal", "webjournal");

	file = g_settings_get_string(webjournal_settings, "filename");
	if (EMPTY_STRING(file)) {
		file = g_build_filename(g_get_user_data_dir(), "roger", "journal.html", NULL);
		g_settings_set_string(webjournal_settings, "filename", file);
	}

	webjournal_plugin->priv->signal_id = g_signal_connect_after(G_OBJECT(app_object), "journal-loaded", G_CALLBACK(webjournal_journal_loaded_cb), webjournal_plugin);

}

/**
 * \brief Deactivate plugin
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	RouterManagerWebJournalPlugin *webjournal_plugin = ROUTERMANAGER_WEBJOURNAL_PLUGIN(plugin);

	/* If signal handler is connected: disconnect */
	if (g_signal_handler_is_connected(G_OBJECT(app_object), webjournal_plugin->priv->signal_id)) {
		g_signal_handler_disconnect(G_OBJECT(app_object), webjournal_plugin->priv->signal_id);
	}
}

void filename_button_clicked_cb(GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Set HTML file"), settings_get_window(), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	GtkFileFilter *filter;

	filter = gtk_file_filter_new();

	gtk_file_filter_add_mime_type(filter, "text/html");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gtk_entry_set_text(GTK_ENTRY(user_data), folder);

		g_free(folder);
	}

	gtk_widget_destroy(dialog);
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
	GtkWidget *grid = gtk_grid_new();
	GtkWidget *group;
	GtkWidget *report_dir_label;

	/* Set standard spacing to 5 */
	gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

	report_dir_label = ui_label_new(_("Web Journal output file"));
	gtk_grid_attach(GTK_GRID(grid), report_dir_label, 0, 1, 1, 1);

	GtkWidget *report_dir_entry = gtk_entry_new();
	GtkWidget *report_dir_button = gtk_button_new_with_label(_("Select"));

	gtk_widget_set_hexpand(report_dir_entry, TRUE);
	g_settings_bind(webjournal_settings, "filename", report_dir_entry, "text", G_SETTINGS_BIND_DEFAULT);

	g_signal_connect(report_dir_button, "clicked", G_CALLBACK(filename_button_clicked_cb), report_dir_entry);

	gtk_grid_attach(GTK_GRID(grid), report_dir_entry, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), report_dir_button, 2, 1, 1, 1);

	group = pref_group_create(grid, _("Web Journal"), TRUE, FALSE);

	return group;
}

