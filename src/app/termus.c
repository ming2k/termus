#include "app/termus.h"
#include "app/options_core_state.h"
#include "app/options_playback_state.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/gbuf.h"
#include "common/locking.h"
#include "common/misc.h"
#include "common/msg.h"
#include "common/path.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "core/input.h"
#include "core/player.h"
#include "library/cache.h"
#include "library/lib.h"
#include "library/load_dir.h"
#include "library/pl.h"
#include "library/pl_env.h"
#include "library/play_queue.h"
#include "ui/job.h"

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* save_playlist_cb, save_ext_playlist_cb */
typedef int (*save_tracks_cb)(void *data, struct track_info *ti);

volatile sig_atomic_t termus_running = 1;
int ui_initialized = 0;
static int termus_client_fd = -1;

static char **playable_exts;
static const char *const playlist_exts[] = {"m3u", "pl", "pls", NULL};

int termus_next_track_request_fd;
static bool play_queue_active = false;
static int termus_next_track_request_fd_priv;
static pthread_mutex_t termus_next_file_mutex = TERMUS_MUTEX_INITIALIZER;
static pthread_cond_t termus_next_file_cond = TERMUS_COND_INITIALIZER;
static int termus_next_file_provided;
static struct track_info *termus_next_file;

static int x11_init_done = 0;
static void *(*x11_open)(void *) = NULL;
static int (*x11_raise)(void *, int) = NULL;
static int (*x11_close)(void *) = NULL;

int termus_init(void)
{
	playable_exts = ip_get_supported_extensions();
	cache_init();
	job_init();
	play_queue_init();
	return 0;
}

void termus_exit(void)
{
	job_exit();
	if (cache_close())
		d_print("error: %s\n", strerror(errno));
}

void termus_next(void)
{
	struct track_info *info = termus_get_next_track();
	if (info)
		player_set_file(info);
}

void termus_prev(void)
{
	struct track_info *info;

	if (options_get_play_library()) {
		info = lib_goto_prev();
	} else {
		info = pl_goto_prev();
	}

	if (info)
		player_set_file(info);
}

void termus_next_album(void)
{
	struct track_info *info;

	if (options_get_play_library()) {
		info = lib_goto_next_album();
	} else {
		info = pl_goto_next();
	}

	if (info)
		player_set_file(info);
}

void termus_prev_album(void)
{
	struct track_info *info;

	if (options_get_play_library()) {
		info = lib_goto_prev_album();
	} else {
		info = pl_goto_prev();
	}

	if (info)
		player_set_file(info);
}

void termus_play_file(const char *filename)
{
	struct track_info *ti;

	cache_lock();
	ti = cache_get_ti(filename, 0);
	cache_unlock();
	if (!ti) {
		error_msg("Couldn't get file information for %s\n", filename);
		return;
	}

	player_play_file(ti);
}

enum file_type termus_detect_ft(const char *name, char **ret)
{
	char *absolute;
	struct stat st;

	if (is_http_url(name) || is_cue_url(name)) {
		*ret = xstrdup(name);
		return FILE_TYPE_URL;
	}

	*ret = NULL;
	absolute = path_absolute(name);
	if (absolute == NULL)
		return FILE_TYPE_INVALID;

	/* stat follows symlinks, lstat does not */
	if (stat(absolute, &st) == -1) {
		free(absolute);
		return FILE_TYPE_INVALID;
	}

	if (S_ISDIR(st.st_mode)) {
		*ret = absolute;
		return FILE_TYPE_DIR;
	}
	if (!S_ISREG(st.st_mode)) {
		free(absolute);
		errno = EINVAL;
		return FILE_TYPE_INVALID;
	}

	*ret = absolute;
	if (termus_is_playlist(absolute))
		return FILE_TYPE_PL;

	/* NOTE: it could be FILE_TYPE_PL too! */
	return FILE_TYPE_FILE;
}

void termus_add(add_ti_cb add, const char *name, enum file_type ft, int jt,
		int force, void *opaque)
{
	struct add_data *data = xnew(struct add_data, 1);

	data->add = add;
	data->name = xstrdup(name);
	data->type = ft;
	data->force = force;
	data->opaque = opaque;

	job_schedule_add(jt, data);
}

