/*
 * Roger Router
 * Copyright (c) 2012-2017 Jan-Michael Brummer
 *
 * This file is part of Roger Router and is based on Claw Mail crash handler
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

#define _DEFAULT_SOURCE

#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifndef G_OS_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <grp.h>

#include <errno.h>
#include <fcntl.h>

#if HAVE_SYS_UTSNAME_H
#       include <sys/utsname.h>
#endif

#if defined(__GNU_LIBRARY__) && !defined(__UCLIBC__)
#       include <gnu/libc-version.h>
#endif

#include <rm/rm.h>

#define DEBUGGERRC "debuggerrc"
#define BUGZILLA_URI "https://www.tabos.org/forum"

static const gchar *DEBUG_SCRIPT = "thread apply all bt full\nkill\nq\n";
extern gchar *argv0;

/**
 * crash_is_dialog_allowed:
 *
 * See if the crash dialog is allowed (because some developers may prefer to run it under gdb...)
 *
 * Return: %TRUE if crash dialog is allowed
 */
static gboolean crash_is_dialog_allowed(void)
{
	return !g_getenv("ROGER_NO_CRASH");
}

/**
 * crash_handler:
 * @sig: signal number
 *
 * This handler will probably evolve into something better.
 */
static void crash_handler(int sig)
{
	pid_t pid;
	static volatile unsigned long crashed_ = 0;

	/* let's hope argv0 aren't trashed. */
	extern gchar *argv0;

	/*
	 * besides guarding entrancy it's probably also better
	 * to mask off signals
	 */
	if (crashed_) return;

	crashed_++;

#ifdef SIGTERM
	if (sig == SIGTERM)
		exit(0);
#endif

	gdk_flush();

	if (0 == (pid = fork())) {
		char buf[128];
		char *args[5];

		/*
		 * probably also some other parameters (like GTK+ ones).
		 * also we pass the full startup dir and the real command
		 * line typed in (argv0)
		 */
		args[0] = argv0;
		args[1] = "--debug";
		args[2] = "--crash";
		sprintf(buf, "%d,%d,%s", getppid(), sig, argv0);
		args[3] = buf;
		args[4] = NULL;

		if (setgid(getgid()) != 0)
			perror("setgid");
		if (setuid(getuid()) != 0 || setgroups(0, NULL) < 0)
			perror("setuid");
		execvp(argv0, args);
	} else {
		waitpid(pid, NULL, 0);
		_exit(253);
	}

	_exit(253);
}

/**
 * crash_install_handlers:
 *
 * Install crash handlers.
 */
void crash_install_handlers(void)
{
	sigset_t mask;

	if (!crash_is_dialog_allowed()) {
		return;
	}

	sigemptyset(&mask);

#ifdef SIGSEGV
	signal(SIGSEGV, crash_handler);
	sigaddset(&mask, SIGSEGV);
#endif

#ifdef SIGFPE
	signal(SIGFPE, crash_handler);
	sigaddset(&mask, SIGFPE);
#endif

#ifdef SIGILL
	signal(SIGILL, crash_handler);
	sigaddset(&mask, SIGILL);
#endif

#ifdef SIGABRT
	signal(SIGABRT, crash_handler);
	sigaddset(&mask, SIGABRT);
#endif

	sigprocmask(SIG_UNBLOCK, &mask, 0);
}

/**
 * crash_create_debugger_file:
 *
 * Create debugger script file in cache directory
 */
static void crash_create_debugger_file(void)
{
	gchar *filespec = g_strconcat(g_get_user_cache_dir(), G_DIR_SEPARATOR_S, DEBUGGERRC, NULL);

	rm_file_save(filespec, DEBUG_SCRIPT, -1);
	g_free(filespec);
}

/**
 * crash_debug:
 * @crash_pid: PID of crashed app
 * @exe_image: executable image
 * @debug_output: debug output
 *
 * Launches debugger and attaches it to crashed app
 */
