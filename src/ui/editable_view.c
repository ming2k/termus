#include "ui/editable_view.h"
#include "common/xmalloc.h"
#include "ui/window.h"

struct editable_view {
	struct window *window;
};

static void editable_view_ops_mark_changed(void *view_data)
{
	editable_view_mark_changed(view_data);
}

static int editable_view_ops_needs_redraw(const void *view_data)
{
	return editable_view_needs_redraw(view_data);
}

static void editable_view_ops_clear_changed(void *view_data)
{
	editable_view_clear_changed(view_data);
}

static void editable_view_ops_goto_top(void *view_data)
{
	editable_view_goto_top(view_data);
}

static void editable_view_ops_down(void *view_data, int n)
{
	editable_view_down(view_data, n);
}

static void editable_view_ops_row_vanishes(void *view_data, struct iter *iter)
{
	editable_view_row_vanishes(view_data, iter);
}

static int editable_view_ops_get_sel(void *view_data, struct iter *iter)
{
	return editable_view_get_sel(view_data, iter);
}

static void editable_view_ops_set_sel(void *view_data, struct iter *iter)
{
	editable_view_set_sel(view_data, iter);
}

static void editable_view_ops_set_nr_rows(void *view_data, int n)
{
	editable_view_set_nr_rows(view_data, n);
}

static void editable_view_ops_set_contents(void *view_data, void *head)
{
	editable_view_set_contents(view_data, head);
}

static const struct editable_view_ops editable_view_ops = {
    .mark_changed = editable_view_ops_mark_changed,
    .needs_redraw = editable_view_ops_needs_redraw,
    .clear_changed = editable_view_ops_clear_changed,
    .goto_top = editable_view_ops_goto_top,
    .down = editable_view_ops_down,
    .row_vanishes = editable_view_ops_row_vanishes,
    .get_sel = editable_view_ops_get_sel,
    .set_sel = editable_view_ops_set_sel,
    .set_nr_rows = editable_view_ops_set_nr_rows,
    .set_contents = editable_view_ops_set_contents,
};

struct editable_view *editable_view_new(int (*get_prev)(struct iter *),
					int (*get_next)(struct iter *))
{
	struct editable_view *view = xnew(struct editable_view, 1);

	view->window = window_new(get_prev, get_next);
	return view;
}

const struct editable_view_ops *editable_view_get_ops(void)
{
	return &editable_view_ops;
}

void editable_view_free(struct editable_view *view)
{
	if (!view)
		return;

	window_free(view->window);
	free(view);
}

struct window *editable_view_window(struct editable_view *view)
{
	return view ? view->window : NULL;
}

void editable_view_mark_changed(struct editable_view *view)
{
	struct window *win = editable_view_window(view);

	if (!win)
		return;

	win->changed = 1;
	window_changed(win);
}

int editable_view_needs_redraw(const struct editable_view *view)
{
	return view && view->window ? view->window->changed : 0;
}

void editable_view_clear_changed(struct editable_view *view)
{
	struct window *win = editable_view_window(view);

	if (win)
		win->changed = 0;
}

void editable_view_goto_top(struct editable_view *view)
{
	struct window *win = editable_view_window(view);

	if (win)
		window_goto_top(win);
}

void editable_view_down(struct editable_view *view, int n)
{
	struct window *win = editable_view_window(view);

	if (win)
		window_down(win, n);
}

void editable_view_row_vanishes(struct editable_view *view, struct iter *iter)
{
	struct window *win = editable_view_window(view);

	if (win)
		window_row_vanishes(win, iter);
}

int editable_view_get_sel(struct editable_view *view, struct iter *iter)
{
	struct window *win = editable_view_window(view);

	if (!win)
		return 0;
	return window_get_sel(win, iter);
}

void editable_view_set_sel(struct editable_view *view, struct iter *iter)
{
	struct window *win = editable_view_window(view);

	if (win)
		window_set_sel(win, iter);
}

void editable_view_set_nr_rows(struct editable_view *view, int n)
{
	struct window *win = editable_view_window(view);

	if (win)
		window_set_nr_rows(win, n);
}

void editable_view_set_contents(struct editable_view *view, void *head)
{
	struct window *win = editable_view_window(view);

	if (win)
		window_set_contents(win, head);
}
