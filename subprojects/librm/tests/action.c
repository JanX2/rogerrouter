#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <rm/rmaction.h>
#include <rm/rmconnection.h>
#include <rm/rmobject.h>

typedef struct {
	RmProfile *profile;
} action_fixture;

static void test_action_init(action_fixture *af, gconstpointer user_data)
{
	rm_object = rm_object_new();
	af->profile = rm_profile_add("Test");
}

static void test_action_shutdown(action_fixture *af, gconstpointer user_data)
{
}

static void test_action_add(action_fixture *af, gconstpointer user_data)
{
	RmAction *action;

	action = rm_action_add(af->profile, "Test");

	g_assert_nonnull(action);
}

static void test_action_regex(action_fixture *af, gconstpointer user_data)
{
	struct rm_action *action;
	gchar *str = g_strdup("Incoming call from %NAME% with %NUMBER% on %LINE%");
	RmConnection *connection;
	gchar *res;

	connection = rm_connection_add(0, RM_CONNECTION_TYPE_INCOMING, "1234", "9876");
	g_assert_nonnull(connection);

	res = rm_action_regex(str, connection);

	g_assert_cmpstr(res, ==, "Incoming call from  with 9876 on 1234");

	rm_connection_remove(connection);
}

int main(int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add("/action/add", action_fixture, "", test_action_init, test_action_add, test_action_shutdown);
	g_test_add("/action/regex", action_fixture, "", test_action_init, test_action_regex, test_action_shutdown);

	return g_test_run();
}
