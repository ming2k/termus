#ifndef TERMUS_LOAD_DIR_H
#define TERMUS_LOAD_DIR_H

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

struct directory {
	DIR *d;
	int len;
	/* we need stat information for symlink targets */
	int is_link;
	/* stat() information. ie. for the symlink target */
	struct stat st;
	char path[1024];
};

int dir_open(struct directory *dir, const char *name);
void dir_close(struct directory *dir);
const char *dir_read(struct directory *dir);

struct ptr_array {
	/* allocated with malloc(). contains pointers */
	void *ptrs;
	int alloc;
	int count;
};

/* ptr_array.ptrs is either char ** or struct dir_entry ** */
struct dir_entry {
	mode_t mode;
	char name[];
};

#define PTR_ARRAY(name) struct ptr_array name = {NULL, 0, 0}

void ptr_array_add(struct ptr_array *array, void *ptr);

static inline void ptr_array_plug(struct ptr_array *array)
{
	ptr_array_add(array, NULL);
	array->count--;
}

static inline void ptr_array_sort(struct ptr_array *array,
				  int (*cmp)(const void *a, const void *b))
{
	int count = array->count;
	if (count)
		qsort(array->ptrs, count, sizeof(void *), cmp);
}

static inline int ptr_array_bsearch(const void *key, struct ptr_array *array,
				    int (*cmp)(const void *a, const void *b))
{
	if (array->count) {
		const void **p;
		p = bsearch(key, array->ptrs, array->count, sizeof(void *),
			    cmp);
		if (p)
			return p - (const void **)array->ptrs;
	}
	return -1;
}

static inline void ptr_array_unique(struct ptr_array *array,
				    int (*cmp)(const void *a, const void *b))
{
	if (array->count < 2)
		return;

	int i, j = 0;
	void **ptrs = array->ptrs;

	for (i = 1; i < array->count; i++) {
		if (cmp(&ptrs[i - 1], &ptrs[i]) != 0)
			j++;
		else
			free(ptrs[j]);
		ptrs[j] = ptrs[i];
	}
	array->count = j + 1;
}

static inline void ptr_array_truncate(struct ptr_array *array, int new_count)
{
	void **ptrs = array->ptrs;
	for (int i = new_count; i < array->count; i++)
		free(ptrs[i]);
	array->count = new_count;
}

static inline void ptr_array_clear(struct ptr_array *array)
{
	void **ptrs = array->ptrs;

	for (int i = 0; i != array->count; i++) {
		free(ptrs[i]);
	}
	free(ptrs);
	array->ptrs = NULL;
	array->alloc = 0;
	array->count = 0;
}

#endif
