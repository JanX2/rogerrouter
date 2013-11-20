/*
 *   Data structures and definitions for
 *   roger backend for the Common UNIX Printing System (CUPS).
 *   Copyright 2008 by Louis Lagendijk
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Louis Lagendijk and are protected by Federal copyright
 *   law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 *   "LICENSE" which should have been included with this file.  If this
 *   file is missing or damaged, see the license at "http://www.cups.org/".
 *
 *   This file is subject to the Apple OS-Developed Software exception.
 *
 * Contents:
 * <to be added>
 */
#ifndef _CUPS_ROGER_SPOOLER_H_
#define _CUPS_ROGER_SPOOLER_H_

#include <config.h>
#include <libintl.h>
#include <cups/cups.h>
#include <cups/backend.h>
#include <cups/http.h>

#define ROGER_SPOOLER_CUPS_VERSION  (100 * CUPS_VERSION_MAJOR + CUPS_VERSION_MINOR )

#if ROGER_SPOOLER_CUPS_VERSION  >= 103
#include <cups/sidechannel.h>
#endif

/* Define _ macro */
#define _(text) gettext(text)

/* 
 *  ROGER_SPOOLER definitions 
 */

#define ROGER_SPOOLER_METHOD_MAX 16      /* max length of method */
#define ROGER_SPOOLER_HOST_MAX 128       /* max length of hostname or address */
#define ROGER_SPOOLER_PORT_MAX 64        /* max length of port string */
#define ROGER_SPOOLER_USER_MAX 64        /* max length of user string */
#define ROGER_SPOOLER_ARGS_MAX 128       /* max size of argument string */
#define ROGER_SPOOLER_PRINTBUF_MAX 16536 /* max size of printbuffer */
#define ROGER_SPOOLER_DIRECTORY "/var/spool/roger"

#define USLEEP_MS 1000                  /* sleep for 1 msec */

typedef enum roger_paper_status_e
{
  ROGER_SPOOLER_PAPER_UNKNOWN = -1,
  ROGER_SPOOLER_PAPER_OK = 0,
  ROGER_SPOOLER_PAPER_OUT = 1
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

#endif /* ! CUPS_ROGER_SPOOLER_H_ */
