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

#include "app/options_misc.h"
#include "app/options_library_state.h"
#include "app/options_playback_state.h"
#include "app/options_ui_state.h"
#include "app/options_registry.h"
#include "app/options_parse.h"
#include "common/misc.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "library/expr.h"
#include "library/lib.h"
#include "ui/options_hooks.h"

#include <string.h>

char *lib_add_filter = NULL;

static void get_lib_add_filter(void *data, char *buf, size_t size)
{
	if (lib_add_filter)
		strscpy(buf, lib_add_filter, size);
	else
		buf[0] = 0;
}

static void set_mouse(void *data, const char *buf)
{
	int value;

	if (options_parse_bool(buf, &value))
		mouse = value;
	update_mouse();
}

static void toggle_mouse(void *data)
{
	mouse = !mouse;
	update_mouse();
}

static void set_status_display_program(void *data, const char *buf)
{
	char *expanded = NULL;

	if (buf[0])
		expanded = expand_filename(buf);
	free(status_display_program);
	status_display_program = expanded;
}

static void set_status_display_file(void *data, const char *buf)
{
	char *expanded = NULL;

	if (buf[0])
		expanded = expand_filename(buf);
	free(status_display_file);
	status_display_file = expanded;
}

static void set_lib_add_filter(void *data, const char *buf)
{
	struct expr *expr = NULL;

	if (strlen(buf) != 0) {
		expr = expr_parse(buf);
		if (!expr)
			return;
	}

	free(lib_add_filter);
	lib_add_filter = xstrdup(buf);
	lib_set_add_filter(expr);
}

static void get_progress_bar(void *data, char *buf, size_t size)
{
	strscpy(buf, progress_bar_names[progress_bar], size);
}

static void set_progress_bar(void *data, const char *buf)
{
	int value;

	if (parse_enum(buf, 0, NR_PROGRESS_BAR_MODES - 1, progress_bar_names, &value))
		progress_bar = value;
}

static void toggle_progress_bar(void *data)
{
	progress_bar = (progress_bar + 1) % NR_PROGRESS_BAR_MODES;
	options_hooks_refresh_statusline();
}

void options_add_misc_options(void)
{
	option_add_bool("auto_hide_playlists_panel", &auto_hide_playlists_panel, 0);
	option_add_bool("auto_reshuffle", &auto_reshuffle, 0);
	option_add_bool("block_key_paste", &block_key_paste, 0);
	option_add_bool("confirm_run", &confirm_run, 0);
	option_add_bool("ignore_duplicates", &ignore_duplicates, 0);
	option_add("lib_add_filter", &lib_add_filter, get_lib_add_filter, set_lib_add_filter, NULL, 0);
	option_add("mouse", NULL, NULL, set_mouse, toggle_mouse, 0);
	option_add_bool("mpris", &mpris, 0);
	option_add_bool("pause_on_output_change", &pause_on_output_change, 0);
	option_add("progress_bar", NULL, get_progress_bar, set_progress_bar, toggle_progress_bar, 0);
	option_add_bool("resume", &resume_termus, 0);
	option_add_bool("search_resets_position", &search_resets_position, 0);
	option_add_bool("set_term_title", &set_term_title, 0);
	option_add_bool("skip_track_info", &skip_track_info, 0);
	option_add("status_display_file", &status_display_file, NULL, set_status_display_file, NULL, OPT_PROGRAM_PATH);
	option_add("status_display_program", &status_display_program, NULL, set_status_display_program, NULL, OPT_PROGRAM_PATH);
	option_add_bool("stop_after_queue", &stop_after_queue, 0);
	option_add_bool("wrap_search", &wrap_search, 0);
}
