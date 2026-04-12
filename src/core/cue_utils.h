#ifndef TERMUS_CUE_UTILS_H
#define TERMUS_CUE_UTILS_H

#include <stdio.h>

int is_cue(const char *filename);
int cue_get_track_nums(const char *filename, int **out_nums);
int cue_get_files(const char *filename, char ***out_files);
char *construct_cue_url(const char *cue_filename, int track_n);


#endif
