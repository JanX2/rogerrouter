/**
 * The libroutermanager project
 * Copyright (c) 2012-2016 Jan-Michael Brummer
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

#include <config.h>

#include <gtk/gtk.h>

#include <pjsua-lib/pjsua.h>

#include <libroutermanager/rmconnection.h>
#include <libroutermanager/rmplugins.h>
#include <libroutermanager/rmprofile.h>
#include <libroutermanager/rmrouter.h>
#include <libroutermanager/rmnetmonitor.h>
#include <libroutermanager/rmaudio.h>
#include <libroutermanager/rmstring.h>
#include <libroutermanager/rmphone.h>
#include <libroutermanager/rmobjectemit.h>
#include <libroutermanager/rmsettings.h>

#include <roger/main.h>
#include <roger/uitools.h>
#include <roger/settings.h>

#define RM_TYPE_SIP_PLUGIN (routermanager_sip_plugin_get_type ())
#define RM_SIP_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_CAST((o), RM_TYPE_SIP_PLUGIN, RmSipPlugin))

typedef struct {
	gconstpointer net_event_id;

	guint id;
} RmSipPluginPrivate;

RM_PLUGIN_REGISTER_CONFIGURABLE(RM_TYPE_SIP_PLUGIN, RmSipPlugin, routermanager_sip_plugin)

struct sip_call {
	gint id;
	pj_pool_t *pool;
	pjmedia_port *tonegen;
	pjsua_conf_port_id toneslot;
};

static GSettings *sip_settings = NULL;

struct sip_call *call_init_tonegen(pjsua_call_id call_id)
{
  pj_pool_t *pool;
  struct sip_call *cd;
  pjsua_call_info ci;

  pool = pjsua_pool_create("mycall", 512, 512);
  cd = PJ_POOL_ZALLOC_T(pool, struct sip_call);
  cd->pool = pool;

  pjmedia_tonegen_create(cd->pool, 8000, 1, 160, 16, 0, &cd->tonegen);
  pjsua_conf_add_port(cd->pool, cd->tonegen, &cd->toneslot);

  pjsua_call_get_info(call_id, &ci);
  pjsua_conf_connect(cd->toneslot, ci.conf_slot);

  pjsua_call_set_user_data(call_id, (void*) cd);

  return cd;
}

void call_play_digit(RmConnection *connection, const char *digits)
{
  pjmedia_tone_digit d[16];
  unsigned i, count = strlen(digits);
  struct sip_call *cd;

  cd = connection->priv;
  if (!cd) {
    cd = call_init_tonegen(connection->id);
    connection->priv = cd;
  }

  if (count > PJ_ARRAY_SIZE(d))
    count = PJ_ARRAY_SIZE(d);

  pj_bzero(d, sizeof(d));
  for (i=0; i<count; ++i) {
    d[i].digit = digits[i];
    d[i].on_msec = 100;
    d[i].off_msec = 200;
    d[i].volume = 0;
  }

  pjmedia_tonegen_play_digits(cd->tonegen, count, d, 0);
}

void call_deinit_tonegen(pjsua_call_id call_id)
{
  struct sip_call *cd;

  cd = (struct sip_call*) pjsua_call_get_user_data(call_id);
  if (!cd)
     return;

  pjsua_conf_remove_port(cd->toneslot);
  pjmedia_port_destroy(cd->tonegen);
  pj_pool_release(cd->pool);

  pjsua_call_set_user_data(call_id, NULL);
}

/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	RmConnection *connection;

	pjsua_call_info ci;

	PJ_UNUSED_ARG(acc_id);
	PJ_UNUSED_ARG(rdata);

	pjsua_call_get_info(call_id, &ci);

	g_debug("Incoming call from %.*s!!", (int)ci.remote_info.slen, ci.remote_info.ptr);

	connection = rm_connection_add(call_id, RM_CONNECTION_TYPE_INCOMING, ci.local_info.ptr, ci.remote_info.ptr);

	rm_object_emit_connection_incoming(connection);
}

/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	RmConnection *connection;
	pjsua_call_info ci;

	PJ_UNUSED_ARG(e);

	pjsua_call_get_info(call_id, &ci);

	g_debug("%s(): Call %d state=%.*s", __FUNCTION__, call_id, (int)ci.state_text.slen, ci.state_text.ptr);
	g_debug("%s(): ID = %s", __FUNCTION__, ci.call_id.ptr);

	connection = rm_connection_find_by_id(call_id);
	if (!connection) {
		g_debug("%s(): No connection found", __FUNCTION__);
		return;
	}

	if (ci.state == PJSIP_INV_STATE_CONFIRMED) {
		rm_object_emit_connection_connect(connection);
	} else if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
		g_debug("%s(): Emitting disconnect", __FUNCTION__);
		rm_object_emit_connection_disconnect(connection);
		g_debug("%s(): Destroying connection", __FUNCTION__);
		call_deinit_tonegen(call_id);
		rm_connection_remove(connection);
	}
}

/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id)
{
	pjsua_call_info ci;

	pjsua_call_get_info(call_id, &ci);

	if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
		// When media is active, connect call to sound device.
		pjsua_conf_connect(ci.conf_slot, 0);
		pjsua_conf_connect(0, ci.conf_slot);
	}
}

void sip_g722_state(gboolean state)
{
	/* Disable G722 codec for the moment (FRITZ!Box seems to have problems on non-internal calls) */
	pj_str_t tmp;

	g_debug("%s(): state = %d", __FUNCTION__, state);
	pjsua_codec_set_priority(pj_cstr(&tmp, "G722"), state == TRUE ? 128 : 0);
}

