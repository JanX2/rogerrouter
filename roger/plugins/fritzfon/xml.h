/**
 * Roger Router
 * Copyright (c) 2012-2013 Jan-Michael Brummer
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

typedef struct sXmlNode {
	char *pnName;
	char *pnXmlNs;
	XMLNodeType nType;
	char *pnData;
	size_t nDataSize;
	struct sXmlNode *psParent;
	struct sXmlNode *psChild;
	struct sXmlNode *psLastChild;
	struct sXmlNode *psNext;
	char *pnPrefix;
	GHashTable *psNameSpaceMap;
} xmlnode;

xmlnode *xmlnode_new( const char *pnName );
xmlnode *xmlnode_new_child( xmlnode *psParent, const char *pnName );
xmlnode *readXmlFromFile( const char *pnFileName, const char *pnDescription );
xmlnode *xmlnode_get_child(const xmlnode *parent, const char *name);
xmlnode *xmlnode_get_next_twin(xmlnode *node);
char *xmlnode_get_data(xmlnode *node);
const char *xmlnode_get_attrib( xmlnode *psNode, const char *pnAttr );
xmlnode *xmlnode_from_str( const char *pnStr, gssize nSize);
void xmlnode_insert_data( xmlnode *psNode, const char *pnData, gssize nSize );

#endif
