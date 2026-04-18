#ifndef TERMUS_DEBUG_H
#define TERMUS_DEBUG_H

#include "common/compiler.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdint.h>

void debug_init(void);
void _debug_bug(const char *function, const char *fmt, ...)
    TERMUS_FORMAT(2, 3) TERMUS_NORETURN;
void _debug_print(const char *function, const char *fmt, ...)
    TERMUS_FORMAT(2, 3);

uint64_t timer_get(void);
void timer_print(const char *what, uint64_t usec);

#define BUG(...) _debug_bug(__FUNCTION__, __VA_ARGS__)

#define TERMUS_STR(a) #a

#define BUG_ON(a)                                                              \
	do {                                                                   \
		if (unlikely(a))                                               \
			BUG("%s\n", TERMUS_STR(a));                            \
	} while (0)

#define d_print(...) _debug_print(__FUNCTION__, __VA_ARGS__)

#endif
