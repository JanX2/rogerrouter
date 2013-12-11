
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
#include <glib.h>
#include <glib/gstdio.h>
#include <roger-backend.h>

/**
 * \brief Create and initialize a new output_descriptor
 * \return output_desc the new initialized output descriptor
 */
static output_t *new_output_desc(void)
{
	output_t *output_desc = g_malloc(sizeof(output_t));
	output_desc->output_fd = -1;
	output_desc->tmp_file_name = NULL;
	output_desc->target_file_name = NULL;
	return output_desc;
}

/**
 * \brief free out descriptor.
 * \param output_desc the output descriptor to be freed
 */
static void free_output_desc(output_t *output_desc)
{
	g_free(output_desc->tmp_file_name);
	g_free(output_desc->target_file_name);
	g_free(output_desc);
}

/**
 * \brief get path to spooler directory for specified user, create directory if it does not yet exist
 * \param user the user whose spool directory is required
 * \param uid the user-id of the user for whom the spooldir is to be looked up
 * \param gid the goup-id of the user for whom the spooldir is to be looked up
 * \return path to users spool directory
 */
static char *get_directory(char *user, int uid, int gid)
{
	/* check existance of roger spool directory */

	gchar *spool_dir_name = NULL;
	gchar *dir_name = NULL;


	/* check for the users own spool directory */

	dir_name = g_build_filename(ROGER_BACKEND_DIRECTORY, user, NULL);

	if (!g_file_test(dir_name, G_FILE_TEST_IS_DIR)) {

		/* directory does not exist create directory */

		if (g_mkdir(dir_name, 0) != 0) {

			/* Cannot create output directory, warn if parent does not exist */

			spool_dir_name = ROGER_BACKEND_DIRECTORY;

			if (!g_file_test(spool_dir_name, G_FILE_TEST_IS_DIR)) {
				fprintf(stderr, "ERROR: Spooler directory %s does not exist!\n",
				        spool_dir_name);
				g_free(dir_name);
				return NULL;
			}

			fprintf(stderr,
			        "ERROR: Cannot create output directory %s: %s\n",
			        dir_name, strerror(errno));
			g_free(dir_name);
			return NULL;
		}

		/* use chmod instead of file mode in mkdir to avoid impact of umask */
		if (g_chmod(dir_name, S_IRWXU | S_IRWXG) != 0) {
			fprintf(stderr,
			        "ERROR: Cannot set file mode for %s: %s\n",
			        dir_name, strerror(errno));
			g_rmdir(dir_name);
			g_free(dir_name);
			return NULL;
		}
		if (chown(dir_name, uid, gid) != 0) {
			fprintf(stderr,
			        "ERROR: Cannot set file owner for %s: %s\n",
			        dir_name, strerror(errno));
			g_rmdir(dir_name);
			g_free(dir_name);
			return NULL;
		}
	}
	return dir_name;
}

/*
 * \brief erorr handling for output Close and remove file
 * \param output_desc output descriptor to be cleaned up
 */
static void cleanup_output(output_t *output_desc)
{
	if (output_desc->output_fd >= 0) {
		close(output_desc->output_fd);
		g_unlink(output_desc->tmp_file_name);
	}
	free_output_desc(output_desc);
}

/*
 * \brief open fax output file
 * \copies nr of copies (unused)
 * \param job jobid
 * \param username of the user submitting the job
 * \parm title title of the printjob
 * \flags flags to be used on open call
 * \return fd of opened file
 */
output_t *open_fax_output(int copies, char *job, char *user, char *title, int flags)
{
	gchar *dir_name = NULL;
	gchar *file_name = NULL;
	output_t *output_desc = NULL;

	struct passwd *pwd_entry = getpwnam(user);
	if (pwd_entry == NULL) {
		fprintf(stderr, "ERROR: Cannot find uid/gid for user  %s\n",
		        user);
		return NULL;
	}

	dir_name = get_directory(user, pwd_entry->pw_uid, pwd_entry->pw_gid);

	if (dir_name == NULL) {
		return NULL;
	}

	output_desc = new_output_desc();
	file_name = g_strdup_printf("%s-%s", job, title);
	output_desc->target_file_name = g_build_filename(dir_name, file_name, NULL);
	g_free(file_name);
	g_free(dir_name);

	/* tmp file must be in the final directory so that a move is atomic */
	output_desc->tmp_file_name = g_strconcat(output_desc->target_file_name, ".tmp", NULL);

	output_desc->output_fd = open(output_desc->tmp_file_name, flags);
	if (output_desc->output_fd < 0) {
		fprintf(stderr, "Error: Cannot open outputfile: %s: %s\n",
		        output_desc->tmp_file_name, strerror(errno));
		cleanup_output(output_desc);
		return NULL;
	}

	if (fchmod(output_desc->output_fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {
		fprintf(stderr,
		        "ERROR: cannot set access mode for outputfile: %s: %s\n",
		        output_desc->tmp_file_name, strerror(errno));
		cleanup_output(output_desc);
		return NULL;
	}
	if (fchown(output_desc->output_fd, pwd_entry->pw_uid, pwd_entry->pw_gid) != 0) {
		fprintf(stderr,
		        "ERROR: cannot change owner of outputfile: %s: %s\n",
		        output_desc->tmp_file_name, strerror(errno));
		cleanup_output(output_desc);
		return NULL;
	}
	return output_desc;
}

/**
 * \brief write to fax  output
 * \param output_desc the output descriptor to write to
 * \param buf buffer to be written
 * \param count nuber of bytes to be written
 */

ssize_t output_write(output_t *output_desc, const void *buf, size_t count)
{
	return write(output_desc->output_fd, buf, count);
}

/**
 * \brief close the fax output file and rename it to its final name
 * \param output_desc output descriptor to be closed
 * \return status (0=ok, -1=error, errno is set)
 */
int close_fax_output(output_t *output_desc)
{
	int result;

	/* close fd */
	if ((result = close(output_desc->output_fd)) != 0) {
		fprintf(stderr, "ERROR: close outputfile %s failed: %s\n",
		        output_desc->tmp_file_name, strerror(errno));
	} else {
		/* ok, rename output file */
		if ((result = rename(output_desc->tmp_file_name, output_desc->target_file_name)) != 0) {
			fprintf(stderr, "ERROR: rename %s to %s failed: %s\n",
			        output_desc->tmp_file_name, output_desc->target_file_name, strerror(errno));
		}
	}
	free_output_desc(output_desc);
	return result;
}

/**
 * \brief get name of currently open output filename
 * \return filename
 */
char *get_output_name(output_t *output_desc)
{
	return output_desc->tmp_file_name;
}
