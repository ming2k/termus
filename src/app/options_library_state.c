#include "app/options_library_state.h"

#include <stddef.h>

int auto_expand_albums_follow = 1;
int auto_expand_albums_search = 1;
int auto_expand_albums_selcur = 1;
int auto_hide_playlists_panel = 0;
int show_all_tracks = 1;
int show_hidden = 0;
int display_artist_sort_name = 0;
int smart_artist_sort = 1;
int sort_albums_by_name = 0;

const char *const aaa_mode_names[] = {"all", "artist", "album", NULL};

int options_get_auto_hide_playlists_panel(void)
{
	return auto_hide_playlists_panel;
}

int options_get_auto_expand_albums_follow(void)
{
	return auto_expand_albums_follow;
}

int options_get_auto_expand_albums_search(void)
{
	return auto_expand_albums_search;
}

int options_get_auto_expand_albums_selcur(void)
{
	return auto_expand_albums_selcur;
}

int options_get_show_all_tracks(void) { return show_all_tracks; }

int options_get_show_hidden(void) { return show_hidden; }

int options_get_display_artist_sort_name(void)
{
	return display_artist_sort_name;
}

int options_get_smart_artist_sort(void) { return smart_artist_sort; }

int options_get_sort_albums_by_name(void) { return sort_albums_by_name; }
