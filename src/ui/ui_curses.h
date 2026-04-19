#ifndef TERMUS_UI_CURSES_H
#define TERMUS_UI_CURSES_H

#include "library/search.h"
#include "ui/ui.h"
#include "ui/curses_compat.h"

enum ui_input_mode { NORMAL_MODE, COMMAND_MODE, SEARCH_MODE };
extern enum ui_input_mode input_mode;
extern int prev_view;
extern struct searchable *searchable;

extern char *lib_filename;
extern char *lib_ext_filename;
extern char *pl_filename;
extern char *play_queue_filename;
extern char *play_queue_ext_filename;

/* color pair indices – used by popup and other modules */
enum cursed_color {
	CURSED_WIN,
	CURSED_WIN_CUR,
	CURSED_WIN_SEL,
	CURSED_WIN_SEL_CUR,

	CURSED_WIN_ACTIVE,
	CURSED_WIN_ACTIVE_CUR,
	CURSED_WIN_ACTIVE_SEL,
	CURSED_WIN_ACTIVE_SEL_CUR,

	CURSED_SEPARATOR,
	CURSED_WIN_TITLE,
	CURSED_COMMANDLINE,
	CURSED_STATUSLINE,

	CURSED_STATUSLINE_PROGRESS,
	CURSED_TITLELINE,
	CURSED_DIR,
	CURSED_ERROR,

	CURSED_INFO,
	CURSED_TRACKWIN_ALBUM,

	NR_CURSED
};

chtype ui_color(int cursed_idx);

#endif
