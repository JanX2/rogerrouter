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

#include <libroutermanager/xml.h>

/**
 * \brief Create new xml node
 * \param name node name
 * \param type node type
 * \return new node pointer
 */
xmlnode *new_node(const gchar *name, xml_node_type type)
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
xmlnode *xmlnode_new(const gchar *name)
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
xmlnode *xmlnode_new_child(xmlnode *parent, const gchar *name)
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
static void xmlnode_remove_attrib(xmlnode *node, const gchar *attr)
{
	xmlnode *attr_node, *sibling = NULL;

	g_return_if_fail(node != NULL);
	g_return_if_fail(attr != NULL);

	for (attr_node = node->child; attr_node != NULL; attr_node = attr_node->next) {
		if (attr_node->type == XMLNODE_TYPE_ATTRIB && !strcmp(attr_node->name, attr)) {
			if (sibling == NULL) {
				node->child = attr_node->next;
			} else {
				sibling->next = attr_node->next;
			}

			if (node->last_child == attr_node) {
				node->last_child = sibling;
			}
			xmlnode_free(attr_node);

			return;
		}
		sibling = attr_node;
	}
}

/**
 * \brief Set attribute for node
 * \param node node pointer
 * \param attr attribute name
 * \param value value to set
 */
void xmlnode_set_attrib(xmlnode *node, const gchar *attr, const gchar *value)
{
	xmlnode *attrib_node;

	g_return_if_fail(node != NULL);
	g_return_if_fail(attr != NULL);
	g_return_if_fail(value != NULL);

	xmlnode_remove_attrib(node, attr);

	attrib_node = new_node(attr, XMLNODE_TYPE_ATTRIB);

	attrib_node->data = g_strdup(value);

	xmlnode_insert_child(node, attrib_node);
}

/**
 * \brief Get node prefix
 * \param node node pointer
 * \return node prefix
 */
static const gchar *xmlnode_get_prefix(xmlnode *node)
{
	g_return_val_if_fail(node != NULL, NULL);

	return node->prefix;
}

/**
 * \brief Convert node to string
 * \param key key name
 * \param value value
 * \param buf buffer to print to
 */
static void xmlnode_to_str_foreach_append_ns(const gchar *key, const gchar *value, GString *buf)
{
	if (*key) {
		g_string_append_printf(buf, " xmlns:%s='%s'", key, value);
	} else {
		g_string_append_printf(buf, " xmlns='%s'", value);
	}
}

/**
 * \brief Helps with converting node to string
 * \param node node to convert
 * \param len pointer for len saving
 * \param formatting format text?
 * \param depth depth
 * \return string data or NULL on error
 */
static gchar *xmlnode_to_str_helper(xmlnode *node, gint *len, gboolean formatting, gint depth)
{
	GString *text = g_string_new("");
	const gchar *prefix;
	xmlnode *c;
	gchar *node_name, *esc, *esc2, *tab = NULL;
	gboolean need_end = FALSE, pretty = formatting;

	g_return_val_if_fail(node != NULL, NULL);

	if (pretty && depth) {
		tab = g_strnfill(depth, '\t');
		text = g_string_append(text, tab);
	}

	node_name = g_markup_escape_text(node->name, -1);
	prefix = xmlnode_get_prefix(node);

	if (prefix) {
		g_string_append_printf(text, "<%s:%s", prefix, node_name);
	} else {
		g_string_append_printf(text, "<%s", node_name);
	}

	if (node->namespace_map) {
		g_hash_table_foreach(node->namespace_map, (GHFunc) xmlnode_to_str_foreach_append_ns, text);
	} else if (node->xml_ns) {
		if (!node->parent || !node->parent->xml_ns || strcmp(node->xml_ns, node->parent->xml_ns)) {
			gchar *xml_ns = g_markup_escape_text(node->xml_ns, -1);

			g_string_append_printf(text, " xmlns='%s'", xml_ns);
			g_free(xml_ns);
		}
	}

	for (c = node->child; c != NULL; c = c->next) {
		if (c->type == XMLNODE_TYPE_ATTRIB) {
			const gchar *a_prefix = xmlnode_get_prefix(c);

			esc = g_markup_escape_text(c->name, -1);
			esc2 = g_markup_escape_text(c->data, -1);

			if (a_prefix) {
				g_string_append_printf(text, " %s:%s='%s'", a_prefix, esc, esc2);
			} else {
				g_string_append_printf(text, " %s='%s'", esc, esc2);
			}

			g_free(esc);
			g_free(esc2);
		} else if (c->type == XMLNODE_TYPE_TAG || c->type == XMLNODE_TYPE_DATA) {
			if (c->type == XMLNODE_TYPE_DATA) {
				pretty = FALSE;
			}
			need_end = TRUE;
		}
	}

	if (need_end) {
		g_string_append_printf(text, ">%s", pretty ? "\n" : "");

		for (c = node->child; c != NULL; c = c->next) {
			if (c->type == XMLNODE_TYPE_TAG) {
				gint esc_len;

				esc = xmlnode_to_str_helper(c, &esc_len, pretty, depth + 1);
				text = g_string_append_len(text, esc, esc_len);
				g_free(esc);
			} else if (c->type == XMLNODE_TYPE_DATA && c->data_size > 0) {
				esc = g_markup_escape_text(c->data, c->data_size);
				text = g_string_append(text, esc);
				g_free(esc);
			}
		}

		if (tab && pretty) {
			text = g_string_append(text, tab);
		}
		if (prefix) {
			g_string_append_printf(text, "</%s:%s>%s", prefix, node_name, formatting ? "\n" : "");
		} else {
			g_string_append_printf(text, "</%s>%s", node_name, formatting ? "\n" : "");
		}
	} else {
		g_string_append_printf(text, "/>%s", formatting ? "\n" : "");
	}

	g_free(node_name);

	g_free(tab);

	if (len) {
		*len = text->len;
	}

	return g_string_free(text, FALSE);
}

