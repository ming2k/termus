#ifndef TERMUS_UI_OPTIONS_UI_H
#define TERMUS_UI_OPTIONS_UI_H

enum {
	COLOR_CMDLINE_BG,
	COLOR_CMDLINE_FG,
	COLOR_ERROR,
	COLOR_INFO,

	COLOR_SEPARATOR,
	COLOR_STATUSLINE_BG,
	COLOR_STATUSLINE_FG,
	COLOR_STATUSLINE_PROGRESS_BG,

	COLOR_STATUSLINE_PROGRESS_FG,
	COLOR_TITLELINE_BG,
	COLOR_TITLELINE_FG,
	COLOR_WIN_BG,

	COLOR_WIN_CUR,
	COLOR_WIN_CUR_SEL_BG,
	COLOR_WIN_CUR_SEL_FG,
	COLOR_WIN_DIR,

	COLOR_WIN_FG,
	COLOR_WIN_INACTIVE_CUR_SEL_BG,
	COLOR_WIN_INACTIVE_CUR_SEL_FG,
	COLOR_WIN_INACTIVE_SEL_BG,

	COLOR_WIN_INACTIVE_SEL_FG,
	COLOR_WIN_SEL_BG,
	COLOR_WIN_SEL_FG,
	COLOR_WIN_TITLE_BG,

	COLOR_WIN_TITLE_FG,
	COLOR_TRACKWIN_ALBUM_BG,
	COLOR_TRACKWIN_ALBUM_FG,

	NR_COLORS
};

enum {
	COLOR_CMDLINE_ATTR,
	COLOR_STATUSLINE_ATTR,
	COLOR_STATUSLINE_PROGRESS_ATTR,
	COLOR_TITLELINE_ATTR,
	COLOR_WIN_ATTR,
	COLOR_WIN_CUR_SEL_ATTR,
	COLOR_CUR_SEL_ATTR,
	COLOR_WIN_INACTIVE_CUR_SEL_ATTR,
	COLOR_WIN_INACTIVE_SEL_ATTR,
	COLOR_WIN_SEL_ATTR,
	COLOR_WIN_TITLE_ATTR,
	COLOR_TRACKWIN_ALBUM_ATTR,
	COLOR_WIN_CUR_ATTR,
	NR_ATTRS
};

#define BRIGHT (1 << 3)

extern int colors[NR_COLORS];
extern int attrs[NR_ATTRS];

extern char *tree_win_format;
extern char *tree_win_artist_format;
extern char *track_win_album_format;
extern char *track_win_format;
extern char *track_win_format_va;
extern char *track_win_alt_format;
extern char *list_win_format;
extern char *list_win_format_va;
extern char *list_win_alt_format;
extern char *clipped_text_format;
extern char *current_format;
extern char *current_alt_format;
extern char *heading_album_format;
extern char *heading_artist_format;
extern char *heading_playlist_format;
extern char *statusline_format;
extern char *window_title_format;
extern char *window_title_alt_format;
extern char *clipped_text_internal;

void options_ui_apply_ascii_clipped_text_fallback(int using_utf8,
		const char *default_clipped_text);

#endif
