#ifndef TERMUS_MERGESORT_H
#define TERMUS_MERGESORT_H

#include "common/list.h"

void list_mergesort(struct list_head *head,
		    int (*compare)(const struct list_head *,
				   const struct list_head *));

#endif
