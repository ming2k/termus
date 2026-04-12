#include "common/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

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

static void expect_null(const char *label, const char *got)
{
	if (got != NULL) {
		fprintf(stderr, "%s: got '%s', want NULL\n", label, got);
		failures++;
	}
}

static void test_get_extension(void)
{
	expect_string("get_extension file", get_extension("song.flac"), "flac");
	expect_string("get_extension path", get_extension("/tmp/archive.tar.gz"), "gz");
	expect_null("get_extension missing", get_extension("playlist"));
}

static void test_basename_and_dirname(void)
{
	char *dirname_path = path_dirname("/tmp/music/song.flac");
	char *dirname_plain = path_dirname("song.flac");

	expect_string("path_basename path", path_basename("/tmp/music/song.flac"), "song.flac");
	expect_string("path_basename plain", path_basename("song.flac"), "song.flac");
	expect_string("path_dirname path", dirname_path, "/tmp/music");
	expect_string("path_dirname plain", dirname_plain, ".");

	free(dirname_path);
	free(dirname_plain);
}

static void test_path_strip(void)
{
	char buf1[] = "/tmp///music/./album/../track.flac/";
	char buf2[] = "artist//live/./set/../encore.flac";

	path_strip(buf1);
	path_strip(buf2);

	expect_string("path_strip absolute", buf1, "/tmp/music/track.flac");
	expect_string("path_strip relative", buf2, "artist/live/encore.flac");
}

static void test_path_absolute_cwd(void)
{
	char *absolute = path_absolute_cwd("../music//artist/./track.flac",
			"/home/test/user");

	expect_string("path_absolute_cwd", absolute,
			"/home/test/music/artist/track.flac");
	free(absolute);
}

int main(void)
{
	test_get_extension();
	test_basename_and_dirname();
	test_path_strip();
	test_path_absolute_cwd();

	if (failures != 0) {
		fprintf(stderr, "test-path: %d failure(s)\n", failures);
		return 1;
	}
	return 0;
}
