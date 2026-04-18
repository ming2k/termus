AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/src
AM_CFLAGS = $(DEV_WARN_CFLAGS) $(WERROR_CFLAGS) $(SANITIZER_CFLAGS)
AM_LDFLAGS = $(SANITIZER_LDFLAGS)

inputdir = $(libdir)/termus/input
input_LTLIBRARIES =

common_ldflags = -module -avoid-version -shared

# ── primary backends (ip_priority = 50) ──────────────────────────────────

if CONFIG_FLAC
input_LTLIBRARIES += flac.la
flac_la_SOURCES  = flac.c
flac_la_CFLAGS   = $(FLAC_CFLAGS)
flac_la_LIBADD   = $(FLAC_LIBS)
flac_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_OPUS
input_LTLIBRARIES += opus.la
opus_la_SOURCES  = opus.c
opus_la_CFLAGS   = $(OPUS_CFLAGS)
opus_la_LIBADD   = $(OPUS_LIBS)
opus_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_VORBIS
input_LTLIBRARIES += vorbis.la
vorbis_la_SOURCES = vorbis.c
vorbis_la_CFLAGS  = $(VORBIS_CFLAGS)
vorbis_la_LIBADD  = -lm $(VORBIS_LIBS)
vorbis_la_LDFLAGS = $(common_ldflags)
endif

# Legacy MP3 — single maintained backend
if CONFIG_MPG123
input_LTLIBRARIES += mpg123.la
mpg123_la_SOURCES = mpg123.c
mpg123_la_CFLAGS  = $(MPG123_CFLAGS)
mpg123_la_LIBADD  = $(MPG123_LIBS) $(ICONV_LIBS)
mpg123_la_LDFLAGS = $(common_ldflags)
endif

# ── FFmpeg backend (ip_priority = 10 fallback; 50 for AAC/M4A primary) ───

if CONFIG_AVCODEC
input_LTLIBRARIES += avcodec.la
avcodec_la_SOURCES = avcodec.c
avcodec_la_CFLAGS  = $(AVCODEC_CFLAGS)
avcodec_la_LIBADD  = $(AVCODEC_LIBS)
avcodec_la_LDFLAGS = $(common_ldflags)
endif

# ── playlist overlay ──────────────────────────────────────────────────────

if CONFIG_CUE
input_LTLIBRARIES += cue.la
cue_la_SOURCES  = cue.c
cue_la_LIBADD   = -lm
cue_la_LDFLAGS  = $(common_ldflags)
endif
