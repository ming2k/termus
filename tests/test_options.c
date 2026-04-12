#include "app/commands.h"
#include "app/options_parse.h"
#include "app/options_registry.h"
#include "common/msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

static int post_set_calls;
static void *last_post_set_data;

const char * const options_bool_names[] = {
	"false", "true", NULL
};

void run_command(const char *buf)
{
	(void)buf;
}

static void quiet_msg_handler(const char *format, ...)
{
	(void)format;
}

int options_parse_bool(const char *buf, int *val)
{
	if (strcmp(buf, "true") == 0 || strcmp(buf, "1") == 0) {
		*val = 1;
		return 1;
	}
	if (strcmp(buf, "false") == 0 || strcmp(buf, "0") == 0) {
		*val = 0;
		return 1;
	}
	return 0;
}

static void expect_int(const char *label, int got, int want)
{
	if (got != want) {
		fprintf(stderr, "%s: got %d, want %d\n", label, got, want);
		failures++;
	}
}

static void expect_string(const char *label, const char *got, const char *want)
{
	if ((got == NULL && want != NULL) || (got != NULL && want == NULL) ||
			(got != NULL && want != NULL && strcmp(got, want) != 0)) {
		fprintf(stderr, "%s: got '%s', want '%s'\n",
				label,
				got ? got : "(null)",
				want ? want : "(null)");
		failures++;
	}
}

static void expect_ptr(const char *label, const void *got, const void *want)
{
	if (got != want) {
		fprintf(stderr, "%s: got %p, want %p\n", label, got, want);
		failures++;
	}
}

static void post_set_capture(void *data)
{
	post_set_calls++;
	last_post_set_data = data;
}

static void test_generic_int_option(void)
{
	int value = 1;
	char buf[OPTION_MAX_SIZE];

	option_add_int_full("test_int", &value, 0, 10, post_set_capture, 0);
	option_set("test_int", "7");

	expect_int("int set", value, 7);
	expect_int("int post_set calls", post_set_calls, 1);
	expect_ptr("int post_set data", last_post_set_data, &value);

	option_find("test_int")->get(option_find("test_int")->data, buf, sizeof(buf));
	expect_string("int get", buf, "7");

	option_set("test_int", "11");
	expect_int("int out of range unchanged", value, 7);
	expect_int("int invalid post_set unchanged", post_set_calls, 1);
}

static void test_generic_bool_option(void)
{
	int value = 0;
	char buf[OPTION_MAX_SIZE];
	struct termus_opt *opt;

	option_add_bool_full("test_bool", &value, post_set_capture, 0);
	opt = option_find("test_bool");
	opt->toggle(opt->data);

	expect_int("bool toggle", value, 1);
	expect_int("bool post_set calls", post_set_calls, 2);
	expect_ptr("bool post_set data", last_post_set_data, &value);

	opt->get(opt->data, buf, sizeof(buf));
	expect_string("bool get", buf, "true");
}

static void test_generic_str_option(void)
{
	char *value = NULL;
	char buf[OPTION_MAX_SIZE];
	struct termus_opt *opt;

	option_add_str_full("test_str", &value, post_set_capture, 0);
	option_set("test_str", "termus");

	expect_string("str set", value, "termus");
	expect_int("str post_set calls", post_set_calls, 3);
	expect_ptr("str post_set data", last_post_set_data, &value);

	opt = option_find("test_str");
	opt->get(opt->data, buf, sizeof(buf));
	expect_string("str get", buf, "termus");

	free(value);
}

int main(void)
{
	error_handler = quiet_msg_handler;

	test_generic_int_option();
	test_generic_bool_option();
	test_generic_str_option();

	if (failures != 0) {
		fprintf(stderr, "test-options: %d failure(s)\n", failures);
		return 1;
	}
	return 0;
}
