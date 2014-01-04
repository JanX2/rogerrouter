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
 * \param name node name
 * \param type node type
 * \return new node pointer
 */
xmlnode *new_node(const char *name, XMLNodeType type)
{
	xmlnode *node = g_new0(xmlnode, 1);

	node->name = g_strdup(name);
	node->type = type;

	return node;
}

/**
 * \brief Create a new tag node
 * \param name node name
 * \return new node pointer or NULL on error
 */
xmlnode *xmlnode_new(const char *name)
{
	g_return_val_if_fail(name != NULL, NULL);

	return new_node(name, XMLNODE_TYPE_TAG);
}

/**
 * \brief Insert child into parent node
 * \param parent parent node
 * \param child child node
 */
void xmlnode_insert_child(xmlnode *parent, xmlnode *child)
{
	g_return_if_fail(parent != NULL);
	g_return_if_fail(child != NULL);

	child->parent = parent;

	if (parent->last_child) {
		parent->last_child->next = child;
	} else {
		parent->child = child;
	}

	parent->last_child = child;
}

/**
 * \brief Create new child node
 * \param parent parent node
 * \param name node name
 * \return new node pointer or NULL on error
 */
xmlnode *xmlnode_new_child(xmlnode *parent, const char *name)
{
	xmlnode *node;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	node = new_node(name, XMLNODE_TYPE_TAG);

	xmlnode_insert_child(parent, node);

	return node;
}

/**
 * \brief Free node
 * \param node node to free
 */
void xmlnode_free(xmlnode *node)
{
	xmlnode *x, *y;

	g_return_if_fail(node != NULL);

	if (node->parent != NULL) {
		if (node->parent->child == node) {
			node->parent->child = node->next;
			if (node->parent->last_child == node) {
				node->parent->last_child = node->next;
			}
		} else {
			xmlnode *prev = node->parent->child;
			while (prev && prev->next != node) {
				prev = prev->next;
			}

			if (prev) {
				prev->next = node->next;
				if (node->parent->last_child == node) {
					node->parent->last_child = prev;
				}
			}
		}
	}

	x = node->child;
	while (x) {
		y = x->next;
		xmlnode_free(x);
		x = y;
	}

	g_free(node->name);
	g_free(node->data);
	g_free(node->xml_ns);
	g_free(node->prefix);

	if (node->namespace_map) {
		g_hash_table_destroy(node->namespace_map);
	}

	g_free(node);
}

/**
 * \brief Remove attribute from node
 * \param node node pointer
 * \param attr attribute name
 */
static void xmlnode_remove_attrib(xmlnode *node, const char *attr)
{
	xmlnode *psAttrNode, *sibling = NULL;

	g_return_if_fail(node != NULL);
	g_return_if_fail(attr != NULL);

	for (psAttrNode = node->child; psAttrNode != NULL; psAttrNode = psAttrNode->next) {
		if (psAttrNode->type == XMLNODE_TYPE_ATTRIB && !strcmp(psAttrNode->name, attr)) {
			if (sibling == NULL) {
				node->child = psAttrNode->next;
			} else {
				sibling->next = psAttrNode->next;
			}
			if (node->last_child == psAttrNode) {
				node->last_child = sibling;
			}
			xmlnode_free(psAttrNode);
			return;
		}
		sibling = psAttrNode;
	}
}

/**
 * \brief Set attribute for node
 * \param node node pointer
 * \param attr attribute name
 * \param value value to set
 */
void xmlnode_set_attrib(xmlnode *node, const char *attr, const char *value)
{
	xmlnode *psAttribNode;

	g_return_if_fail(node != NULL);
	g_return_if_fail(attr != NULL);
	g_return_if_fail(value != NULL);

	xmlnode_remove_attrib(node, attr);

	psAttribNode = new_node(attr, XMLNODE_TYPE_ATTRIB);

	psAttribNode->data = g_strdup(value);

	xmlnode_insert_child(node, psAttribNode);
}