static int save_ext_playlist_cb(void *data, struct track_info *ti)
{
	GBUF(buf);
	int fd = *(int *)data;
	int i, rc;

	gbuf_addf(&buf, "file %s\n", escape(ti->filename));
	gbuf_addf(&buf, "duration %d\n", ti->duration);
	gbuf_addf(&buf, "codec %s\n", ti->codec);
	gbuf_addf(&buf, "bitrate %ld\n", ti->bitrate);
	for (i = 0; ti->comments[i].key; i++)
		gbuf_addf(&buf, "tag %s %s\n", ti->comments[i].key,
			  escape(ti->comments[i].val));

	rc = write_all(fd, buf.buffer, buf.len);
	gbuf_free(&buf);

	if (rc == -1)
		return -1;
	return 0;
}

static int save_playlist_cb(void *data, struct track_info *ti)
{
	char *proc_filename = pl_env_reduce(ti->filename);
	int fd = *(int *)data;
	const char nl = '\n';
	int rc;

	rc = write_all(fd, proc_filename, strlen(proc_filename));
	free(proc_filename);
	if (rc == -1)
		return -1;

	rc = write_all(fd, &nl, 1);
	if (rc == -1)
		return -1;

	return 0;
}

static int do_termus_save(for_each_ti_cb for_each_ti, const char *filename,
			  save_tracks_cb save_tracks, void *opaque)
{
	int fd, rc;

	if (strcmp(filename, "-") == 0) {
		if (termus_get_client_fd() == -1) {
			error_msg("saving to stdout works only remotely");
			return 0;
		}
		fd = dup(termus_get_client_fd());
	} else
		fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd == -1)
		return -1;
	rc = for_each_ti(save_tracks, &fd, opaque);
	close(fd);
	return rc;
}

int termus_save(for_each_ti_cb for_each_ti, const char *filename, void *opaque)
{
	return do_termus_save(for_each_ti, filename, save_playlist_cb, opaque);
}

int termus_save_ext(for_each_ti_cb for_each_ti, const char *filename,
		    void *opaque)
{
	return do_termus_save(for_each_ti, filename, save_ext_playlist_cb,
			      opaque);
}

static int update_cb(void *data, struct track_info *ti)
{
	struct update_data *d = data;

	if (d->size == d->used) {
		if (d->size == 0)
			d->size = 16;
		d->size *= 2;
		d->ti = xrenew(struct track_info *, d->ti, d->size);
	}
	track_info_ref(ti);
	d->ti[d->used++] = ti;
	return 0;
}

void termus_update_cache(int force)
{
	struct update_cache_data *data;

	data = xnew(struct update_cache_data, 1);
	data->force = force;

	job_schedule_update_cache(JOB_TYPE_LIB, data);
}

void termus_update_lib(void)
{
	struct update_data *data;

	data = xnew0(struct update_data, 1);

	lib_for_each(update_cb, data, NULL);

	job_schedule_update(data);
}

void termus_update_tis(struct track_info **tis, int nr, int force)
{
	struct update_data *data;

	data = xnew(struct update_data, 1);
	data->size = nr;
	data->used = nr;
	data->ti = tis;
	data->force = force;

	job_schedule_update(data);
}

static const char *get_ext(const char *filename)
{
	const char *ext = strrchr(filename, '.');

	if (ext)
		ext++;
	return ext;
}

static int str_in_array(const char *str, const char *const *array)
{
	int i;

	for (i = 0; array[i]; i++) {
		if (strcasecmp(str, array[i]) == 0)
			return 1;
	}
	return 0;
}

int termus_is_playlist(const char *filename)
{
	const char *ext = get_ext(filename);

	return ext && str_in_array(ext, playlist_exts);
}

int termus_is_playable(const char *filename)
{
	const char *ext = get_ext(filename);

	return ext && str_in_array(ext, (const char *const *)playable_exts);
}

int termus_is_supported(const char *filename)
{
	const char *ext = get_ext(filename);

	return ext && (str_in_array(ext, (const char *const *)playable_exts) ||
		       str_in_array(ext, playlist_exts));
}

struct pl_data {
	int (*cb)(void *data, const char *line);
	void *data;
};

static int pl_handle_line(void *data, const char *line)
{
	struct pl_data *d = data;
	int i = 0;

	while (isspace((unsigned char)line[i]))
		i++;
	if (line[i] == 0)
		return 0;

	if (line[i] == '#')
		return 0;

	return d->cb(d->data, line);
}

