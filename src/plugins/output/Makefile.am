AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/src
AM_CFLAGS = $(DEV_WARN_CFLAGS) $(WERROR_CFLAGS) $(SANITIZER_CFLAGS)
AM_LDFLAGS = $(SANITIZER_LDFLAGS)

outputdir = $(libdir)/termus/output
output_LTLIBRARIES =

common_ldflags = -module -avoid-version -shared

if CONFIG_PIPEWIRE
output_LTLIBRARIES += pipewire.la
pipewire_la_SOURCES = pipewire.c
pipewire_la_CFLAGS  = $(PIPEWIRE_CFLAGS)
pipewire_la_LIBADD  = $(PIPEWIRE_LIBS)
pipewire_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_ALSA
output_LTLIBRARIES += alsa.la
alsa_la_SOURCES  = alsa.c mixer_alsa.c mixer_alsa.h
alsa_la_CFLAGS   = $(ALSA_CFLAGS)
alsa_la_LIBADD   = $(ALSA_LIBS) -lm
alsa_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_COREAUDIO
output_LTLIBRARIES += coreaudio.la
coreaudio_la_SOURCES = coreaudio.c
coreaudio_la_LIBADD  = $(COREAUDIO_LIBS)
coreaudio_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_OSS
output_LTLIBRARIES += oss.la
oss_la_SOURCES  = oss.c mixer_oss.c mixer_oss.h
oss_la_CFLAGS   = $(OSS_CFLAGS)
oss_la_LIBADD   = $(OSS_LIBS)
oss_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_SNDIO
output_LTLIBRARIES += sndio.la
sndio_la_SOURCES = sndio.c
sndio_la_LIBADD  = $(SNDIO_LIBS)
sndio_la_LDFLAGS = $(common_ldflags)
endif
