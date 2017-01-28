/*
 * capi_debug.h
 *
 * Author       Karsten Keil <kkeil@linux-pingi.de>
 *
 * Copyright 2011  by Karsten Keil <kkeil@linux-pingi.de>
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef _CAPI_DEBUG_H
#define _CAPI_DEBUG_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*capi_debug_t)(const char *file, int line, const char *func, const char *fmt, va_list va);

extern int _capi_dprintf(const char *, int, const char *, const char *, ...);

#define capi_dprintf(f, ...)	_capi_dprintf(__FILE__, __LINE__, __PRETTY_FUNCTION__, f, ##__VA_ARGS__)

extern void register_dbg_vprintf(capi_debug_t);

#ifdef __cplusplus
}
#endif

#endif
