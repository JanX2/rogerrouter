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

#ifndef XML_H
#define XML_H

typedef enum _XMLNodeType {
	XMLNODE_TYPE_TAG,
	XMLNODE_TYPE_ATTRIB,
	XMLNODE_TYPE_DATA
} XMLNodeType;

typedef struct xml_node {
	char *name;
	char *xml_ns;
	XMLNodeType type;
	char *data;
	size_t data_size;
	struct xml_node *parent;
	struct xml_node *child;
	struct xml_node *last_child;
	struct xml_node *next;
	char *prefix;
	GHashTable *namespace_map;
} xmlnode;

xmlnode *xmlnode_new(const char *name);
xmlnode *xmlnode_new_child(xmlnode *parent, const char *name);
xmlnode *read_xml_from_file(const char *file_name);
xmlnode *xmlnode_get_child(const xmlnode *parent, const char *name);
xmlnode *xmlnode_get_next_twin(xmlnode *node);
char *xmlnode_get_data(xmlnode *node);
const char *xmlnode_get_attrib(xmlnode *node, const char *attr);
xmlnode *xmlnode_from_str(const char *str, gssize size);
void xmlnode_insert_data(xmlnode *node, const char *data, gssize size);
void xmlnode_free(xmlnode *node);
void xmlnode_set_attrib(xmlnode *node, const char *attr, const char *value);
void xmlnode_insert_child(xmlnode *parent, xmlnode *child);
char *xmlnode_to_formatted_str(xmlnode *node, int *len);
xmlnode *xmlnode_copy(const xmlnode *node);

#endif
