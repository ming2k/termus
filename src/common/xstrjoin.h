#ifndef TERMUS_XSTRJOIN_H
#define TERMUS_XSTRJOIN_H

#include "common/utils.h"

char *xstrjoin_slice(struct slice);
#define xstrjoin(...) xstrjoin_slice(TO_SLICE(const char *, __VA_ARGS__))

#endif
