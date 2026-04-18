#ifndef TERMUS_OPTIONS_UI_STATE_H
#define TERMUS_OPTIONS_UI_STATE_H

enum {
	TREE_VIEW,
	SORTED_VIEW,
	PLAYLIST_VIEW,
	QUEUE_VIEW,
	BROWSER_VIEW,
	FILTERS_VIEW,
	HELP_VIEW,
	NR_VIEWS
};

enum progress_bar_mode {
	PROGRESS_BAR_DISABLED,
	PROGRESS_BAR_LINE,
	PROGRESS_BAR_SHUTTLE,
	PROGRESS_BAR_COLOR,
	PROGRESS_BAR_COLOR_SHUTTLE,
	NR_PROGRESS_BAR_MODES
};

extern const char *const view_names[NR_VIEWS + 1];
extern const char *const progress_bar_names[NR_PROGRESS_BAR_MODES + 1];

extern int cur_view;
extern char *status_display_program;
extern char *status_display_file;
extern int confirm_run;
extern int resume_termus;
extern int set_term_title;
extern int wrap_search;
extern int skip_track_info;
extern int ignore_duplicates;
extern int mouse;
extern int mpris;
extern int start_view;
extern int tree_width_percent;
extern int tree_width_max;
extern int pause_on_output_change;
extern int block_key_paste;
extern int progress_bar;
extern int search_resets_position;
extern char *colorscheme;

int options_get_resume_termus(void);

int options_get_start_view(void);
void options_set_start_view(int view);

int options_get_tree_width_percent(void);
void options_set_tree_width_percent(int percent);

int options_get_tree_width_max(void);
void options_set_tree_width_max(int cols);

int options_get_progress_bar(void);

int options_get_confirm_run(void);

int options_get_wrap_search(void);

int options_get_skip_track_info(void);

int options_get_ignore_duplicates(void);

int options_get_mouse(void);

int options_get_search_resets_position(void);

int options_get_set_term_title(void);

int options_get_mpris(void);

int options_get_pause_on_output_change(void);

int options_get_block_key_paste(void);

const char *options_get_status_display_program(void);
const char *options_get_status_display_file(void);

const char *options_get_colorscheme(void);
void options_set_colorscheme(const char *name);

#endif