/**
 * \brief Get node prefix
 * \param node node pointer
 * \return node prefix
 */
static const char *xmlnode_get_prefix(xmlnode *node)
{
	g_return_val_if_fail(node != NULL, NULL);
	return node->prefix;
}

/**
 * \brief Convert node to string
 * \param pnKey key name
 * \param value value
 * \param psBuf buffer to print to
 */
static void xmlnode_to_str_foreach_append_ns(const char *pnKey, const char *value, GString *psBuf)
{
	if (*pnKey) {
		g_string_append_printf(psBuf, " xmlns:%s='%s'", pnKey, value);
	} else {
		g_string_append_printf(psBuf, " xmlns='%s'", value);
	}
}

/**
 * \brief Helps with converting node to string
 * \param node node to convert
 * \param pnLen pointer for len saving
 * \param bFormatting format text?
 * \param nDepth depth
 * \return string data or NULL on error
 */
static char *xmlnode_to_str_helper(xmlnode *node, int *pnLen, gboolean bFormatting, int nDepth)
{
	GString *psText = g_string_new("");
	const char *prefix;
	xmlnode *c;
	char *pnNodeName, *esc, *esc2, *tab = NULL;
	gboolean bNeedEnd = FALSE, bPretty = bFormatting;

	g_return_val_if_fail(node != NULL, NULL);

	if (bPretty && nDepth) {
		tab = g_strnfill(nDepth, '\t');
		psText = g_string_append(psText, tab);
	}

	pnNodeName = g_markup_escape_text(node->name, -1);
	prefix = xmlnode_get_prefix(node);

	if (prefix) {
		g_string_append_printf(psText, "<%s:%s", prefix, pnNodeName);
	} else {
		g_string_append_printf(psText, "<%s", pnNodeName);
	}

	if (node->namespace_map) {
		g_hash_table_foreach(node->namespace_map, (GHFunc) xmlnode_to_str_foreach_append_ns, psText);
	} else if (node->xml_ns) {
		if (!node->parent || !node->parent->xml_ns || strcmp(node->xml_ns, node->parent->xml_ns)) {
			char *xml_ns = g_markup_escape_text(node->xml_ns, -1);
			g_string_append_printf(psText, " xmlns='%s'", xml_ns);
			g_free(xml_ns);
		}
	}

	for (c = node->child; c != NULL; c = c->next) {
		if (c->type == XMLNODE_TYPE_ATTRIB) {
			const char *pnAprefix = xmlnode_get_prefix(c);
			esc = g_markup_escape_text(c->name, -1);
			esc2 = g_markup_escape_text(c->data, -1);

			if (pnAprefix) {
				g_string_append_printf(psText, " %s:%s='%s'", pnAprefix, esc, esc2);
			} else {
				g_string_append_printf(psText, " %s='%s'", esc, esc2);
			}
			g_free(esc);
			g_free(esc2);
		} else if (c->type == XMLNODE_TYPE_TAG || c->type == XMLNODE_TYPE_DATA) {
			if (c->type == XMLNODE_TYPE_DATA) {
				bPretty = FALSE;
			}
			bNeedEnd = TRUE;
		}
	}

	if (bNeedEnd) {
		g_string_append_printf(psText, ">%s", bPretty ? "\n" : "");

		for (c = node->child; c != NULL; c = c->next) {
			if (c->type == XMLNODE_TYPE_TAG) {
				int nEscLen;
				esc = xmlnode_to_str_helper(c, &nEscLen, bPretty, nDepth + 1);
				psText = g_string_append_len(psText, esc, nEscLen);
				g_free(esc);
			} else if (c->type == XMLNODE_TYPE_DATA && c->data_size > 0) {
				esc = g_markup_escape_text(c->data, c->data_size);
				psText = g_string_append(psText, esc);
				g_free(esc);
			}
		}

		if (tab && bPretty) {
			psText = g_string_append(psText, tab);
		}
		if (prefix) {
			g_string_append_printf(psText, "</%s:%s>%s", prefix, pnNodeName, bFormatting ? "\n" : "");
		} else {
			g_string_append_printf(psText, "</%s>%s", pnNodeName, bFormatting ? "\n" : "");
		}
	} else {
		g_string_append_printf(psText, "/>%s", bFormatting ? "\n" : "");
	}

	g_free(pnNodeName);

	g_free(tab);

	if (pnLen) {
		*pnLen = psText->len;
	}

	return g_string_free(psText, FALSE);
}

