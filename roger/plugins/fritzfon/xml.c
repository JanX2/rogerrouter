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
 * \file xml.c
 * \brief XML config file parsing
 */

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <libxml/parser.h>

#include "xml.h"

/**
 * \brief Create new xml node
 * \param pnName node name
 * \param nType node type
 * \return new node pointer
 */
xmlnode *new_node( const char *pnName, XMLNodeType nType ) {
	xmlnode *psNode = g_new0( xmlnode, 1 );

	psNode -> pnName = g_strdup( pnName );
	psNode -> nType = nType;

	return psNode;
}

/**
 * \brief Create a new tag node
 * \param pnName node name
 * \return new node pointer or NULL on error
 */
xmlnode *xmlnode_new( const char *pnName ) {
	g_return_val_if_fail( pnName != NULL, NULL );

	return new_node( pnName, XMLNODE_TYPE_TAG );
}

/**
 * \brief Insert child into parent node
 * \param psParent parent node
 * \param psChild child node
 */
void xmlnode_insert_child( xmlnode *psParent, xmlnode *psChild ) {
	g_return_if_fail( psParent != NULL );
	g_return_if_fail( psChild != NULL );

	psChild -> psParent = psParent;

	if ( psParent -> psLastChild ) {
		psParent -> psLastChild -> psNext = psChild;
	} else {
		psParent -> psChild = psChild;
	}

	psParent -> psLastChild = psChild;
}

/**
 * \brief Create new child node
 * \param psParent parent node
 * \param pnName node name
 * \return new node pointer or NULL on error
 */
xmlnode *xmlnode_new_child( xmlnode *psParent, const char *pnName ) {
	xmlnode *psNode;

	g_return_val_if_fail( psParent != NULL, NULL );
	g_return_val_if_fail( pnName != NULL, NULL );

	psNode = new_node( pnName, XMLNODE_TYPE_TAG );

	xmlnode_insert_child( psParent, psNode );

	return psNode;
}

/**
 * \brief Free node
 * \param psNode node to free
 */
void xmlnode_free( xmlnode *psNode ) {
	xmlnode *psX, *psY;

	g_return_if_fail( psNode != NULL );

	if ( psNode -> psParent != NULL ) {
		if ( psNode -> psParent -> psChild == psNode ) {
			psNode -> psParent -> psChild = psNode -> psNext;
			if ( psNode -> psParent -> psLastChild == psNode ) {
				psNode -> psParent -> psLastChild = psNode -> psNext;
			}
		} else {
			xmlnode *psPrev = psNode -> psParent -> psChild;
			while ( psPrev && psPrev -> psNext != psNode ) {
				psPrev = psPrev -> psNext;
			}

			if ( psPrev ) {
				psPrev -> psNext = psNode -> psNext;
				if ( psNode -> psParent -> psLastChild == psNode ) {
					psNode -> psParent -> psLastChild = psPrev;
				}
			}
		}
	}

	psX = psNode -> psChild;
	while ( psX ) {
		psY = psX -> psNext;
		xmlnode_free( psX );
		psX = psY;
	}

	g_free( psNode -> pnName );
	g_free( psNode -> pnData );
	g_free( psNode -> pnXmlNs );
	g_free( psNode -> pnPrefix );

	if ( psNode -> psNameSpaceMap ) {
		g_hash_table_destroy( psNode -> psNameSpaceMap );
	}

	g_free( psNode );
}

/**
 * \brief Remove attribute from node
 * \param psNode node pointer
 * \param pnAttr attribute name
 */
static void xmlnode_remove_attrib( xmlnode *psNode, const char *pnAttr ) {
	xmlnode *psAttrNode, *psSibling = NULL;

	g_return_if_fail( psNode != NULL );
	g_return_if_fail( pnAttr != NULL );

	for ( psAttrNode = psNode -> psChild; psAttrNode != NULL; psAttrNode = psAttrNode -> psNext ) {
		if ( psAttrNode -> nType == XMLNODE_TYPE_ATTRIB && !strcmp( psAttrNode -> pnName, pnAttr ) ) {
			if ( psSibling == NULL ) {
				psNode -> psChild = psAttrNode -> psNext;
			} else {
				psSibling -> psNext = psAttrNode -> psNext;
			}
			if ( psNode -> psLastChild == psAttrNode ) {
				psNode -> psLastChild = psSibling;
			}
			xmlnode_free( psAttrNode );
			return;
		}
		psSibling = psAttrNode;
	}
}

