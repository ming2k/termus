#ifndef TERMUS_OPTIONS_PLAYBACK_STATE_H
#define TERMUS_OPTIONS_PLAYBACK_STATE_H

enum shuffle_mode {
	SHUFFLE_OFF,
	SHUFFLE_TRACKS,
	SHUFFLE_ALBUMS,
	SHUFFLE_FALSE,
	SHUFFLE_TRUE
};

extern int show_current_bitrate;
extern int show_playback_position;
extern int show_remaining_time;
extern int time_show_leading_zero;
extern int stop_after_queue;

extern int auto_reshuffle;
extern int play_library;
extern int repeat;
extern int shuffle;
extern int follow;

int options_get_auto_reshuffle(void);

int options_get_play_library(void);
void options_set_play_library(int enabled);

int options_get_repeat(void);
void options_set_repeat(int enabled);

int options_get_shuffle(void);
void options_set_shuffle(int mode);

int options_get_follow(void);

#endif
