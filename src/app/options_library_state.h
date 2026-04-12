#ifndef TERMUS_OPTIONS_LIBRARY_STATE_H
#define TERMUS_OPTIONS_LIBRARY_STATE_H

extern const char * const aaa_mode_names[];

extern int auto_expand_albums_follow;
extern int auto_expand_albums_search;
extern int auto_expand_albums_selcur;
extern int auto_hide_playlists_panel;
extern int show_all_tracks;
extern int show_hidden;
extern int display_artist_sort_name;
extern int smart_artist_sort;
extern int sort_albums_by_name;

int options_get_auto_hide_playlists_panel(void);

int options_get_auto_expand_albums_follow(void);

int options_get_auto_expand_albums_search(void);

int options_get_auto_expand_albums_selcur(void);

int options_get_show_all_tracks(void);

int options_get_show_hidden(void);

int options_get_display_artist_sort_name(void);

int options_get_smart_artist_sort(void);

int options_get_sort_albums_by_name(void);

#endif
