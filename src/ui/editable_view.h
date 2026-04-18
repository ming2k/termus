#ifndef TERMUS_EDITABLE_VIEW_H
#define TERMUS_EDITABLE_VIEW_H

#include "common/iter.h"
#include "library/editable.h"

struct window;
struct editable_view;

struct editable_view *editable_view_new(int (*get_prev)(struct iter *),
					int (*get_next)(struct iter *));
void editable_view_free(struct editable_view *view);
const struct editable_view_ops *editable_view_get_ops(void);
struct window *editable_view_window(struct editable_view *view);
void editable_view_mark_changed(struct editable_view *view);
int editable_view_needs_redraw(const struct editable_view *view);
void editable_view_clear_changed(struct editable_view *view);
void editable_view_goto_top(struct editable_view *view);
void editable_view_down(struct editable_view *view, int n);
void editable_view_row_vanishes(struct editable_view *view, struct iter *iter);
int editable_view_get_sel(struct editable_view *view, struct iter *iter);
void editable_view_set_sel(struct editable_view *view, struct iter *iter);
void editable_view_set_nr_rows(struct editable_view *view, int n);
void editable_view_set_contents(struct editable_view *view, void *head);

#endif
