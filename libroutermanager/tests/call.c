#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libroutermanager/rmcall.h>

static void test_scramble_call(void)
{
	gchar *number = "012345678";
	gchar *result;

	/* Check full number */
	result = rm_call_scramble_number(number);
	g_assert_cmpstr(result, ==, "01XXXXXX8");
	g_free(result);

	/* Check for empty string */
	result = rm_call_scramble_number("");
	g_assert_cmpstr(result, ==, "");
	g_free(result);

	/* Check for short number */
	result = rm_call_scramble_number("12");
	g_assert_cmpstr(result, ==, "12");
	g_free(result);

	/* Check for invalid argument */
	result = rm_call_scramble_number(NULL);
	g_assert_cmpstr(result, ==, "");
	g_free(result);
}

static void test_rm_call_canonize_number(void)
{
	gchar *number = "0123akjhdsg\"=\"§)(/!4567§/(!&(/&!ÄÖX;AS  8\n\r\"";

	g_assert_cmpstr(rm_call_canonize_number(number), ==, "012345678");
}

static void test_rm_call_full_number(void)
{
	gchar *number = "040123456";

	g_assert_cmpstr(rm_call_full_number(number, FALSE), ==, "040123456");
}

int main(int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func("/call/scramble", test_scramble_call);
	g_test_add_func("/call/canonize", test_rm_call_canonize_number);
	//g_test_add("/call/fullnumber", NULL, test_rm_call_full_number);

	return g_test_run();
}
