#include "library/lib.h"
#include "app/options_library_state.h"
#include "app/options_playback_state.h"
#include "app/options_ui_state.h"
#include "common/debug.h"
#include "common/rbtree.h"
#include "common/u_collate.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "core/track_info.h"
#include "library/editable.h"
#include "ui/window.h"

#include <pthread.h>
#include <string.h>

struct editable lib_editable;
static struct editable_shared lib_editable_shared;
static struct window *lib_sorted_window;

struct tree_track *lib_cur_track = NULL;
unsigned int play_sorted = 0;
enum aaa_mode aaa_mode = AAA_MODE_ALL;
/* used in ui_curses.c for status display */
char *lib_live_filter = NULL;

struct rb_root lib_shuffle_root;
struct rb_root lib_album_shuffle_root;
static struct expr *filter = NULL;
static struct expr *add_filter = NULL;
static int remove_from_hash = 1;

static struct expr *live_filter_expr = NULL;
static struct track_info *cur_track_ti = NULL;
static struct track_info *sel_track_ti = NULL;

struct window *lib_sorted_win(void) { return lib_sorted_window; }

void lib_sorted_attach_view(void *view_data,
			    const struct editable_view_ops *view_ops,
			    struct window *win)
{
	lib_sorted_window = win;
	editable_shared_set_view(&lib_editable_shared, view_data, view_ops);
}

void lib_sorted_set_nr_rows(int rows)
{
	editable_shared_set_nr_rows(&lib_editable_shared, rows);
}

int lib_sorted_needs_redraw(void)
{
	return editable_shared_needs_redraw(&lib_editable_shared);
}

void lib_sorted_clear_changed(void)
{
	editable_shared_clear_changed(&lib_editable_shared);
}

const char *artist_sort_name(const struct artist *a)
{
	if (a->sort_name)
		return a->sort_name;

	if (options_get_smart_artist_sort() && a->auto_sort_name)
		return a->auto_sort_name;

	return a->name;
}

static inline void sorted_track_to_iter(struct tree_track *track,
					struct iter *iter)
{
	iter->data0 = &lib_editable.head;
	iter->data1 = track;
	iter->data2 = NULL;
}

static void all_wins_changed(void)
{
	editable_mark_changed(&lib_editable);
}

static void shuffle_add(struct tree_track *track)
{
	shuffle_list_add(&track->simple_track.shuffle_info, &lib_shuffle_root,
			 track->album);
}

static void album_shuffle_list_add(struct album *album)
{
	shuffle_list_add(&album->shuffle_info, &lib_album_shuffle_root, album);
}

static void album_shuffle_list_remove(struct album *album)
{
	rb_erase(&album->shuffle_info.tree_node, &lib_album_shuffle_root);
}

static void views_add_track(struct track_info *ti)
{
	struct tree_track *track = xnew(struct tree_track, 1);

	/* NOTE: does not ref ti */
	simple_track_init((struct simple_track *)track, ti);

	/* both the hash table and views have refs */
	track_info_ref(ti);

	shuffle_add(track);
	editable_add(&lib_editable, (struct simple_track *)track);
}

struct fh_entry {
	struct fh_entry *next;

	/* ref count is increased when added to this hash */
	struct track_info *ti;
};

#define FH_SIZE (1024)
static struct fh_entry *ti_hash[FH_SIZE] = {
    NULL,
};

static int hash_insert(struct track_info *ti)
{
	const char *filename = ti->filename;
	unsigned int pos = hash_str(filename) % FH_SIZE;
	struct fh_entry **entryp;
	struct fh_entry *e;

	entryp = &ti_hash[pos];
	e = *entryp;
	while (e) {
		if (strcmp(e->ti->filename, filename) == 0) {
			/* found, don't insert */
			return 0;
		}
		e = e->next;
	}

	e = xnew(struct fh_entry, 1);
	track_info_ref(ti);
	e->ti = ti;
	e->next = *entryp;
	*entryp = e;
	return 1;
}

