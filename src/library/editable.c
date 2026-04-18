#include "library/editable.h"
#include "common/locking.h"
#include "common/mergesort.h"
#include "common/xmalloc.h"
#include "core/track.h"
#include "core/track_info.h"
#include "library/expr.h"
#include "library/filters.h"
#include "library/search.h"

static void shared_view_mark_changed(struct editable_shared *shared)
{
	if (shared->view_ops && shared->view_ops->mark_changed)
		shared->view_ops->mark_changed(shared->view_data);
}

static int shared_view_needs_redraw(const struct editable_shared *shared)
{
	if (shared->view_ops && shared->view_ops->needs_redraw)
		return shared->view_ops->needs_redraw(shared->view_data);
	return 0;
}

static void shared_view_clear_changed(struct editable_shared *shared)
{
	if (shared->view_ops && shared->view_ops->clear_changed)
		shared->view_ops->clear_changed(shared->view_data);
}

static void shared_view_goto_top(struct editable_shared *shared)
{
	if (shared->view_ops && shared->view_ops->goto_top)
		shared->view_ops->goto_top(shared->view_data);
}

static void shared_view_down(struct editable_shared *shared, int n)
{
	if (shared->view_ops && shared->view_ops->down)
		shared->view_ops->down(shared->view_data, n);
}

static void shared_view_row_vanishes(struct editable_shared *shared,
				     struct iter *iter)
{
	if (shared->view_ops && shared->view_ops->row_vanishes)
		shared->view_ops->row_vanishes(shared->view_data, iter);
}

static int shared_view_get_sel(struct editable_shared *shared,
			       struct iter *iter)
{
	if (shared->view_ops && shared->view_ops->get_sel)
		return shared->view_ops->get_sel(shared->view_data, iter);
	return 0;
}

static void shared_view_set_sel(struct editable_shared *shared,
				struct iter *iter)
{
	if (shared->view_ops && shared->view_ops->set_sel)
		shared->view_ops->set_sel(shared->view_data, iter);
}

static void shared_view_set_nr_rows(struct editable_shared *shared, int rows)
{
	if (shared->view_ops && shared->view_ops->set_nr_rows)
		shared->view_ops->set_nr_rows(shared->view_data, rows);
}

static void shared_view_set_contents(struct editable_shared *shared, void *head)
{
	if (shared->view_ops && shared->view_ops->set_contents)
		shared->view_ops->set_contents(shared->view_data, head);
}

static int simple_track_search_get_current(void *data, struct iter *iter,
					   enum search_direction dir)
{
	struct editable_shared *shared = data;
	return shared_view_get_sel(shared, iter);
}

static int simple_track_search_matches(void *data, struct iter *iter,
				       const char *text)
{
	struct editable_shared *shared = data;
	int rc = _simple_track_search_matches(iter, text);
	if (rc)
		shared_view_set_sel(shared, iter);
	return rc;
}

static const struct searchable_ops simple_search_ops = {
    .get_prev = simple_track_get_prev,
    .get_next = simple_track_get_next,
    .get_current = simple_track_search_get_current,
    .matches = simple_track_search_matches};

static struct simple_track *get_selected(struct editable *e)
{
	struct iter sel;

	if (editable_get_sel(e, &sel))
		return iter_to_simple_track(&sel);
	return NULL;
}

void editable_shared_init(struct editable_shared *shared,
			  editable_free_track free_track)
{
	shared->view_data = NULL;
	shared->view_ops = NULL;
	shared->sort_keys = xnew(sort_key_t, 1);
	shared->sort_keys[0] = SORT_INVALID;
	shared->sort_str[0] = 0;
	shared->free_track = free_track;
	shared->owner = NULL;

	struct iter iter = {0};
	shared->searchable = searchable_new(shared, &iter, &simple_search_ops);
}

void editable_shared_set_view(struct editable_shared *shared, void *view_data,
			      const struct editable_view_ops *view_ops)
{
	shared->view_data = view_data;
	shared->view_ops = view_ops;

	if (shared->owner && shared->view_data) {
		shared_view_set_contents(shared, &shared->owner->head);
		shared_view_mark_changed(shared);
	}
}

