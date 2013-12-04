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

#include <sys/select.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include "roger-backend.h"

/*
 * 'roger_backend_run_loop()' - Read and write print and back-channel data.
 */

ssize_t roger_backend_run_loop(int input_fd, output_t *output_desc, char *output_name)
{
	int nfds;
	fd_set input_set;
	ssize_t bytes_to_print;
	ssize_t total_bytes_written;
	ssize_t bytes_written;
	int draining;
	char print_buffer[ROGER_BACKEND_PRINTBUF_MAX];
	char *print_ptr;
	int result;

#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action;	/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */

#if ROGER_BACKEND_CUPS_VERSION >= 103
	int side_channel_open;		/* side channel status */
	cups_sc_command_t command;
	cups_sc_status_t status;
	char data[16536];
	int datalen;
#endif /* cups >= 1.3 */

	fprintf(stderr,
	        "DEBUG: roger_backend_run_loop(input_fd=%d\n",
	        input_fd);

	/*
	 * If we are printing data from a print driver on stdin, ignore SIGTERM
	 * so that the driver can finish out any page data, e.g. to eject the
	 * current page.  We only do this for stdin printing as otherwise there
	 * is no way to cancel a raw print job...
	 */

	if (!input_fd) {
#ifdef HAVE_SIGSET				/* Use System V signals over POSIX to avoid
								   bugs */
		sigset(SIGTERM, SIG_IGN);
#elif defined(HAVE_SIGACTION)
		memset(&action, 0, sizeof(action));

		sigemptyset(&action.sa_mask);
		action.sa_handler = SIG_IGN;
		sigaction(SIGTERM, &action, NULL);
#else
		signal(SIGTERM, SIG_IGN);
#endif /* HAVE_SIGSET */
	}

	/*
	 * Figure out the maximum file descriptor value to use with select()...
	 */

	nfds = (input_fd > CUPS_SC_FD ? input_fd : CUPS_SC_FD) + 1;

	bytes_to_print = 0;
	print_ptr = print_buffer;
	total_bytes_written = 0;
	draining = 0;
	side_channel_open = 1;

	/*
	 * Now loop until we are out of data from input_fd...
	 */

	while (1) {
		/*
		 * Use select() to determine whether we have data to copy around...
		 */

		FD_ZERO(&input_set);
		if (! bytes_to_print) {
			FD_SET(input_fd, &input_set);
		}


		/*
		 * Accept side channel data, unless there is print data pending (cups >= 1.3)
		 */

#if  ROGER_BACKEND_CUPS_VERSION >= 103
		if (side_channel_open && !draining) {
			FD_SET(CUPS_SC_FD, &input_set);
		}
#endif

		result =  select(nfds, &input_set, NULL, NULL, NULL);
		if (result < 0) {
			if (errno == EINTR && total_bytes_written == 0) {
				fputs("DEBUG: Received an interrupt before any bytes_writtenwere "
				      "written, aborting!\n", stderr);
				return 0;
			}
		}


#if  ROGER_BACKEND_CUPS_VERSION >= 103
		/*
		 * Check if we have a side-channel request ready (cups >= 1.3)...
		 */

		if (FD_ISSET(CUPS_SC_FD, &input_set)) {
			/*
			 * Do the side-channel request
			 */

			datalen = sizeof(data) - 1;

			if (cupsSideChannelRead(&command, &status, data, &datalen, 1.0)
			        != 0) {
				/*
				   side channel is closed, or we lost synchronization
				 */
				side_channel_open = 0;
			} else {
				switch (command) {
				case CUPS_SC_CMD_DRAIN_OUTPUT:

					/*
					 * Our sockets disable the Nagle algorithm and data is sent immediately.
					 *
					 */

					draining = 1;

					/*
					   we will do cupsSideChannelWrite() once there is no
					   data left !
					 */
					break;

				case CUPS_SC_CMD_GET_BIDI:
					status = CUPS_SC_STATUS_OK;
					data[0] = CUPS_SC_BIDI_NOT_SUPPORTED;
					datalen = 1;
					cupsSideChannelWrite(command, status, data, datalen,
					                     1.0);
					break;

				case CUPS_SC_CMD_GET_DEVICE_ID:
					status = CUPS_SC_STATUS_OK;
					strncpy(data, ROGER_BACKEND_DEVICE_ID, 16535);
					data[16535] = '\0';
					cupsSideChannelWrite(command, status, data, strlen(data),
					                     1.0);
					break;


#if ROGER_BACKEND_CUPS_VERSION >= 105
				case CUPS_SC_CMD_GET_CONNECTED:
					status = CUPS_SC_STATUS_OK;
					data[0] = (output_desc != NULL);
					datalen = 1;
					break;
#endif

				default:

					/*
					 * this covers the following values
					 *
					 * case CUPS_SC_CMD_GET_STATE:
					 * case CUPS_SC_CMD_SOFT_RESET:
					 * for CUPS 1.4 and later
					 *
					 * case CUPS_SC_CMD_SNMP_GET:
					 * case CUPS_SC_CMD_SNMP_GET_NEXT:
					 *
					 * these values should not occur
					 * case CUPS_SC_CMD_NONE:
					 * case CUPS_SC_CMD_MAX:
					 */

					status = CUPS_SC_STATUS_NOT_IMPLEMENTED;
					datalen = 0;
					cupsSideChannelWrite(command, status, data, datalen,
					                     1.0);
					break;
				}

			}
		}
#endif
		/*
		 * Check if we have print data ready...
		 */

		if (FD_ISSET(input_fd, &input_set)) {
			if ((bytes_to_print = read(input_fd, print_buffer,
			                           sizeof(print_buffer))) < 0) {
				/*
				 * Read error - bail if we don't see EAGAIN or EINTR...
				 */

				if (errno != EAGAIN && errno != EINTR) {
					perror("ERROR: Unable to read print data");
					return -1;
				}

				bytes_to_print = 0;
			} else if (bytes_to_print == 0) {
				/*
				 * End of input file, break out of the loop
				 */

#if ROGER_BACKEND_CUPS_VERSION >= 103
				if (draining) {
					command = CUPS_SC_CMD_DRAIN_OUTPUT;
					status = CUPS_SC_STATUS_OK;
					datalen = 0;
					cupsSideChannelWrite(command, status, data, datalen,
					                     1.0);
					draining = 0;
				}
#endif

				break;
			} else {
				print_ptr = print_buffer;

				fprintf(stderr, "DEBUG: Read %d bytes_writtenof print data...\n",
				        (int) bytes_to_print);
			}
		}


		if (bytes_to_print > 0) {
			bytes_written = output_write(output_desc, print_ptr, bytes_to_print);
			if (bytes_written < 0) {
				/*
				 * Write error - bail if we don't see an error we can retry...
				 */

				if (errno != EAGAIN && errno != EINTR) {
					fprintf(stderr,
					        "ERROR: Unable to write print data: %s\n",
					        strerror(errno));
					return -1;
				}
			} else {
				bytes_to_print = bytes_to_print - bytes_written;
				total_bytes_written = total_bytes_written + bytes_written;
			}
		}
	}

	/*
	 * Return with success...
	 */

	return (total_bytes_written);
}
