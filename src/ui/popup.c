#include "ui/popup.h"

#include "app/cmdline.h"
#include "common/path.h"
#include "common/uchar.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "core/player.h"
#include "core/sf.h"
#include "core/track_info.h"
#include "library/filters.h"
#include "library/lib.h"
#include "ui/ui.h"
#include "ui/ui_curses.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static enum popup_type active_type = POPUP_NONE;
static WINDOW *popup_win = NULL;
static int popup_h, popup_w, popup_y, popup_x;
static char *saved_live_filter = NULL;
static int track_info_tab = 0;
static int popup_cursor_row = -1;
static int popup_cursor_col = -1;

enum {
	TRACK_INFO_TAB_METADATA,
	TRACK_INFO_TAB_FILE,
	NR_TRACK_INFO_TABS
};

static void close_window(void)
{
	if (popup_win) {
		delwin(popup_win);
		popup_win = NULL;
	}
	active_type = POPUP_NONE;
}

static void clear_filter_session(void)
{
	free(saved_live_filter);
	saved_live_filter = NULL;
	cmdline_clear();
}

static void apply_live_filter(const char *text)
{
	filters_set_live(text && text[0] ? text : NULL);
	update_filterline();
}

static void calc_dims(void)
{
	if (active_type == POPUP_FILTERS) {
		popup_h = 8;
		popup_w = min_i(88, max_i(44, COLS - 6));
	} else {
		popup_h = 18;
		popup_w = min_i(84, max_i(32, COLS - 4));
	}
	if (popup_h > LINES)
		popup_h = LINES;
	if (popup_w > COLS)
		popup_w = COLS;
	popup_y = (LINES - popup_h) / 2;
	popup_x = (COLS - popup_w) / 2;
}

static WINDOW *make_win(void)
{
	return newwin(popup_h, popup_w, popup_y, popup_x);
}

void popup_open(enum popup_type type)
{
	if (active_type != POPUP_NONE)
		popup_cancel();

	active_type = type;
	calc_dims();
	popup_win = make_win();
	if (!popup_win) {
		active_type = POPUP_NONE;
		return;
	}

	if (active_type == POPUP_FILTERS) {
		saved_live_filter =
		    xstrdup(lib_live_filter ? lib_live_filter : "");
		cmdline_set_text(saved_live_filter);
	} else if (active_type == POPUP_TRACK_INFO) {
		track_info_tab = TRACK_INFO_TAB_METADATA;
	}
}

void popup_close(void)
{
	if (active_type == POPUP_FILTERS)
		clear_filter_session();
	close_window();
}

void popup_cancel(void)
{
	if (active_type == POPUP_FILTERS)
		apply_live_filter(saved_live_filter);
	popup_close();
}

void popup_resize(void)
{
	if (active_type == POPUP_NONE)
		return;
	if (popup_win) {
		delwin(popup_win);
		popup_win = NULL;
	}
	calc_dims();
	popup_win = make_win();
	if (!popup_win)
		active_type = POPUP_NONE;
}

int popup_is_open(void) { return active_type != POPUP_NONE; }

enum popup_type popup_get_type(void) { return active_type; }

int popup_wants_cursor(void) { return active_type == POPUP_FILTERS; }

/* write a padded string into popup_win at (row, col), filling 'width' cols */
static void popup_sprint(int row, int col, const char *str, int width)
{
	wmove(popup_win, row, col);
	int remaining = width;
	if (str) {
		int len = (int)strlen(str);
		int n = len < remaining ? len : remaining;
		waddnstr(popup_win, str, n);
		remaining -= n;
	}
	while (remaining-- > 0)
		waddch(popup_win, ' ');
}

static void draw_title(int row, const char *title)
{
	wbkgdset(popup_win, ui_color(CURSED_WIN_TITLE));
	popup_sprint(row, 1, title, popup_w - 2);
}

