#include "core/cue_utils.h"
#include "common/path.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "library/cue.h"

#include <stdio.h>

int is_cue(const char *filename)
{
	const char *ext = get_extension(filename);
	return ext != NULL && strcmp(ext, "cue") == 0;
}

int cue_get_track_nums(const char *filename, int **out_nums)
{
	struct cue_sheet *cd = cue_from_file(filename);
	if (!cd)
		return -1;

	int n = cd->num_tracks;
	*out_nums = xnew(int, n);

	int i;
	for (i = 0; i < n; i++)
		(*out_nums)[i] = cd->tracks[i].number;

	cue_free(cd);
	return n;
}

int cue_get_files(const char *filename, char ***out_files)
{
	struct cue_sheet *cd = cue_from_file(filename);
	if (!cd)
		return -1;

	int n = list_len(&cd->files);
	*out_files = xnew(char *, n);

	int i = 0;
	struct cue_track_file *tf;
	list_for_each_entry(tf, &cd->files, node)
	{
		(*out_files)[i] = tf->file;
		tf->file = NULL;
		i++;
	}

	cue_free(cd);
	return n;
}

char *construct_cue_url(const char *cue_filename, int track_n)
{
	char buf[4096] = {0};

	snprintf(buf, sizeof(buf), "cue://%s/%d", cue_filename, track_n);

	return xstrdup(buf);
}
