#ifndef TERMUS_OPTIONS_LIBRARY_STATE_H
#define TERMUS_OPTIONS_LIBRARY_STATE_H

extern const char *const aaa_mode_names[];

extern int smart_artist_sort;
extern char *library_columns;
extern char *now_playing_fields;

int options_get_smart_artist_sort(void);

#endif