static void hash_remove(struct track_info *ti)
{
	const char *filename = ti->filename;
	unsigned int pos = hash_str(filename) % FH_SIZE;
	struct fh_entry **entryp;

	entryp = &ti_hash[pos];
	while (1) {
		struct fh_entry *e = *entryp;

		BUG_ON(e == NULL);
		if (strcmp(e->ti->filename, filename) == 0) {
			*entryp = e->next;
			track_info_unref(e->ti);
			free(e);
			break;
		}
		entryp = &e->next;
	}
}

static int is_filtered(struct track_info *ti)
{
	if (live_filter_expr && !expr_eval(live_filter_expr, ti))
		return 1;
	if (!live_filter_expr && lib_live_filter &&
	    !track_info_matches(ti, lib_live_filter, TI_MATCH_ALL))
		return 1;
	if (filter && !expr_eval(filter, ti))
		return 1;
	return 0;
}

void lib_add_track(struct track_info *ti, void *opaque)
{
	if (!ti)
		return;

	if (add_filter && !expr_eval(add_filter, ti)) {
		/* filter any files excluded by lib_add_filter */
		return;
	}

	if (!hash_insert(ti)) {
		/* duplicate files not allowed */
		return;
	}

	if (!is_filtered(ti))
		views_add_track(ti);
}

static int aaa_mode_filter(const struct album *album)
{
	if (aaa_mode == AAA_MODE_ALBUM)
		return CUR_ALBUM == album;

	if (aaa_mode == AAA_MODE_ARTIST)
		return CUR_ARTIST == album->artist;

	/* AAA_MODE_ALL */
	return 1;
}

static int cur_album_filter(const struct album *album)
{
	return CUR_ALBUM == album;
}

static struct tree_track *shuffle_album_get_next(void)
{
	struct shuffle_info *shuffle_info = NULL;
	struct album *album;

	if (lib_cur_track != NULL)
		shuffle_info = &lib_cur_track->album->shuffle_info;
	album = (struct album *)shuffle_list_get_next(
	    &lib_album_shuffle_root, shuffle_info, aaa_mode_filter);
	if (album != NULL) {
		// In a flat list, we don't easily have 'album_first_track' without tree
		// but we can find it in lib_editable
		struct simple_track *track;
		list_for_each_entry(track, &lib_editable.head, node) {
			struct tree_track *tt = (struct tree_track *)track;
			if (tt->album == album)
				return tt;
		}
	}
	return NULL;
}

static struct tree_track *shuffle_album_get_prev(void)
{
	struct shuffle_info *shuffle_info = NULL;
	struct album *album;

	if (lib_cur_track != NULL)
		shuffle_info = &lib_cur_track->album->shuffle_info;
	album = (struct album *)shuffle_list_get_prev(
	    &lib_album_shuffle_root, shuffle_info, aaa_mode_filter);
	if (album != NULL) {
		struct simple_track *track;
		struct tree_track *last = NULL;
		list_for_each_entry(track, &lib_editable.head, node) {
			struct tree_track *tt = (struct tree_track *)track;
			if (tt->album == album)
				last = tt;
		}
		return last;
	}
	return NULL;
}

static struct tree_track *sorted_album_first_track(struct tree_track *track)
{
	struct tree_track *prev = track;

	while (true) {
		prev = (struct tree_track *)simple_list_get_prev(
		    &lib_editable.head, (struct simple_track *)prev, NULL,
		    false);
		if (prev == NULL)
			return track;
		if (prev->album == track->album)
			track = prev;
	}
}

static struct tree_track *sorted_album_last_track(struct tree_track *track)
{
	struct tree_track *next = track;

	while (true) {
		next = (struct tree_track *)simple_list_get_next(
		    &lib_editable.head, (struct simple_track *)next, NULL,
		    false);
		if (next == NULL)
			return track;
		if (next->album == track->album)
			track = next;
	}
}

void lib_reshuffle(void)
{
	shuffle_list_reshuffle(&lib_shuffle_root);
	shuffle_list_reshuffle(&lib_album_shuffle_root);
	if (lib_cur_track) {
		shuffle_insert(&lib_shuffle_root, NULL,
			       &lib_cur_track->simple_track.shuffle_info);
		shuffle_insert(&lib_album_shuffle_root, NULL,
			       &lib_cur_track->album->shuffle_info);
	}
}