static void crash_debug(unsigned long crash_pid, gchar *exe_image, GString *debug_output)
{
	int choutput[2];
	pid_t pid;

	if (pipe(choutput) == -1) {
		g_print("can't pipe - error %s", g_strerror(errno));
		return;
	}

	if (0 == (pid = fork())) {
		char *argp[10];
		char **argptr = argp;
		gchar *filespec = g_strconcat(g_get_user_cache_dir(), G_DIR_SEPARATOR_S, DEBUGGERRC, NULL);

		if (setgid(getgid()) != 0)
			perror("setgid");
		if (setuid(getuid()) != 0 || setgroups(0, NULL) < 0)
			perror("setuid");

		/*
		 * setup debugger to attach to crashed roger
		 */
		*argptr++ = "gdb";
		*argptr++ = "--nw";
		*argptr++ = "--nx";
		*argptr++ = "--quiet";
		*argptr++ = "--batch";
		*argptr++ = "-x";
		*argptr++ = filespec;
		*argptr++ = exe_image;
		*argptr++ = g_strdup_printf("%ld", crash_pid);
		*argptr = NULL;

		/*
		 * redirect output to write end of pipe
		 */
		close(1);
		if (dup(choutput[1]) < 0)
			perror("dup");
		close(choutput[0]);
		if (-1 == execvp("gdb", argp))
			g_print("error execvp\n");
	} else {
		char buf[100];
		int r;

		waitpid(pid, NULL, 0);

		/*
		 * make it non blocking
		 */
		if (-1 == fcntl(choutput[0], F_SETFL, O_NONBLOCK))
			g_print("set to non blocking failed\n");

		/*
		 * get the output
		 */
		do {
			r = read(choutput[0], buf, sizeof buf - 1);
			if (r > 0) {
				buf[r] = 0;
				g_string_append(debug_output, buf);
			}
		} while (r > 0);

		close(choutput[0]);
		close(choutput[1]);

		/*
		 * kill the process we attached to
		 */
		kill(crash_pid, SIGCONT);
	}
}

/**
 * crash_get_libc_version:
 *
 * Get ibrary version
 *
 * Returns: libc version
 */
static const gchar *crash_get_libc_version(void)
{
#if defined(__UCLIBC__)
	return g_strdup_printf("uClibc %i.%i.%i", __UCLIBC_MAJOR__, __UCLIBC_MINOR__, __UCLIBC_SUBLEVEL__);
#elif defined(__GNU_LIBRARY__)
	return g_strdup_printf("GNU libc %s", gnu_get_libc_version());
#else
	return g_strdup(_("Unknown"));
#endif
}

/**
 * crash_get_os:
 *
 * Get operating system.
 *
 * Returns: operating system
 */
static const gchar *crash_get_os(void)
{
#if HAVE_SYS_UTSNAME_H
	struct utsname utsbuf;
	uname(&utsbuf);
	return g_strdup_printf("%s %s (%s)",
			       utsbuf.sysname,
			       utsbuf.release,
			       utsbuf.machine);
#else
	return g_strdup(_("Unknown"));
#endif
}

/**
 * crash_save_log:
 * @button: #GtkButton
 * @text: text to store
 *
 * Saves crash log to a file
 */
static void crash_save_crash_log(GtkButton *button, const gchar *text)
{
	GtkFileChooserNative *native;
	GtkFileChooser *chooser;
	gchar *time;
	gchar *buf;
	GDateTime *datetime = g_date_time_new_now_local();
	gint res;

	time = g_date_time_format(datetime, "%Y-%m-%d-%H-%M-%S");
	buf = g_strdup_printf("roger-crash-log-%s.txt", time);
	g_free(time);
	g_date_time_unref(datetime);

	native = gtk_file_chooser_native_new("Save Crash Log", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, _("Save"), _("Cancel"));
	chooser = GTK_FILE_CHOOSER(native);

	gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	gtk_file_chooser_set_current_name(chooser, buf);

	res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename = gtk_file_chooser_get_filename(chooser);
		rm_file_save(filename, text, -1);
		g_free(filename);

		gtk_main_quit();
	}

	g_free(buf);

	g_object_unref(native);
}

/**
 * crash_delete_event_cb:
 * @widget: a #GtkWidget
 * @event: a #GdkEvent
 * @data: data (UNUSED)
 *
 * Destroys widget window
 *
 * Returns: %TRUE
 */
gboolean crash_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_destroy(widget);
	return TRUE;
}

void crash_exit_clicked_cb(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit();
}

