#include "common/misc.h"
#include "common/prog.h"
#include "common/xmalloc.h"
#include "common/xstrjoin.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char *termus_config_dir = NULL;
const char *termus_state_dir = NULL;
const char *termus_cache_dir = NULL;
const char *termus_user_data_dir = NULL;
const char *termus_playlist_dir = NULL;
const char *termus_socket_path = NULL;
const char *termus_data_dir = NULL;
const char *termus_lib_dir = NULL;
const char *home_dir = NULL;

char **get_words(const char *text)
{
	char **words;
	int i, j, count;

	while (*text == ' ')
		text++;

	count = 0;
	i = 0;
	while (text[i]) {
		count++;
		while (text[i] && text[i] != ' ')
			i++;
		while (text[i] == ' ')
			i++;
	}
	words = xnew(char *, count + 1);

	i = 0;
	j = 0;
	while (text[i]) {
		int start = i;

		while (text[i] && text[i] != ' ')
			i++;
		words[j++] = xstrndup(text + start, i - start);
		while (text[i] == ' ')
			i++;
	}
	words[j] = NULL;
	return words;
}

int strptrcmp(const void *a, const void *b)
{
	const char *as = *(char **)a;
	const char *bs = *(char **)b;

	return strcmp(as, bs);
}

int strptrcoll(const void *a, const void *b)
{
	const char *as = *(char **)a;
	const char *bs = *(char **)b;

	return strcoll(as, bs);
}

const char *escape(const char *str)
{
	static char *buf = NULL;
	static size_t alloc = 0;
	size_t len = strlen(str);
	size_t need = len * 2 + 1;
	int s, d;

	if (need > alloc) {
		alloc = (need + 16) & ~(16 - 1);
		buf = xrealloc(buf, alloc);
	}

	d = 0;
	for (s = 0; str[s]; s++) {
		if (str[s] == '\\') {
			buf[d++] = '\\';
			buf[d++] = '\\';
			continue;
		}
		if (str[s] == '\n') {
			buf[d++] = '\\';
			buf[d++] = 'n';
			continue;
		}
		buf[d++] = str[s];
	}
	buf[d] = 0;
	return buf;
}

const char *unescape(const char *str)
{
	static char *buf = NULL;
	static size_t alloc = 0;
	size_t need = strlen(str) + 1;
	int do_escape = 0;
	int s, d;

	if (need > alloc) {
		alloc = (need + 16) & ~(16 - 1);
		buf = xrealloc(buf, alloc);
	}

	d = 0;
	for (s = 0; str[s]; s++) {
		if (!do_escape && str[s] == '\\')
			do_escape = 1;
		else {
			buf[d++] = (do_escape && str[s] == 'n') ? '\n' : str[s];
			do_escape = 0;
		}
	}
	buf[d] = 0;
	return buf;
}

static int dir_exists(const char *dirname)
{
	DIR *dir;

	dir = opendir(dirname);
	if (dir == NULL) {
		if (errno == ENOENT)
			return 0;
		return -1;
	}
	closedir(dir);
	return 1;
}

static void make_dir(const char *dirname)
{
	int rc;

	rc = dir_exists(dirname);
	if (rc == 1)
		return;
	if (rc == -1)
		die_errno("error: opening `%s'", dirname);
	rc = mkdir(dirname, 0700);
	if (rc == -1)
		die_errno("error: creating directory `%s'", dirname);
}

static void make_dir_recursive(const char *dirname)
{
	char *parent, *slash;

	if (!dirname || !dirname[0] || strcmp(dirname, "/") == 0)
		return;

	if (dir_exists(dirname) == 1)
		return;

	parent = xstrdup(dirname);
	slash = strrchr(parent, '/');
	if (slash && slash != parent) {
		*slash = '\0';
		make_dir_recursive(parent);
	} else if (slash == parent) {
		parent[1] = '\0';
	}
	free(parent);

	make_dir(dirname);
}

static char *get_non_empty_env(const char *name)
{
	const char *val;

	val = getenv(name);
	if (val == NULL || val[0] == 0)
		return NULL;
	return xstrdup(val);
}

static char *get_non_empty_absolute_env(const char *name)
{
	char *val = get_non_empty_env(name);

	if (val == NULL)
		return NULL;
	if (val[0] != '/') {
		free(val);
		return NULL;
	}
	return val;
}

const char *get_filename(const char *path)
{
	const char *file = strrchr(path, '/');
	if (!file)
		file = path;
	else
		file++;
	if (!*file)
		return NULL;
	return file;
}