void lib_sort_artists(void)
{
	// No-op without tree view
}

static void free_lib_track(struct editable *e, struct list_head *item)
{
	struct tree_track *track = (struct tree_track *)to_simple_track(item);
	struct track_info *ti = tree_track_info(track);

	if (track == lib_cur_track)
		lib_cur_track = NULL;

	if (remove_from_hash)
		hash_remove(ti);

	rb_erase(&track->simple_track.shuffle_info.tree_node,
		 &lib_shuffle_root);

	track_info_unref(ti);
	free(track);
}

void lib_init(void)
{
	editable_shared_init(&lib_editable_shared, free_lib_track);
	lib_sorted_window = NULL;
	editable_init(&lib_editable, &lib_editable_shared, 1);
	srand(time(NULL));
}

struct track_info *lib_set_track(struct tree_track *track)
{
	struct track_info *ti = NULL;

	if (track) {
		lib_cur_track = track;
		ti = tree_track_info(track);
		track_info_ref(ti);
		if (options_get_follow()) {
			sorted_sel_current();
		}
		all_wins_changed();
	}
	return ti;
}

struct track_info *lib_goto_next(void)
{
	struct tree_track *track;

	if (list_empty(&lib_editable.head)) {
		BUG_ON(lib_cur_track != NULL);
		return NULL;
	}
	if (options_get_shuffle() == SHUFFLE_TRACKS) {
		track = (struct tree_track *)shuffle_list_get_next(
		    &lib_shuffle_root, (struct shuffle_info *)lib_cur_track,
		    aaa_mode_filter);
	} else if (options_get_shuffle() == SHUFFLE_ALBUMS) {
		track = (struct tree_track *)simple_list_get_next(
		    &lib_editable.head,
		    (struct simple_track *)lib_cur_track,
		    cur_album_filter, false);
		if (track == NULL) {
			track = shuffle_album_get_next();
			track = sorted_album_first_track(track);
		}
	} else {
		track = (struct tree_track *)simple_list_get_next(
		    &lib_editable.head, (struct simple_track *)lib_cur_track,
		    aaa_mode_filter, true);
	}
	return lib_set_track(track);
}

struct track_info *lib_goto_prev(void)
{
	struct tree_track *track;

	if (list_empty(&lib_editable.head)) {
		BUG_ON(lib_cur_track != NULL);
		return NULL;
	}
	if (options_get_shuffle() == SHUFFLE_TRACKS) {
		track = (struct tree_track *)shuffle_list_get_prev(
		    &lib_shuffle_root, (struct shuffle_info *)lib_cur_track,
		    aaa_mode_filter);
	} else if (options_get_shuffle() == SHUFFLE_ALBUMS) {
		track = (struct tree_track *)simple_list_get_prev(
		    &lib_editable.head,
		    (struct simple_track *)lib_cur_track,
		    cur_album_filter, false);
		if (track == NULL) {
			track = shuffle_album_get_prev();
			track = sorted_album_last_track(track);
		}
	} else {
		track = (struct tree_track *)simple_list_get_prev(
		    &lib_editable.head, (struct simple_track *)lib_cur_track,
		    aaa_mode_filter, true);
	}
	return lib_set_track(track);
}

struct track_info *lib_goto_next_album(void)
{
	struct tree_track *track = NULL;

	if (list_empty(&lib_editable.head)) {
		BUG_ON(lib_cur_track != NULL);
		return NULL;
	}
	if (options_get_shuffle() == SHUFFLE_TRACKS) {
		return lib_goto_next();
	} else if (options_get_shuffle() == SHUFFLE_ALBUMS) {
		track = shuffle_album_get_next();
		track = sorted_album_first_track(track);
	} else {
		track = sorted_album_last_track(lib_cur_track);
		track = (struct tree_track *)simple_list_get_next(
		    &lib_editable.head, (struct simple_track *)track,
		    aaa_mode_filter, true);
	}