static RmConnection *sip_phone_dial(const char *trg_no, gboolean anonymous)
{
	RmConnection *connection;
	RmProfile *profile = rm_profile_get_active();
	pj_status_t status;
	pjsua_acc_id acc_id = 0;
	pjsua_call_id call_id;
	gchar *target;
	pj_str_t uri;

	target = g_strdup_printf("sip:%s@%s", trg_no, profile->router_info->host);

	g_debug("%s(): target: %s", __FUNCTION__, target);
	if (trg_no && (!strncmp(trg_no, "**797", 5) || !strncmp(trg_no, "**799", 5))) {
		sip_g722_state(TRUE);
	} else {
		sip_g722_state(FALSE);
	}

	uri = pj_str((gchar*)target);

	status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, &call_id);
	g_free(target);

	if (status != PJ_SUCCESS) {
		gchar buf[256];

		g_warning("%s(): Error making call = %d", __FUNCTION__, status);
		pj_strerror(status, buf, 255);
		g_warning("%s(): %s", __FUNCTION__, buf);
		return NULL;
	}

	connection = rm_connection_add(call_id, RM_CONNECTION_TYPE_OUTGOING, g_settings_get_string(sip_settings, "msn"), trg_no);

	return connection;
}

static gint sip_phone_pickup(RmConnection *connection)
{
	return pjsua_call_answer(connection->id, 200, NULL, NULL);
}

void sip_phone_hangup(RmConnection *connection)
{
	pjsua_call_hangup(connection->id, 0, NULL, NULL);
}

void sip_phone_hold(RmConnection *connection, gboolean hold)
{
	if (hold) {
		pjsua_call_set_hold(connection->id, NULL);
	} else {
		pjsua_call_reinvite(connection->id, PJ_TRUE, NULL);
	}
}

void sip_phone_send_dtmf_code(RmConnection *connection, guchar code)
{
	gchar *dtmf = g_strdup_printf("%c", code);

	pj_str_t dtmf_code = pj_str((gchar*)dtmf);

	//call_play_digit(connection, (const char*) dtmf);

	pjsua_call_dial_dtmf(connection->id, &dtmf_code);
	g_free(dtmf);
}

gboolean sip_connect(gpointer user_data)
{
	//RmSipPlugin *sip_plugin = user_data;
	RmProfile *profile = rm_profile_get_active();
	pj_status_t status;
	pj_thread_desc rtpdesc;
	pj_thread_t *thread = 0;
	pjsua_logging_config log_cfg;
	pjsua_config sua_cfg;
	pjsua_transport_config transport_cfg;
	pjsua_acc_config acc_cfg;

	g_debug("%s(): called", __FUNCTION__);

	/* Create pjsua first! */
	status = pjsua_create();
	if (status != PJ_SUCCESS) {
		g_warning("SIP: Error in pjsua_create() = %d", status);
		return FALSE;
	}

	/* Register the thread with PJLIB, this is must for any external threads
	 * which need to use the PJLIB framework
	 */
	if (!pj_thread_is_registered()) {
		status = pj_thread_register("Threadname", rtpdesc, &thread );
		if (status != PJ_SUCCESS) {
			g_warning("SIP: Threadname Failed to register with PJLIB,exit = %d", status);
			pjsua_destroy();
			return FALSE;
		}
	}

	/* Init pjsua */
	pjsua_config_default(&sua_cfg);
	sua_cfg.cb.on_incoming_call = &on_incoming_call;
	sua_cfg.cb.on_call_media_state = &on_call_media_state;
	sua_cfg.cb.on_call_state = &on_call_state;

	pjsua_logging_config_default(&log_cfg);
	log_cfg.console_level = 0;

	status = pjsua_init(&sua_cfg, &log_cfg, NULL);
	if (status != PJ_SUCCESS) {
		g_warning("SIP: Error in pjsua_init() = %d", status);
		pjsua_destroy();
		return FALSE;
	}

	/* Add UDP transport. */
	pjsua_transport_config_default(&transport_cfg);
	transport_cfg.port = 5060;
	status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transport_cfg, NULL);
	if (status != PJ_SUCCESS) {
		g_warning("SIP: Error creating transport = %d", status);
		pjsua_destroy();
		return FALSE;
	}

	/* Initialization is done, now start pjsua */
	status = pjsua_start();
	if (status != PJ_SUCCESS) {
		g_warning("SIP: Error starting pjsua = %d", status);
		pjsua_destroy();
		return FALSE;
	}

	/* Register to SIP server by creating SIP account. */
	gchar *id = g_strdup_printf("sip:%s@%s", g_settings_get_string(sip_settings, "user"), profile->router_info->host);
	gchar *uri = g_strdup_printf("sip:%s", profile->router_info->host);

	pjsua_acc_config_default(&acc_cfg);
	acc_cfg.id = pj_str(id);
	acc_cfg.reg_uri = pj_str(uri);
	acc_cfg.cred_count = 1;
	acc_cfg.cred_info[0].realm = pj_str("fritz.box");//profile->router_info->host);
	acc_cfg.cred_info[0].scheme = pj_str("digest");
	acc_cfg.cred_info[0].username = pj_str(g_settings_get_string(sip_settings, "user"));
	acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
	acc_cfg.cred_info[0].data = pj_str(g_settings_get_string(sip_settings, "password"));

	status = pjsua_acc_add(&acc_cfg, PJ_TRUE, NULL);
	g_free(uri);
	g_free(id);

	if (status != PJ_SUCCESS) {
		g_warning("SIP: Error adding account = %d", status);
		pjsua_destroy();
		return FALSE;
	}

	{
	int dev_count;

	pjmedia_aud_dev_index dev_idx;
	pj_status_t status;
	dev_count = pjmedia_aud_dev_count();
	printf("Got %d audio devices\n", dev_count);

	for (dev_idx=0; dev_idx<dev_count; ++dev_idx) {
		pjmedia_aud_dev_info info;
		status = pjmedia_aud_dev_get_info(dev_idx, &info);

		printf("%d. %s/%s (in=%d, out=%d)\n", dev_idx, info.name, info.driver, info.input_count, info.output_count);
		status = status;
	}
	}

	return TRUE;
}

