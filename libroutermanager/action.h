/**
 * The libroutermanager project
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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

#ifndef LIBROUTERMANAGER_ACTION_H
#define LIBROUTERMANAGER_ACTION_H

#include <glib.h>

G_BEGIN_DECLS

#include <libroutermanager/profile.h>

/** ACTION_XXXX_YYYY flags */
#define ACTION_INCOMING_RING   0x01
#define ACTION_INCOMING_BEGIN  0x02
#define ACTION_INCOMING_END    0x04
#define ACTION_INCOMING_MISSED 0x08
#define ACTION_OUTGOING_DIAL   0x10
#define ACTION_OUTGOING_BEGIN  0x20
#define ACTION_OUTGOING_END    0x40

/** action structure */
struct action {
	/** name */
	gchar *name;
	/** description */
	gchar *description;
	/** execution line */
	gchar *exec;
	/** flags - contains ACTION_XXXX_YYYY values */
	guchar flags;
	/** list of numbers used within this action */
	gchar **numbers;

	/** settings storage of this action */
	GSettings *settings;
};

void action_init(struct profile *profile);
void action_shutdown(struct profile *profile);
GSList *action_get_list(struct profile *profile);
void action_remove(struct profile *profile, struct action *action);
void action_add(struct profile *profile, struct action *action);
void action_commit(struct profile *profile);
void action_free(gpointer data);
struct action *action_create(void);
struct action *action_modify(struct action *action, const gchar *name, const gchar *description, const gchar *exec, gchar **numbers);

G_END_DECLS

#endif
