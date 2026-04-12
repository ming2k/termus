#ifndef TERMUS_UI_CURSES_H
#define TERMUS_UI_CURSES_H

#include "ui/ui.h"
#include "library/search.h"

enum ui_input_mode {
	NORMAL_MODE,
	COMMAND_MODE,
	SEARCH_MODE
};
extern enum ui_input_mode input_mode;
extern int prev_view;
extern struct searchable *searchable;

extern char *lib_filename;
extern char *lib_ext_filename;
extern char *pl_filename;
extern char *play_queue_filename;
extern char *play_queue_ext_filename;

#endif
