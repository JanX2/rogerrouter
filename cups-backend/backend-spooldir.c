
/*
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
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <assert.h>
#include <roger-backend.h>
#include <glib.h>
#include <glib/gstdio.h>

static char *tmp_file_name = NULL;
static char *final_file_name = NULL;
static int open_fd = -1;
static struct passwd *pwd_entry = NULL;

/**
 * \brief get path to spooler directory for specified user, create directory if it does not yet exist
 * \param user the user whose spool directory is required
 * \return path to users spool directory
 */
static char * get_directory ( char *user )
{
	/* check existance of roger spool directory */

	gchar *spool_dir_name = g_strdup_printf ( ROGER_SPOOLER_DIRECTORY );
	gchar *dir_name = NULL;

	if ( pwd_entry == NULL ) {
		fprintf ( stderr, _( "ERROR: Cannot find uid/gid for user  %s\n" ),
				  user );
		g_free ( spool_dir_name );
		return NULL;
	}

	if ( !g_file_test ( spool_dir_name, G_FILE_TEST_IS_DIR ) ) {
		fprintf ( stderr, _( "ERROR: Spooler directory %s does not exist!\n" ),
				  spool_dir_name );
		g_free ( spool_dir_name );
		return NULL;
	}

	/* now check for the users own spool directory */

	dir_name = g_strdup_printf ( "%s/%s", ROGER_SPOOLER_DIRECTORY, user );

	if ( !g_file_test ( dir_name, G_FILE_TEST_IS_DIR ) ) {
		if ( g_file_test ( dir_name, G_FILE_TEST_EXISTS ) ) {
			fprintf( stderr, _( "ERROR: Cannot create output directory: %s exists, but is not a directory\n"), dir_name );
			return NULL;
		}
		if ( mkdir ( dir_name, 0 ) != 0 ) {
			fprintf ( stderr,
					  _( "ERROR: Cannot create output directory %s: %s\n" ),
					  dir_name, strerror ( errno ) );
			g_free ( spool_dir_name );
			g_free ( dir_name );
			return ( NULL );
		}
		/*
		   use chmod instead of file mode in mkdir to avoid impact of umask 
		 */
		chmod ( dir_name, S_IRWXU | S_IRWXG );
		chown ( dir_name, pwd_entry->pw_uid, pwd_entry->pw_gid );
	}
	g_free ( spool_dir_name );
	return dir_name;
}

/*
 * \brief open fax output file
 * \param job jobid
 * \param user user submitting the job
 * \parm title title of the printjob
 * \flags flags to be used on open call
 * \returns fd of opened file
 */
int open_fax_output ( int copies, char *job, char *user, char *title, int flags )
{
	/* TODO: create multiple ouput files when copies > 1 */
	gchar *dir_name = NULL;

	pwd_entry = getpwnam ( user );
	dir_name = get_directory ( user );

	if ( dir_name == NULL ) {
		return ( -1 );
	}

	final_file_name = g_strdup_printf ( "%s/%s-%s", dir_name, job, title );
	tmp_file_name = g_strdup_printf ( "%s.tmp", final_file_name );
	open_fd = open ( tmp_file_name, flags );
	if ( open_fd < 0 ) {
		fprintf ( stderr, _( "Error: cannot open outputfile: %s: %s\n" ),
				  tmp_file_name, strerror ( errno ) );
		return open_fd;
	}

	if ( fchmod ( open_fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP ) != 0 ) {
		fprintf ( stderr,
				  _
				  ( "ERROR: cannot set access mode for outputfile: %s: %s\n" ),
				  tmp_file_name, strerror ( errno ) );
	}
	if ( fchown ( open_fd, pwd_entry->pw_uid, pwd_entry->pw_gid ) != 0 ) {
		fprintf ( stderr,
				  _( "ERROR: cannot change owner of outputfile: %s: %s\n" ),
				  tmp_file_name, strerror ( errno ) );
	}
	return open_fd;
}

/**
 * \brief close the fax output file and rename it to its final name
 * \param output_fd file descriptor to be closed 
 * \return status of close and rename calls
 */
int close_fax_output ( int output_fd )
{
	int result;

	assert ( output_fd == open_fd );

	/* close fd */
	if ( ( result = close ( output_fd ) ) != 0 ) {
		fprintf ( stderr, _( "ERROR: close outputfile %s failed: %s\n" ),
				  tmp_file_name, strerror ( errno ) );
	} else {
		/* ok, rename output file */
		if ( ( result = rename ( tmp_file_name, final_file_name ) ) != 0 ) {
			fprintf ( stderr, _( "ERROR: rename %s to %s failed: %s\n" ),
					  tmp_file_name, final_file_name, strerror ( errno ) );
		}
	}
	return result;
}

/**
 * \brief get name of currently open output filename
 * \return filenam
 */
char * get_output_name ( void )
{
	return tmp_file_name;
}
