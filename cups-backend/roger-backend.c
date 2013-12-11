/**
 * Roger print spooler backend
 * Copyright (c) 2013 Louis Lagendijk
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
 *
 * Based on cups AppSocket sources
 * Copyright 2007 by Apple Inc.
 * Copyright 1997-2007 by Easy Software Products, all rights reserved.
 *
 * These coded instructions, statements, and computer programs are the
 * property of Apple Inc. and are protected by Federal copyright
 * law.  Distribution and use rights are outlined in the file
 * "COPYING" which should have been included with this file.  If this
 * file is missing or damaged, see the license at "http://www.cups.org/".
 */

/*
 * Include necessary headers.
 */

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif /* WIN32 */
#include "roger-backend.h"

/*
 * \brief Send a file to the roger fax spool directory
 * \argc number of argments
 * \argv argements
 * \return error code if any
 *
 *  Usage: printer-uri job-id user title copies options [file]
 */

int main(int argc, char *argv[])
{
	int copies;
	int input_fd;
	output_t *output_desc;
	ssize_t bytes_written;

#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action;	/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */


	/*
	 * Make sure status messages are not buffered...
	 */

	setbuf(stderr, NULL);

	/*
	 * Ignore SIGPIPE signals...
	 */

#ifdef HAVE_SIGSET
	sigset(SIGPIPE, SIG_IGN);
#elif defined(HAVE_SIGACTION)
	memset(&action, 0, sizeof(action));
	action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &action, NULL);
#else
	signal(SIGPIPE, SIG_IGN);
#endif /* HAVE_SIGSET */


	/*
	 * Check command-line...
	 */

	switch (argc) {

	case 1:
		printf(
		    /* device-class device-uril */
		    "file %s:/ "
		    /* device make and model */
		    "\"Roger-Router Fax\" "
		    /* device-info */
		    "\"Roger Router Fax\" "
		    /* device-id */
		    "\"%s\"\n",
		    argv[0], ROGER_BACKEND_DEVICE_ID);
		return (CUPS_BACKEND_OK);

	case 6:
		/* print from stdin */
		input_fd = 0;
		copies = 1;
		break;

	case 7:
		/* Print file from command line... */

		if ((input_fd = open(argv[6], O_RDONLY)) < 0) {
			perror("ERROR: unable to open print file");
			return (CUPS_BACKEND_FAILED);
		}
		copies = atoi(argv[4]);
		break;

	default:
		fprintf(stderr, "roger-fax backend - version %s\n" , VERSION);
		fprintf(stderr, "Usage: %s job-id user title copies options [file]\n",
		        argv[0]);
		return (CUPS_BACKEND_FAILED);
	}

	/*
	 * Create output descriptor
	 * outputname includes job-id, username and original name
	 */

	output_desc = open_fax_output(copies, argv[1], argv[2], argv[3], O_WRONLY | O_CREAT);

	if (output_desc == NULL) {
		/* TODO: improve backend return value setting? we may use BACEND_HOLD to keep queue going */
		return (CUPS_BACKEND_STOP);
	}

	fprintf(stderr, "INFO: Creating file %s\n",
	        get_output_name(output_desc));


	fprintf(stderr, "INFO: Connected to %s...\n", get_output_name(output_desc));

	/*
	 * Print everything...
	 */

	bytes_written = 0;

	if (input_fd != 0) {
		fputs("PAGE: 1 1\n", stderr);
		lseek(input_fd, 0, SEEK_SET);
	}

	bytes_written = roger_backend_run_loop(input_fd, output_desc, get_output_name(output_desc));

	if (input_fd != 0 && bytes_written >= 0) {
#ifdef HAVE_LONG_LONG
		fprinf(stderr,
		       _
		       ("INFO: Sent print file, %lld bytes...\n"),
		       CUPS_LLCAST bytes_written);
#else
		fprintf(stderr, "INFO: Sent print file, %ld bytes...\n",
		        CUPS_LLCAST bytes_written);
#endif /* HAVE_LONG_LONG */
	}

	/*
	 * Close the socket connection...
	 */

	close_fax_output(output_desc);

	/*
	 * Close the input file and return...
	 */

	if (input_fd != 0) {
		close(input_fd);
	}

	if (bytes_written >= 0) {
		fputs("INFO: Ready to print.\n", stderr);
	}
	return (bytes_written < 0 ? CUPS_BACKEND_FAILED : CUPS_BACKEND_OK);
}
