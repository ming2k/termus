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

#include "ui/options_display.h"
#include "app/options_parse.h"
#include "app/options_registry.h"
#include "app/options_ui_state.h"
#include "common/misc.h"
#include "common/prog.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "ui/options_hooks.h"
#include "ui/options_ui.h"
#include "ui/ui.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(__sun__)
#include <ncurses.h>
#else
#include <curses.h>
#endif

enum format_id {
	FMT_CLIPPED_TEXT,
	FMT_CURRENT_ALT,
	FMT_CURRENT,
	FMT_HEADING_ALBUM,
	FMT_HEADING_ARTIST,
	FMT_HEADING_PLAYLIST,
	FMT_STATUSLINE,
	FMT_PLAYLIST,
	FMT_PLAYLIST_ALT,
	FMT_PLAYLIST_VA,
	FMT_TITLE,
	FMT_TITLE_ALT,
	FMT_TRACKWIN,
	FMT_TRACKWIN_ALBUM,
	FMT_TRACKWIN_ALT,
	FMT_TRACKWIN_VA,
	FMT_TREEWIN,
	FMT_TREEWIN_ARTIST,

	NR_FMTS
};

static const struct {
	const char *name;
	const char *value;
} str_defaults[] = {
    [FMT_CLIPPED_TEXT] = {"format_clipped_text", "…"},
    [FMT_CURRENT_ALT] = {"altformat_current", " %F "},
    [FMT_CURRENT] = {"format_current", " %a - %l%! - %n. %t%= %y "},
    [FMT_HEADING_ALBUM] = {"format_heading_album", "%a - %l%= %y %{duration}"},
    [FMT_HEADING_ARTIST] = {"format_heading_artist", "%a%= %{duration}"},
    [FMT_HEADING_PLAYLIST] =
	{"format_heading_playlist",
	 "%{?!panel?Playlist - }%{title}%= %{duration}    "},
    [FMT_STATUSLINE] =
	{"format_statusline",
	 " %{status} %{?show_playback_position?%{position} %{?duration?/ "
	 "%{duration} }?%{?duration?%{duration} }}"
	 "%{?bpm>0?at %{bpm} BPM }"
	 "%{?stream?buf: %{buffer} }"
	 "%{?show_current_bitrate & bitrate>=0? %{bitrate} kbps }"
	 "%{speed}x "
	 "%= "
	 "%{?volume>=0?vol %{?lvolume!=rvolume?%{lvolume}%% "
	 "%{rvolume}?%{volume}}%% | }"
	 "%1{continue}%1{follow}%1{repeat}%1{shuffle} "},
    [FMT_PLAYLIST_ALT] = {"altformat_playlist", " %f%= %d %{?X!=0?%3X ?    }"},
    [FMT_PLAYLIST] = {"format_playlist",
		      " %-21%a %3n. %t%= %y %d %{?X!=0?%3X ?    }"},
    [FMT_PLAYLIST_VA] = {"format_playlist_va",
			 " %-21%A %3n. %t (%a)%= %y %d %{?X!=0?%3X ?    }"},
    [FMT_TITLE_ALT] = {"altformat_title", "%f"},
    [FMT_TITLE] = {"format_title", "%a - %l - %t (%y)"},
    [FMT_TRACKWIN_ALBUM] = {"format_trackwin_album", " %l %= %y %{duration} "},
    [FMT_TRACKWIN_ALT] = {"altformat_trackwin", " %f%= %d "},
    [FMT_TRACKWIN] = {"format_trackwin", "%3n. %t%= %d "},
    [FMT_TRACKWIN_VA] = {"format_trackwin_va", "%3n. %t (%a)%= %d "},
    [FMT_TREEWIN] = {"format_treewin", "  %l"},
    [FMT_TREEWIN_ARTIST] = {"format_treewin_artist", "%a"},

    [NR_FMTS] =

	{"lib_sort", "albumartist date album discnumber tracknumber title "
		     "filename play_count"},
    {"pl_sort", ""},
    {"id3_default_charset", "ISO-8859-1"},
    {"icecast_default_charset", "ISO-8859-1"},
    {"pl_env_vars", ""},
    {NULL, NULL}};

#define DEFAULT_COLORSCHEME_NAME "default"

static const char *const color_enum_names[1 + 8 * 2 + 1] = {
    "default",	 "black",      "red",	      "green",	   "yellow",
    "blue",	 "magenta",    "cyan",	      "gray",	   "darkgray",
    "lightred",	 "lightgreen", "lightyellow", "lightblue", "lightmagenta",
    "lightcyan", "white",      NULL};

