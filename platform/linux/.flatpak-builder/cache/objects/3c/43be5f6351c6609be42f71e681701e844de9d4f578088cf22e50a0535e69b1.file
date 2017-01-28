/*
 * The rm project
 * Copyright (c) 2012-2017 Jan-Michael Brummer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __RMACTION_H__
#define __RMACTION_H__

//#if !defined (__RM_H_INSIDE__) && !defined(RM_COMPILATION)
//#error "Only <rm/rm.h> can be included directly."
//#endif

#include <glib.h>

G_BEGIN_DECLS

#include <rm/rmprofile.h>

/**
 * RM_ACTION_INCOMING_RING:
 *
 * Indicator for Incoming call rings.
 */
#define RM_ACTION_INCOMING_RING   0x01

/**
 * RM_ACTION_INCOMING_RING:
 *
 * Indicator for incoming call rings.
 */
#define RM_ACTION_INCOMING_BEGIN  0x02

/**
 * RM_ACTION_INCOMING_END:
 *
 * Indicator for incoming call ended.
 */
#define RM_ACTION_INCOMING_END    0x04

/**
 * RM_ACTION_INCOMING_MISSED:
 *
 * Indicator for incoming call missed.
 */
#define RM_ACTION_INCOMING_MISSED 0x08

/**
 * RM_ACTION_OUTGOING_DIAL:
 *
 * Indicator for outgoing call is dialing.
 */
#define RM_ACTION_OUTGOING_DIAL   0x10

/**
 * RM_ACTION_OUTGOING_BEGIN:
 *
 * Indicator for outgoing call begins.
 */
#define RM_ACTION_OUTGOING_BEGIN  0x20

/**
 * RM_ACTION_OUTGOING_END:
 *
 * Indicator for outgoing call ended.
 */
#define RM_ACTION_OUTGOING_END    0x40

/** Scheme for action */
#define RM_SCHEME_PROFILE_ACTION "org.tabos.rm.profile.action"

/**
 * RmAction:
 *
 * Keeps track of internal action information.
 */
typedef GSettings RmAction;

void rm_action_init(RmProfile *profile);
void rm_action_shutdown(RmProfile *profile);
GSList *rm_action_get_list(RmProfile *profile);
RmAction *rm_action_new(RmProfile *profile);
void rm_action_remove(RmProfile *profile, RmAction *action);
void rm_action_set_name(RmAction *action, const gchar *name);
gchar *rm_action_get_name(RmAction *action);
void rm_action_set_description(RmAction *action, const gchar *description);
gchar *rm_action_get_description(RmAction *action);
void rm_action_set_exec(RmAction *action, const gchar *exec);
gchar *rm_action_get_exec(RmAction *action);
void rm_action_set_numbers(RmAction *action, const gchar **numbers);
gchar **rm_action_get_numbers(RmAction *action);
guchar rm_action_get_flags(RmAction *action);
void rm_action_set_flags(RmAction *action, guchar flags);

G_END_DECLS

#endif
