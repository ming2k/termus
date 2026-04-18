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

static void post_set_tree_changed(void *data)
{
	options_hooks_mark_library_tree_changed();
}

static void post_set_reload_browser(void *data)
{
	options_hooks_reload_browser();
}

static void post_set_refresh_statusline(void *data)
{
	options_hooks_refresh_statusline();
}

static void set_show_all_tracks_int(int value);

static void set_auto_expand_albums_follow_int(int value)
{
	auto_expand_albums_follow = !!value;
	if (!auto_expand_albums_follow && !show_all_tracks)
		set_show_all_tracks_int(1);
}

static void post_set_auto_expand_follow(void *data)
{
	set_auto_expand_albums_follow_int(auto_expand_albums_follow);
}

static void set_auto_expand_albums_search_int(int value)
{
	auto_expand_albums_search = !!value;
	if (!auto_expand_albums_search && !show_all_tracks)
		set_show_all_tracks_int(1);
}

static void post_set_auto_expand_search(void *data)
{
	set_auto_expand_albums_search_int(auto_expand_albums_search);
}

static void set_auto_expand_albums_selcur_int(int value)
{
	auto_expand_albums_selcur = !!value;
	if (!auto_expand_albums_selcur && !show_all_tracks)
		set_show_all_tracks_int(1);
}

static void post_set_auto_expand_selcur(void *data)
{
	set_auto_expand_albums_selcur_int(auto_expand_albums_selcur);
}

static void set_show_all_tracks_int(int value)
{
	value = !!value;
	if (show_all_tracks == value)
		return;

	show_all_tracks = value;
	if (!show_all_tracks) {
		if (!auto_expand_albums_follow)
			set_auto_expand_albums_follow_int(1);
		if (!auto_expand_albums_search)
			set_auto_expand_albums_search_int(1);
		if (!auto_expand_albums_selcur)
			set_auto_expand_albums_selcur_int(1);
	}
	options_hooks_update_tree_selection();
}

static void post_set_show_all_tracks(void *data)
{
	set_show_all_tracks_int(show_all_tracks);
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
	option_add_bool_full("auto_expand_albums_follow",
			     &auto_expand_albums_follow,
			     post_set_auto_expand_follow, 0);
	option_add_bool_full("auto_expand_albums_search",
			     &auto_expand_albums_search,
			     post_set_auto_expand_search, 0);
	option_add_bool_full("auto_expand_albums_selcur",
			     &auto_expand_albums_selcur,
			     post_set_auto_expand_selcur, 0);
	option_add_bool_full("display_artist_sort_name",
			     &display_artist_sort_name, post_set_tree_changed,
			     0);
	option_add_bool_full("show_all_tracks", &show_all_tracks,
			     post_set_show_all_tracks, 0);
	option_add_bool_full("show_hidden", &show_hidden,
			     post_set_reload_browser, 0);
	option_add_bool_full("smart_artist_sort", &smart_artist_sort,
			     post_set_sort_artists, 0);
	option_add_bool_full("sort_albums_by_name", &sort_albums_by_name,
			     post_set_sort_artists, 0);
}
