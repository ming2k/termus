#ifndef TERMUS_SEARCH_MODE_H
#define TERMUS_SEARCH_MODE_H

#include "app/search_state.h"
#include "common/uchar.h"

#if defined(__sun__)
#include <ncurses.h>
#else
#include <curses.h>
#endif

void search_mode_ch(uchar ch);
void search_mode_escape(int c);
void search_mode_key(int key);
void search_mode_mouse(MEVENT *event);
void search_mode_init(void);
void search_mode_exit(void);
#endif