void editable_init(struct editable *e, struct editable_shared *shared,
		   int take_ownership)
{
	list_init(&e->head);
	e->tree_root = RB_ROOT;
	e->nr_tracks = 0;
	e->nr_marked = 0;
	e->total_time = 0;
	e->shared = shared;

	if (take_ownership)
		editable_take_ownership(e);
}

static int editable_owns_shared(struct editable *e)
{
	return e->shared->owner == e;
}

void editable_take_ownership(struct editable *e)
{
	if (!editable_owns_shared(e)) {
		e->shared->owner = e;
		shared_view_set_contents(e->shared, &e->head);
		shared_view_mark_changed(e->shared);

		struct iter iter = {.data0 = &e->head};
		searchable_set_head(e->shared->searchable, &iter);
	}
}

static void do_editable_add(struct editable *e, struct simple_track *track,
			    int tiebreak)
{
	sorted_list_add_track(&e->head, &e->tree_root, track,
			      e->shared->sort_keys, tiebreak);
	e->nr_tracks++;
	if (track->info->duration != -1)
		e->total_time += track->info->duration;
	if (editable_owns_shared(e))
		shared_view_mark_changed(e->shared);
}

void editable_add(struct editable *e, struct simple_track *track)
{
	do_editable_add(e, track, +1);
}

void editable_add_before(struct editable *e, struct simple_track *track)
{
	do_editable_add(e, track, -1);
}

void editable_remove_track(struct editable *e, struct simple_track *track)
{
	struct track_info *ti = track->info;
	struct iter iter;

	editable_track_to_iter(e, track, &iter);
	if (editable_owns_shared(e))
		shared_view_row_vanishes(e->shared, &iter);

	e->nr_tracks--;
	e->nr_marked -= track->marked;
	if (ti->duration != -1)
		e->total_time -= ti->duration;

	sorted_list_remove_track(&e->head, &e->tree_root, track);
	e->shared->free_track(e, &track->node);
}

void editable_remove_sel(struct editable *e)
{
	struct simple_track *t;

	if (e->nr_marked) {
		/* treat marked tracks as selected */
		struct list_head *next, *item = e->head.next;

		while (item != &e->head) {
			next = item->next;
			t = to_simple_track(item);
			if (t->marked)
				editable_remove_track(e, t);
			item = next;
		}
	} else {
		t = get_selected(e);
		if (t)
			editable_remove_track(e, t);
	}
}

void editable_sort(struct editable *e)
{
	if (e->nr_tracks <= 1)
		return;
	sorted_list_rebuild(&e->head, &e->tree_root, e->shared->sort_keys);

	if (editable_owns_shared(e)) {
		shared_view_mark_changed(e->shared);
		shared_view_goto_top(e->shared);
	}
}

void editable_shared_set_sort_keys(struct editable_shared *shared,
				   sort_key_t *keys)
{
	free(shared->sort_keys);
	shared->sort_keys = keys;
}

void editable_shared_mark_changed(struct editable_shared *shared)
{
	shared_view_mark_changed(shared);
}

int editable_shared_needs_redraw(const struct editable_shared *shared)
{
	return shared_view_needs_redraw(shared);
}

void editable_shared_clear_changed(struct editable_shared *shared)
{
	shared_view_clear_changed(shared);
}

int editable_shared_get_sel(struct editable_shared *shared, struct iter *iter)
{
	return shared_view_get_sel(shared, iter);
}

void editable_shared_set_sel(struct editable_shared *shared, struct iter *iter)
{
	shared_view_set_sel(shared, iter);
}

void editable_shared_set_nr_rows(struct editable_shared *shared, int rows)
{
	shared_view_set_nr_rows(shared, rows);
}

void editable_mark_changed(struct editable *e)
{
	editable_shared_mark_changed(e->shared);
}

