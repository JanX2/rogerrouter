#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <rm/rmcall.h>

static void test_scramble_call(void)
{
	gchar *number = "012345678";
	gchar *result;

	/* Check full number */
	result = rm_number_scramble(number);
	g_assert_cmpstr(result, ==, "01XXXXXX8");
	g_free(result);

	/* Check for empty string */
	result = rm_number_scramble("");
	g_assert_cmpstr(result, ==, "");
	g_free(result);

	/* Check for short number */
	result = rm_number_scramble("12");
	g_assert_cmpstr(result, ==, "12");
	g_free(result);

	/* Check for invalid argument */
	result = rm_number_scramble(NULL);
	g_assert_cmpstr(result, ==, "");
	g_free(result);
}

static void test_rm_call_canonize_number(void)
{
	gchar *number = "0123akjhdsg\"=\"§)(/!4567§/(!&(/&!ÄÖX;AS  8\n\r\"";

	g_assert_cmpstr(rm_number_canonize(number), ==, "012345678");
}

static void test_rm_call_full_number(void)
{
	gchar *number = "040123456";

	g_assert_cmpstr(rm_number_full(number, FALSE), ==, "040123456");
}

int main(int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func("/call/scramble", test_scramble_call);
	g_test_add_func("/call/canonize", test_rm_call_canonize_number);
	//g_test_add("/call/fullnumber", NULL, test_rm_call_full_number);

	return g_test_run();
}