int misc_init(void)
{
	char *xdg_runtime_dir = get_non_empty_absolute_env("XDG_RUNTIME_DIR");
	char *termus_home = get_non_empty_env("TERMUS_HOME");
	char *xdg_config_home = NULL;
	char *xdg_state_home = NULL;
	char *xdg_cache_home = NULL;
	char *xdg_data_home = NULL;

	home_dir = get_non_empty_env("HOME");
	if (home_dir == NULL)
		die("error: environment variable HOME not set\n");

	if (termus_home) {
		make_dir_recursive(termus_home);
		termus_config_dir = xstrjoin(termus_home, "/config");
		termus_state_dir = xstrjoin(termus_home, "/state");
		termus_cache_dir = xstrjoin(termus_home, "/cache");
		termus_user_data_dir = xstrjoin(termus_home, "/data");
	} else {
		xdg_config_home = get_non_empty_absolute_env("XDG_CONFIG_HOME");
		if (xdg_config_home == NULL)
			xdg_config_home = xstrjoin(home_dir, "/.config");

		xdg_state_home = get_non_empty_absolute_env("XDG_STATE_HOME");
		if (xdg_state_home == NULL)
			xdg_state_home = xstrjoin(home_dir, "/.local/state");

		xdg_cache_home = get_non_empty_absolute_env("XDG_CACHE_HOME");
		if (xdg_cache_home == NULL)
			xdg_cache_home = xstrjoin(home_dir, "/.cache");

		xdg_data_home = get_non_empty_absolute_env("XDG_DATA_HOME");
		if (xdg_data_home == NULL)
			xdg_data_home = xstrjoin(home_dir, "/.local/share");

		make_dir_recursive(xdg_config_home);
		make_dir_recursive(xdg_state_home);
		make_dir_recursive(xdg_cache_home);
		make_dir_recursive(xdg_data_home);

		termus_config_dir = xstrjoin(xdg_config_home, "/termus");
		termus_state_dir = xstrjoin(xdg_state_home, "/termus");
		termus_cache_dir = xstrjoin(xdg_cache_home, "/termus");
		termus_user_data_dir = xstrjoin(xdg_data_home, "/termus");
	}
	make_dir_recursive(termus_config_dir);
	make_dir_recursive(termus_state_dir);
	make_dir_recursive(termus_cache_dir);
	make_dir_recursive(termus_user_data_dir);

	termus_playlist_dir = get_non_empty_env("TERMUS_PLAYLIST_DIR");
	if (!termus_playlist_dir)
		termus_playlist_dir =
		    xstrjoin(termus_user_data_dir, "/playlists");
	make_dir_recursive(termus_playlist_dir);

	termus_socket_path = get_non_empty_env("TERMUS_SOCKET");
	if (termus_socket_path == NULL) {
		if (xdg_runtime_dir == NULL) {
			termus_socket_path =
			    xstrjoin(termus_config_dir, "/socket");
		} else {
			termus_socket_path =
			    xstrjoin(xdg_runtime_dir, "/termus-socket");
		}
	}

	termus_lib_dir = getenv("TERMUS_LIB_DIR");
	if (!termus_lib_dir)
		termus_lib_dir = LIBDIR "/termus";

	termus_data_dir = getenv("TERMUS_DATA_DIR");
	if (!termus_data_dir)
		termus_data_dir = DATADIR "/termus";

	free(termus_home);
	free(xdg_config_home);
	free(xdg_state_home);
	free(xdg_cache_home);
	free(xdg_data_home);
	free(xdg_runtime_dir);
	return 0;
}

int replaygain_decode(unsigned int field, int *gain)
{
	unsigned int name_code, originator_code, sign_bit, val;

	name_code = (field >> 13) & 0x7;
	if (!name_code || name_code > 2)
		return 0;
	originator_code = (field >> 10) & 0x7;
	if (!originator_code)
		return 0;
	sign_bit = (field >> 9) & 0x1;
	val = field & 0x1ff;
	if (sign_bit && !val)
		return 0;
	*gain = (sign_bit ? -1 : 1) * val;
	return name_code;
}

static char *get_home_dir(const char *username)
{
	struct passwd *passwd;

	if (username == NULL)
		return xstrdup(home_dir);
	passwd = getpwnam(username);
	if (passwd == NULL)
		return NULL;
	/* don't free passwd */
	return xstrdup(passwd->pw_dir);
}

char *expand_filename(const char *name)
{
	if (name[0] == '~') {
		char *slash;

		slash = strchr(name, '/');
		if (slash) {
			char *username, *home;

			if (slash - name - 1 > 0) {
				/* ~user/... */
				username = xstrndup(name + 1, slash - name - 1);
			} else {
				/* ~/... */
				username = NULL;
			}
			home = get_home_dir(username);
			free(username);
			if (home) {
				char *expanded;

				expanded = xstrjoin(home, slash);
				free(home);
				return expanded;
			} else {
				return xstrdup(name);
			}
		} else {
			if (name[1] == 0) {
				return xstrdup(home_dir);
			} else {
				char *home;

				home = get_home_dir(name + 1);
				if (home)
					return home;
				return xstrdup(name);
			}
		}
	} else {
		return xstrdup(name);
	}
}

void shuffle_array(void *array, size_t n, size_t size)
{
	char tmp[size];
	char *arr = array;
	for (ssize_t i = 0; i < (ssize_t)n - 1; ++i) {
		size_t rnd = (size_t)rand();
		size_t j = i + rnd / (RAND_MAX / (n - i) + 1);
		memcpy(tmp, arr + j * size, size);
		memcpy(arr + j * size, arr + i * size, size);
		memcpy(arr + i * size, tmp, size);
	}
}
