#include "ui/options_ui.h"
#include "common/xmalloc.h"

#include <stdlib.h>
#include <string.h>

#if defined(__sun__)
#include <ncurses.h>
#else
#include <curses.h>
#endif

/*
 * Fallback palette used only when the packaged startup theme could not be
 * loaded. The intended default palette lives in data/default.theme.
 */
int colors[NR_COLORS] = {
    -1,
    -1,
    COLOR_RED | BRIGHT,
    COLOR_YELLOW | BRIGHT,

    COLOR_BLACK | BRIGHT,
    COLOR_BLACK | BRIGHT,
    COLOR_WHITE,
    COLOR_CYAN,

    COLOR_BLACK,
    COLOR_GREEN | BRIGHT,
    COLOR_GREEN | BRIGHT,
    -1,

    COLOR_GREEN | BRIGHT,
    COLOR_GREEN,
    COLOR_BLACK,
    COLOR_CYAN | BRIGHT,

    -1,
    COLOR_BLACK | BRIGHT,
    COLOR_GREEN | BRIGHT,
    COLOR_BLACK | BRIGHT,

    COLOR_WHITE,
    COLOR_CYAN,
    COLOR_BLACK,
    COLOR_BLACK | BRIGHT,

    COLOR_CYAN | BRIGHT,
    -1,
    -1,
};

int attrs[NR_ATTRS] = {
    A_NORMAL, A_NORMAL, A_NORMAL, A_NORMAL, A_NORMAL, A_NORMAL, A_NORMAL,
    A_NORMAL, A_NORMAL, A_NORMAL, A_NORMAL, A_BOLD,   A_NORMAL,
};

char *tree_win_format = NULL;
char *tree_win_artist_format = NULL;
char *track_win_album_format = NULL;
char *track_win_format = NULL;
char *track_win_format_va = NULL;
char *track_win_alt_format = NULL;
char *list_win_format = NULL;
char *list_win_format_va = NULL;
char *list_win_alt_format = NULL;
char *clipped_text_format = NULL;
char *clipped_text_internal = NULL;
char *current_format = NULL;
char *current_alt_format = NULL;
char *heading_album_format = NULL;
char *heading_artist_format = NULL;
char *heading_playlist_format = NULL;
char *statusline_format = NULL;
char *window_title_format = NULL;
char *window_title_alt_format = NULL;

void options_ui_apply_ascii_clipped_text_fallback(
    int using_utf8, const char *default_clipped_text)
{
	if (using_utf8)
		return;

	if (strcmp(clipped_text_format, default_clipped_text) != 0)
		return;

	clipped_text_internal = xstrdup("...");
}