/**
 * \brief Convet node to formatted string
 * \param node node
 * \param len pointer to len
 * \return formatted string or NULL on error
 */
gchar *xmlnode_to_formatted_str(xmlnode *node, gint *len)
{
	gchar *xml, *xml_with_declaration;

	g_return_val_if_fail(node != NULL, NULL);

	xml = xmlnode_to_str_helper(node, len, TRUE, 0);
	xml_with_declaration = g_strdup_printf("<?xml version='1.0' encoding='UTF-8' ?>\n\n%s", xml);
	g_free(xml);

	if (len) {
		*len += sizeof("<?xml version='1.0' encoding='UTF-8' ?>\n\n") - 1;
	}

	return xml_with_declaration;
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
void xmlnode_set_namespace(xmlnode *node, const gchar *xml_ns)
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
static void xmlnode_set_prefix(xmlnode *node, const gchar *prefix)
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
void xmlnode_insert_data(xmlnode *node, const gchar *data, gssize size)
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
static void xmlnode_set_attrib_with_prefix(xmlnode *node, const gchar *attr, const gchar *prefix, const gchar *value)
{
	xmlnode *attrib_node;

	g_return_if_fail(node != NULL);
	g_return_if_fail(attr != NULL);
	g_return_if_fail(value != NULL);

	attrib_node = new_node(attr, XMLNODE_TYPE_ATTRIB);

	attrib_node->data = g_strdup(value);
	attrib_node->prefix = g_strdup(prefix);

	xmlnode_insert_child(node, attrib_node);
}

/**
 * \brief Get attribute from node
 * \param node xml node structure
 * \param attr attribute name
 * \return attribute data
 */
const gchar *xmlnode_get_attrib(xmlnode *node, const gchar *attr)
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
 * \param html html text
 * \return unescaped text
 */
static gchar *unescape_html(const gchar *html)
{
	if (html != NULL) {
		const gchar *c = html;
		GString *ret = g_string_new("");

		while (*c) {
			if (!strncmp(c, "<br>", 4)) {
				ret = g_string_append_c(ret, '\n');
				c += 4;
			} else {
				ret = g_string_append_c(ret, *c);
				c++;
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
 * \param nb_namespaces number of namespaces
 * \param namespaces pointer to xml namespaces
 * \param nb_attributes number of attributes
 * \param nb_defaulted number of defaulted
 * \param attributes pointer to xml attributes
 */
static void xmlnode_parser_element_start_libxml(void *user_data, const xmlChar *element_name, const xmlChar *prefix,
        const xmlChar *xml_ns, gint nb_namespaces, const xmlChar **namespaces, gint nb_attributes, gint nb_defaulted,
        const xmlChar **attributes)
{
	struct _xmlnode_parser_data *xpd = user_data;
	xmlnode *node;
	gint i, j;

	if (!element_name || xpd->error) {
		return;
	}

	if (xpd->current) {
		node = xmlnode_new_child(xpd->current, (const gchar *) element_name);
	} else {
		node = xmlnode_new((const gchar *) element_name);
	}

	xmlnode_set_namespace(node, (const gchar *) xml_ns);
	xmlnode_set_prefix(node, (const gchar *) prefix);

	if (nb_namespaces != 0) {
		node->namespace_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

		for (i = 0, j = 0; i < nb_namespaces; i++, j += 2) {
			const gchar *key = (const gchar *) namespaces[j];
			const gchar *val = (const gchar *) namespaces[j + 1];

			g_hash_table_insert(node->namespace_map, g_strdup(key ? key : ""), g_strdup(val ? val : ""));
		}
	}

	for (i = 0; i < nb_attributes * 5; i += 5) {
		const gchar *prefix = (const gchar *) attributes[i + 1];
		gchar *txt;
		gint attrib_len = attributes[i + 4] - attributes[i + 3];
		gchar *attrib = g_malloc(attrib_len + 1);

		memcpy(attrib, attributes[i + 3], attrib_len);
		attrib[attrib_len] = '\0';
		txt = attrib;
		attrib = unescape_html(txt);
		g_free(txt);

		if (prefix && *prefix) {
			xmlnode_set_attrib_with_prefix(node, (const gchar *) attributes[i], prefix, attrib);
		} else {
			xmlnode_set_attrib(node, (const gchar *) attributes[i], attrib);
		}
		g_free(attrib);
	}

	xpd->current = node;
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
static void xmlnode_parser_element_text_libxml(void *user_data, const xmlChar *text, gint text_len)
{
	struct _xmlnode_parser_data *xpd = user_data;

	if (!xpd->current || xpd->error) {
		return;
	}

	if (!text || !text_len) {
		return;
	}

	xmlnode_insert_data(xpd->current, (const gchar *) text, text_len);
}

/**
 * \brief Parser error
 * \param user_data xmlnode parser data
 * \param msg error message
 */
static void xmlnode_parser_error_libxml(void *user_data, const gchar *msg, ...)
{
	struct _xmlnode_parser_data *xpd = user_data;
	gchar err_msg[2048];
	va_list args;

	xpd->error = TRUE;

	va_start(args, msg);
	vsnprintf(err_msg, sizeof(err_msg), msg, args);
	va_end(args);

	g_debug("Error parsing xml file: %s", err_msg);
}

/** xmlnode parser libxml */
static xmlSAXHandler xml_node_parser_libxml = {
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
xmlnode *xmlnode_from_str(const gchar *str, gssize size)
{
	struct _xmlnode_parser_data *xpd;
	xmlnode *ret;
	gsize real_size;

	g_return_val_if_fail(str != NULL, NULL);

	real_size = size < 0 ? strlen(str) : size;
	xpd = g_new0(struct _xmlnode_parser_data, 1);

	if (xmlSAXUserParseMemory(&xml_node_parser_libxml, xpd, str, real_size) < 0) {
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
const gchar *xmlnode_get_namespace(xmlnode *node)
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
xmlnode *xmlnode_get_child_with_namespace(const xmlnode *parent, const gchar *name, const gchar *ns)
{
	xmlnode *x, *ret = NULL;
	gchar **names;
	gchar *parent_name, *child_name;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	names = g_strsplit(name, "/", 2);
	parent_name = names[0];
	child_name = names[1];

	for (x = parent->child; x; x = x->next) {
		const gchar *xml_ns = NULL;

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
xmlnode *xmlnode_get_child(const xmlnode *parent, const gchar *name)
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
	const gchar *ns = xmlnode_get_namespace(node);

	g_return_val_if_fail(node != NULL, NULL);
	g_return_val_if_fail(node->type == XMLNODE_TYPE_TAG, NULL);

	for (sibling = node->next; sibling; sibling = sibling->next) {
		const gchar *xml_ns = NULL;

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
gchar *xmlnode_get_data(xmlnode *node)
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
gint xmlnode_set_data(xmlnode *node, gchar *data)
{
	xmlnode *c;
	gint ret = -1;

	g_return_val_if_fail(node != NULL, ret);

	for (c = node->child; c != NULL; c = c->next) {
		if (c->type == XMLNODE_TYPE_DATA) {
			if (c->data) {
				g_free(c->data);
				c->data = NULL;
			}

			c->data = g_strdup(data);
			c->data_size = strlen(c->data);
			ret = 0;
		}
	}

	return ret;
}

/**
 * \brief Read xml from file
 * \param file_name xml file name
 * \return new xml node or NULL
 */
xmlnode *read_xml_from_file(const gchar *file_name)
{
	const gchar *user_dir = g_get_user_data_dir();
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

/**
 * \brief Insert key/value into hash-table
 * \param key key type
 * \param value value type
 * \param user_data pointer to hash table
 */
static void xmlnode_copy_foreach_ns(gpointer key, gpointer value, gpointer user_data)
{
	GHashTable *ret = (GHashTable *)user_data;
	g_hash_table_insert(ret, g_strdup(key), g_strdup(value));
}

/**
 * \brief Make a copy of a given xmlnode
 * \param src source xml node
 * \return new xml node
 */
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
		ret->namespace_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
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
