/*
 * Copyright 2008-2013 Various Authors
 * Copyright 2006 Timo Hirvonen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "app/options_core.h"
#include "app/options_core_state.h"
#include "app/options_registry.h"
#include "app/options_ui_state.h"
#include "app/options_plugins.h"
#include "common/misc.h"
#include "common/msg.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "core/buffer.h"
#include "core/player.h"
#include "library/lib.h"
#include "library/pl.h"
#include "ui/options_hooks.h"

#include <stdio.h>
#include <string.h>

static void options_buf_int(char *buf, int val, size_t size)
{
	snprintf(buf, size, "%d", val);
}

static int options_parse_int(const char *buf, int minval, int maxval, int *val)
{
	long int tmp;

	if (str_to_int(buf, &tmp) == -1 || tmp < minval || tmp > maxval) {
		error_msg("integer in range %d..%d expected", minval, maxval);
		return 0;
	}
	*val = tmp;
	return 1;
}

static void get_device(void *data, char *buf, size_t size)
{
	strscpy(buf, cdda_device, size);
}

static void set_device(void *data, const char *buf)
{
	free(cdda_device);
	cdda_device = expand_filename(buf);
}

#define SECOND_SIZE (44100 * 16 / 8 * 2)

static void get_buffer_seconds(void *data, char *buf, size_t size)
{
	int val = (player_get_buffer_chunks() * CHUNK_SIZE + SECOND_SIZE / 2) /
		SECOND_SIZE;
	options_buf_int(buf, val, size);
}

static void set_buffer_seconds(void *data, const char *buf)
{
	int sec;

	if (options_parse_int(buf, 1, 300, &sec))
		player_set_buffer_chunks((sec * SECOND_SIZE + CHUNK_SIZE / 2) / CHUNK_SIZE);
}

static void get_lib_sort(void *data, char *buf, size_t size)
{
	strscpy(buf, lib_editable.shared->sort_str, size);
}

static void set_lib_sort(void *data, const char *buf)
{
	sort_key_t *keys = parse_sort_keys(buf);

	if (keys) {
		editable_shared_set_sort_keys(lib_editable.shared, keys);
		editable_sort(&lib_editable);
		sort_keys_to_str(keys, lib_editable.shared->sort_str,
				sizeof(lib_editable.shared->sort_str));
	}
}

static void get_pl_sort(void *data, char *buf, size_t size)
{
	pl_get_sort_str(buf, size);
}

static void set_pl_sort(void *data, const char *buf)
{
	pl_set_sort_str(buf);
}

static void get_output_plugin(void *data, char *buf, size_t size)
{
	const char *value = options_get_current_output_plugin();

	if (value)
		strscpy(buf, value, size);
}

static void set_output_plugin(void *data, const char *buf)
{
	options_hooks_switch_output_plugin(buf);
}

static void get_passwd(void *data, char *buf, size_t size)
{
	const char *server_password = options_get_server_password();

	if (server_password)
		strscpy(buf, server_password, size);
}

static void set_passwd(void *data, const char *buf)
{
	int len = strlen(buf);

	if (len == 0) {
		options_set_server_password(NULL);
	} else if (len < 6) {
		error_msg("unsafe password");
	} else {
		options_set_server_password(buf);
	}
}

static void get_tree_width_percent(void *data, char *buf, size_t size)
{
	options_buf_int(buf, options_get_tree_width_percent(), size);
}

static void set_tree_width_percent(void *data, const char *buf)
{
	int percent;

	if (options_parse_int(buf, 1, 100, &percent))
		options_set_tree_width_percent(percent);
	options_hooks_refresh_layout();
}

static void get_tree_width_max(void *data, char *buf, size_t size)
{
	options_buf_int(buf, options_get_tree_width_max(), size);
}

static void set_tree_width_max(void *data, const char *buf)
{
	int cols;

	if (options_parse_int(buf, 0, 9999, &cols))
		options_set_tree_width_max(cols);
	options_hooks_refresh_layout();
}

static void get_pl_env_vars(void *data, char *buf, size_t size)
{
	if (!pl_env_vars) {
		strscpy(buf, "", size);
		return;
	}

	char *p = buf;
	size_t r = size - 1;

	for (char **x = pl_env_vars; *x; x++) {
		if (x != pl_env_vars) {
			if (!--r)
				return;
			*p++ = ',';
		}
		size_t l = strlen(*x);
		if (!(r -= l))
			return;
		strcpy(p, *x);
		p += l;
	}
	*p = '\0';
}

static void set_pl_env_vars(void *data, const char *buf)
{
	if (pl_env_vars) {
		free(*pl_env_vars);
		free(pl_env_vars);
	}
	if (!*buf)
		pl_env_vars = NULL;

	size_t n = 1;
	for (const char *x = buf; *x; x++) {
		if (*x == ',')
			n++;
	}

	char **a = pl_env_vars = xnew(char *, n + 1);
	for (char *x = *a++ = xstrdup(buf); *x; x++) {
		if (*x == ',') {
			*a++ = x + 1;
			*x = '\0';
		}
	}
	*a = NULL;
}

#define DN(name) option_add(#name, NULL, get_ ## name, set_ ## name, NULL, 0)
#define DN_FLAGS(name, flags) option_add(#name, NULL, get_ ## name, set_ ## name, NULL, flags)

void options_add_core_options(void)
{
	DN_FLAGS(device, OPT_PROGRAM_PATH);
	DN(buffer_seconds);
	option_add_int("scroll_offset", &scroll_offset, 0, 9999, 0);
	option_add_int("rewind_offset", &rewind_offset, -1, 9999, 0);
	option_add_str("id3_default_charset", &id3_default_charset, 0);
	option_add_str("icecast_default_charset", &icecast_default_charset, 0);
	DN(lib_sort);
	DN(output_plugin);
	DN(passwd);
	DN(pl_sort);
	DN(tree_width_percent);
	DN(tree_width_max);
	DN(pl_env_vars);
}
