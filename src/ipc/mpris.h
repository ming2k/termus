#ifndef TERMUS_MPRIS_H
#define TERMUS_MPRIS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CONFIG_MPRIS

extern int mpris_fd;
void mpris_init(void);
void mpris_process(void);
void mpris_free(void);
void mpris_playback_status_changed(void);
void mpris_loop_status_changed(void);
void mpris_rate_changed(void);
void mpris_shuffle_changed(void);
void mpris_volume_changed(void);
void mpris_metadata_changed(void);
void mpris_seeked(void);

#else

#define mpris_fd -1
#define mpris_init()                                                           \
	{                                                                      \
	}
#define mpris_process()                                                        \
	{                                                                      \
	}
#define mpris_free()                                                           \
	{                                                                      \
	}
#define mpris_playback_status_changed()                                        \
	{                                                                      \
	}
#define mpris_loop_status_changed()                                            \
	{                                                                      \
	}
#define mpris_rate_changed()                                                   \
	{                                                                      \
	}
#define mpris_shuffle_changed()                                                \
	{                                                                      \
	}
#define mpris_volume_changed()                                                 \
	{                                                                      \
	}
#define mpris_metadata_changed()                                               \
	{                                                                      \
	}
#define mpris_seeked()                                                         \
	{                                                                      \
	}

#endif

#endif