/**
 * \brief Set attribute for node
 * \param psNode node pointer
 * \param pnAttr attribute name
 * \param pnValue value to set
 */
void xmlnode_set_attrib( xmlnode *psNode, const char *pnAttr, const char *pnValue ) {
	xmlnode *psAttribNode;

	g_return_if_fail( psNode != NULL );
	g_return_if_fail( pnAttr != NULL );
	g_return_if_fail( pnValue != NULL );

	xmlnode_remove_attrib( psNode, pnAttr );

	psAttribNode = new_node( pnAttr, XMLNODE_TYPE_ATTRIB );

	psAttribNode -> pnData = g_strdup( pnValue );

	xmlnode_insert_child( psNode, psAttribNode );
}

/**
 * \brief Get node prefix
 * \param psNode node pointer
 * \return node prefix
 */
static const char *xmlnode_get_prefix( xmlnode *psNode ) {
	g_return_val_if_fail( psNode != NULL, NULL );
	return psNode -> pnPrefix;
}

/**
 * \brief Convert node to string
 * \param pnKey key name
 * \param pnValue value
 * \param psBuf buffer to print to
 */
static void xmlnode_to_str_foreach_append_ns( const char *pnKey, const char *pnValue, GString *psBuf ) {
	if ( *pnKey ) {
		g_string_append_printf( psBuf, " xmlns:%s='%s'", pnKey, pnValue );
	} else {
		g_string_append_printf( psBuf, " xmlns='%s'", pnValue );
	}
}

/**
 * \brief Helps with converting node to string
 * \param psNode node to convert
 * \param pnLen pointer for len saving
 * \param bFormatting format text?
 * \param nDepth depth
 * \return string data or NULL on error
 */
