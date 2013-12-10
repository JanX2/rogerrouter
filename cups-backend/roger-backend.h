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

#ifndef _CUPS_ROGER_BACKEND_H_
#define _CUPS_ROGER_BACKEND_H_

#include <config.h>
#include <libintl.h>
#include <cups/cups.h>
#include <cups/backend.h>
#include <cups/http.h>
#include <glib.h>

#define ROGER_BACKEND_CUPS_VERSION  (100 * CUPS_VERSION_MAJOR + CUPS_VERSION_MINOR )

#if ROGER_BACKEND_CUPS_VERSION  >= 103
#include <cups/sidechannel.h>
#endif

/*
 *  ROGER_BACKEND definitions
 */

#define ROGER_BACKEND_METHOD_MAX 16      /* max length of method */
#define ROGER_BACKEND_HOST_MAX 128       /* max length of hostname or address */
#define ROGER_BACKEND_PORT_MAX 64        /* max length of port string */
#define ROGER_BACKEND_USER_MAX 64        /* max length of user string */
#define ROGER_BACKEND_ARGS_MAX 128       /* max size of argument string */
#define ROGER_BACKEND_PRINTBUF_MAX 16536 /* max size of printbuffer */
#define ROGER_BACKEND_DIRECTORY "/var/spool/roger"
#define ROGER_BACKEND_DEVICE_ID "MFG:Roger-Router;MDL:Roger-fax;" \
                    "DES:Roger-Router Fax Printer for Fritz!Box routers;" \
                    "CLS:PRINTER;CMD:POSTSCRIPT;"
#define USLEEP_MS 1000                  /* sleep for 1 msec */

typedef struct output_struct {
	gchar *tmp_file_name;
	gchar *target_file_name;
	gint output_fd;
} output_t;

/*
 * roger-runloop.c
 */
extern ssize_t roger_backend_run_loop(int input_fd, output_t *output_desc,
                                      char *output_name);
/*
 * roger-spooldir.c
 */
output_t *open_fax_output(int copies, char *job, char *user, char *title, int flags);
int close_fax_output(output_t *output_desc);
ssize_t output_write(output_t *output_desc, const void *buf, size_t nbyte);
char *get_output_name(output_t *output_desc);

#ifndef CUPS_LLCAST
#  define CUPS_LLCAST	(long)
#endif

#endif /* ! CUPS_ROGER_BACKEND_H_ */
