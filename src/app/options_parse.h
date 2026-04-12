#ifndef TERMUS_OPTIONS_PARSE_H
#define TERMUS_OPTIONS_PARSE_H

extern const char * const options_bool_names[];

int options_parse_bool(const char *buf, int *val);

#endif
