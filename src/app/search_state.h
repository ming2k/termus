#ifndef TERMUS_APP_SEARCH_STATE_H
#define TERMUS_APP_SEARCH_STATE_H

#include "library/search.h"

extern char *search_str;
extern enum search_direction search_direction;

/* //WORDS or ??WORDS search mode */
extern int search_restricted;

void search_text(const char *text, int restricted, int beginning);

#endif