	return lib_set_track(track);
}

struct track_info *lib_goto_prev_album(void)
{
	struct tree_track *track = NULL;

	if (list_empty(&lib_editable.head)) {
		BUG_ON(lib_cur_track != NULL);
		return NULL;
	}
	if (options_get_shuffle() == SHUFFLE_TRACKS) {
		return lib_goto_prev();
	} else if (options_get_shuffle() == SHUFFLE_ALBUMS) {
		track = shuffle_album_get_prev();
		track = sorted_album_first_track(track);
	} else {
		track = sorted_album_first_track(lib_cur_track);
		track = (struct tree_track *)simple_list_get_prev(
		    &lib_editable.head, (struct simple_track *)track,
		    aaa_mode_filter, true);
		track = sorted_album_first_track(track);
	}

	return lib_set_track(track);
}

static struct tree_track *sorted_get_selected(void)
{
	struct iter sel;

	if (list_empty(&lib_editable.head))
		return NULL;

	editable_get_sel(&lib_editable, &sel);
	return iter_to_sorted_track(&sel);
}

struct track_info *sorted_activate_selected(void)
{
	return lib_set_track(sorted_get_selected());
}

static void hash_add_to_views(void)
{
	int i;
	for (i = 0; i < FH_SIZE; i++) {
		struct fh_entry *e;

		e = ti_hash[i];
		while (e) {
			struct track_info *ti = e->ti;

			if (!is_filtered(ti))
				views_add_track(ti);
			e = e->next;
		}
	}
}

struct tree_track *lib_find_track(struct track_info *ti)
{
	struct simple_track *track;

	list_for_each_entry(track, &lib_editable.head, node)
	{
		if (strcmp(track->info->filename, ti->filename) == 0) {
			struct tree_track *tt = (struct tree_track *)track;
			return tt;
		}
	}
	return NULL;
}

void lib_store_cur_track(struct track_info *ti)
{
	if (cur_track_ti)
		track_info_unref(cur_track_ti);
	cur_track_ti = ti;
	track_info_ref(cur_track_ti);
}

struct track_info *lib_get_cur_stored_track(void)
{
	if (cur_track_ti && lib_find_track(cur_track_ti))
		return cur_track_ti;
	return NULL;
}

static void restore_cur_track(struct track_info *ti)
{
	struct tree_track *tt = lib_find_track(ti);
	if (tt)
		lib_cur_track = tt;
}

static int is_filtered_cb(void *data, struct track_info *ti)
{
	return is_filtered(ti);
}

static void do_lib_filter(int clear_before)
{
	/* try to save cur_track */
	if (lib_cur_track)
		lib_store_cur_track(tree_track_info(lib_cur_track));

	if (clear_before)
		d_print("filter results could grow, clear tracks and re-add "
			"(slow)\n");

	remove_from_hash = 0;
	if (clear_before) {
		editable_clear(&lib_editable);
		hash_add_to_views();
	} else
		editable_remove_matching_tracks(&lib_editable, is_filtered_cb,
						NULL);
	remove_from_hash = 1;

	window_changed(lib_sorted_win());
	window_goto_top(lib_sorted_win());

	/* restore cur_track */
	if (cur_track_ti && !lib_cur_track)
		restore_cur_track(cur_track_ti);
}

static void unset_live_filter(void)
{
	free(lib_live_filter);
	lib_live_filter = NULL;
	free(live_filter_expr);
	live_filter_expr = NULL;
}

void lib_set_filter(struct expr *expr)
{
	int clear_before = lib_live_filter || filter;
	unset_live_filter();
	if (filter)
		expr_free(filter);
	filter = expr;
	do_lib_filter(clear_before);
}

void lib_set_add_filter(struct expr *expr)
{
	if (add_filter)
		expr_free(add_filter);
	add_filter = expr;
}

static struct tree_track *get_sel_track(void)
{
	if (cur_view == LIBRARY_VIEW)
		return sorted_get_selected();
	return NULL;
}

