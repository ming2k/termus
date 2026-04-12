#ifndef TERMUS_COMMAND_MODE_H
#define TERMUS_COMMAND_MODE_H

#include "app/commands.h"
#include "common/uchar.h"

#if defined(__sun__)
#include <ncurses.h>
#else
#include <curses.h>
#endif

void command_mode_ch(uchar ch);
void command_mode_escape(int c);
void command_mode_key(int key);
void command_mode_mouse(MEVENT *event);

void view_clear(int view);
void view_add(int view, char *arg, int prepend);
void view_load(int view, char *arg);
void view_save(int view, char *arg, int to_stdout, int filtered, int extended);

struct window *current_win(void);

#endif
