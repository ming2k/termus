#include "app/options_playback_state.h"

int auto_reshuffle = 1;
int play_library = 1;
int repeat = 0;
int shuffle = 0;
int follow = 0;

int show_current_bitrate = 0;
int show_playback_position = 1;
int show_remaining_time = 0;
int time_show_leading_zero = 1;
int stop_after_queue = 0;

int options_get_auto_reshuffle(void)
{
	return auto_reshuffle;
}

int options_get_play_library(void)
{
	return play_library;
}

void options_set_play_library(int enabled)
{
	play_library = !!enabled;
}

int options_get_repeat(void)
{
	return repeat;
}

void options_set_repeat(int enabled)
{
	repeat = !!enabled;
}

int options_get_shuffle(void)
{
	return shuffle;
}

void options_set_shuffle(int mode)
{
	shuffle = mode;
}

int options_get_follow(void)
{
	return follow;
}