static char *xmlnode_to_str_helper( xmlnode *psNode, int *pnLen, gboolean bFormatting, int nDepth ) {
	GString *psText = g_string_new( "" );
	const char *pnPrefix;
	xmlnode *psC;
	char *pnNodeName, *pnEsc, *pnEsc2, *pnTab = NULL;
	gboolean bNeedEnd = FALSE, bPretty = bFormatting;

	g_return_val_if_fail( psNode != NULL, NULL );

	if ( bPretty && nDepth ) {
		pnTab = g_strnfill( nDepth, '\t' );
		psText = g_string_append( psText, pnTab );
	}

	pnNodeName = g_markup_escape_text( psNode -> pnName, -1 );
	pnPrefix = xmlnode_get_prefix( psNode );

	if ( pnPrefix ) {
		g_string_append_printf( psText, "<%s:%s", pnPrefix, pnNodeName );
	} else {
		g_string_append_printf( psText, "<%s", pnNodeName );
	}

	if ( psNode -> psNameSpaceMap ) {
		g_hash_table_foreach( psNode -> psNameSpaceMap, ( GHFunc ) xmlnode_to_str_foreach_append_ns, psText );
	} else if ( psNode -> pnXmlNs ) {
		if ( !psNode -> psParent || !psNode -> psParent -> pnXmlNs || strcmp( psNode -> pnXmlNs, psNode -> psParent -> pnXmlNs ) ) {
			char *pnXmlNs = g_markup_escape_text( psNode -> pnXmlNs, -1 );
			g_string_append_printf( psText, " xmlns='%s'", pnXmlNs );
			g_free( pnXmlNs );
		}
	}

	for ( psC = psNode -> psChild; psC != NULL; psC = psC -> psNext ) {
		if ( psC -> nType == XMLNODE_TYPE_ATTRIB ) {
			const char *pnAprefix = xmlnode_get_prefix( psC );
			pnEsc = g_markup_escape_text( psC -> pnName, -1 );
			pnEsc2 = g_markup_escape_text( psC -> pnData, -1 );

			if ( pnAprefix ) {
				g_string_append_printf( psText, " %s:%s='%s'", pnAprefix, pnEsc, pnEsc2 );
			} else {
				g_string_append_printf( psText, " %s='%s'", pnEsc, pnEsc2 );
			}
			g_free( pnEsc );
			g_free( pnEsc2 );
		} else if ( psC -> nType == XMLNODE_TYPE_TAG || psC -> nType == XMLNODE_TYPE_DATA ) {
			if ( psC -> nType == XMLNODE_TYPE_DATA ) {
				bPretty = FALSE;
			}
			bNeedEnd = TRUE;
		}
	}

	if ( bNeedEnd ) {
		g_string_append_printf( psText, ">%s", bPretty ? "\n" : "" );

		for ( psC = psNode -> psChild; psC != NULL; psC = psC -> psNext ) {
			if ( psC -> nType == XMLNODE_TYPE_TAG ) {
				int nEscLen;
				pnEsc = xmlnode_to_str_helper( psC, &nEscLen, bPretty, nDepth + 1 );
				psText = g_string_append_len( psText, pnEsc, nEscLen );
				g_free( pnEsc );
			} else if ( psC -> nType == XMLNODE_TYPE_DATA && psC -> nDataSize > 0 ) {
				pnEsc = g_markup_escape_text( psC -> pnData, psC -> nDataSize );
				psText = g_string_append( psText, pnEsc );
				g_free( pnEsc );
			}
		}

		if ( pnTab && bPretty ) {
			psText = g_string_append( psText, pnTab );
		}
		if ( pnPrefix ) {
			g_string_append_printf( psText, "</%s:%s>%s", pnPrefix, pnNodeName, bFormatting ? "\n" : "" );
		} else {
			g_string_append_printf( psText, "</%s>%s", pnNodeName, bFormatting ? "\n" : "" );
		}
	} else {
		g_string_append_printf( psText, "/>%s", bFormatting ? "\n" : "" );
	}

	g_free( pnNodeName );

	g_free( pnTab );

	if ( pnLen ) {
		*pnLen = psText -> len;
	}

	return g_string_free( psText, FALSE );
}

/**
 * \brief Convet node to formatted string
 * \param psNode node
 * \param pnLen pointer to len
 * \return formatted string or NULL on error
 */
char *xmlnode_to_formatted_str( xmlnode *psNode, int *pnLen ) {
	char *pnXml, *pnXmlWithDeclaration;

	g_return_val_if_fail( psNode != NULL, NULL );

	pnXml = xmlnode_to_str_helper( psNode, pnLen, TRUE, 0 );
	pnXmlWithDeclaration = g_strdup_printf( "<?xml version='1.0' encoding='UTF-8' ?>\n\n%s", pnXml );
	g_free( pnXml );

	if ( pnLen ) {
		*pnLen += sizeof( "<?xml version='1.0' encoding='UTF-8' ?>\n\n" ) - 1;
	}

	return pnXmlWithDeclaration;
}

/** xmlnode parser data structure */
struct _xmlnode_parser_data {
	xmlnode *current;
	gboolean error;
};

/**
 * \brief Set namespace
 * \param psNode xml node
 * \param pnXmlNs xml namespace
 */
void xmlnode_set_namespace( xmlnode *psNode, const char *pnXmlNs ) {
	g_return_if_fail( psNode != NULL );

	g_free( psNode -> pnXmlNs );
	psNode -> pnXmlNs = g_strdup( pnXmlNs );
}

/**
 * \brief Set prefix
 * \param psNode xml node
 * \param pnPrefix prefix
 */
static void xmlnode_set_prefix( xmlnode *psNode, const char *pnPrefix ) {
	g_return_if_fail( psNode != NULL );

	g_free( psNode -> pnPrefix );
	psNode -> pnPrefix = g_strdup( pnPrefix );
}

/**
 * \brief Insert data into xmlnode
 * \param psNode xml node
 * \param pnData data pointer
 * \param nSize size of data
 */
