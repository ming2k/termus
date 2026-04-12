AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/src
AM_CFLAGS = $(DEV_WARN_CFLAGS) $(WERROR_CFLAGS) $(SANITIZER_CFLAGS)
AM_LDFLAGS = $(SANITIZER_LDFLAGS)

inputdir = $(libdir)/termus/input
input_LTLIBRARIES =

common_ldflags = -module -avoid-version -shared

if CONFIG_CDIO
input_LTLIBRARIES += cdio.la
cdio_la_SOURCES  = cdio.c
cdio_la_CFLAGS   = $(CDIO_CFLAGS) $(CDDB_CFLAGS)
cdio_la_LIBADD   = $(CDIO_LIBS) $(CDDB_LIBS)
cdio_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_FLAC
input_LTLIBRARIES += flac.la
flac_la_SOURCES  = flac.c
flac_la_CFLAGS   = $(FLAC_CFLAGS)
flac_la_LIBADD   = $(FLAC_LIBS)
flac_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_MAD
input_LTLIBRARIES += mad.la
mad_la_SOURCES   = mad.c nomad.c nomad.h
mad_la_CFLAGS    = $(MAD_CFLAGS)
mad_la_LIBADD    = -lm $(MAD_LIBS) $(ICONV_LIBS)
mad_la_LDFLAGS   = $(common_ldflags)
endif

if CONFIG_MPG123
input_LTLIBRARIES += mpg123.la
mpg123_la_SOURCES = mpg123.c
mpg123_la_CFLAGS  = $(MPG123_CFLAGS)
mpg123_la_LIBADD  = $(MPG123_LIBS) $(ICONV_LIBS)
mpg123_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_MIKMOD
input_LTLIBRARIES += mikmod.la
mikmod_la_SOURCES = mikmod.c
mikmod_la_CFLAGS  = $(MIKMOD_CFLAGS)
mikmod_la_LIBADD  = $(MIKMOD_LIBS)
mikmod_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_MODPLUG
input_LTLIBRARIES += modplug.la
modplug_la_SOURCES = modplug.c
modplug_la_CFLAGS  = $(MODPLUG_CFLAGS)
modplug_la_LIBADD  = $(MODPLUG_LIBS)
modplug_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_BASS
input_LTLIBRARIES += bass.la
bass_la_SOURCES  = bass.c
bass_la_LIBADD   = $(BASS_LIBS)
bass_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_MPC
input_LTLIBRARIES += mpc.la
mpc_la_SOURCES  = mpc.c
mpc_la_LIBADD   = $(MPC_LIBS)
mpc_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_VORBIS
input_LTLIBRARIES += vorbis.la
vorbis_la_SOURCES = vorbis.c
vorbis_la_CFLAGS  = $(VORBIS_CFLAGS)
vorbis_la_LIBADD  = -lm $(VORBIS_LIBS)
vorbis_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_OPUS
input_LTLIBRARIES += opus.la
opus_la_SOURCES  = opus.c
opus_la_CFLAGS   = $(OPUS_CFLAGS)
opus_la_LIBADD   = $(OPUS_LIBS)
opus_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_WAVPACK
input_LTLIBRARIES += wavpack.la
wavpack_la_SOURCES = wavpack.c
wavpack_la_CFLAGS  = $(WAVPACK_CFLAGS)
wavpack_la_LIBADD  = $(WAVPACK_LIBS)
wavpack_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_WAV
input_LTLIBRARIES += wav.la
wav_la_SOURCES  = wav.c
wav_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_MP4
input_LTLIBRARIES += mp4.la
mp4_la_SOURCES  = mp4.c
mp4_la_CFLAGS   = $(MP4_CFLAGS)
mp4_la_LIBADD   = $(MP4_LIBS)
mp4_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_AAC
input_LTLIBRARIES += aac.la
aac_la_SOURCES  = aac.c aac.h
aac_la_CFLAGS   = $(AAC_CFLAGS)
aac_la_LIBADD   = $(AAC_LIBS)
aac_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_FFMPEG
input_LTLIBRARIES += ffmpeg.la
ffmpeg_la_SOURCES = ffmpeg.c
ffmpeg_la_CFLAGS  = $(FFMPEG_CFLAGS)
ffmpeg_la_LIBADD  = $(FFMPEG_LIBS)
ffmpeg_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_CUE
input_LTLIBRARIES += cue.la
cue_la_SOURCES  = cue.c
cue_la_LIBADD   = -lm
cue_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_VTX
input_LTLIBRARIES += vtx.la
vtx_la_SOURCES  = vtx.c
vtx_la_LIBADD   = $(VTX_LIBS)
vtx_la_LDFLAGS  = $(common_ldflags)
endif
