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

#include "roger-backend.h"
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


/*
 * \brief Send a file to the roger fax spool directory
 * \argc number of argments
 * \argv argements
 * \return error code if any
 *
 *  Usage: printer-uri job-id user title copies options [file]
 */

int main ( int argc, char *argv[] )
{
	char method[ROGER_BACKEND_METHOD_MAX];
	char hostname[ROGER_BACKEND_HOST_MAX];
	char username[ROGER_BACKEND_USER_MAX];
	char resource[ROGER_BACKEND_ARGS_MAX];
	int port;
	char *options;
	// char *option_name;
	// char *option_value;
	// char sep;
	int input_fd;
	int output_fd;
	int copies;
	ssize_t bytes_written;

#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action;	/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */


	/*
	 * Make sure status messages are not buffered...
	 */

	setbuf ( stderr, NULL );

	/*
	 * Ignore SIGPIPE signals...
	 */

#ifdef HAVE_SIGSET
	sigset ( SIGPIPE, SIG_IGN );
#elif defined(HAVE_SIGACTION)
	memset ( &action, 0, sizeof ( action ) );
	action.sa_handler = SIG_IGN;
	sigaction ( SIGPIPE, &action, NULL );
#else
	signal ( SIGPIPE, SIG_IGN );
#endif /* HAVE_SIGSET */


	/*
	 * Check command-line...
	 */

	if ( argc == 1 ) {
		puts ( 
			/* device-class device-uril */
			/* TODO: use command line/device-uri? */
			"file roger-cups:/ "
			/* device make and model */
			"\"Roger-Router Fax\" "
			/* device-info */
			"\"Roger Router Fax Fritz!Box\" "
			/* device-id */
			"\"MFG:Roger-Router;MDL:Roger-fax;"
			"DES:Roger-Router Fax Printer for Fritz!Box routers;"
			"CLS:PRINTER;CMD:POSTSCRIPT;\"" );
		return ( CUPS_BACKEND_OK );
	} else if ( argc < 6 || argc > 7 ) {
		fprintf ( stderr, _("roger-fax backend - version %s\n") , VERSION );
		fprintf( stderr, _ ( "Usage: %s job-id user title copies options [file]\n" ),
						  argv[0] );
		return ( CUPS_BACKEND_FAILED );
	}

	/*
	 * If we have 7 arguments, print the file named on the command-line.
	 * Otherwise, send stdin (fd = 0 )instead...
	 */

	if ( argc == 6 ) {
		input_fd = 0;
		copies = 1;
	} else {
		/*
		 * Try to open the print file...
		 */

		if ( ( input_fd = open ( argv[6], O_RDONLY ) ) < 0 ) {
			perror ( "ERROR: unable to open print file" );
			return ( CUPS_BACKEND_FAILED );
		}
		copies = atoi ( argv[4] );
	}

	/* This is left in for when we add (debug) options */

	httpSeparateURI ( HTTP_URI_CODING_ALL, cupsBackendDeviceURI ( argv ),
					  method, sizeof ( method ), username, sizeof ( username ),
					  hostname, sizeof ( hostname ), &port,
					  resource, sizeof ( resource ) );

	/*
	 * Get options, if any...
	 */

	if ( ( options = strchr ( resource, '?' ) ) != NULL ) {
		/*
		 * terminate the device name string and move to the first
		 * character of the options...
		 */

		*options++ = '\0';

		/*
		 * Parse options...
		 */

		/* We do not have options right now... 

		while ( *options ) {

			option_name = options;

			while ( *options && *options != '=' && *options != '+'
					&& *options != '&' )
				options++;

			if ( ( sep = *options ) != '\0' )
				*options++ = '\0';

			if ( sep == '=' ) {

				option_value = options;

				while ( *options && *options != '+' && *options != '&' )
					options++;

				if ( *options )
					*options++ = '\0';
			} else
				option_value = ( char * ) "";

		}
		*/
	}

	/*
	 * Create output file
	 */

	/*
	   outputname inludes job-id, username and original name 
	 */
	output_fd = open_fax_output ( copies, argv[1], argv[2], argv[3], O_WRONLY | O_CREAT);

	if ( output_fd < 0 ) {
		/* TODO: improve backend return value setting? */
		return ( CUPS_BACKEND_STOP );
	}

	fprintf( stderr, _( "INFO: Creating file %s\n" ),
					  get_output_name() );


	fprintf( stderr, _( "INFO: Connected to %s...\n" ), get_output_name() );

	/*
	 * Print everything...
	 */

	bytes_written = 0;

	while ( copies > 0 && bytes_written >= 0 ) {
		copies--;
	}

	if ( input_fd != 0 ) {
		fputs ( "PAGE: 1 1\n", stderr);
		lseek ( input_fd, 0, SEEK_SET );
	}

	bytes_written = roger_backend_run_loop ( input_fd, output_fd, get_output_name() );

	if ( input_fd != 0 && bytes_written >= 0 ) {
#ifdef HAVE_LONG_LONG
		fprinf( stderr,
						  _
						  ( "INFO: Sent print file, %lld bytes...\n" ),
						  CUPS_LLCAST bytes_written );
#else
		fprintf( stderr,
						  _( "INFO: Sent print file, %ld bytes...\n" ),
						  CUPS_LLCAST bytes_written );
#endif /* HAVE_LONG_LONG */
	}

	/*
	 * Close the socket connection...
	 */

	close_fax_output ( output_fd );

	/*
	 * Close the input file and return...
	 */

	if ( input_fd != 0 ) {
		close ( input_fd );
	}

	if ( bytes_written >= 0 ) {
		fputs ( _( "INFO: Ready to print.\n" ), stderr );
	}
	return ( bytes_written < 0 ? CUPS_BACKEND_FAILED : CUPS_BACKEND_OK );
}
