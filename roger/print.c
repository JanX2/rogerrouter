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

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include <cairo-pdf.h>

#include <libroutermanager/appobject-emit.h>
#include <libroutermanager/router.h>
#include <libroutermanager/libfaxophone/faxophone.h>
#include <libroutermanager/libfaxophone/fax.h>

#include <roger/print.h>
#include <roger/journal.h>
#include <roger/main.h>

#define FONT "cairo:monospace 10"

#ifndef GTK_PRINT_SETTINGS_OUTPUT_BASENAME
#define GTK_PRINT_SETTINGS_OUTPUT_BASENAME "output-basename"
#endif

/** Structure holds information about the page and start positions of columns */
typedef struct {
	gdouble font_width;
	gdouble line_height;
	gdouble char_width;
	PangoLayout *layout;

	GtkTreeView *view;
	gint lines_per_page;
	gint num_lines;
	gint num_pages;

	gint logo_pos;
	gint date_time_pos;
	gint name_pos;
	gint number_pos;
	gint local_name_pos;
	gint local_number_pos;
	gint duration_pos;
} PrintData;

/**
 * \brief Check if font is monospace
 * \param pc pango context
 * \param desc font description
 * \return wheter font is monospace or not (TRUE/FALSE)
 */
static gboolean check_monospace(PangoContext *pc, PangoFontDescription *desc)
{
	PangoFontFamily **families;
	gint num_families, i;
	const gchar *font;
	gboolean ret = TRUE;

	font = pango_font_description_get_family(desc);
	pango_context_list_families(pc, &families, &num_families);

	for (i = 0; i < num_families; i++) {
		const gchar *check = pango_font_family_get_name(families[i]);

		if (font && check && strcasecmp(font, check) == 0) {
			if (!pango_font_family_is_monospace(families[i])) {
				ret = FALSE;
			}
		}
	}

	g_free(families);

	return ret;
}

/**
 * \brief Get font width
 * \param context print context
 * \param desc font description
 * \return font size
 */
static gint get_font_width(GtkPrintContext *context, PangoFontDescription *desc)
{
	PangoContext *pc;
	PangoFontMetrics *metrics;
	gint width;

	pc = gtk_print_context_create_pango_context(context);
	if (!check_monospace(pc, desc)) {
		g_debug("The request font is not a monospace font!");
	}

	metrics = pango_context_get_metrics(pc, desc, pango_context_get_language(pc));
	width = pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
	if (width == 0) {
		width = pango_font_metrics_get_approximate_char_width(metrics) / PANGO_SCALE;
		if (width == 0) {
			width = pango_font_description_get_size(desc) / PANGO_SCALE;
		}
	}

	pango_font_metrics_unref(metrics);
	g_object_unref(pc);

	return width;
}

/**
 * \brief Get page count
 * \param context print context
 * \param print_data print data
 * \return page count in print data
 */
static int get_page_count(GtkPrintContext *context, PrintData *print_data)
{
	gdouble width, height;
	gint layout_h;
	gint layout_v;

	if (print_data == NULL) {
		return -1;
	}

	width = gtk_print_context_get_width(context);
	height = gtk_print_context_get_height(context);

	pango_layout_set_width(print_data->layout, width * PANGO_SCALE);

	pango_layout_set_text(print_data->layout, "Z", -1);
	pango_layout_get_size(print_data->layout, &layout_v, &layout_h);
	if (layout_h <= 0) {
		g_debug("Invalid layout h (%d). Falling back to default height (%d)", layout_h, 100 * PANGO_SCALE);
		layout_h = 100 * PANGO_SCALE;
	}
	if (layout_v <= 0) {
		g_debug("Invalid layout v (%d). Falling back to default width (%d)", layout_v, 100 * PANGO_SCALE);
		layout_v = 100 * PANGO_SCALE;
	}

	print_data->line_height = (gdouble) layout_h / PANGO_SCALE + 2;
	print_data->char_width = (gdouble) layout_v / PANGO_SCALE;
	print_data->lines_per_page = ceil((height - print_data->line_height) / print_data->line_height);

	/* 3 for header, 1 for bar */
	print_data->lines_per_page -= 5;

	return (print_data->num_lines / print_data->lines_per_page) + 1;
}

/**
 * \brief Begin printing, calculate values and store them in PrintData
 * \param operation print operation
 * \param context print context
 * \param user_data user data pointer (PrintData)
 */
