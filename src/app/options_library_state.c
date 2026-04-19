#include "app/options_library_state.h"

#include <stddef.h>

int smart_artist_sort = 1;
char *library_columns = NULL;
char *now_playing_fields = NULL;

const char *const aaa_mode_names[] = {"all", "artist", "album", NULL};

int options_get_smart_artist_sort(void) { return smart_artist_sort; }
