#ifndef TERMUS_EDITABLE_H
#define TERMUS_EDITABLE_H

#include "common/iter.h"
#include "common/list.h"
#include "common/rbtree.h"
#include "core/track_info_cb.h"
#include "core/track.h"
#include "common/locking.h"

struct editable;
struct searchable;

typedef void (*editable_free_track)(struct editable *e, struct list_head *head);

struct editable_view_ops {
	void (*mark_changed)(void *view_data);
	int (*needs_redraw)(const void *view_data);
	void (*clear_changed)(void *view_data);
	void (*goto_top)(void *view_data);
	void (*down)(void *view_data, int n);
	void (*row_vanishes)(void *view_data, struct iter *iter);
	int (*get_sel)(void *view_data, struct iter *iter);
	void (*set_sel)(void *view_data, struct iter *iter);
	void (*set_nr_rows)(void *view_data, int rows);
	void (*set_contents)(void *view_data, void *head);
};

struct editable_shared {
	struct editable *owner;

	void *view_data;
	const struct editable_view_ops *view_ops;
	sort_key_t *sort_keys;
	char sort_str[128];
	editable_free_track free_track;
	struct searchable *searchable;
};

struct editable {
	struct list_head head;
	struct rb_root tree_root;
	unsigned int nr_tracks;
	unsigned int nr_marked;
	unsigned int total_time;
	struct editable_shared *shared;
};

void editable_shared_init(struct editable_shared *shared,
		editable_free_track free_track);
void editable_shared_set_view(struct editable_shared *shared,
		void *view_data, const struct editable_view_ops *view_ops);
void editable_shared_set_sort_keys(struct editable_shared *shared,
		sort_key_t *keys);
void editable_shared_mark_changed(struct editable_shared *shared);
int editable_shared_needs_redraw(const struct editable_shared *shared);
void editable_shared_clear_changed(struct editable_shared *shared);
int editable_shared_get_sel(struct editable_shared *shared, struct iter *iter);
void editable_shared_set_sel(struct editable_shared *shared, struct iter *iter);
void editable_shared_set_nr_rows(struct editable_shared *shared, int rows);

void editable_init(struct editable *e, struct editable_shared *shared,
		int take_ownership);
void editable_take_ownership(struct editable *e);
void editable_mark_changed(struct editable *e);
int editable_needs_redraw(const struct editable *e);
void editable_clear_changed(struct editable *e);
int editable_get_sel(struct editable *e, struct iter *iter);
void editable_set_sel(struct editable *e, struct iter *iter);
void editable_set_nr_rows(struct editable *e, int rows);
void editable_add(struct editable *e, struct simple_track *track);
void editable_add_before(struct editable *e, struct simple_track *track);
void editable_remove_track(struct editable *e, struct simple_track *track);
void editable_remove_sel(struct editable *e);
void editable_sort(struct editable *e);
void editable_rand(struct editable *e);
void editable_toggle_mark(struct editable *e);
void editable_move_after(struct editable *e);
void editable_move_before(struct editable *e);
void editable_clear(struct editable *e);
void editable_remove_matching_tracks(struct editable *e,
		int (*cb)(void *data, struct track_info *ti), void *data);
void editable_mark(struct editable *e, const char *filter);
void editable_unmark(struct editable *e);
void editable_invert_marks(struct editable *e);
int _editable_for_each_sel(struct editable *e, track_info_cb cb, void *data,
		int reverse);
int editable_for_each_sel(struct editable *e, track_info_cb cb, void *data,
		int reverse, int advance);
int editable_for_each(struct editable *e, track_info_cb cb, void *data,
		int reverse);
void editable_update_track(struct editable *e, struct track_info *old, struct track_info *new);
int editable_empty(struct editable *e);

static inline void editable_track_to_iter(struct editable *e, struct simple_track *track, struct iter *iter)
{
	iter->data0 = &e->head;
	iter->data1 = track;
	iter->data2 = NULL;
}

#endif