static int pls_handle_line(void *data, const char *line)
{
	struct pl_data *d = data;

	if (strncasecmp(line, "file", 4))
		return 0;
	line = strchr(line, '=');
	if (line == NULL)
		return 0;
	return d->cb(d->data, line + 1);
}

int termus_playlist_for_each(const char *buf, int size, int reverse,
			     int (*cb)(void *data, const char *line),
			     void *data)
{
	struct pl_data d = {cb, data};
	int (*handler)(void *, const char *);

	handler = pl_handle_line;
	if (size >= 10 && strncasecmp(buf, "[playlist]", 10) == 0)
		handler = pls_handle_line;

	if (reverse) {
		buffer_for_each_line_reverse(buf, size, handler, &d);
	} else {
		buffer_for_each_line(buf, size, handler, &d);
	}
	return 0;
}

/* multi-threaded next track requests */

#define termus_next_file_lock() termus_mutex_lock(&termus_next_file_mutex)
#define termus_next_file_unlock() termus_mutex_unlock(&termus_next_file_mutex)

static struct track_info *termus_get_next_from_main_thread(void)
{
	struct track_info *ti = play_queue_remove();
	if (ti) {
		play_queue_active = true;
	} else {
		if (!play_queue_active || !stop_after_queue)
			ti = options_get_play_library() ? lib_goto_next()
							: pl_goto_next();
		play_queue_active = false;
	}
	return ti;
}

static struct track_info *termus_get_next_from_other_thread(void)
{
	static pthread_mutex_t mutex = TERMUS_MUTEX_INITIALIZER;
	termus_mutex_lock(&mutex);

	/* only one thread may request a track at a time */

	notify_via_pipe(termus_next_track_request_fd_priv);

	termus_next_file_lock();
	while (!termus_next_file_provided)
		pthread_cond_wait(&termus_next_file_cond,
				  &termus_next_file_mutex);
	struct track_info *ti = termus_next_file;
	termus_next_file_provided = 0;
	termus_next_file_unlock();

	termus_mutex_unlock(&mutex);

	return ti;
}

struct track_info *termus_get_next_track(void)
{
	pthread_t this_thread = pthread_self();
	if (pthread_equal(this_thread, main_thread))
		return termus_get_next_from_main_thread();
	return termus_get_next_from_other_thread();
}

void termus_provide_next_track(void)
{
	clear_pipe(termus_next_track_request_fd, 1);

	termus_next_file_lock();
	termus_next_file = termus_get_next_from_main_thread();
	termus_next_file_provided = 1;
	termus_next_file_unlock();

	pthread_cond_broadcast(&termus_next_file_cond);
}

void termus_track_request_init(void)
{
	init_pipes(&termus_next_track_request_fd,
		   &termus_next_track_request_fd_priv);
}

static int termus_can_raise_vte_x11(void)
{
	return getenv("DISPLAY") && getenv("WINDOWID");
}

int termus_can_raise_vte(void) { return termus_can_raise_vte_x11(); }

static int termus_raise_vte_x11_error(void) { return 0; }

void termus_raise_vte(void)
{
	if (termus_can_raise_vte_x11()) {
		if (!x11_init_done) {
			void *x11;

			x11_init_done = 1;
			x11 = dlopen("libX11.so", RTLD_LAZY);

			if (x11) {
				int (*x11_error)(void *);

				x11_error = dlsym(x11, "XSetErrorHandler");
				x11_open = dlsym(x11, "XOpenDisplay");
				x11_raise = dlsym(x11, "XRaiseWindow");
				x11_close = dlsym(x11, "XCloseDisplay");

				if (x11_error) {
					x11_error(termus_raise_vte_x11_error);
				}
			}
		}

		if (x11_open && x11_raise && x11_close) {
			char *xid_str;
			long int xid = 0;

			xid_str = getenv("WINDOWID");
			if (!str_to_int(xid_str, &xid) && xid != 0) {
				void *display;

				display = x11_open(NULL);
				if (display) {
					x11_raise(display, (int)xid);
					x11_close(display);
				}
			}
		}
	}
}

bool termus_queue_active(void) { return play_queue_active; }

void termus_set_client_fd(int fd) { termus_client_fd = fd; }

int termus_get_client_fd(void) { return termus_client_fd; }