void xmlnode_insert_data( xmlnode *psNode, const char *pnData, gssize nSize ) {
	xmlnode *psChild;
	gsize nRealSize;

	g_return_if_fail( psNode != NULL );
	g_return_if_fail( pnData != NULL );
	g_return_if_fail( nSize != 0 );

	if ( nSize == -1 ) {
		nRealSize = strlen( pnData );
	} else {
 		nRealSize = nSize;
	}

	psChild = new_node( NULL, XMLNODE_TYPE_DATA );

	psChild -> pnData = g_memdup( pnData, nRealSize );
	psChild -> nDataSize = nRealSize;

	xmlnode_insert_child( psNode, psChild );
}

/**
 * \brief Set attribute with prefix
 * \param psNode xml node
 * \param pnAttr attribute
 * \param pnPrefix prefix
 * \param pnValue value
 */
static void xmlnode_set_attrib_with_prefix( xmlnode *psNode, const char *pnAttr, const char *pnPrefix, const char *pnValue ) {
	xmlnode *psAttribNode;

	g_return_if_fail( psNode != NULL );
	g_return_if_fail( pnAttr != NULL );
	g_return_if_fail( pnValue != NULL );

	psAttribNode = new_node( pnAttr, XMLNODE_TYPE_ATTRIB );

	psAttribNode -> pnData = g_strdup( pnValue );
	psAttribNode -> pnPrefix = g_strdup( pnPrefix );

	xmlnode_insert_child( psNode, psAttribNode );
}

/**
 * \brief Get attribute from node
 * \param psNode xml node structure
 * \param pnAttr attribute name
 * \return attribute data
 */
const char *xmlnode_get_attrib( xmlnode *psNode, const char *pnAttr ) {
	xmlnode *psX;

	g_return_val_if_fail( psNode != NULL, NULL );
	g_return_val_if_fail( pnAttr != NULL, NULL );

	for ( psX = psNode -> psChild; psX != NULL; psX = psX -> psNext ) {
		if ( psX -> nType == XMLNODE_TYPE_ATTRIB && strcmp( pnAttr, psX -> pnName ) == 0 ) {
			return psX -> pnData;
		}
	}

	return NULL;
}

/**
 * \brief Unescape html text
 * \param pnHtml html text
 * \return unescaped text
 */
static char *unescape_html( const char *pnHtml ) {
	if ( pnHtml != NULL ) {
		const char *pnC = pnHtml;
		GString *psRet = g_string_new( "" );
		while ( *pnC ) {
			if ( !strncmp( pnC, "<br>", 4 ) ) {
				psRet = g_string_append_c( psRet, '\n' );
				pnC += 4;
			} else {
				psRet = g_string_append_c( psRet, *pnC );
				pnC++;
			}
		}

		return g_string_free( psRet, FALSE );
	}

	return NULL;
}

/**
 * \brief Parser: Element start
 * \param pUserData xmlnode parser data
 * \param psElementName element name
 * \param psPrefix prefix
 * \param psXmlNs xml namespace
 * \param nNbNamespaces number of namespaces
 * \param ppsNamespaces pointer to xml namespaces
 * \param nNbAttributes number of attributes
 * \param nNbDefaulted number of defaulted
 * \param ppsAttributes pointer to xml attributes
 */
