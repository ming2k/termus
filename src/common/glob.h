#ifndef TERMUS_GLOB_H
#define TERMUS_GLOB_H

#include "common/list.h"

void glob_compile(struct list_head *head, const char *pattern);
void glob_free(struct list_head *head);
int glob_match(struct list_head *head, const char *text);

#endif
