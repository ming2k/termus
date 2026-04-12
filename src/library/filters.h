#ifndef TERMUS_FILTERS_H
#define TERMUS_FILTERS_H

#include "common/list.h"
#include "common/iter.h"
#include "library/search.h"

#include <stddef.h>

struct window;

struct filters_view_ops {
	int (*get_sel)(void *view_data, struct iter *iter);
	void (*set_sel)(void *view_data, struct iter *iter);
	void (*row_vanishes)(void *view_data, struct iter *iter);
	void (*mark_changed)(void *view_data);
};

/* factivate foo !bar
 *
 * foo: FS_YES
 * bar: FS_NO
 * baz: FS_IGNORE
 */
enum {
	/* [ ] filter not selected */
	FS_IGNORE,
	/* [*] filter selected */
	FS_YES,
	/* [!] filter selected and inverted */
	FS_NO,
};

struct filter_entry {
	struct list_head node;

	char *name;
	char *filter;
	unsigned visited : 1;

	/* selected and activated status (FS_* enum) */
	unsigned sel_stat : 2;
	unsigned act_stat : 2;
};

static inline struct filter_entry *iter_to_filter_entry(struct iter *iter)
{
	return iter->data1;
}

extern struct searchable *filters_searchable;
extern struct list_head filters_head;

int filters_get_prev(struct iter *iter);
int filters_get_next(struct iter *iter);

void filters_init(void);
void filters_exit(void);
void filters_set_view(void *view_data, const struct filters_view_ops *view_ops);
struct window *filters_get_window(void);
int filters_get_edit_command(char *buf, size_t size);

/* parse filter and expand sub filters */
struct expr *parse_filter(const char *val);

/* add filter to filter list (replaces old filter with same name)
 *
 * @keyval  "name=value" where value is filter
 */
void filters_set_filter(const char *keyval);

/* set throwaway filter (not saved to the filter list)
 *
 * @val   filter or NULL to disable filtering
 */
void filters_set_anonymous(const char *val);

/* set live filter (not saved to the filter list)
 *
 * @val   filter or NULL to disable filtering
 */
void filters_set_live(const char *val);

void filters_activate_names(const char *str);

int filters_activate(void);
void filters_toggle_filter(void);
void filters_delete_filter(void);

#endif