static void draw_tabs(int row, const char *const *tabs, int nr_tabs, int active)
{
	int col = 1;

	wbkgdset(popup_win, ui_color(CURSED_WIN_ACTIVE));
	popup_sprint(row, 1, "", popup_w - 2);
	for (int i = 0; i < nr_tabs && col < popup_w - 2; i++) {
		char label[64];
		int width;

		snprintf(label, sizeof(label), " %d:%s ", i + 1, tabs[i]);
		width = min_i((int)strlen(label), popup_w - 1 - col);
		wbkgdset(popup_win, ui_color(i == active ? CURSED_WIN_TITLE
							 : CURSED_WIN_ACTIVE));
		popup_sprint(row, col, label, width);
		col += width + 1;
	}
}

static void popup_apply_and_refresh(void)
{
	apply_live_filter(cmdline.line);
	update_full();
}

static void popup_filter_backspace(void)
{
	if (cmdline.clen > 0)
		cmdline_backspace();
}

static void draw_filter_input_line(int row)
{
	static char visible[1024];
	const char *prefix = " > ";
	const int prefix_w = 3;
	int input_w = popup_w - 2 - prefix_w;
	const char *str = cmdline.line;
	int cursor_col;
	int copy_w;
	int skip;
	size_t idx = 0;
	size_t copied;

	if (input_w < 1)
		input_w = 1;

	if (u_str_nwidth(str, cmdline.cpos) <= input_w) {
		cursor_col = 1 + prefix_w + u_str_nwidth(str, cmdline.cpos);
	} else {
		int context_w = min_i(u_str_width(str + cmdline.bpos), input_w / 3);

		skip = u_str_nwidth(str, cmdline.cpos) + context_w - input_w;
		idx = u_skip_chars(str, &skip, true);
		cursor_col = popup_w - 1 - context_w;
	}

	copy_w = input_w;
	copied = u_copy_chars(visible, str + idx, &copy_w);
	visible[copied] = 0;

	wbkgdset(popup_win, ui_color(CURSED_WIN_ACTIVE));
	popup_sprint(row, 1, prefix, prefix_w);
	popup_sprint(row, 1 + prefix_w, visible, input_w);
	popup_cursor_row = row;
	popup_cursor_col = clamp(cursor_col, 1 + prefix_w, popup_w - 2);
}

static void draw_filter_popup(void)
{
	int inner_w = popup_w - 2;

	draw_title(1, " Filter");

	wbkgdset(popup_win, ui_color(CURSED_WIN_ACTIVE));
	popup_sprint(2, 1, " Enter a library filter. Empty input restores all items.",
		     inner_w);
	popup_sprint(3, 1, " The list updates while you type.", inner_w);
	draw_filter_input_line(5);
	popup_sprint(6, 1, " Enter apply  Esc cancel", inner_w);
}

static void draw_field(int *row, const char *label, const char *value)
{
	int inner_w;
	char line[256];

	if (!value || !value[0])
		return;

	inner_w = popup_w - 2;
	wbkgdset(popup_win, ui_color(CURSED_WIN_ACTIVE));

	snprintf(line, sizeof(line), "%-14s%s", label, value);
	popup_sprint((*row)++, 1, line, inner_w);
}

static void draw_track_info_metadata(struct track_info *ti, int row)
{
	draw_field(&row, "Title:", ti->title);
	draw_field(&row, "Artist:", ti->artist);
	draw_field(&row, "Album:", ti->album);
	draw_field(&row, "Genre:", ti->genre);

	if (ti->date > 0) {
		char buf[16];

		snprintf(buf, sizeof(buf), "%d", ti->date / 10000);
		draw_field(&row, "Year:", buf);
	}
	if (ti->tracknumber > 0) {
		char buf[16];

		snprintf(buf, sizeof(buf), "%d", ti->tracknumber);
		draw_field(&row, "Track:", buf);
	}
	if (ti->duration > 0) {
		char buf[16];
		int m = ti->duration / 60;
		int s = ti->duration % 60;

		snprintf(buf, sizeof(buf), "%d:%02d", m, s);
		draw_field(&row, "Duration:", buf);
	}
	if (ti->bitrate > 0) {
		char buf[16];

		snprintf(buf, sizeof(buf), "%ld kbps", ti->bitrate / 1000);
		draw_field(&row, "Bitrate:", buf);
	}
	if (ti->codec && ti->codec[0]) {
		const char *profile = ti->codec_profile;

		if (profile && profile[0]) {
			char buf[64];

			snprintf(buf, sizeof(buf), "%s (%s)", ti->codec, profile);
			draw_field(&row, "Codec:", buf);
		} else {
			draw_field(&row, "Codec:", ti->codec);
		}
	}
}

