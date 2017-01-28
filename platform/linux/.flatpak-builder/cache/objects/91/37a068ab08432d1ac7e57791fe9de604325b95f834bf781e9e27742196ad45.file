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

/**
 * \file xml.h
 * \brief XML node header
 */

#ifndef LIBROUTERMANAGER_XML_H
#define LIBROUTERMANAGER_XML_H

G_BEGIN_DECLS

typedef enum {
	XMLNODE_TYPE_TAG,
	XMLNODE_TYPE_ATTRIB,
	XMLNODE_TYPE_DATA
} xml_node_type;

typedef struct xml_node {
	gchar *name;
	gchar *xml_ns;
	xml_node_type type;
	gchar *data;
	size_t data_size;
	struct xml_node *parent;
	struct xml_node *child;
	struct xml_node *last_child;
	struct xml_node *next;
	gchar *prefix;
	GHashTable *namespace_map;
} xmlnode;

xmlnode *xmlnode_new(const gchar *name);
xmlnode *xmlnode_new_child(xmlnode *parent, const gchar *name);
xmlnode *read_xml_from_file(const gchar *file_name);
xmlnode *xmlnode_get_child(const xmlnode *parent, const gchar *name);
xmlnode *xmlnode_get_next_twin(xmlnode *node);
gchar *xmlnode_get_data(xmlnode *node);
const gchar *xmlnode_get_attrib(xmlnode *node, const gchar *attr);
xmlnode *xmlnode_from_str(const char *str, gssize size);
void xmlnode_insert_data(xmlnode *node, const gchar *data, gssize size);
void xmlnode_free(xmlnode *node);
void xmlnode_set_attrib(xmlnode *node, const gchar *attr, const gchar *value);
void xmlnode_insert_child(xmlnode *parent, xmlnode *child);
gchar *xmlnode_to_formatted_str(xmlnode *node, gint *len);
xmlnode *xmlnode_copy(const xmlnode *node);

G_END_DECLS

#endif
