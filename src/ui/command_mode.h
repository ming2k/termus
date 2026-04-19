#ifndef TERMUS_COMMAND_MODE_H
#define TERMUS_COMMAND_MODE_H

#include "app/commands.h"
#include "common/uchar.h"
#include "ui/curses_compat.h"

void command_mode_ch(uchar ch);
void command_mode_escape(int c);
void command_mode_key(int key);
void command_mode_mouse(MEVENT *event);

void view_clear(int view);
void view_add(int view, const char *arg, int prepend);
void view_load(int view, const char *arg);
void view_save(int view, const char *arg, int to_stdout, int filtered,
	       int extended);

struct window *current_win(void);

#endif
