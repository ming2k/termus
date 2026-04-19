#ifndef TERMUS_UI_OPTIONS_HOOKS_H
#define TERMUS_UI_OPTIONS_HOOKS_H

void update_mouse(void);
void options_hooks_refresh_statusline(void);
void options_hooks_refresh_colors_full(void);
void options_hooks_refresh_full(void);
void options_hooks_refresh_layout(void);
void options_hooks_notify_loop_status_changed(void);
void options_hooks_notify_shuffle_changed(void);
void options_hooks_switch_output_plugin(const char *name);
void options_hooks_set_softvol(int soft);
void options_hooks_set_playback_speed(double speed);
void options_hooks_set_replaygain(int mode);
void options_hooks_set_replaygain_limit(int enabled);
void options_hooks_set_replaygain_preamp(double value);

#endif