static const char *const color_names[NR_COLORS] = {
    "color_cmdline_bg",
    "color_cmdline_fg",
    "color_error",
    "color_info",
    "color_separator",
    "color_statusline_bg",
    "color_statusline_fg",
    "color_statusline_progress_bg",
    "color_statusline_progress_fg",
    "color_titleline_bg",
    "color_titleline_fg",
    "color_win_bg",
    "color_win_cur",
    "color_win_cur_sel_bg",
    "color_win_cur_sel_fg",
    "color_win_dir",
    "color_win_fg",
    "color_win_inactive_cur_sel_bg",
    "color_win_inactive_cur_sel_fg",
    "color_win_inactive_sel_bg",
    "color_win_inactive_sel_fg",
    "color_win_sel_bg",
    "color_win_sel_fg",
    "color_win_title_bg",
    "color_win_title_fg",
    "color_trackwin_album_bg",
    "color_trackwin_album_fg",
};

static const char *const attr_names[NR_ATTRS] = {
    "color_cmdline_attr",
    "color_statusline_attr",
    "color_statusline_progress_attr",
    "color_titleline_attr",
    "color_win_attr",
    "color_win_cur_sel_attr",
    "color_cur_sel_attr",
    "color_win_inactive_cur_sel_attr",
    "color_win_inactive_sel_attr",
    "color_win_sel_attr",
    "color_win_title_attr",
    "color_trackwin_album_attr",
    "color_win_cur_attr",
};

static void options_display_buf_int(char *buf, int val, size_t size)
{
	snprintf(buf, size, "%d", val);
}

static char **id_to_fmt(enum format_id id)
{
	switch (id) {
	case FMT_CLIPPED_TEXT:
		return &clipped_text_format;
	case FMT_CURRENT_ALT:
		return &current_alt_format;
	case FMT_PLAYLIST_ALT:
		return &list_win_alt_format;
	case FMT_TITLE_ALT:
		return &window_title_alt_format;
	case FMT_TRACKWIN_ALT:
		return &track_win_alt_format;
	case FMT_CURRENT:
		return &current_format;
	case FMT_HEADING_ALBUM:
		return &heading_album_format;
	case FMT_HEADING_ARTIST:
		return &heading_artist_format;
	case FMT_HEADING_PLAYLIST:
		return &heading_playlist_format;
	case FMT_PLAYLIST:
		return &list_win_format;
	case FMT_PLAYLIST_VA:
		return &list_win_format_va;
	case FMT_TITLE:
		return &window_title_format;
	case FMT_TRACKWIN:
		return &track_win_format;
	case FMT_TRACKWIN_ALBUM:
		return &track_win_album_format;
	case FMT_TRACKWIN_VA:
		return &track_win_format_va;
	case FMT_TREEWIN:
		return &tree_win_format;
	case FMT_TREEWIN_ARTIST:
		return &tree_win_artist_format;
	case FMT_STATUSLINE:
		return &statusline_format;
	default:
		die("unhandled format code: %d\n", id);
	}
	return NULL;
}

static void get_color(void *data, char *buf, size_t size)
{
	int val = *(int *)data;

	if (val < 16)
		strscpy(buf, color_enum_names[val + 1], size);
	else
		options_display_buf_int(buf, val, size);
}

static void set_color(void *data, const char *buf)
{
	int color;

	if (!parse_enum(buf, -1, 255, color_enum_names, &color))
		return;

	*(int *)data = color;
	options_set_colorscheme(NULL);
	options_hooks_refresh_colors_full();
}

int options_display_apply_colorscheme(const char *name)
{
	char filename[512];

	if (name == NULL || name[0] == '\0') {
		options_set_colorscheme(NULL);
		return 0;
	}

	snprintf(filename, sizeof(filename), "%s/%s.theme", termus_config_dir,
		 name);
	if (source_file(filename) == -1) {
		snprintf(filename, sizeof(filename), "%s/%s.theme",
			 termus_data_dir, name);
		if (source_file(filename) == -1)
			return -1;
	}

	options_set_colorscheme(name);
	return 0;
}

static void get_colorscheme(void *data, char *buf, size_t size)
{
	strscpy(buf, options_get_colorscheme(), size);
}

static void set_colorscheme(void *data, const char *buf)
{
	if (options_display_apply_colorscheme(buf) == -1)
		error_msg("loading colorscheme %s: %s", buf, strerror(errno));
}