/**
 * crash_dialog:
 * @text: description
 * @debug_output: output text by gdb
 *
 * \brief show crash dialog
 *
 * Returns: dialog widget
 */
static GtkWidget *crash_dialog(const gchar *text, const gchar *debug_output)
{
	GtkWidget *window1;
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *frame1;
	GtkWidget *scrolledwindow1;
	GtkWidget *text1;
	GtkWidget *hbuttonbox3;
	GtkWidget *hbuttonbox4;
	GtkWidget *button3;
	GtkWidget *button4;
	gchar *crash_report;
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window1), 18);
	gtk_window_set_title(GTK_WINDOW(window1), _("Roger Router has crashed"));
	gtk_window_set_position(GTK_WINDOW(window1), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(window1), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(window1), 460, 272);

	vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(window1), vbox1);

	hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox1), 4);

	label1 = gtk_label_new
			 (g_strdup_printf(_("%s.\nPlease file a bug report and include the information below\nand the debug log (~/.cache/rm/debug.log)."), text));
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox1), label1, TRUE, TRUE, 0);
	gtk_widget_set_halign(label1, GTK_ALIGN_START);

	frame1 = gtk_frame_new(_("Debug log"));
	gtk_widget_show(frame1);
	gtk_box_pack_start(GTK_BOX(vbox1), frame1, TRUE, TRUE, 0);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow1);
	gtk_container_add(GTK_CONTAINER(frame1), scrolledwindow1);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	text1 = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text1), FALSE);
	gtk_widget_show(text1);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), text1);

	crash_report = g_strdup_printf(
		"Roger Router version %s\n"
		"GTK+ version %d.%d.%d / GLib %d.%d.%d\n"
		"Operating system: %s\n"
		"C Library: %s\n"
		"Message: %s\n--\n%s",
		PACKAGE_VERSION,
		gtk_major_version, gtk_minor_version, gtk_micro_version,
		glib_major_version, glib_minor_version, glib_micro_version,
		crash_get_os(),
		crash_get_libc_version(),
		text,
		debug_output);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text1));
	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, crash_report, -1);

	hbuttonbox3 = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(hbuttonbox3);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox3, FALSE, FALSE, 0);

	hbuttonbox4 = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(hbuttonbox4);
	gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox4, FALSE, FALSE, 0);

	button3 = gtk_button_new_with_label(_("Close"));
	gtk_widget_show(button3);
	gtk_container_add(GTK_CONTAINER(hbuttonbox4), button3);
	gtk_widget_set_can_default(button3, TRUE);

	button4 = gtk_button_new_with_label(_("Save"));
	gtk_widget_show(button4);
	gtk_container_add(GTK_CONTAINER(hbuttonbox4), button4);
	gtk_widget_set_can_default(button4, TRUE);

	g_signal_connect(G_OBJECT(window1), "delete_event", G_CALLBACK(crash_delete_event_cb), NULL);
	g_signal_connect(G_OBJECT(button3), "clicked", G_CALLBACK(crash_exit_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(button4), "clicked", G_CALLBACK(crash_save_crash_log), crash_report);

	gtk_window_set_default_size(GTK_WINDOW(window1), 800, 600);

	gtk_widget_show(window1);

	return window1;
}

/**
 * crash_main:
 * @arg: arguments
 *
 * Crash dialog entry point
 */
void crash_main(const char *arg)
{
	gchar *text;
	gchar **tokens;
	unsigned long pid;
	GString *output;
	gint signal;

	crash_create_debugger_file();
	tokens = g_strsplit(arg, ",", 0);

	pid = atol(tokens[0]);
	signal = atol(tokens[1]);
	text = g_strdup_printf(_("Roger Router process (%ld) received signal %d"), pid, signal);

	output = g_string_new("");
	crash_debug(pid, tokens[2], output);

	crash_dialog(text, output->str);
	g_string_free(output, TRUE);
	g_free(text);
	g_strfreev(tokens);
}

#else
/**
 * crash_install_handlers:
 *
 * Install crash handlers.
 */
void crash_install_handlers(void)
{
}

/**
 * crash_main:
 * @arg: arguments
 *
 * Crash dialog entry point
 */
void crash_main(const char *arg)
{
}
#endif