static void xmlnode_parser_element_start_libxml( void *pUserData, const xmlChar *psElementName, const xmlChar *psPrefix,
		const xmlChar *psXmlNs, int nNbNamespaces, const xmlChar **ppsNamespaces, int nNbAttributes, int nNbDefaulted,
		const xmlChar **ppsAttributes ) {
	struct _xmlnode_parser_data *psXpd = pUserData;
	xmlnode *psNode;
	int nI, nJ;

	if( !psElementName || psXpd -> error ) {
		return;
	} else {
		if ( psXpd -> current ) {
			psNode = xmlnode_new_child( psXpd -> current, ( const char * ) psElementName );
		} else {
			psNode = xmlnode_new( ( const char * ) psElementName );
		}

		xmlnode_set_namespace( psNode, ( const char * ) psXmlNs );
		xmlnode_set_prefix( psNode, ( const char * ) psPrefix );

		if ( nNbNamespaces != 0 ) {
			psNode -> psNameSpaceMap = g_hash_table_new_full( g_str_hash, g_str_equal, g_free, g_free );

			for ( nI = 0, nJ = 0; nI < nNbNamespaces; nI++, nJ += 2 ) {
				const char *pnKey = ( const char * ) ppsNamespaces[ nJ ];
				const char *pnVal = ( const char * ) ppsNamespaces[ nJ + 1 ];
				g_hash_table_insert( psNode -> psNameSpaceMap, g_strdup( pnKey ? pnKey : "" ), g_strdup( pnVal ? pnVal : "" ) );
			}
		}

		for ( nI = 0; nI < nNbAttributes * 5; nI += 5 ) {
			const char *pnPrefix = ( const char * ) ppsAttributes[ nI + 1 ];
			char *pnTxt;
			int nAttribLen = ppsAttributes[ nI + 4 ] - ppsAttributes[ nI + 3 ];
			char *pnAttrib = g_malloc( nAttribLen + 1 );

			memcpy( pnAttrib, ppsAttributes[ nI + 3 ], nAttribLen );
			pnAttrib[ nAttribLen ] = '\0';
			pnTxt = pnAttrib;
			pnAttrib = unescape_html( pnTxt );
			g_free( pnTxt );

			if ( psPrefix && *psPrefix ) {
				xmlnode_set_attrib_with_prefix( psNode, ( const char * ) ppsAttributes[ nI ], pnPrefix, pnAttrib );
			} else {
				xmlnode_set_attrib( psNode, ( const char * ) ppsAttributes[ nI ], pnAttrib );
			}
			g_free( pnAttrib );
		}

		psXpd -> current = psNode;
	}
}

/**
 * \brief Parser: Element end
 * \param pUserData xmlnode parser data
 * \param psElementName element name
 * \param psPrefix prefix
 * \param psXmlNs xml namespace
 */
static void xmlnode_parser_element_end_libxml( void *pUserData, const xmlChar *psElementName, const xmlChar *psPrefix, const xmlChar *psXmlNs ) {
	struct _xmlnode_parser_data *psXpd = pUserData;

	if ( !psElementName || !psXpd -> current || psXpd -> error ) {
		return;
	}

	if ( psXpd -> current -> psParent ) {
		if ( !xmlStrcmp( ( xmlChar * ) psXpd -> current -> pnName, psElementName ) ) {
			psXpd -> current = psXpd -> current -> psParent;
		}
	}
}

/**
 * \brief Parser: Element text
 * \param pUserData xmlnode parser data
 * \param pnText text element
 * \param nTextLen text length
 */
static void xmlnode_parser_element_text_libxml( void *pUserData, const xmlChar *pnText, int nTextLen ) {
	struct _xmlnode_parser_data *psXpd = pUserData;

	if ( !psXpd -> current || psXpd -> error ) {
		return;
	}

	if ( !pnText || !nTextLen ) {
		return;
	}

	xmlnode_insert_data( psXpd -> current, ( const char * ) pnText, nTextLen );
}

/**
 * \brief Parser error
 * \param pUserData xmlnode parser data
 * \param pnMsg error message
 */
static void xmlnode_parser_error_libxml( void *pUserData, const char *pnMsg, ...) {
	struct _xmlnode_parser_data *psXpd = pUserData;
	char anErrMsg[ 2048 ];
	va_list args;

	psXpd -> error = TRUE;

	va_start( args, pnMsg );
	vsnprintf( anErrMsg, sizeof( anErrMsg ), pnMsg, args );
	va_end( args );

	g_debug("Error parsing xml file: %s", anErrMsg );
}

