/**
 * Roger Router
 * Copyright (c) 2012-2014 Jan-Michael Brummer
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
 */

#ifndef EBOOK_SOURCES_H
#define EBOOK_SOURCES_H

struct ebook_data {
	char *name;
	char *id;
};

const gchar *get_selected_ebook_id(void);
EBookClient *get_selected_ebook_client(void);
GList *get_ebook_list(void);
void free_ebook_list(GList *ebook_list);

#endif