gboolean sip_disconnect(gpointer user_data)
{
	pj_status_t status;

	g_debug("%s(): called", __FUNCTION__);
	/* Destroy pjsua */
	status = pjsua_destroy();

	return status == PJ_SUCCESS;
}

gboolean sip_phone_number_is_handled(gchar *number)
{
	if (!strcmp(number, g_settings_get_string(sip_settings, "msn"))) {
		return TRUE;
	}

	return FALSE;
}

RmPhone sip_phone = {
	"SIP Phone",
	sip_phone_dial,
	sip_phone_pickup,
	sip_phone_hangup,
	sip_phone_hold,
	sip_phone_send_dtmf_code,
	sip_phone_number_is_handled,
};

/**
 * \brief Activate plugin (add net event)
 * \param plugin peas plugin
 */
static void impl_activate(PeasActivatable *plugin)
{
	RmSipPlugin *sip_plugin = RM_SIP_PLUGIN(plugin);

	g_debug("%s(): sip", __FUNCTION__);

	sip_settings = rm_settings_new("org.tabos.roger.plugins.sip");

	/* Add network event */
	sip_plugin->priv->net_event_id = rm_netmonitor_add_event("SIP", sip_connect, sip_disconnect, sip_plugin);

	rm_phone_register(&sip_phone);
}

/**
 * \brief Deactivate plugin (remote net event)
 * \param plugin peas plugin
 */
static void impl_deactivate(PeasActivatable *plugin)
{
	RmSipPlugin *sip_plugin = RM_SIP_PLUGIN(plugin);

	g_debug("%s(): sip", __FUNCTION__);
	/* Remove network event */
	rm_netmonitor_remove_event(sip_plugin->priv->net_event_id);
	rm_phone_unregister(&sip_phone);

	g_clear_object(&sip_settings);
}

GtkWidget *impl_create_configure_widget(PeasGtkConfigurable *config)
{
	GtkWidget *user_entry;
	GtkWidget *password_entry;
	GtkWidget *msn_entry;
	GtkWidget *label;
	GtkWidget *grid;
	GtkWidget *group;

	grid = gtk_grid_new();
	//gtk_widget_set_margin(grid, 18, 18, 18, 18);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	label = gtk_label_new(_("User"));
	gtk_widget_set_sensitive(label, FALSE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	user_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), user_entry, 1, 0, 1, 1);
	g_settings_bind(sip_settings, "user", user_entry, "text", G_SETTINGS_BIND_DEFAULT);

	label = gtk_label_new(_("Password"));
	gtk_widget_set_sensitive(label, FALSE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

	password_entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);
	g_settings_bind(sip_settings, "password", password_entry, "text", G_SETTINGS_BIND_DEFAULT);

	label = gtk_label_new(_("MSN"));
	gtk_widget_set_sensitive(label, FALSE);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);

	msn_entry = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), msn_entry, 1, 2, 1, 1);
	g_settings_bind(sip_settings, "msn", msn_entry, "text", G_SETTINGS_BIND_DEFAULT);

	//group = pref_group_create(grid, _("Access data"), TRUE, FALSE);

	return grid;
}