int editable_needs_redraw(const struct editable *e)
{
	return editable_shared_needs_redraw(e->shared);
}

void editable_clear_changed(struct editable *e)
{
	editable_shared_clear_changed(e->shared);
}

int editable_get_sel(struct editable *e, struct iter *iter)
{
	return editable_shared_get_sel(e->shared, iter);
}

void editable_set_sel(struct editable *e, struct iter *iter)
{
	editable_shared_set_sel(e->shared, iter);
}

void editable_set_nr_rows(struct editable *e, int rows)
{
	editable_shared_set_nr_rows(e->shared, rows);
}

void editable_toggle_mark(struct editable *e)
{
	struct simple_track *t;

	t = get_selected(e);
	if (t) {
		e->nr_marked -= t->marked;
		t->marked ^= 1;
		e->nr_marked += t->marked;
		if (editable_owns_shared(e)) {
			shared_view_mark_changed(e->shared);
			shared_view_down(e->shared, 1);
		}
	}
}

static void move_item(struct editable *e, struct list_head *head,
		      struct list_head *item)
{
	struct simple_track *t = to_simple_track(item);
	struct iter iter;

	editable_track_to_iter(e, t, &iter);
	if (editable_owns_shared(e))
		shared_view_row_vanishes(e->shared, &iter);

	list_del(item);
	list_add(item, head);
}

static void reset_tree(struct editable *e)
{
	struct simple_track *old, *first_track;

	old = tree_node_to_simple_track(rb_first(&e->tree_root));
	first_track = to_simple_track(e->head.next);
	if (old != first_track) {
		rb_replace_node(&old->tree_node, &first_track->tree_node,
				&e->tree_root);
		RB_CLEAR_NODE(&old->tree_node);
	}
}

static void move_sel(struct editable *e, struct list_head *after)
{
	struct simple_track *t;
	struct list_head *item, *next;
	struct iter iter;
	LIST_HEAD(tmp_head);

	if (e->nr_marked) {
		/* collect marked */
		item = e->head.next;
		while (item != &e->head) {
			t = to_simple_track(item);
			next = item->next;
			if (t->marked)
				move_item(e, &tmp_head, item);
			item = next;
		}
	} else {
		/* collect the selected track */
		t = get_selected(e);
		if (t)
			move_item(e, &tmp_head, &t->node);
	}

	/* put them back to the list after @after */
	item = tmp_head.next;
	while (item != &tmp_head) {
		next = item->next;
		list_add(item, after);
		item = next;
	}
	reset_tree(e);

	/* select top-most of the moved tracks */
	editable_track_to_iter(e, to_simple_track(after->next), &iter);

	if (editable_owns_shared(e)) {
		shared_view_mark_changed(e->shared);
		shared_view_set_sel(e->shared, &iter);
	}
}

static struct list_head *find_insert_after_point(struct editable *e,
						 struct list_head *item)
{
	if (e->nr_marked == 0) {
		/* move the selected track down one row */
		return item->next;
	}

	/* move marked after the selected
	 *
	 * if the selected track itself is marked we find the first unmarked
	 * track (or head) before the selected one
	 */
	while (item != &e->head) {
		struct simple_track *t = to_simple_track(item);

		if (!t->marked)
			break;
		item = item->prev;
	}
	return item;
}

static struct list_head *find_insert_before_point(struct editable *e,
						  struct list_head *item)
{
	item = item->prev;
	if (e->nr_marked == 0) {
		/* move the selected track up one row */
		return item->prev;
	}

	/* move marked before the selected
	 *
	 * if the selected track itself is marked we find the first unmarked
	 * track (or head) before the selected one
	 */
	while (item != &e->head) {
		struct simple_track *t = to_simple_track(item);

		if (!t->marked)
			break;
		item = item->prev;
	}
	return item;
}

void editable_move_after(struct editable *e)
{
	struct simple_track *sel;

	if (e->nr_tracks <= 1 || e->shared->sort_keys[0] != SORT_INVALID)
		return;

	sel = get_selected(e);
	if (sel)
		move_sel(e, find_insert_after_point(e, &sel->node));
}

