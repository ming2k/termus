#include "library/play_queue.h"
#include "library/editable.h"
#include "core/track.h"
#include "common/xmalloc.h"

struct editable pq_editable;
static struct editable_shared pq_editable_shared;
static struct window *pq_window;

static void pq_free_track(struct editable *e, struct list_head *item)
{
	struct simple_track *track = to_simple_track(item);

	track_info_unref(track->info);
	free(track);
}

void play_queue_init(void)
{
	editable_shared_init(&pq_editable_shared, pq_free_track);
	pq_window = NULL;
	editable_init(&pq_editable, &pq_editable_shared, 1);
}

void play_queue_attach_view(void *view_data,
		const struct editable_view_ops *view_ops, struct window *win)
{
	pq_window = win;
	editable_shared_set_view(&pq_editable_shared, view_data, view_ops);
}

void play_queue_append(struct track_info *ti, void *opaque)
{
	if (!ti)
		return;

	struct simple_track *t = simple_track_new(ti);
	editable_add(&pq_editable, t);
}

void play_queue_prepend(struct track_info *ti, void *opaque)
{
	if (!ti)
		return;

	struct simple_track *t = simple_track_new(ti);
	editable_add_before(&pq_editable, t);
}

struct track_info *play_queue_remove(void)
{
	struct track_info *info = NULL;

	if (!list_empty(&pq_editable.head)) {
		struct simple_track *t = to_simple_track(pq_editable.head.next);
		info = t->info;
		track_info_ref(info);
		editable_remove_track(&pq_editable, t);
	}

	return info;
}

int play_queue_for_each(int (*cb)(void *data, struct track_info *ti),
		void *data, void *opaque)
{
	struct simple_track *track;
	int rc = 0;

	list_for_each_entry(track, &pq_editable.head, node) {
		rc = cb(data, track->info);
		if (rc)
			break;
	}
	return rc;
}

unsigned int play_queue_total_time(void)
{
	return pq_editable.total_time;
}

struct window *play_queue_win(void)
{
	return pq_window;
}

void play_queue_set_nr_rows(int rows)
{
	editable_shared_set_nr_rows(&pq_editable_shared, rows);
}

int queue_needs_redraw(void)
{
	return editable_needs_redraw(&pq_editable);
}

void queue_post_update(void)
{
	editable_clear_changed(&pq_editable);
}
