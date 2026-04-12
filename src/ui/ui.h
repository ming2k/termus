#ifndef TERMUS_UI_H
#define TERMUS_UI_H

#include "common/locale.h"
#include "common/compiler.h"
#include "ui/format_print.h"
#include "app/termus.h"
#include "common/msg.h"

#include <signal.h>

void update_titleline(void);
void update_statusline(void);
void update_filterline(void);
void update_colors(void);
void update_full(void);
void update_size(void);
void search_not_found(void);
void set_view(int view);
void enter_command_mode(void);
void enter_search_mode(void);
void enter_search_backward_mode(void);

int track_format_valid(const char *format);

/* lock player_info ! */
const char *get_stream_title(void);
const struct format_option *get_global_fopts(void);

int get_track_win_x(void);

#endif
