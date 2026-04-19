#ifndef TERMUS_POPUP_H
#define TERMUS_POPUP_H

#include "common/uchar.h"
#include "ui/curses_compat.h"

enum popup_type {
	POPUP_NONE,
	POPUP_FILTERS,
	POPUP_TRACK_INFO,
};

void popup_open(enum popup_type type);
void popup_close(void);
void popup_cancel(void);
void popup_draw(void);
void popup_resize(void);
int popup_handle_ch(uchar ch);
int popup_handle_escape(int c);
int popup_handle_key(int key);
int popup_handle_mouse(MEVENT *event);
int popup_wants_cursor(void);
int popup_is_open(void);
enum popup_type popup_get_type(void);

#endif
