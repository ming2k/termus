#ifndef TERMUS_LOCALE_H
#define TERMUS_LOCALE_H

extern const char *charset;
extern int using_utf8;

void locale_init_charset(const char *name);

#endif
