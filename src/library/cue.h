#ifndef TERMUS_CUE_H
#define TERMUS_CUE_H

#include <stdint.h>

#include "common/list.h"

struct cue_meta {
	char *performer;
	char *songwriter;
	char *title;
	char *genre;
	char *date;
	char *comment;
	char *compilation;
	char *discnumber;
	char *rg_gain;
	char *rg_peak;
};

struct cue_track {
	char *file; /* owned by cue_sheet */
	double offset;
	double length;
	size_t number;

	struct cue_meta meta;
};

struct cue_track_file {
	struct list_head node;

	char *file;
};

struct cue_sheet {
	struct list_head files;
	struct cue_track *tracks;
	size_t num_tracks;

	struct cue_meta meta;
};

struct cue_sheet *cue_parse(const char *src, size_t len);
struct cue_sheet *cue_from_file(const char *file);
void cue_free(struct cue_sheet *s);
struct cue_track *cue_get_track(struct cue_sheet *s, size_t n);

#endif
