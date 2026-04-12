#include "common/locale.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void expect_string(const char *label, const char *got, const char *want)
{
	if (strcmp(got, want) != 0) {
		fprintf(stderr, "%s: got '%s', want '%s'\n", label, got, want);
		failures++;
	}
}

static void expect_int(const char *label, int got, int want)
{
	if (got != want) {
		fprintf(stderr, "%s: got %d, want %d\n", label, got, want);
		failures++;
	}
}

int main(void)
{
	locale_init_charset("UTF-8");
	expect_string("utf8 charset", charset, "UTF-8");
	expect_int("utf8 flag", using_utf8, 1);

	locale_init_charset("GB18030");
	expect_string("legacy charset", charset, "GB18030");
	expect_int("legacy flag", using_utf8, 0);

	locale_init_charset("");
	expect_string("empty charset fallback", charset, "ISO-8859-1");
	expect_int("empty charset flag", using_utf8, 0);

	locale_init_charset(NULL);
	expect_string("null charset fallback", charset, "ISO-8859-1");
	expect_int("null charset flag", using_utf8, 0);

	if (failures != 0) {
		fprintf(stderr, "test-locale: %d failure(s)\n", failures);
		return 1;
	}
	return 0;
}