static void begin_print_cb(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data)
{
	PrintData *print_data = (PrintData *) user_data;
	GtkTreeModel *model;
	GtkTreeIter sIter;
	gboolean valid;
	PangoFontDescription *desc = pango_font_description_from_string(FONT);

	print_data->num_lines = 0;

	model = gtk_tree_view_get_model(print_data->view);
	valid = gtk_tree_model_get_iter_first(model, &sIter);
	while (valid) {
		valid = gtk_tree_model_iter_next(model, &sIter);
		print_data->num_lines++;
	}

	print_data->layout = gtk_print_context_create_pango_layout(context);
	pango_layout_set_wrap(print_data->layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_spacing(print_data->layout, 0);
	pango_layout_set_attributes(print_data->layout, NULL);
	pango_layout_set_font_description(print_data->layout, desc);

	print_data->num_pages = get_page_count(context, print_data);
	print_data->font_width = get_font_width(context, desc) + 1;
	if (print_data->font_width == 0) {
		print_data->font_width = print_data->char_width;
	}

	print_data->logo_pos = 4;
	print_data->date_time_pos = print_data->logo_pos + print_data->font_width * 4;
	print_data->name_pos = print_data->date_time_pos + print_data->font_width * 12;
	print_data->number_pos = print_data->name_pos + print_data->font_width * 19;
	print_data->local_name_pos = print_data->number_pos + print_data->font_width * 14;
	print_data->local_number_pos = print_data->local_name_pos + print_data->font_width * 11;
	print_data->duration_pos = print_data->local_number_pos + print_data->font_width * 14;

	if (print_data->num_pages >= 0) {
		gtk_print_operation_set_n_pages(operation, print_data->num_pages);
	}

	pango_font_description_free(desc);
}

/**
 * \brief Short text name
 * \param cairo cairo pointer
 * \param layout pango layout
 * \param text text string
 * \param width maximum column width
 */
void show_text(cairo_t *cairo, PangoLayout *layout, gchar *text, gint width)
{
	gint text_width, text_height;

	pango_layout_set_text(layout, text, -1);
	pango_layout_get_pixel_size(layout, &text_width, &text_height);

	if (text_width > width) {
		pango_layout_set_width(layout, width * PANGO_SCALE);
		pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	}
	pango_cairo_show_layout(cairo, layout);

	if (text_width > width) {
		pango_layout_set_width(layout, -1);
	}
}

/**
 * \brief Get date/time in specifc format
 * \param format date/time format
 * \return date/time string
 */
static gchar *get_date_time(const gchar *format)
{
	const struct tm *time_m;
	static gchar date[1024];
	gchar *locale_format;
	gsize len;

	if (!g_utf8_validate(format, -1, NULL)) {
		locale_format = g_locale_from_utf8(format, -1, NULL, NULL, NULL);
		if (locale_format == NULL) {
			return NULL;
		}
	} else {
		locale_format = g_strdup(format);
	}

	time_t time_s = time(NULL);
	time_m = localtime(&time_s);

	len = strftime(date, 1024, locale_format, time_m);
	g_free(locale_format);

	if (len == 0) {
		return NULL;
	}

	if (!g_utf8_validate(date, len, NULL)) {
		return g_locale_to_utf8(date, len, NULL, NULL, NULL);
	}

	return g_strdup(date);
}

/**
 * \brief Draw one single page
 * \param operation print operation
 * \param context print context
 * \param nPageNr page number to print
 * \param user_data user data pointer (PrintData)
 */
static void draw_page_cb(GtkPrintOperation *operation, GtkPrintContext *context, gint nPageNr, gpointer user_data)
{
	PrintData *print_data = (PrintData *) user_data;
	cairo_t *cairo;
	gint line, i = 0;
	GtkTreeModel *model;
	GtkTreeIter sIter;
	GdkPixbuf *pix, *dst_pix;
	gboolean valid;
	gchar *date_time;
	gchar *name;
	gchar *number;
	gchar *local_name;
	gchar *local_number;
	gchar *duration;
	gint tmp;
	gint line_height = 0;

	cairo = gtk_print_context_get_cairo_context(context);
	gdouble width = gtk_print_context_get_width(context);

	cairo_set_source_rgb(cairo, 0, 0, 0);
	cairo_move_to(cairo, 0, 0);
	pango_layout_set_width(print_data->layout, width * PANGO_SCALE);
	pango_layout_set_alignment(print_data->layout, PANGO_ALIGN_LEFT);
	pango_layout_set_ellipsize(print_data->layout, FALSE);
	pango_layout_set_justify(print_data->layout, FALSE);

	pango_layout_set_width(print_data->layout, (width - 8) * PANGO_SCALE);

	/* Title */
	gchar *data = g_strdup_printf("<b>%s - %s</b>", PACKAGE_NAME, _("Anruferliste"));
	pango_layout_set_markup(print_data->layout, data, -1);
	pango_layout_set_alignment(print_data->layout, PANGO_ALIGN_CENTER);
	cairo_move_to(cairo, 3, print_data->line_height * 0.5);
	pango_cairo_show_layout(cairo, print_data->layout);
	g_free(data);

	/* Page */
	data = g_strdup_printf(_("<small>Page %d of %d</small>"), nPageNr + 1, print_data->num_pages);
	pango_layout_set_markup(print_data->layout, data, -1);
	pango_layout_set_alignment(print_data->layout, PANGO_ALIGN_LEFT);
	cairo_move_to(cairo, 4, print_data->line_height * 1.5);
	pango_cairo_show_layout(cairo, print_data->layout);
	g_free(data);

	/* Date */
	gchar *pnDate = get_date_time("%d.%m.%Y %H:%M:%S");
	data = g_strdup_printf("<small>%s</small>", pnDate);
	pango_layout_set_markup(print_data->layout, data, -1);
	pango_layout_set_alignment(print_data->layout, PANGO_ALIGN_RIGHT);
	cairo_move_to(cairo, 2, print_data->line_height * 1.5);
	pango_cairo_show_layout(cairo, print_data->layout);
	g_free(pnDate);
	g_free(data);

	/* Reset */
	cairo_move_to(cairo, 0, 0);
	pango_layout_set_width(print_data->layout, width * PANGO_SCALE);
	pango_layout_set_ellipsize(print_data->layout, FALSE);
	pango_layout_set_justify(print_data->layout, FALSE);
	pango_layout_set_attributes(print_data->layout, NULL);
	pango_layout_set_alignment(print_data->layout, PANGO_ALIGN_LEFT);

	line = nPageNr * print_data->lines_per_page;

	model = gtk_tree_view_get_model(print_data->view);
	valid = gtk_tree_model_get_iter_first(model, &sIter);
	tmp = 0;
	if (line != 0) {
		while (valid) {
			valid = gtk_tree_model_iter_next(model, &sIter);
			if (++tmp == line) {
				break;
			}
		}
	}
	cairo_set_line_width(cairo, 0);

	/* Draw header */
	cairo_rectangle(cairo, 2, print_data->line_height * 3, width - 2, print_data->line_height);
	cairo_set_source_rgb(cairo, 0.9, 0.9, 0.9);
	cairo_fill(cairo);
	cairo_stroke(cairo);
	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);

	cairo_move_to(cairo, print_data->logo_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Type"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	cairo_move_to(cairo, print_data->date_time_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Date/Time"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	cairo_move_to(cairo, print_data->name_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Name"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	cairo_move_to(cairo, print_data->number_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Number"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	cairo_move_to(cairo, print_data->local_name_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Local Name"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	cairo_move_to(cairo, print_data->local_number_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Local Number"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	cairo_move_to(cairo, print_data->duration_pos, print_data->line_height * 3 + 1);
	pango_layout_set_text(print_data->layout, _("Duration"), -1);
	pango_cairo_show_layout(cairo, print_data->layout);

	/* print caller rows */
	for (i = 1; valid && i <= print_data->lines_per_page && line < print_data->num_lines; i++) {
		cairo_rectangle(cairo, 2, (3 + i) * print_data->line_height, width - 4, print_data->line_height);
		if (!(i & 1)) {
			cairo_set_source_rgb(cairo, 0.9, 0.9, 0.9);
			cairo_fill(cairo);
			cairo_stroke(cairo);
			cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
		}
		cairo_stroke(cairo);

		gtk_tree_model_get(model, &sIter, JOURNAL_COL_TYPE, &pix, JOURNAL_COL_DATETIME, &date_time, JOURNAL_COL_NAME, &name, JOURNAL_COL_NUMBER, &number, JOURNAL_COL_EXTENSION, &local_name, JOURNAL_COL_LINE, &local_number, JOURNAL_COL_DURATION, &duration, -1);
		dst_pix = gdk_pixbuf_scale_simple(pix, 10, 10, GDK_INTERP_BILINEAR);

		cairo_save(cairo);
		gdk_cairo_set_source_pixbuf(cairo, dst_pix, print_data->logo_pos, (3 + i) * print_data->line_height + 2);
		cairo_paint(cairo);
		cairo_restore(cairo);
		g_object_unref(dst_pix);

		cairo_move_to(cairo, print_data->date_time_pos, (3 + i) * print_data->line_height + 1);
		pango_layout_set_text(print_data->layout, date_time, -1);
		pango_cairo_show_layout(cairo, print_data->layout);

		cairo_move_to(cairo, print_data->name_pos, (3 + i) * print_data->line_height + 1);
		show_text(cairo, print_data->layout, name, print_data->number_pos - print_data->name_pos);

		cairo_move_to(cairo, print_data->number_pos, (3 + i) * print_data->line_height + 1);
		show_text(cairo, print_data->layout, number, print_data->local_name_pos - print_data->number_pos);

		cairo_move_to(cairo, print_data->local_name_pos, (3 + i) * print_data->line_height + 1);
		if (local_name != NULL && strlen(local_name) > 0) {
			show_text(cairo, print_data->layout, local_name, print_data->local_number_pos - print_data->local_name_pos);
		}

		cairo_move_to(cairo, print_data->local_number_pos, (3 + i) * print_data->line_height + 1);
		if (local_number != NULL && strlen(local_number) > 0) {
			show_text(cairo, print_data->layout, local_number, print_data->duration_pos - print_data->local_number_pos);
		}

		cairo_move_to(cairo, print_data->duration_pos, (3 + i) * print_data->line_height + 1);
		if (duration != NULL && strlen(duration) > 0) {
			show_text(cairo, print_data->layout, duration, width - print_data->duration_pos);
		}

		valid = gtk_tree_model_iter_next(model, &sIter);
		g_free(date_time);
		g_free(name);
		g_free(number);
		g_free(local_name);
		g_free(local_number);
		g_free(duration);

		cairo_rel_move_to(cairo, 2, line_height);
		line++;
	}
}

/**
 * \brief Called on end of printing, free PrintData
 * \param operation print operation
 * \param context print context
 * \param user_data user data pointer (PrintData)
 */
static void end_print_cb(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data)
{
	PrintData *print_data = (PrintData *) user_data;

	g_free(print_data);
}

/**
 * \brief Print treeview callback function
 * \param view_widget text view widget
 */
void journal_print(GtkWidget *view_widget)
{
	GtkTreeView *view = GTK_TREE_VIEW(view_widget);
	GtkPrintOperation *operation;
	GtkPrintSettings *settings;
	PrintData *print_data;
	GError *error = NULL;

	operation = gtk_print_operation_new();
	print_data = g_new0(PrintData, 1);
	print_data->view = view;

	settings = gtk_print_settings_new();
	gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_OUTPUT_BASENAME, _("Roger Router-Journal"));
	gtk_print_operation_set_print_settings(operation, settings);
	gtk_print_operation_set_unit(operation, GTK_UNIT_POINTS);
	gtk_print_operation_set_show_progress(operation, TRUE);

	gtk_print_operation_set_embed_page_setup(operation, TRUE);

	g_signal_connect(G_OBJECT(operation), "begin-print", G_CALLBACK(begin_print_cb), print_data);
	g_signal_connect(G_OBJECT(operation), "draw-page", G_CALLBACK(draw_page_cb), print_data);
	g_signal_connect(G_OBJECT(operation), "end-print", G_CALLBACK(end_print_cb), print_data);

	gtk_print_operation_run(operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, &error);

	g_object_unref(operation);

	if (error != NULL) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", error->message);
		g_error_free(error);

		g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);

		gtk_widget_show(dialog);
	}
}

