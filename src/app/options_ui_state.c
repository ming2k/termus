#include "app/options_ui_state.h"

#include "common/xmalloc.h"

#include <stddef.h>
#include <stdlib.h>

int cur_view = TREE_VIEW;
char *status_display_program = NULL;
char *status_display_file = NULL;
int confirm_run = 1;
int resume_termus = 1;
int set_term_title = 1;
int wrap_search = 1;
int skip_track_info = 0;
int ignore_duplicates = 0;
int mouse = 0;
int mpris = 1;
int start_view = TREE_VIEW;
int tree_width_percent = 33;
int tree_width_max = 0;
int pause_on_output_change = 0;
int block_key_paste = 1;
int progress_bar = 1;
int search_resets_position = 1;
char *colorscheme = NULL;

const char *const view_names[NR_VIEWS + 1] = {"tree",	  "sorted",  "playlist",
					      "queue",	  "browser", "filters",
					      "settings", NULL};

const char *const progress_bar_names[NR_PROGRESS_BAR_MODES + 1] = {
    "disabled", "line", "shuttle", "color", "color-shuttle", NULL};

int options_get_resume_termus(void) { return resume_termus; }

int options_get_start_view(void) { return start_view; }

void options_set_start_view(int view) { start_view = view; }

int options_get_tree_width_percent(void) { return tree_width_percent; }

void options_set_tree_width_percent(int percent)
{
	tree_width_percent = percent;
}

int options_get_tree_width_max(void) { return tree_width_max; }

void options_set_tree_width_max(int cols) { tree_width_max = cols; }

int options_get_progress_bar(void) { return progress_bar; }

int options_get_confirm_run(void) { return confirm_run; }

int options_get_wrap_search(void) { return wrap_search; }

int options_get_skip_track_info(void) { return skip_track_info; }

int options_get_ignore_duplicates(void) { return ignore_duplicates; }

int options_get_mouse(void) { return mouse; }

int options_get_search_resets_position(void) { return search_resets_position; }

int options_get_set_term_title(void) { return set_term_title; }

int options_get_mpris(void) { return mpris; }

int options_get_pause_on_output_change(void) { return pause_on_output_change; }

int options_get_block_key_paste(void) { return block_key_paste; }

const char *options_get_status_display_program(void)
{
	return status_display_program;
}

const char *options_get_status_display_file(void)
{
	return status_display_file;
}

const char *options_get_colorscheme(void)
{
	return colorscheme ? colorscheme : "";
}

void options_set_colorscheme(const char *name)
{
	free(colorscheme);
	colorscheme = (name && name[0]) ? xstrdup(name) : NULL;
}
