#ifndef TERMUS_UI_CURSES_COMPAT_H
#define TERMUS_UI_CURSES_COMPAT_H

#if defined(__sun__) || defined(__CYGWIN__)
/* TIOCGWINSZ */
#include <termios.h>
#endif

#include <ncurses.h>

#endif
