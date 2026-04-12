#ifndef TERMUS_TRACK_INFO_CB_H
#define TERMUS_TRACK_INFO_CB_H

#include "core/track_info.h"

typedef int (*track_info_cb)(void *data, struct track_info *ti);

#endif
