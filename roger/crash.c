/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

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

#include <errno.h>
#include <fcntl.h>

#if HAVE_SYS_UTSNAME_H
#	include <sys/utsname.h>
#endif

#if defined(__GNU_LIBRARY__) && !defined(__UCLIBC__)
#	include <gnu/libc-version.h>
#endif

#include <libroutermanager/file.h>
#include <libroutermanager/profile.h>
#include <libroutermanager/router.h>

#define DEBUGGERRC "debuggerrc"
#define BUGZILLA_URI "https://www.tabos.org/forum"

static const gchar *DEBUG_SCRIPT = "thread apply all bt full\nkill\nq";
extern gchar *argv0;

/**
 * \brief see if the crash dialog is allowed (because some developers may prefer to run claws-mail under gdb...)
 */
static gboolean is_crash_dialog_allowed(void)
{
	return !g_getenv("ROGER_NO_CRASH");
}

/**
 * \brief this handler will probably evolve into something better.
 */
static void crash_handler(int sig)
{
	pid_t pid;
	static volatile unsigned long crashed_ = 0;
	struct profile *profile = profile_get_active();

	/*
	 * let's hope argv0 aren't trashed.
	 * both are defined in main.c.
	 */
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
		const gchar *name = router_get_name(profile);
		const gchar *firmware = router_get_version(profile);

		/*
		 * probably also some other parameters (like GTK+ ones).
		 * also we pass the full startup dir and the real command
		 * line typed in (argv0)
		 */
		args[0] = argv0;
		args[1] = "--debug";
		args[2] = "--crash";
		sprintf(buf, "%d,%d,%s,%s,%s", getppid(), sig, argv0, name ? name : "", firmware ? firmware : "");
		args[3] = buf;
		args[4] = NULL;

		if (setgid(getgid()) != 0)
			perror("setgid");
		if (setuid(getuid()) != 0 )
			perror("setuid");
		execvp(argv0, args);
	} else {
		waitpid(pid, NULL, 0);
		_exit(253);
	}

	_exit(253);
}

/**
 *\brief install crash handlers
 */
void crash_install_handlers(void)
{
	sigset_t mask;

	if (!is_crash_dialog_allowed()) return;

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
 * \brief create debugger script file in roger directory
 */
static void crash_create_debugger_file(void)
{
	gchar *filespec = g_strconcat(g_get_user_cache_dir(), G_DIR_SEPARATOR_S, DEBUGGERRC, NULL);

	file_save(filespec, DEBUG_SCRIPT, -1);
	g_free(filespec);
}

/**
 * \brief launches debugger and attaches it to crashed roger
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
		if (setuid(getuid()) != 0)
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
		*argptr   = NULL;

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
 * \brief library version
 */
static const gchar *get_lib_version(void)
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
 * \brief operating system
 */
static const gchar *get_operating_system(void)
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
 * \brief saves crash log to a file
 */
static void crash_save_crash_log(GtkButton *button, const gchar *text)
{
	gchar *time;
	gchar *buf;
	GDateTime *datetime = g_date_time_new_now_local();

	time = g_date_time_format(datetime, "%Y-%m-%d-%H-%M-%S");
	buf = g_strdup_printf("%s/roger-crash-log-%s.txt", g_get_home_dir(), time);
	g_free(time);
	g_date_time_unref(datetime);

	file_save(buf, text, -1);
	g_free(buf);
}

/**
 * \brief show crash dialog
 * \param text Description
 * \param debug_output Output text by gdb
 * \return GtkWidget *Dialog widget
 */
static GtkWidget *crash_dialog_show(const gchar *text, const gchar *name, const gchar *firmware, const gchar *debug_output)
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
	    (g_strdup_printf(_("%s.\nPlease file a bug report and include the information below\nand the debug log (~/.cache/routermanager/debug.log)."), text));
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox1), label1, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(label1), 7.45058e-09, 0.5);

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
		"Router: %s (%s)\n"
		"Message: %s\n--\n%s",
		VERSION,
		gtk_major_version, gtk_minor_version, gtk_micro_version,
		glib_major_version, glib_minor_version, glib_micro_version,
		get_operating_system(),
		get_lib_version(),
		name, firmware,
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

	button4 = gtk_button_new_with_label(_("Save in HOME"));
	gtk_widget_show(button4);
	gtk_container_add(GTK_CONTAINER(hbuttonbox4), button4);
	gtk_widget_set_can_default(button4, TRUE);

	g_signal_connect(G_OBJECT(window1), "delete_event",
			 G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(button3),   "clicked",
			 G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(button4), "clicked",
			 G_CALLBACK(crash_save_crash_log), crash_report);

	gtk_window_set_default_size(GTK_WINDOW(window1), 800, 600);

	gtk_widget_show(window1);

	gtk_main();
	return window1;
}


/**
 * \brief crash dialog entry point
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

	crash_dialog_show(text, tokens[3], tokens[4], output->str);
	g_string_free(output, TRUE);
	g_free(text);
	g_strfreev(tokens);
}

#else
void crash_install_handlers(void)
{
}

void crash_main(const char *arg)
{
}
#endif
