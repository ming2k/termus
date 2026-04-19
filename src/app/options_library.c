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

#include "app/options_library.h"
#include "app/options_library_state.h"
#include "app/options_parse.h"
#include "app/options_playback_state.h"
#include "app/options_registry.h"
#include "common/gbuf.h"
#include "common/msg.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "library/lib.h"
#include "ui/options_hooks.h"
#include "ui/options_ui.h"

#include <ctype.h>
#include <string.h>

#define DEFAULT_LIBRARY_COLUMNS "artist,title,duration"
#define DEFAULT_NOW_PLAYING_FIELDS "artist,title"

struct library_column_def {
	const char *name;
	const char *fixed_format;
	const char *flex_format;
};

struct now_playing_field_def {
	const char *name;
	const char *cond_key;
	const char *value_format;
};

static const struct library_column_def library_column_defs[] = {
    {"artist", "%-24%a", "%a"},
    {"albumartist", "%-24%A", "%A"},
    {"album", "%-20%l", "%l"},
    {"track", "%3n", "%n"},
    {"title", "%-32%t", "%t"},
    {"year", "%4y", "%y"},
    {"duration", "%d", "%d"},
    {"filename", "%-36%F", "%F"},
    {"genre", "%-16%g", "%g"},
    {NULL, NULL, NULL},
};

static const struct now_playing_field_def now_playing_field_defs[] = {
    {"artist", "artist", "%a"},
    {"albumartist", "albumartist", "%A"},
    {"album", "album", "%l"},
    {"track", "tracknumber", "%n"},
    {"title", "title", "%t"},
    {"year", "date", "%y"},
    {"duration", "duration", "%d"},
    {"filename", "filename", "%F"},
    {"genre", "genre", "%g"},
    {NULL, NULL, NULL},
};

static void post_set_sort_artists(void *data) { lib_sort_artists(); }

static void set_string(char **dst, const char *src)
{
	free(*dst);
	*dst = xstrdup(src);
}

static void get_aaa_mode(void *data, char *buf, size_t size)
{
	strscpy(buf, aaa_mode_names[aaa_mode], size);
}

static void set_aaa_mode(void *data, const char *buf)
{
	int value;

	if (!parse_enum(buf, 0, 2, aaa_mode_names, &value))
		return;

	aaa_mode = value;
	options_hooks_refresh_statusline();
}

static void toggle_aaa_mode(void *data)
{
	play_library = 1;
	aaa_mode++;
	aaa_mode %= 3;
	options_hooks_refresh_statusline();
}

static const struct library_column_def *find_library_column(const char *name)
{
	for (int i = 0; library_column_defs[i].name; i++) {
		if (strcmp(library_column_defs[i].name, name) == 0)
			return &library_column_defs[i];
	}
	return NULL;
}

static const struct now_playing_field_def *
find_now_playing_field(const char *name)
{
	for (int i = 0; now_playing_field_defs[i].name; i++) {
		if (strcmp(now_playing_field_defs[i].name, name) == 0)
			return &now_playing_field_defs[i];
	}
	return NULL;
}

static int next_csv_token(const char **cursor, char *token, size_t size)
{
	const char *p = *cursor;
	const char *start;
	const char *end;
	size_t len;

	while (*p == ',' || isspace((unsigned char)*p))
		p++;
	if (*p == '\0') {
		*cursor = p;
		return 0;
	}

	start = p;
	while (*p && *p != ',')
		p++;
	end = p;
	while (end > start && isspace((unsigned char)end[-1]))
		end--;

	len = end - start;
	if (len == 0 || len >= size)
		return -1;

	memcpy(token, start, len);
	token[len] = '\0';
	*cursor = p;
	return 1;
}

static int append_csv_name(struct gbuf *buf, const char *name)
{
	if (buf->len)
		gbuf_add_ch(buf, ',');
	gbuf_add_str(buf, name);
	return 1;
}

static char *build_library_format(
    const struct library_column_def *columns[], int nr_columns)
{
	GBUF(buf);
	const struct library_column_def *left_columns[16];
	const struct library_column_def *duration = NULL;
	int nr_left_columns = 0;

	for (int i = 0; i < nr_columns; i++) {
		if (strcmp(columns[i]->name, "duration") == 0)
			duration = columns[i];
		else
			left_columns[nr_left_columns++] = columns[i];
	}

	gbuf_add_ch(&buf, ' ');
	for (int i = 0; i < nr_left_columns; i++) {
		const char *format = (i == nr_left_columns - 1)
					 ? left_columns[i]->flex_format
					 : left_columns[i]->fixed_format;
		if (i)
			gbuf_add_ch(&buf, ' ');
		gbuf_add_str(&buf, format);
	}
	if (duration) {
		if (nr_left_columns)
			gbuf_add_str(&buf, "%= ");
		gbuf_add_str(&buf, duration->flex_format);
	}
	gbuf_add_ch(&buf, ' ');
	return gbuf_steal(&buf);
}