/**
 * \brief Create fax report based on give information
 * \param status fax status structure
 * \param report_dir storage directory
 */
void create_fax_report(struct fax_status *status, const char *report_dir)
{
	cairo_t *cairo;
	cairo_surface_t *out;
	int width, height;
	time_t time_s = time(NULL);
	struct tm *time_ptr = localtime(&time_s);
	char *buffer;
	GdkPixbuf *pixbuf;
	char *file = status->tiff_file;
	struct contact *contact = NULL;
	char *remote = status->trg_no;
	char *local = status->src_no;
	char *status_code = status->error_code == 0 ? _("SUCCESS") : _("FAILED");
	int pages = status->page_current;
	int bitrate = status->bitrate;
	struct profile *profile = profile_get_active();

	if (file == NULL) {
		g_warning("File is NULL\n");
		return;
	}

	if (report_dir == NULL) {
		g_warning("ReportDir is NULL\n");
		return;
	}

	pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	if (pixbuf == NULL) {
		g_warning("PixBuf is null (file '%s')\n", file);
		return;
	}

	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	if (width != 1728 || height != 2292) {
		width = 1728;
		height = 2292;
		GdkPixbuf *scale = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);

		g_object_unref(pixbuf);
		pixbuf = scale;
	}

	buffer = g_strdup_printf("%s/fax-report_%s_%s-%02d_%02d_%d_%02d_%02d_%02d.pdf",
	                         report_dir, local, remote,
	                         time_ptr->tm_mday, time_ptr->tm_mon + 1, time_ptr->tm_year + 1900,
	                         time_ptr->tm_hour, time_ptr->tm_min, time_ptr->tm_sec);
	out = cairo_pdf_surface_create(buffer, width, height + 200);
	g_free(buffer);

	cairo = cairo_create(out);
	gdk_cairo_set_source_pixbuf(cairo, pixbuf, 0, 200);
	cairo_paint(cairo);

	cairo_set_source_rgb(cairo, 0, 0, 0);
	cairo_select_font_face(cairo, "cairo:monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cairo, 20.0);

	cairo_move_to(cairo, 60.0, 60.0);
	buffer = g_strconcat(_("Fax Transfer Protocol"), " (", router_get_name(profile), ")", NULL);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	cairo_select_font_face(cairo, "cairo:monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

	/* Date/Time */
	cairo_move_to(cairo, 60.0, 95.0);
	cairo_show_text(cairo, _("Date/Time:"));
	cairo_move_to(cairo, 280.0, 95.0);
	buffer = get_date_time("%a %b %d %Y - %X");
	cairo_show_text(cairo, buffer);

	/* Status */
	cairo_move_to(cairo, 625.0, 95.0);
	cairo_show_text(cairo, _("Transfer status:"));
	cairo_move_to(cairo, 850.0, 95.0);
	cairo_show_text(cairo, status_code);

	/* FAX-ID */
	cairo_move_to(cairo, 60.0, 120.0);
	cairo_show_text(cairo, _("Receiver ID:"));
	cairo_move_to(cairo, 280.0, 120.0);
	buffer = g_strdup_printf("%s", status->remote_ident);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	/* Pages */
	cairo_move_to(cairo, 625.0, 120.0);
	cairo_show_text(cairo, _("Pages sent:"));
	cairo_move_to(cairo, 850.0, 120.0);
	buffer = g_strdup_printf("%d", pages);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	/* Remote name */
	cairo_move_to(cairo, 60.0, 145.0);
	cairo_show_text(cairo, _("Recipient name:"));

	struct contact contact_s;

	contact = &contact_s;
	/** Ask for contact information */
	memset(contact, 0, sizeof(struct contact));
	contact_s.number = remote;
	emit_contact_process(contact);
	cairo_move_to(cairo, 280.0, 145.0);
	cairo_show_text(cairo, contact ? contact->name : "");

	/* Remote number */
	cairo_move_to(cairo, 625.0, 145.0);
	cairo_show_text(cairo, _("Recipient number:"));
	cairo_move_to(cairo, 850.0, 145.0);
	buffer = call_full_number(remote, FALSE);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	/* Local name */
	cairo_move_to(cairo, 60.0, 170.0);
	cairo_show_text(cairo, _("Sender name:"));
	buffer = g_strdup_printf("%s", status->header);
	cairo_move_to(cairo, 280.0, 170.0);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	/* Local number */
	cairo_move_to(cairo, 625.0, 170.0);
	cairo_show_text(cairo, _("Sender number:"));
	cairo_move_to(cairo, 850.0, 170.0);
	buffer = call_full_number(local, FALSE);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	/* Bitrate */
	cairo_move_to(cairo, 60.0, 195.0);
	cairo_show_text(cairo, _("Bitrate:"));
	cairo_move_to(cairo, 280.0, 195.0);
	buffer = g_strdup_printf("%d", bitrate);
	cairo_show_text(cairo, buffer);
	g_free(buffer);

	/* Error message */
	cairo_move_to(cairo, 625.0, 195.0);
	cairo_show_text(cairo, _("Message:"));
	cairo_move_to(cairo, 850.0, 195.0);
	buffer = (char *) t30_completion_code_to_str(status->error_code);
	cairo_show_text(cairo, buffer);

	/* line */
	cairo_set_line_width(cairo, 0.5);
	cairo_move_to(cairo, 0, 200);
	cairo_rel_line_to(cairo, width, 0);
	cairo_stroke(cairo);

	cairo_show_page(cairo);

	cairo_destroy(cairo);

	cairo_surface_flush(out);
	cairo_surface_destroy(out);

	g_object_unref(pixbuf);
}
