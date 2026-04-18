#include "ui/options_hooks.h"

#include "app/options_core_state.h"
#include "app/options_ui_state.h"
#include "common/xmalloc.h"
#include "core/output.h"
#include "core/player.h"
#include "ipc/mpris.h"
#include "library/lib.h"
#include "ui/browser.h"
#include "ui/ui.h"

#if defined(__sun__)
#include <ncurses.h>
#else
#include <curses.h>
#endif

void update_mouse(void)
{
	if (options_get_mouse()) {
		mouseinterval(25);
		mousemask(
		    BUTTON_CTRL | BUTTON_ALT | BUTTON1_PRESSED |
			BUTTON1_RELEASED | BUTTON1_CLICKED |
			BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED |
			BUTTON2_PRESSED | BUTTON2_RELEASED | BUTTON2_CLICKED |
			BUTTON3_PRESSED | BUTTON3_RELEASED | BUTTON3_CLICKED |
			BUTTON3_DOUBLE_CLICKED | BUTTON3_TRIPLE_CLICKED |
			BUTTON4_PRESSED | BUTTON4_RELEASED | BUTTON4_CLICKED |
			BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED
#if NCURSES_MOUSE_VERSION >= 2
			| BUTTON5_PRESSED | BUTTON5_RELEASED | BUTTON5_CLICKED |
			BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED
#endif
		    ,
		    NULL);
	} else {
		mousemask(0, NULL);
	}
}

void options_hooks_reload_browser(void) { browser_reload(); }

void options_hooks_mark_library_tree_changed(void) { tree_win()->changed = 1; }

void options_hooks_refresh_statusline(void) { update_statusline(); }

void options_hooks_refresh_colors_full(void)
{
	update_colors();
	update_full();
}

void options_hooks_refresh_full(void) { update_full(); }

void options_hooks_refresh_layout(void) { update_size(); }

void options_hooks_notify_loop_status_changed(void)
{
	mpris_loop_status_changed();
}

void options_hooks_notify_shuffle_changed(void) { mpris_shuffle_changed(); }

void options_hooks_update_tree_selection(void) { tree_sel_update(0); }

void options_hooks_switch_output_plugin(const char *name)
{
	if (ui_initialized) {
		if (!soft_vol || options_get_pause_on_output_change())
			mixer_close();
		player_set_op(name);
		if (!soft_vol || options_get_pause_on_output_change())
			mixer_open();
	} else {
		free(output_plugin);
		output_plugin = xstrdup(name);
	}
}

void options_hooks_set_softvol(int soft)
{
	if (!soft_vol || options_get_pause_on_output_change())
		mixer_close();
	player_set_soft_vol(soft);
	if (!soft_vol || options_get_pause_on_output_change())
		mixer_open();
	update_statusline();
}

void options_hooks_set_playback_speed(double speed)
{
	player_set_speed(speed);
	update_statusline();
}

void options_hooks_set_replaygain(int mode) { player_set_rg(mode); }

void options_hooks_set_replaygain_limit(int enabled)
{
	player_set_rg_limit(enabled);
}

void options_hooks_set_replaygain_preamp(double value)
{
	player_set_rg_preamp(value);
}