/** xmlnode parser libxml */
static xmlSAXHandler sXmlnodeParserLibxml = {
	/* internalSubset */
	NULL,
	/* isStandalone */
	NULL,
	/* hasInternalSubset */
	NULL,
	/* hasExternalSubset */
	NULL,
	/* resolveEntity */
	NULL,
	/* getEntity */
	NULL,
	/* entityDecl */
	NULL,
	/* notationDecl */
	NULL,
	/* attributeDecl */
	NULL,
	/* elementDecl */
	NULL,
	/* unparsedEntityDecl */
	NULL,
	/* setDocumentLocator */
	NULL,
	/* startDocument */
	NULL,
	/* endDocument */
	NULL,
	/* startElement */
	NULL,
	/* endElement */
	NULL,
	/* reference */
	NULL,
	/* characters */
	xmlnode_parser_element_text_libxml,
	/* ignorableWhitespace */
	NULL,
	/* processingInstruction */
	NULL,
	/* comment */
	NULL,
	/* warning */
	NULL,
	/* error */
	xmlnode_parser_error_libxml,
	/* fatalError */
	NULL,
	/* getParameterEntity */
	NULL,
	/* cdataBlock */
	NULL,
	/* externalSubset */
	NULL,
	/* initialized */
	XML_SAX2_MAGIC,
	/* _private */
	NULL,
	/* startElementNs */
	xmlnode_parser_element_start_libxml,
	/* endElementNs */
	xmlnode_parser_element_end_libxml,
	/* serror */
	NULL,
};

/**
 * \brief Create xmlnode from string
 * \param pnStr string
 * \param nSize size of string
 * \return new xml node
 */
xmlnode *xmlnode_from_str( const char *pnStr, gssize nSize ) {
	struct _xmlnode_parser_data *psXpd;
	xmlnode *psRet;
	gsize nRealSize;

	g_return_val_if_fail( pnStr != NULL, NULL );

	nRealSize = nSize < 0 ? strlen( pnStr ) : nSize;
	psXpd = g_new0( struct _xmlnode_parser_data, 1 );

	if ( xmlSAXUserParseMemory( &sXmlnodeParserLibxml, psXpd, pnStr, nRealSize) < 0 ) {
		while ( psXpd -> current && psXpd -> current -> psParent ) {
			psXpd -> current = psXpd -> current -> psParent;
		}

		if ( psXpd -> current ) {
			xmlnode_free( psXpd -> current );
		}
		psXpd -> current = NULL;
	}
	psRet = psXpd -> current;

	if ( psXpd -> error ) {
		psRet = NULL;

		if ( psXpd -> current ) {
			xmlnode_free( psXpd -> current );
		}
	}

	g_free( psXpd );

	return psRet;
}

/**
 * \brief Get namespace
 * \param psNode xml node
 * \return namespace
 */
const char *xmlnode_get_namespace( xmlnode *psNode ) {
	g_return_val_if_fail( psNode != NULL, NULL );

	return psNode -> pnXmlNs;
}

/**
 * \brief Get child with namespace
 * \param psParent parent xml node
 * \param pnName child name
 * \param pnNs namespace
 * \return chuld xmlnode
 */
xmlnode *xmlnode_get_child_with_namespace( const xmlnode *psParent, const char *pnName, const char *pnNs ) {
	xmlnode *psX, *psRet = NULL;
	char **ppnNames;
	char *pnParentName, *pnChildName;

	g_return_val_if_fail( psParent != NULL, NULL );
	g_return_val_if_fail( pnName != NULL, NULL );

	ppnNames = g_strsplit( pnName, "/", 2 );
	pnParentName = ppnNames[ 0 ];
	pnChildName = ppnNames[ 1 ];

	for ( psX = psParent -> psChild; psX; psX = psX -> psNext ) {
		const char *pnXmlNs = NULL;

		if ( pnNs != NULL ) {
			pnXmlNs = xmlnode_get_namespace( psX );
		}

		if ( psX -> nType == XMLNODE_TYPE_TAG && pnName && !strcmp( pnParentName, psX -> pnName ) && ( !pnNs || ( pnXmlNs && !strcmp( pnNs, pnXmlNs ) ) ) ) {
			psRet = psX;
			break;
		}
	}

	if ( pnChildName && psRet ) {
		psRet = xmlnode_get_child( psRet, pnChildName );
	}

	g_strfreev( ppnNames );

	return psRet;
}

/**
 * \brief Get xml node child
 * \param psParent xml node parent
 * \param pnName child name
 * \return child xmlnode
 */
xmlnode *xmlnode_get_child( const xmlnode *psParent, const char *pnName ) {
	return xmlnode_get_child_with_namespace( psParent, pnName, NULL );
}

