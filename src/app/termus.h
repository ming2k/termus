#ifndef TERMUS_API_H
#define TERMUS_API_H

#include "core/track_info.h"
#include "core/track_info_cb.h"

#include <signal.h>
#include <stdbool.h>

extern volatile sig_atomic_t termus_running;
extern int ui_initialized;

enum file_type {
	FILE_TYPE_INVALID,
	FILE_TYPE_URL,
	FILE_TYPE_PL,
	FILE_TYPE_DIR,
	FILE_TYPE_FILE,
};

/* lib_for_each, lib_for_each_filtered, pl_for_each, play_queue_for_each */
typedef int (*for_each_ti_cb)(track_info_cb cb, void *data, void *opaque);

/* lib_for_each_sel, pl_for_each_sel, play_queue_for_each_sel */
typedef int (*for_each_sel_ti_cb)(track_info_cb cb, void *data, int reverse,
				  int advance);

/* lib_add_track, pl_add_track, play_queue_append, play_queue_prepend */
typedef void (*add_ti_cb)(struct track_info *, void *opaque);

/* termus_save, termus_save_ext */
typedef int (*save_ti_cb)(for_each_ti_cb for_each_ti, const char *filename,
			  void *opaque);

int termus_init(void);
void termus_exit(void);
void termus_play_file(const char *filename);

/* detect file type, returns absolute path or url in @ret */
enum file_type termus_detect_ft(const char *name, char **ret);

/* add to library, playlist or queue view
 *
 * @add   callback that does the actual adding
 * @name  playlist, directory, file, URL
 * @ft    detected FILE_TYPE_*
 * @jt    JOB_TYPE_{LIB,PL,QUEUE}
 *
 * returns immediately, actual work is done in the worker thread.
 */
void termus_add(add_ti_cb, const char *name, enum file_type ft, int jt,
		int force, void *opaque);

int termus_save(for_each_ti_cb for_each_ti, const char *filename, void *opaque);
int termus_save_ext(for_each_ti_cb for_each_ti, const char *filename,
		    void *opaque);

void termus_update_cache(int force);
void termus_update_lib(void);
void termus_update_tis(struct track_info **tis, int nr, int force);

int termus_is_playlist(const char *filename);
int termus_is_playable(const char *filename);
int termus_is_supported(const char *filename);

int termus_playlist_for_each(const char *buf, int size, int reverse,
			     int (*cb)(void *data, const char *line),
			     void *data);

void termus_next(void);
void termus_prev(void);
void termus_next_album(void);
void termus_prev_album(void);

extern int termus_next_track_request_fd;
struct track_info *termus_get_next_track(void);
void termus_provide_next_track(void);
void termus_track_request_init(void);

int termus_can_raise_vte(void);
void termus_raise_vte(void);

bool termus_queue_active(void);
void termus_set_client_fd(int fd);
int termus_get_client_fd(void);

#endif
