#ifndef TERMUS_PLAY_QUEUE_H
#define TERMUS_PLAY_QUEUE_H

#include "core/track_info.h"
#include "library/editable.h"

struct window;

extern struct editable pq_editable;

void play_queue_init(void);
void play_queue_attach_view(void *view_data,
			    const struct editable_view_ops *view_ops,
			    struct window *win);
void play_queue_append(struct track_info *ti, void *opaque);
void play_queue_prepend(struct track_info *ti, void *opaque);
struct track_info *play_queue_remove(void);
int play_queue_for_each(int (*cb)(void *data, struct track_info *ti),
			void *data, void *opaque);
unsigned int play_queue_total_time(void);
struct window *play_queue_win(void);
void play_queue_set_nr_rows(int rows);
int queue_needs_redraw(void);
void queue_post_update(void);

#endif