static void draw_track_info_file(struct track_info *ti, int row)
{
	sample_format_t sf = player_get_current_sf();
	struct stat st;
	int have_stat = 0;

	draw_field(&row, "Path:", ti->filename);
	if (ti->codec && ti->codec[0])
		draw_field(&row, "Codec:", ti->codec);
	if (ti->codec_profile && ti->codec_profile[0])
		draw_field(&row, "Profile:", ti->codec_profile);
	if (ti->bitrate > 0) {
		char buf[32];

		snprintf(buf, sizeof(buf), "%ld kbps", ti->bitrate / 1000);
		draw_field(&row, "Bitrate:", buf);
	}
	if (sf_get_rate(sf) > 0) {
		char buf[32];

		snprintf(buf, sizeof(buf), "%u Hz", sf_get_rate(sf));
		draw_field(&row, "Sample Rate:", buf);
	}
	if (sf_get_bits(sf) > 0) {
		char buf[32];

		snprintf(buf, sizeof(buf), "%u-bit", sf_get_bits(sf));
		draw_field(&row, "Bit Depth:", buf);
	}
	if (sf_get_channels(sf) > 0) {
		char buf[32];

		snprintf(buf, sizeof(buf), "%u", sf_get_channels(sf));
		draw_field(&row, "Channels:", buf);
	}
	if (ti->filename && !is_http_url(ti->filename))
		have_stat = stat(ti->filename, &st) == 0;
	if (have_stat) {
		char buf[64];
		struct tm *tm;

		snprintf(buf, sizeof(buf), "%lld bytes", (long long)st.st_size);
		draw_field(&row, "Size:", buf);
		tm = localtime(&st.st_mtime);
		if (tm && strftime(buf, sizeof(buf), "%F %T", tm) > 0)
			draw_field(&row, "Modified:", buf);
	}
}

static void draw_track_info(void)
{
	static const char *const tabs[] = {"Metadata", "File"};
	struct track_info *ti = player_info.ti;
	int inner_w = popup_w - 2;
	int row = 2;

	draw_title(1, " Track Info");
	draw_tabs(2, tabs, NR_TRACK_INFO_TABS, track_info_tab);

	wbkgdset(popup_win, ui_color(CURSED_WIN_ACTIVE));

	if (!ti) {
		popup_sprint(4, 1, "  (nothing playing)", inner_w);
		return;
	}

	switch (track_info_tab) {
	case TRACK_INFO_TAB_METADATA:
		draw_track_info_metadata(ti, 4);
		break;
	case TRACK_INFO_TAB_FILE:
		draw_track_info_file(ti, 4);
		break;
	default:
		break;
	}

	popup_sprint(popup_h - 2, 1, " Left/Right or 1/2 to switch tabs, Esc close",
		     inner_w);
}

void popup_draw(void)
{
	if (!popup_win || active_type == POPUP_NONE)
		return;

	popup_cursor_row = -1;
	popup_cursor_col = -1;

	werase(popup_win);
	wbkgd(popup_win, ui_color(CURSED_WIN_ACTIVE));
	box(popup_win, 0, 0);

	switch (active_type) {
	case POPUP_FILTERS:
		draw_filter_popup();
		break;
	case POPUP_TRACK_INFO:
		draw_track_info();
		break;
	default:
		break;
	}

	if (popup_cursor_row >= 0 && popup_cursor_col >= 0)
		wmove(popup_win, popup_cursor_row, popup_cursor_col);

	wrefresh(popup_win);
}