static void set_sel_track(struct tree_track *tt)
{
	struct iter iter;

	if (cur_view == LIBRARY_VIEW) {
		sorted_track_to_iter(tt, &iter);
		editable_set_sel(&lib_editable, &iter);
	}
}

static void store_sel_track(void)
{
	struct tree_track *tt = get_sel_track();
	if (tt) {
		sel_track_ti = tree_track_info(tt);
		track_info_ref(sel_track_ti);
	}
}

static void restore_sel_track(void)
{
	if (sel_track_ti) {
		struct tree_track *tt = lib_find_track(sel_track_ti);
		if (tt) {
			set_sel_track(tt);
			track_info_unref(sel_track_ti);
			sel_track_ti = NULL;
		}
	}
}

/* determine if filter results could grow, in which case all tracks must be
 * cleared and re-added */
static int do_clear_before(const char *str, struct expr *expr)
{
	if (!lib_live_filter)
		return 0;
	if (!str)
		return 1;
	if ((!expr && live_filter_expr) || (expr && !live_filter_expr))
		return 1;
	if (!expr || expr_is_harmless(expr))
		return !strstr(str, lib_live_filter);
	return 1;
}

void lib_set_live_filter(const char *str)
{
	int clear_before;
	struct expr *expr = NULL;

	if (strcmp0(str, lib_live_filter) == 0)
		return;

	if (str && expr_is_short(str)) {
		expr = expr_parse(str);
		if (!expr)
			return;
	}

	clear_before = do_clear_before(str, expr);

	if (!str)
		store_sel_track();

	unset_live_filter();
	lib_live_filter = str ? xstrdup(str) : NULL;
	live_filter_expr = expr;
	do_lib_filter(clear_before);

	if (!str)
		restore_sel_track();
}

int lib_remove(struct track_info *ti)
{
	struct simple_track *track;

	list_for_each_entry(track, &lib_editable.head, node)
	{
		if (track->info == ti) {
			editable_remove_track(&lib_editable, track);
			return 1;
		}
	}
	return 0;
}

void lib_clear_store(void)
{
	int i;

	for (i = 0; i < FH_SIZE; i++) {
		struct fh_entry *e, *next;

		e = ti_hash[i];
		while (e) {
			next = e->next;
			track_info_unref(e->ti);
			free(e);
			e = next;
		}
		ti_hash[i] = NULL;
	}
}

void sorted_sel_current(void)
{
	if (lib_cur_track) {
		struct iter iter;

		sorted_track_to_iter(lib_cur_track, &iter);
		editable_set_sel(&lib_editable, &iter);
	}
}

static int ti_cmp(const void *a, const void *b)
{
	const struct track_info *ai = *(const struct track_info **)a;
	const struct track_info *bi = *(const struct track_info **)b;

	return track_info_cmp(ai, bi, lib_editable.shared->sort_keys);
}

static int do_lib_for_each(int (*cb)(void *data, struct track_info *ti),
			   void *data, int filtered)
{
	int i, rc = 0, count = 0, size = 1024;
	struct track_info **tis;

	tis = xnew(struct track_info *, size);

	/* collect all track_infos */
	for (i = 0; i < FH_SIZE; i++) {
		struct fh_entry *e;

		e = ti_hash[i];
		while (e) {
			if (count == size) {
				size *= 2;
				tis = xrenew(struct track_info *, tis, size);
			}
			if (!filtered || !filter || expr_eval(filter, e->ti))
				tis[count++] = e->ti;
			e = e->next;
		}
	}

	/* sort to speed up playlist loading */
	qsort(tis, count, sizeof(struct track_info *), ti_cmp);
	for (i = 0; i < count; i++) {
		rc = cb(data, tis[i]);
		if (rc)
			break;
	}

	free(tis);
	return rc;
}

int lib_for_each(int (*cb)(void *data, struct track_info *ti), void *data,
		 void *opaque)
{
	return do_lib_for_each(cb, data, 0);
}

int lib_for_each_filtered(int (*cb)(void *data, struct track_info *ti),
			  void *data, void *opaque)
{
	return do_lib_for_each(cb, data, 1);
}