void editable_move_before(struct editable *e)
{
	struct simple_track *sel;

	if (e->nr_tracks <= 1 || e->shared->sort_keys[0] != SORT_INVALID)
		return;

	sel = get_selected(e);
	if (sel)
		move_sel(e, find_insert_before_point(e, &sel->node));
}

void editable_clear(struct editable *e)
{
	struct list_head *item, *tmp;

	list_for_each_safe(item, tmp, &e->head)
	    editable_remove_track(e, to_simple_track(item));
}

void editable_remove_matching_tracks(struct editable *e,
				     int (*cb)(void *data,
					       struct track_info *ti),
				     void *data)
{
	struct list_head *item, *tmp;

	list_for_each_safe(item, tmp, &e->head)
	{
		struct simple_track *t = to_simple_track(item);
		if (cb(data, t->info))
			editable_remove_track(e, t);
	}
}

void editable_mark(struct editable *e, const char *filter)
{
	struct expr *expr = NULL;
	struct simple_track *t;

	if (filter) {
		expr = parse_filter(filter);
		if (expr == NULL)
			return;
	}

	list_for_each_entry(t, &e->head, node)
	{
		e->nr_marked -= t->marked;
		t->marked = 0;
		if (expr == NULL || expr_eval(expr, t->info)) {
			t->marked = 1;
			e->nr_marked++;
		}
	}

	if (editable_owns_shared(e))
		shared_view_mark_changed(e->shared);
}

void editable_unmark(struct editable *e)
{
	struct simple_track *t;

	list_for_each_entry(t, &e->head, node)
	{
		e->nr_marked -= t->marked;
		t->marked = 0;
	}

	if (editable_owns_shared(e))
		shared_view_mark_changed(e->shared);
}

void editable_invert_marks(struct editable *e)
{
	struct simple_track *t;

	list_for_each_entry(t, &e->head, node)
	{
		e->nr_marked -= t->marked;
		t->marked ^= 1;
		e->nr_marked += t->marked;
	}

	if (editable_owns_shared(e))
		shared_view_mark_changed(e->shared);
}

int _editable_for_each_sel(struct editable *e, track_info_cb cb, void *data,
			   int reverse)
{
	int rc = 0;

	if (e->nr_marked) {
		/* treat marked tracks as selected */
		rc = simple_list_for_each_marked(&e->head, cb, data, reverse);
	} else {
		struct simple_track *t = get_selected(e);

		if (t)
			rc = cb(data, t->info);
	}
	return rc;
}

int editable_for_each_sel(struct editable *e, track_info_cb cb, void *data,
			  int reverse, int advance)
{
	int rc;

	rc = _editable_for_each_sel(e, cb, data, reverse);
	if (advance && e->nr_marked == 0 && editable_owns_shared(e))
		shared_view_down(e->shared, 1);
	return rc;
}

int editable_for_each(struct editable *e, track_info_cb cb, void *data,
		      int reverse)
{
	return simple_list_for_each(&e->head, cb, data, reverse);
}

void editable_update_track(struct editable *e, struct track_info *old,
			   struct track_info *new)
{
	struct list_head *item, *tmp;
	int changed = 0;

	list_for_each_safe(item, tmp, &e->head)
	{
		struct simple_track *track = to_simple_track(item);
		if (track->info == old) {
			if (new) {
				track_info_unref(old);
				track_info_ref(new);
				track->info = new;
			} else {
				editable_remove_track(e, track);
			}
			changed = 1;
		}
	}
	if (editable_owns_shared(e) && changed)
		shared_view_mark_changed(e->shared);
}

void editable_rand(struct editable *e)
{
	if (e->nr_tracks <= 1)
		return;
	rand_list_rebuild(&e->head, &e->tree_root);

	if (editable_owns_shared(e)) {
		shared_view_mark_changed(e->shared);
		shared_view_goto_top(e->shared);
	}
}

int editable_empty(struct editable *e) { return list_empty(&e->head); }
