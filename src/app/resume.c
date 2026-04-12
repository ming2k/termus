#include "app/resume.h"
#include "app/commands.h"
#include "app/options_library_state.h"
#include "app/options_registry.h"
#include "app/options_ui_state.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/misc.h"
#include "common/prog.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "core/player.h"
#include "library/cache.h"
#include "library/filters.h"
#include "library/lib.h"
#include "library/pl.h"
#include "ui/browser.h"
#include "ui/ui.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

struct resume {
	enum player_status status;
	char *filename;
	long int position;
	char *lib_filename;
	int view;
	char *live_filter;
	char *browser_dir;
	char *browser_sel;
	char *marked_pl;
};

static int handle_resume_line(void *data, const char *line)
{
	struct resume *resume = data;
	char *cmd, *arg;

	if (!parse_command(line, &cmd, &arg))
		return 0;
	if (!arg)
		goto out;

	if (strcmp(cmd, "status") == 0) {
		parse_enum(arg, 0, NR_PLAYER_STATUS, player_status_names, (int *)&resume->status);
	} else if (strcmp(cmd, "file") == 0) {
		free(resume->filename);
		resume->filename = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "position") == 0) {
		str_to_int(arg, &resume->position);
	} else if (strcmp(cmd, "lib_file") == 0) {
		free(resume->lib_filename);
		resume->lib_filename = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "view") == 0) {
		parse_enum(arg, 0, NR_VIEWS, view_names, &resume->view);
	} else if (strcmp(cmd, "live-filter") == 0) {
		free(resume->live_filter);
		resume->live_filter = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "browser-dir") == 0) {
		free(resume->browser_dir);
		resume->browser_dir = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "browser-sel") == 0) {
		free(resume->browser_sel);
		resume->browser_sel = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "active-pl") == 0) {
		free(pl_resume_name);
		pl_resume_name = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "active-pl-row") == 0) {
		str_to_int(arg, &pl_resume_row);
	} else if (strcmp(cmd, "visible-pl") == 0) {
		free(pl_visible_resume_name);
		pl_visible_resume_name = xstrdup(unescape(arg));
	} else if (strcmp(cmd, "visible-pl-row") == 0) {
		str_to_int(arg, &pl_visible_resume_row);
	} else if (strcmp(cmd, "marked-pl") == 0) {
		free(resume->marked_pl);
		resume->marked_pl = xstrdup(unescape(arg));
	}

	free(arg);
out:
	free(cmd);
	return 0;
}

void resume_load(void)
{
	char filename[512];
	struct track_info *ti, *old;
	struct resume resume = { .status = PLAYER_STATUS_STOPPED, .view = -1 };

	snprintf(filename, sizeof(filename), "%s/resume", termus_state_dir);
	if (file_for_each_line(filename, handle_resume_line, &resume) == -1) {
		if (errno != ENOENT) {
			error_msg("loading %s: %s", filename, strerror(errno));
		}
		return;
	}
	if (resume.view >= 0 && resume.view != cur_view)
		set_view(resume.view);

	if (resume.lib_filename) {
		cache_lock();
		ti = old = cache_get_ti(resume.lib_filename, 0);
		cache_unlock();
		if (ti) {
			lib_add_track(ti, NULL);
			lib_store_cur_track(ti);
			track_info_unref(ti);
			ti = lib_set_track(lib_find_track(ti));
			if (ti) {
				BUG_ON(ti != old);
				track_info_unref(ti);
				tree_sel_current(options_get_auto_expand_albums_follow());
				sorted_sel_current();
			}
		}
	}

	if (resume.filename) {
		cache_lock();
		ti = cache_get_ti(resume.filename, 0);
		cache_unlock();
		if (ti) {
			player_set_file(ti);
			player_play();
			player_seek(resume.position, 0, 0);
			if (resume.status != PLAYER_STATUS_STOPPED)
				player_pause();
		}
	}

	if (resume.live_filter) {
		filters_set_live(resume.live_filter);
		update_filterline();
	}

	if (resume.browser_dir) {
		browser_chdir(resume.browser_dir);
		if (resume.browser_sel) {
			browser_set_sel_name(resume.browser_sel);
		}
	}

	if (resume.marked_pl) {
		pl_set_marked_pl_by_name(resume.marked_pl);
	}

	free(resume.filename);
	free(resume.lib_filename);
	free(resume.live_filter);
	free(resume.browser_dir);
	free(resume.browser_sel);
	free(resume.marked_pl);
}

void resume_exit(void)
{
	char filename_tmp[512];
	char filename[512];
	const char *pl_name;
	struct track_info *ti;
	FILE *f;
	int rc;

	snprintf(filename_tmp, sizeof(filename_tmp), "%s/resume.tmp", termus_state_dir);
	f = fopen(filename_tmp, "w");
	if (!f) {
		warn_errno("creating %s", filename_tmp);
		return;
	}

	fprintf(f, "status %s\n", player_status_names[player_info.status]);
	ti = player_info.ti;
	if (ti) {
		fprintf(f, "file %s\n", escape(ti->filename));
		fprintf(f, "position %d\n", player_info.pos);
	}
	if (lib_cur_track)
		ti = tree_track_info(lib_cur_track);
	else
		ti = lib_get_cur_stored_track();
	if (ti)
		fprintf(f, "lib_file %s\n", escape(ti->filename));
	fprintf(f, "view %s\n", view_names[cur_view]);
	if (lib_live_filter)
		fprintf(f, "live-filter %s\n", escape(lib_live_filter));
	fprintf(f, "browser-dir %s\n", escape(browser_dir));
	char *browser_sel = browser_get_sel_name();
	if (browser_sel) {
		fprintf(f, "browser-sel %s\n", escape(browser_sel));
		free(browser_sel);
	}

	if ((pl_name = pl_playing_pl_name())) {
		fprintf(f, "active-pl %s\n", escape(pl_name));
		fprintf(f, "active-pl-row %d\n", pl_playing_pl_row());
	}

	fprintf(f, "visible-pl %s\n", escape(pl_visible_get_name()));
	fprintf(f, "visible-pl-row %d\n", pl_visible_get_sel_row());

	fprintf(f, "marked-pl %s\n", escape(pl_marked_pl_name()));

	fclose(f);

	snprintf(filename, sizeof(filename), "%s/resume", termus_state_dir);
	rc = rename(filename_tmp, filename);
	if (rc)
		warn_errno("renaming %s to %s", filename_tmp, filename);
}
