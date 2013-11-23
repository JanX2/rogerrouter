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

#define ROGER_BACKEND_CUPS_VERSION  (100 * CUPS_VERSION_MAJOR + CUPS_VERSION_MINOR )

#if ROGER_BACKEND_CUPS_VERSION  >= 103
#include <cups/sidechannel.h>
#endif

/* Define _ macro */
#define _(text) gettext(text)

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

#define USLEEP_MS 1000                  /* sleep for 1 msec */

typedef enum roger_paper_status_e
{
  ROGER_BACKEND_PAPER_UNKNOWN = -1,
  ROGER_BACKEND_PAPER_OK = 0,
  ROGER_BACKEND_PAPER_OUT = 1
} roger_paper_status_t;


typedef enum roger_loglevel_e
{
  LOG_NONE,
  LOG_EMERG,
  LOG_ALERT,
  LOG_CRIT,
  LOG_ERROR,
  LOG_WARN,
  LOG_NOTICE,
  LOG_INFO,
  LOG_DEBUG,
  LOG_DEBUG2,
  LOG_END		/* not a real loglevel, but indicates end of list */
} roger_loglevel_t;

/*
 * roger-runloop.c
 */
extern ssize_t roger_backend_run_loop (int input_fd, int output_fd,
				    char * output_name);
/*
 * roger-utils.c
 */
int open_fax_output( int copies, char *job, char *user, char *title, int flags );
int close_fax_output( int outputFd );
char * get_output_name( void );

#ifndef CUPS_LLCAST
#  define CUPS_LLCAST	(long)
#endif

#endif /* ! CUPS_ROGER_BACKEND_H_ */
