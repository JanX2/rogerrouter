extern "C" {
extern int kwallet4_get_password( const char *, char ** );
extern int kwallet4_store_password( const char *, const char * );
extern int kwallet4_init( void );

#include <libroutermanager/call.h>
#include <libroutermanager/plugins.h>
#include <libroutermanager/password.h>

#define ROUTERMANAGER_TYPE_KWALLET_PLUGIN        (routermanager_kwallet_plugin_get_type ())
#define ROUTERMANAGER_KWALLET_PLUGIN(o)          (G_TYPE_CHECK_INSTANCE_CAST((o), ROUTERMANAGER_TYPE_KWALLET_PLUGIN, RouterManagerKWalletPlugin))

typedef struct {
	guint signal_id;
} RouterManagerKWalletPluginPrivate;

ROUTERMANAGER_PLUGIN_REGISTER(ROUTERMANAGER_TYPE_KWALLET_PLUGIN, RouterManagerKWalletPlugin, routermanager_kwallet_plugin)


static gchar *kwallet_get_password(struct profile *profile, const gchar *name)
{
	gint error = -1;
	gchar *pwd_name;
	gchar *secret_password = NULL;

	pwd_name = g_strdup_printf("%s/%s", profile->name, name);
	error = kwallet4_get_password(pwd_name, &secret_password);
	g_free(pwd_name);

	if (error != 0) {
		return NULL;
	}

	return g_strdup(secret_password);
}

static void kwallet_store_password(struct profile *profile, const gchar *name, const gchar *password)
{
	gchar *pwd_name;

	pwd_name = g_strdup_printf("%s/%s", profile->name, name);
	kwallet4_store_password(pwd_name, password);
	g_free(pwd_name);
}

static gboolean kwallet_remove_password(struct profile *profile, const gchar *name)
{
	return FALSE;
}

struct password_manager kwallet = {
	"KWallet",
	kwallet_store_password,
	kwallet_get_password,
	kwallet_remove_password,
};

void impl_activate(PeasActivatable *plugin)
{
	if (kwallet4_init()) {
		g_debug("Register kwallet password manager plugin");
		password_manager_register(&kwallet);
	}
}

void impl_deactivate(PeasActivatable *plugin)
{
}
}