/**
 * \brief Get next twin from xml node
 * \param psNode xml node
 * \return next xml node twin
 */
xmlnode *xmlnode_get_next_twin( xmlnode *psNode ) {
	xmlnode *psSibling;
	const char *pnNs = xmlnode_get_namespace( psNode );

	g_return_val_if_fail( psNode != NULL, NULL );
	g_return_val_if_fail( psNode -> nType == XMLNODE_TYPE_TAG, NULL );

	for ( psSibling = psNode -> psNext; psSibling; psSibling = psSibling -> psNext ) {
		const char *pnXmlNs = NULL;

		if ( pnNs != NULL ) {
			pnXmlNs = xmlnode_get_namespace( psSibling );
		}

		if ( psSibling -> nType == XMLNODE_TYPE_TAG && !strcmp( psNode -> pnName, psSibling -> pnName ) && ( !pnNs || ( pnXmlNs && !strcmp( pnNs, pnXmlNs ) ) ) ) {
				return psSibling;
		}
	}

	return NULL;
}

/**
 * \brief Get data from xmlnode
 * \param psNode xml node
 * \return xmlnode data
 */
char *xmlnode_get_data( xmlnode *psNode ) {
	GString *psStr = NULL;
	xmlnode *psC;

	g_return_val_if_fail( psNode != NULL, NULL );

	for ( psC = psNode -> psChild; psC; psC = psC -> psNext ) {
		if ( psC -> nType == XMLNODE_TYPE_DATA ) {
			if( !psStr ) {
				psStr = g_string_new_len( psC -> pnData, psC -> nDataSize );
			} else {
				psStr = g_string_append_len( psStr, psC -> pnData, psC -> nDataSize );
			}
		}
	}

	if ( psStr == NULL ) {
		return NULL;
	}

	return g_string_free( psStr, FALSE );
}

/**
 * \brief Set data from xmlnode
 * \param psNode xml node
 * \param pnData data string
 * \return error code
 */
int xmlnode_set_data( xmlnode *psNode, gchar *pnData ) {
	xmlnode *psC;
	int nRet = -1;

	g_return_val_if_fail( psNode != NULL, nRet );

	for ( psC = psNode -> psChild; psC; psC = psC -> psNext ) {
		g_debug("nType: %d == %d?\n", psC -> nType, XMLNODE_TYPE_DATA );
		if ( psC -> nType == XMLNODE_TYPE_DATA ) {
			if ( psC -> pnData ) {
				g_free( psC -> pnData );
				psC -> pnData = NULL;
			}
			g_debug("Set data!\n" );
			psC -> pnData = g_strdup( pnData );
			psC -> nDataSize = strlen( psC -> pnData );
			nRet = 0;
		}
	}

	return nRet;
}

/**
 * \brief Read xml from file
 * \param file_name xml file name
 * \return new xml node or NULL
 */
xmlnode *read_xml_from_file(const char *file_name) {
	const char *user_dir = g_get_user_data_dir();
	gchar *file_nameFull = NULL;
	gchar *pnContents = NULL;
	gsize nLength;
	GError *psError = NULL;
	xmlnode *psNode = NULL;

	g_return_val_if_fail( user_dir != NULL, NULL );

	file_nameFull = g_build_filename( user_dir, file_name, NULL );
	if ( !g_file_test( file_nameFull, G_FILE_TEST_EXISTS ) ) {
		g_free( file_nameFull );
		file_nameFull = g_strdup( file_name );
		if ( !g_file_test( file_nameFull, G_FILE_TEST_EXISTS ) ) {
			g_debug("File does not exists\n" );
			g_free( file_nameFull );
			return NULL;
		}
	}

	if ( !g_file_get_contents( file_nameFull, &pnContents, &nLength, &psError ) ) {
		g_warning("'%s'\n", psError -> message );
		g_error_free( psError );
	}

	if ( ( pnContents != NULL ) && ( nLength > 0 ) ) {
		psNode = xmlnode_from_str( pnContents, nLength );
		if ( psNode == NULL ) {
			g_warning("Could not parse node\n" );
		}
		g_free( pnContents );
	}

	g_free( file_nameFull );

	return psNode;
}