static char *build_now_playing_format(
    const struct now_playing_field_def *fields[], int nr_fields, int padded)
{
	GBUF(buf);
	GBUF(previous_conditions);

	if (padded)
		gbuf_add_ch(&buf, ' ');

	for (int i = 0; i < nr_fields; i++) {
		const struct now_playing_field_def *field = fields[i];

		if (previous_conditions.len == 0) {
			gbuf_addf(&buf, "%%{?%s?%s}", field->cond_key,
				  field->value_format);
		} else {
			gbuf_addf(&buf, "%%{?%s?%%{?%s? - }%s}", field->cond_key,
				  previous_conditions.buffer,
				  field->value_format);
		}

		if (previous_conditions.len)
			gbuf_add_ch(&previous_conditions, '|');
		gbuf_add_str(&previous_conditions, field->cond_key);
	}

	if (padded)
		gbuf_add_ch(&buf, ' ');
	gbuf_free(&previous_conditions);
	return gbuf_steal(&buf);
}

static void get_library_columns(void *data, char *buf, size_t size)
{
	(void)data;
	strscpy(buf, library_columns ? library_columns : "", size);
}

static void set_library_columns(void *data, const char *buf)
{
	(void)data;
	const char *cursor = buf;
	const struct library_column_def *columns[16];
	GBUF(normalized);
	int nr_columns = 0;
	char token[64];
	char *format;

	while (1) {
		int status = next_csv_token(&cursor, token, sizeof(token));
		if (status == 0)
			break;
		if (status < 0) {
			error_msg("library_columns expects comma separated names");
			goto out;
		}
		const struct library_column_def *column =
		    find_library_column(token);
		if (!column) {
			error_msg("unsupported library column: %s", token);
			goto out;
		}
		if (nr_columns == (int)(sizeof(columns) / sizeof(columns[0]))) {
			error_msg("too many library columns");
			goto out;
		}
		columns[nr_columns++] = column;
		append_csv_name(&normalized, column->name);
		if (*cursor == ',')
			cursor++;
	}

	if (nr_columns == 0) {
		error_msg("library_columns must not be empty");
		goto out;
	}

	format = build_library_format(columns, nr_columns);
	set_string(&library_columns, normalized.buffer);
	set_string(&list_win_format, format);
	set_string(&list_win_format_va, format);
	free(format);
	options_hooks_refresh_full();

out:
	gbuf_free(&normalized);
}

static void get_now_playing_fields(void *data, char *buf, size_t size)
{
	(void)data;
	strscpy(buf, now_playing_fields ? now_playing_fields : "", size);
}

static void set_now_playing_fields(void *data, const char *buf)
{
	(void)data;
	const char *cursor = buf;
	const struct now_playing_field_def *fields[16];
	GBUF(normalized);
	int nr_fields = 0;
	char token[64];
	char *current_line;
	char *window_title;

	while (1) {
		int status = next_csv_token(&cursor, token, sizeof(token));
		if (status == 0)
			break;
		if (status < 0) {
			error_msg(
			    "now_playing_fields expects comma separated names");
			goto out;
		}
		const struct now_playing_field_def *field =
		    find_now_playing_field(token);
		if (!field) {
			error_msg("unsupported now playing field: %s", token);
			goto out;
		}
		if (nr_fields == (int)(sizeof(fields) / sizeof(fields[0]))) {
			error_msg("too many now playing fields");
			goto out;
		}
		fields[nr_fields++] = field;
		append_csv_name(&normalized, field->name);
		if (*cursor == ',')
			cursor++;
	}

	if (nr_fields == 0) {
		error_msg("now_playing_fields must not be empty");
		goto out;
	}

	current_line = build_now_playing_format(fields, nr_fields, 1);
	window_title = build_now_playing_format(fields, nr_fields, 0);
	set_string(&now_playing_fields, normalized.buffer);
	set_string(&current_format, current_line);
	set_string(&window_title_format, window_title);
	free(current_line);
	free(window_title);
	options_hooks_refresh_full();

out:
	gbuf_free(&normalized);
}

void options_add_library_options(void)
{
	option_add("aaa_mode", NULL, get_aaa_mode, set_aaa_mode,
		   toggle_aaa_mode, 0);
	option_add("library_columns", NULL, get_library_columns,
		   set_library_columns, NULL, 0);
	option_add("now_playing_fields", NULL, get_now_playing_fields,
		   set_now_playing_fields, NULL, 0);
	option_add_bool_full("smart_artist_sort", &smart_artist_sort,
			     post_set_sort_artists, 0);
}

void options_library_load_defaults(void)
{
	option_set("library_columns", DEFAULT_LIBRARY_COLUMNS);
	option_set("now_playing_fields", DEFAULT_NOW_PLAYING_FIELDS);
}
