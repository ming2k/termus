#include "app/options_playback.h"
#include "app/options_parse.h"
#include "app/options_playback_state.h"
#include "app/options_registry.h"
#include "common/msg.h"
#include "common/utils.h"
#include "core/player.h"
#include "library/lib.h"
#include "ui/options_hooks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void get_softvol_state(void *data, char *buf, size_t size)
{
	snprintf(buf, size, "%d %d", soft_vol_l, soft_vol_r);
}

static void set_softvol_state(void *data, const char *buf)
{
	char buffer[OPTION_MAX_SIZE];
	char *ptr;
	long int left;
	long int right;

	strscpy(buffer, buf, sizeof(buffer));
	ptr = strchr(buffer, ' ');
	if (!ptr)
		goto err;
	while (*ptr == ' ')
		*ptr++ = 0;

	if (str_to_int(buffer, &left) == -1 || left < 0 || left > 100)
		goto err;
	if (str_to_int(ptr, &right) == -1 || right < 0 || right > 100)
		goto err;

	player_set_soft_volume(left, right);
	return;
err:
	error_msg("two integers in range 0..100 expected");
}

static const char *const replaygain_names[] = {
    "disabled",	       "track", "album", "track-preferred",
    "album-preferred", "smart", NULL};

static const size_t replaygain_names_len =
    sizeof(replaygain_names) / sizeof(replaygain_names[0]) - 1;

static void get_replaygain(void *data, char *buf, size_t size)
{
	strscpy(buf, replaygain_names[replaygain], size);
}

static void set_replaygain(void *data, const char *buf)
{
	int value;

	if (!parse_enum(buf, 0, replaygain_names_len - 1, replaygain_names,
			&value))
		return;
	options_hooks_set_replaygain(value);
}

static void toggle_replaygain(void *data)
{
	options_hooks_set_replaygain((replaygain + 1) % replaygain_names_len);
}

static void get_replaygain_limit(void *data, char *buf, size_t size)
{
	strscpy(buf, options_bool_names[replaygain_limit], size);
}

static void set_replaygain_limit(void *data, const char *buf)
{
	int value;

	if (!options_parse_bool(buf, &value))
		return;
	options_hooks_set_replaygain_limit(value);
}

static void toggle_replaygain_limit(void *data)
{
	options_hooks_set_replaygain_limit(replaygain_limit ^ 1);
}

static void get_replaygain_preamp(void *data, char *buf, size_t size)
{
	snprintf(buf, size, "%f", replaygain_preamp);
}

static void set_replaygain_preamp(void *data, const char *buf)
{
	double value;
	char *end;

	value = strtod(buf, &end);
	if (end == buf) {
		error_msg("floating point number expected (dB)");
		return;
	}
	options_hooks_set_replaygain_preamp(value);
}

static const char *const shuffle_names[] = {"off",   "tracks", "albums",
					    "false", "true",   NULL};

static void get_shuffle(void *data, char *buf, size_t size)
{
	strscpy(buf, shuffle_names[shuffle], size);
}

static void set_shuffle(void *data, const char *buf)
{
	int value;

	if (!parse_enum(buf, 0, 4, shuffle_names, &value))
		return;

	if (value == SHUFFLE_FALSE)
		value = SHUFFLE_OFF;
	else if (value == SHUFFLE_TRUE)
		value = SHUFFLE_TRACKS;

	if (value != shuffle)
		options_hooks_notify_shuffle_changed();

	shuffle = value;
	options_hooks_refresh_statusline();
}

static void toggle_shuffle(void *data)
{
	int s = shuffle + 1;
	s %= 3;

	if (!play_library && s == SHUFFLE_ALBUMS)
		s = SHUFFLE_OFF;

	shuffle = s;
	options_hooks_refresh_statusline();
}

static void get_speed(void *data, char *buf, size_t size)
{
	snprintf(buf, size, "%.2f", playback_speed);
}

static void set_speed(void *data, const char *buf)
{
	double value;
	char *end;

	value = strtod(buf, &end);
	if (end == buf) {
		error_msg("floating point number expected");
		return;
	}
	if (value < 0.1 || value > 4.0) {
		error_msg("speed must be between 0.1 and 4.0");
		return;
	}
	options_hooks_set_playback_speed(value);
}

static void get_softvol(void *data, char *buf, size_t size)
{
	strscpy(buf, options_bool_names[soft_vol], size);
}

static void do_set_softvol(int soft) { options_hooks_set_softvol(soft); }

static void set_softvol(void *data, const char *buf)
{
	int soft;

	if (!options_parse_bool(buf, &soft))
		return;
	do_set_softvol(soft);
}

static void toggle_softvol(void *data) { do_set_softvol(soft_vol ^ 1); }

static void post_set_refresh_statusline(void *data)
{
	options_hooks_refresh_statusline();
}

static void post_set_loop_status(void *data)
{
	options_hooks_notify_loop_status_changed();
	options_hooks_refresh_statusline();
}

#define DN(name) option_add(#name, NULL, get_##name, set_##name, NULL, 0)
#define DT(name)                                                               \
	option_add(#name, NULL, get_##name, set_##name, toggle_##name, 0)

void options_add_playback_options(void)
{
	option_add_bool_full("continue", &player_cont,
			     post_set_refresh_statusline, 0);
	option_add_bool_full("continue_album", &player_cont_album,
			     post_set_refresh_statusline, 0);
	option_add_bool_full("follow", &follow, post_set_refresh_statusline, 0);
	option_add_bool_full("play_library", &play_library,
			     post_set_refresh_statusline, 0);
	option_add_bool_full("play_sorted", &play_sorted,
			     post_set_refresh_statusline, 0);
	option_add_bool_full("repeat", &repeat, post_set_refresh_statusline, 0);
	option_add_bool_full("repeat_current", &player_repeat_current,
			     post_set_loop_status, 0);
	DT(replaygain);
	DT(replaygain_limit);
	DN(replaygain_preamp);
	option_add_bool_full("show_current_bitrate", &show_current_bitrate,
			     post_set_refresh_statusline, 0);
	option_add_bool_full("show_playback_position", &show_playback_position,
			     post_set_refresh_statusline, 0);
	option_add_bool_full("show_remaining_time", &show_remaining_time,
			     post_set_refresh_statusline, 0);
	DT(shuffle);
	DN(speed);
	DT(softvol);
	DN(softvol_state);
	option_add_bool_full("time_show_leading_zero", &time_show_leading_zero,
			     post_set_refresh_statusline, 0);
}