static void get_start_view(void *data, char *buf, size_t size)
{
	strscpy(buf, view_names[options_get_start_view()], size);
}

static void set_start_view(void *data, const char *buf)
{
	int view;

	if (parse_enum(buf, 0, NR_VIEWS - 1, view_names, &view))
		options_set_start_view(view);
}

static void get_attr(void *data, char *buf, size_t size)
{
	int attr = *(int *)data;

	if (attr == 0) {
		strscpy(buf, "default", size);
		return;
	}

	const char *standout = "";
	const char *underline = "";
	const char *reverse = "";
	const char *blink = "";
	const char *bold = "";
	const char *italic = "";

	if (attr & A_STANDOUT)
		standout = "standout|";
	if (attr & A_UNDERLINE)
		underline = "underline|";
	if (attr & A_REVERSE)
		reverse = "reverse|";
	if (attr & A_BLINK)
		blink = "blink|";
	if (attr & A_BOLD)
		bold = "bold|";
#if HAVE_ITALIC
	if (attr & A_ITALIC)
		italic = "italic|";
#endif

	size_t len = snprintf(buf, size, "%s%s%s%s%s%s", standout, underline,
			      reverse, blink, bold, italic);

	if (0 < len && len < size)
		buf[len - 1] = 0;
}

static void set_attr(void *data, const char *buf)
{
	int attr = 0;
	size_t i = 0;
	size_t offset = 0;
	size_t length = 0;
	char *current;

	do {
		if (buf[i] == '|' || buf[i] == '\0') {
			current = xstrndup(&buf[offset], length);

			if (strcmp(current, "default") == 0)
				attr |= A_NORMAL;
			else if (strcmp(current, "standout") == 0)
				attr |= A_STANDOUT;
			else if (strcmp(current, "underline") == 0)
				attr |= A_UNDERLINE;
			else if (strcmp(current, "reverse") == 0)
				attr |= A_REVERSE;
			else if (strcmp(current, "blink") == 0)
				attr |= A_BLINK;
			else if (strcmp(current, "bold") == 0)
				attr |= A_BOLD;
#if HAVE_ITALIC
			else if (strcmp(current, "italic") == 0)
				attr |= A_ITALIC;
#endif

			free(current);

			offset = i;
			length = -1;
		}

		i++;
		length++;
	} while (buf[i - 1] != '\0');

	*(int *)data = attr;
	options_set_colorscheme(NULL);
	options_hooks_refresh_colors_full();
}

static void get_format(void *data, char *buf, size_t size)
{
	char **fmtp = data;

	strscpy(buf, *fmtp, size);
}

static void set_format(void *data, const char *buf)
{
	char **fmtp = data;

	if (!track_format_valid(buf)) {
		error_msg("invalid format string");
		return;
	}
	free(*fmtp);
	*fmtp = xstrdup(buf);

	options_hooks_refresh_full();
}

static void set_clipped_text_format(void *data, const char *buf)
{
	free(clipped_text_format);
	clipped_text_format = clipped_text_internal = xstrdup(buf);

	options_hooks_refresh_full();
}

void options_add_display_options(void)
{
	for (int i = 0; i < NR_FMTS; i++)
		option_add(str_defaults[i].name, id_to_fmt(i), get_format,
			   set_format, NULL, 0);

	option_find("format_clipped_text")->set = set_clipped_text_format;
	option_add("start_view", NULL, get_start_view, set_start_view, NULL, 0);
	option_add("colorscheme", NULL, get_colorscheme, set_colorscheme, NULL,
		   0);

	for (int i = 0; i < NR_COLORS; i++)
		option_add(color_names[i], &colors[i], get_color, set_color,
			   NULL, 0);

	for (int i = 0; i < NR_ATTRS; i++)
		option_add(attr_names[i], &attrs[i], get_attr, set_attr, NULL,
			   0);
}

void options_display_load_defaults(void)
{
	for (int i = 0; str_defaults[i].name; i++)
		option_set(str_defaults[i].name, str_defaults[i].value);

	char filename[512];
	snprintf(filename, sizeof(filename), "%s/%s.theme", termus_data_dir,
		 DEFAULT_COLORSCHEME_NAME);
	if (source_file(filename) == -1) {
		d_print("loading default colorscheme %s failed: %s\n", filename,
			strerror(errno));
		errno = 0;
	}
	options_set_colorscheme(NULL);
}

const char *options_display_default_clipped_text(void)
{
	return str_defaults[FMT_CLIPPED_TEXT].value;
}
