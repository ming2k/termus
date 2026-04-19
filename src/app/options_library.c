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
#include "common/utils.h"
#include "library/lib.h"
#include "ui/options_hooks.h"

#include <string.h>

static void post_set_sort_artists(void *data) { lib_sort_artists(); }

static void post_set_refresh_statusline(void *data)
{
	options_hooks_refresh_statusline();
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

void options_add_library_options(void)
{
	option_add("aaa_mode", NULL, get_aaa_mode, set_aaa_mode,
		   toggle_aaa_mode, 0);
	option_add_bool_full("smart_artist_sort", &smart_artist_sort,
			     post_set_sort_artists, 0);
}