/**
 * \brief Convet node to formatted string
 * \param node node
 * \param pnLen pointer to len
 * \return formatted string or NULL on error
 */
char *xmlnode_to_formatted_str(xmlnode *node, int *pnLen)
{
	char *pnXml, *pnXmlWithDeclaration;

	g_return_val_if_fail(node != NULL, NULL);

	pnXml = xmlnode_to_str_helper(node, pnLen, TRUE, 0);
	pnXmlWithDeclaration = g_strdup_printf("<?xml version='1.0' encoding='UTF-8' ?>\n\n%s", pnXml);
	g_free(pnXml);

	if (pnLen) {
		*pnLen += sizeof("<?xml version='1.0' encoding='UTF-8' ?>\n\n") - 1;
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
 * \param node xml node
 * \param xml_ns xml namespace
 */
void xmlnode_set_namespace(xmlnode *node, const char *xml_ns)
{
	g_return_if_fail(node != NULL);

	g_free(node->xml_ns);
	node->xml_ns = g_strdup(xml_ns);
}

/**
 * \brief Set prefix
 * \param node xml node
 * \param prefix prefix
 */
static void xmlnode_set_prefix(xmlnode *node, const char *prefix)
{
	g_return_if_fail(node != NULL);

	g_free(node->prefix);
	node->prefix = g_strdup(prefix);
}

/**
 * \brief Insert data into xmlnode
 * \param node xml node
 * \param data data pointer
 * \param size size of data
 */
void xmlnode_insert_data(xmlnode *node, const char *data, gssize size)
{
	xmlnode *child;
	gsize real_size;

	g_return_if_fail(node != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(size != 0);

	if (size == -1) {
		real_size = strlen(data);
	} else {
		real_size = size;
	}

	child = new_node(NULL, XMLNODE_TYPE_DATA);

	child->data = g_memdup(data, real_size);
	child->data_size = real_size;

	xmlnode_insert_child(node, child);
}

/**
 * \brief Set attribute with prefix
 * \param node xml node
 * \param attr attribute
 * \param prefix prefix
 * \param value value
 */
static void xmlnode_set_attrib_with_prefix(xmlnode *node, const char *attr, const char *prefix, const char *value)
{
	xmlnode *psAttribNode;

	g_return_if_fail(node != NULL);
	g_return_if_fail(attr != NULL);
	g_return_if_fail(value != NULL);

	psAttribNode = new_node(attr, XMLNODE_TYPE_ATTRIB);

	psAttribNode->data = g_strdup(value);
	psAttribNode->prefix = g_strdup(prefix);

	xmlnode_insert_child(node, psAttribNode);
}

/**
 * \brief Get attribute from node
 * \param node xml node structure
 * \param attr attribute name
 * \return attribute data
 */
const char *xmlnode_get_attrib(xmlnode *node, const char *attr)
{
	xmlnode *x;

	g_return_val_if_fail(node != NULL, NULL);
	g_return_val_if_fail(attr != NULL, NULL);

	for (x = node->child; x != NULL; x = x->next) {
		if (x->type == XMLNODE_TYPE_ATTRIB && strcmp(attr, x->name) == 0) {
			return x->data;
		}
	}

	return NULL;
}

/**
 * \brief Unescape html text
 * \param pnHtml html text
 * \return unescaped text
 */
static char *unescape_html(const char *pnHtml)
{
	if (pnHtml != NULL) {
		const char *pnC = pnHtml;
		GString *ret = g_string_new("");
		while (*pnC) {
			if (!strncmp(pnC, "<br>", 4)) {
				ret = g_string_append_c(ret, '\n');
				pnC += 4;
			} else {
				ret = g_string_append_c(ret, *pnC);
				pnC++;
			}
		}

		return g_string_free(ret, FALSE);
	}

	return NULL;
}

/**
 * \brief Parser: Element start
 * \param user_data xmlnode parser data
 * \param element_name element name
 * \param prefix prefix
 * \param xml_ns xml namespace
 * \param nNbNamespaces number of namespaces
 * \param ppsNamespaces pointer to xml namespaces
 * \param nNbAttributes number of attributes
 * \param nNbDefaulted number of defaulted
 * \param attributes pointer to xml attributes
 */
static void xmlnode_parser_element_start_libxml(void *user_data, const xmlChar *element_name, const xmlChar *prefix,
        const xmlChar *xml_ns, int nNbNamespaces, const xmlChar **ppsNamespaces, int nNbAttributes, int nNbDefaulted,
        const xmlChar **attributes)
{
	struct _xmlnode_parser_data *xpd = user_data;
	xmlnode *node;
	int nI, nJ;

	if (!element_name || xpd->error) {
		return;
	} else {
		if (xpd->current) {
			node = xmlnode_new_child(xpd->current, (const char *) element_name);
		} else {
			node = xmlnode_new((const char *) element_name);
		}

		xmlnode_set_namespace(node, (const char *) xml_ns);
		xmlnode_set_prefix(node, (const char *) prefix);

		if (nNbNamespaces != 0) {
			node->namespace_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

			for (nI = 0, nJ = 0; nI < nNbNamespaces; nI++, nJ += 2) {
				const char *pnKey = (const char *) ppsNamespaces[nJ];
				const char *pnVal = (const char *) ppsNamespaces[nJ + 1];
				g_hash_table_insert(node->namespace_map, g_strdup(pnKey ? pnKey : ""), g_strdup(pnVal ? pnVal : ""));
			}
		}

		for (nI = 0; nI < nNbAttributes * 5; nI += 5) {
			const char *prefix = (const char *) attributes[nI + 1];
			char *pnTxt;
			int attrib_len = attributes[nI + 4] - attributes[nI + 3];
			char *attrib = g_malloc(attrib_len + 1);

			memcpy(attrib, attributes[nI + 3], attrib_len);
			attrib[attrib_len] = '\0';
			pnTxt = attrib;
			attrib = unescape_html(pnTxt);
			g_free(pnTxt);

			if (prefix && *prefix) {
				xmlnode_set_attrib_with_prefix(node, (const char *) attributes[nI], prefix, attrib);
			} else {
				xmlnode_set_attrib(node, (const char *) attributes[nI], attrib);
			}
			g_free(attrib);
		}

		xpd->current = node;
	}
}

/**
 * \brief Parser: Element end
 * \param user_data xmlnode parser data
 * \param element_name element name
 * \param prefix prefix
 * \param xml_ns xml namespace
 */
static void xmlnode_parser_element_end_libxml(void *user_data, const xmlChar *element_name, const xmlChar *prefix, const xmlChar *xml_ns)
{
	struct _xmlnode_parser_data *xpd = user_data;

	if (!element_name || !xpd->current || xpd->error) {
		return;
	}

	if (xpd->current->parent) {
		if (!xmlStrcmp((xmlChar *) xpd->current->name, element_name)) {
			xpd->current = xpd->current->parent;
		}
	}
}

/**
 * \brief Parser: Element text
 * \param user_data xmlnode parser data
 * \param text text element
 * \param text_len text length
 */
static void xmlnode_parser_element_text_libxml(void *user_data, const xmlChar *text, int text_len)
{
	struct _xmlnode_parser_data *xpd = user_data;

	if (!xpd->current || xpd->error) {
		return;
	}

	if (!text || !text_len) {
		return;
	}

	xmlnode_insert_data(xpd->current, (const char *) text, text_len);
}

/**
 * \brief Parser error
 * \param user_data xmlnode parser data
 * \param msg error message
 */
static void xmlnode_parser_error_libxml(void *user_data, const char *msg, ...)
{
	struct _xmlnode_parser_data *xpd = user_data;
	char err_msg[2048];
	va_list args;

	xpd->error = TRUE;

	va_start(args, msg);
	vsnprintf(err_msg, sizeof(err_msg), msg, args);
	va_end(args);

	g_debug("Error parsing xml file: %s", err_msg);
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
 * \param str string
 * \param size size of string
 * \return new xml node
 */
xmlnode *xmlnode_from_str(const char *str, gssize size)
{
	struct _xmlnode_parser_data *xpd;
	xmlnode *ret;
	gsize real_size;

	g_return_val_if_fail(str != NULL, NULL);

	real_size = size < 0 ? strlen(str) : size;
	xpd = g_new0(struct _xmlnode_parser_data, 1);

	if (xmlSAXUserParseMemory(&sXmlnodeParserLibxml, xpd, str, real_size) < 0) {
		while (xpd->current && xpd->current->parent) {
			xpd->current = xpd->current->parent;
		}

		if (xpd->current) {
			xmlnode_free(xpd->current);
		}
		xpd->current = NULL;
	}
	ret = xpd->current;

	if (xpd->error) {
		ret = NULL;

		if (xpd->current) {
			xmlnode_free(xpd->current);
		}
	}

	g_free(xpd);

	return ret;
}

/**
 * \brief Get namespace
 * \param node xml node
 * \return namespace
 */
const char *xmlnode_get_namespace(xmlnode *node)
{
	g_return_val_if_fail(node != NULL, NULL);

	return node->xml_ns;
}

/**
 * \brief Get child with namespace
 * \param parent parent xml node
 * \param name child name
 * \param ns namespace
 * \return chuld xmlnode
 */
xmlnode *xmlnode_get_child_with_namespace(const xmlnode *parent, const char *name, const char *ns)
{
	xmlnode *x, *ret = NULL;
	char **names;
	char *parent_name, *child_name;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	names = g_strsplit(name, "/", 2);
	parent_name = names[0];
	child_name = names[1];

	for (x = parent->child; x; x = x->next) {
		const char *xml_ns = NULL;

		if (ns != NULL) {
			xml_ns = xmlnode_get_namespace(x);
		}

		if (x->type == XMLNODE_TYPE_TAG && name && !strcmp(parent_name, x->name) && (!ns || (xml_ns && !strcmp(ns, xml_ns)))) {
			ret = x;
			break;
		}
	}

	if (child_name && ret) {
		ret = xmlnode_get_child(ret, child_name);
	}

	g_strfreev(names);

	return ret;
}

/**
 * \brief Get xml node child
 * \param parent xml node parent
 * \param name child name
 * \return child xmlnode
 */
xmlnode *xmlnode_get_child(const xmlnode *parent, const char *name)
{
	return xmlnode_get_child_with_namespace(parent, name, NULL);
}

/**
 * \brief Get next twin from xml node
 * \param node xml node
 * \return next xml node twin
 */
xmlnode *xmlnode_get_next_twin(xmlnode *node)
{
	xmlnode *sibling;
	const char *ns = xmlnode_get_namespace(node);

	g_return_val_if_fail(node != NULL, NULL);
	g_return_val_if_fail(node->type == XMLNODE_TYPE_TAG, NULL);

	for (sibling = node->next; sibling; sibling = sibling->next) {
		const char *xml_ns = NULL;

		if (ns != NULL) {
			xml_ns = xmlnode_get_namespace(sibling);
		}

		if (sibling->type == XMLNODE_TYPE_TAG && !strcmp(node->name, sibling->name) && (!ns || (xml_ns && !strcmp(ns, xml_ns)))) {
			return sibling;
		}
	}

	return NULL;
}

/**
 * \brief Get data from xmlnode
 * \param node xml node
 * \return xmlnode data
 */
char *xmlnode_get_data(xmlnode *node)
{
	GString *str = NULL;
	xmlnode *c;

	g_return_val_if_fail(node != NULL, NULL);

	for (c = node->child; c; c = c->next) {
		if (c->type == XMLNODE_TYPE_DATA) {
			if (!str) {
				str = g_string_new_len(c->data, c->data_size);
			} else {
				str = g_string_append_len(str, c->data, c->data_size);
			}
		}
	}

	if (str == NULL) {
		return NULL;
	}

	return g_string_free(str, FALSE);
}

/**
 * \brief Set data from xmlnode
 * \param node xml node
 * \param data data string
 * \return error code
 */
int xmlnode_set_data(xmlnode *node, gchar *data)
{
	xmlnode *c;
	int nRet = -1;

	g_return_val_if_fail(node != NULL, nRet);

	for (c = node->child; c; c = c->next) {
		g_debug("type: %d == %d?", c->type, XMLNODE_TYPE_DATA);
		if (c->type == XMLNODE_TYPE_DATA) {
			if (c->data) {
				g_free(c->data);
				c->data = NULL;
			}
			g_debug("Set data!");
			c->data = g_strdup(data);
			c->data_size = strlen(c->data);
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
xmlnode *read_xml_from_file(const char *file_name)
{
	const char *user_dir = g_get_user_data_dir();
	gchar *file_name_full = NULL;
	gchar *contents = NULL;
	gsize length;
	GError *error = NULL;
	xmlnode *node = NULL;

	g_return_val_if_fail(user_dir != NULL, NULL);

	file_name_full = g_build_filename(user_dir, file_name, NULL);
	if (!g_file_test(file_name_full, G_FILE_TEST_EXISTS)) {
		g_free(file_name_full);
		file_name_full = g_strdup(file_name);
		if (!g_file_test(file_name_full, G_FILE_TEST_EXISTS)) {
			g_free(file_name_full);
			return NULL;
		}
	}

	if (!g_file_get_contents(file_name_full, &contents, &length, &error)) {
		g_warning("'%s'", error->message);
		g_error_free(error);
	}

	if ((contents != NULL) && (length > 0)) {
		node = xmlnode_from_str(contents, length);
		if (node == NULL) {
			g_warning("Could not parse node");
		}
		g_free(contents);
	}

	g_free(file_name_full);

	return node;
}

static void xmlnode_copy_foreach_ns(gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *ret = (GHashTable *)user_data;
	g_hash_table_insert(ret, g_strdup(key), g_strdup(value));
}

xmlnode *xmlnode_copy(const xmlnode *src)
{
	xmlnode *ret;
	xmlnode *child;
	xmlnode *sibling = NULL;

	g_return_val_if_fail(src != NULL, NULL);

	ret = new_node(src->name, src->type);
	ret->xml_ns = g_strdup(src->xml_ns);
	if (src->data) {
		if (src->data_size) {
			ret->data = g_memdup(src->data, src->data_size);
			ret->data_size = src->data_size;
		} else {
			ret->data = g_strdup(src->data);
		}
	}
	ret->prefix = g_strdup(src->prefix);
	if (src->namespace_map) {
		ret->namespace_map = g_hash_table_new_full(g_str_hash, g_str_equal,
		                     g_free, g_free);
		g_hash_table_foreach(src->namespace_map, xmlnode_copy_foreach_ns, ret->namespace_map);
	}

	for (child = src->child; child; child = child->next) {
		if (sibling) {
			sibling->next = xmlnode_copy(child);
			sibling = sibling->next;
		} else {
			ret->child = xmlnode_copy(child);
			sibling = ret->child;
		}
		sibling->parent = ret;
	}

	ret->last_child = sibling;

	return ret;
}