int popup_handle_ch(uchar ch)
{
	if (active_type == POPUP_NONE)
		return 0;

	if (active_type == POPUP_TRACK_INFO) {
		switch (ch) {
		case '1':
			track_info_tab = TRACK_INFO_TAB_METADATA;
			update_full();
			return 1;
		case '2':
			track_info_tab = TRACK_INFO_TAB_FILE;
			update_full();
			return 1;
		case '\t':
			track_info_tab =
			    (track_info_tab + 1) % NR_TRACK_INFO_TABS;
			update_full();
			return 1;
		case 27:
		case 'q':
		case '\n':
		case '\r':
			popup_cancel();
			update_full();
			return 1;
		}
		return 1;
	}

	switch (ch) {
	case 0x01:
		cmdline_move_home();
		break;
	case 0x02:
		cmdline_move_left();
		break;
	case 0x03:
	case 0x07:
	case 0x1B:
		popup_cancel();
		update_full();
		return 1;
	case 0x04:
		cmdline_delete_ch();
		popup_apply_and_refresh();
		return 1;
	case 0x05:
		cmdline_move_end();
		break;
	case 0x06:
		cmdline_move_right();
		break;
	case 0x0A:
	case 0x0D:
		popup_close();
		update_full();
		return 1;
	case 0x0B:
		cmdline_clear_end();
		popup_apply_and_refresh();
		return 1;
	case 0x15:
		cmdline_backspace_to_bol();
		popup_apply_and_refresh();
		return 1;
	case 0x17:
		cmdline_backward_delete_word(cmdline_word_delimiters);
		popup_apply_and_refresh();
		return 1;
	case 0x08:
	case 127:
		popup_filter_backspace();
		popup_apply_and_refresh();
		return 1;
	default:
		if (ch >= 0x20) {
			cmdline_insert_ch(ch);
			popup_apply_and_refresh();
		}
		return 1;
	}

	update_full();
	return 1;
}

int popup_handle_escape(int c)
{
	if (active_type == POPUP_NONE)
		return 0;

	if (active_type == POPUP_TRACK_INFO) {
		popup_cancel();
		update_full();
		return 1;
	}

	switch (c) {
	case 98:
		cmdline_backward_word(cmdline_filename_delimiters);
		break;
	case 100:
		cmdline_delete_word(cmdline_filename_delimiters);
		popup_apply_and_refresh();
		return 1;
	case 102:
		cmdline_forward_word(cmdline_filename_delimiters);
		break;
	case 127:
	case KEY_BACKSPACE:
		cmdline_backward_delete_word(cmdline_filename_delimiters);
		popup_apply_and_refresh();
		return 1;
	default:
		break;
	}

	update_full();
	return 1;
}

int popup_handle_key(int key)
{
	if (active_type == POPUP_NONE)
		return 0;

	if (active_type == POPUP_TRACK_INFO) {
		switch (key) {
#ifdef KEY_ENTER
		case KEY_ENTER:
			popup_cancel();
			update_full();
			return 1;
#endif
		case '\n':
		case '\r':
			popup_cancel();
			update_full();
			return 1;
		case KEY_LEFT:
			track_info_tab =
			    (track_info_tab + NR_TRACK_INFO_TABS - 1) %
			    NR_TRACK_INFO_TABS;
			update_full();
			return 1;
		case KEY_RIGHT:
		case '\t':
			track_info_tab =
			    (track_info_tab + 1) % NR_TRACK_INFO_TABS;
			update_full();
			return 1;
		}
		return 1;
	}

	switch (key) {
	case KEY_DC:
		cmdline_delete_ch();
		popup_apply_and_refresh();
		return 1;
	case KEY_BACKSPACE:
		popup_filter_backspace();
		popup_apply_and_refresh();
		return 1;
	case KEY_LEFT:
		cmdline_move_left();
		break;
	case KEY_RIGHT:
		cmdline_move_right();
		break;
	case KEY_HOME:
		cmdline_move_home();
		break;
	case KEY_END:
		cmdline_move_end();
		break;
#ifdef KEY_ENTER
	case KEY_ENTER:
		popup_close();
		update_full();
		return 1;
#endif
	default:
		break;
	}

	update_full();
	return 1;
}

int popup_handle_mouse(MEVENT *event)
{
	(void)event;
	return active_type != POPUP_NONE;
}
