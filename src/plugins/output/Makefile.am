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

if CONFIG_JACK
output_LTLIBRARIES += jack.la
jack_la_SOURCES  = jack.c
jack_la_CFLAGS   = $(JACK_CFLAGS) $(SAMPLERATE_CFLAGS)
jack_la_LIBADD   = $(JACK_LIBS) $(SAMPLERATE_LIBS)
jack_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_AO
output_LTLIBRARIES += ao.la
ao_la_SOURCES  = ao.c
ao_la_CFLAGS   = $(AO_CFLAGS)
ao_la_LIBADD   = $(AO_LIBS)
ao_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_COREAUDIO
output_LTLIBRARIES += coreaudio.la
coreaudio_la_SOURCES = coreaudio.c
coreaudio_la_LIBADD  = $(COREAUDIO_LIBS)
coreaudio_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_ARTS
output_LTLIBRARIES += arts.la
arts_la_SOURCES  = arts.c
arts_la_CFLAGS   = $(ARTS_CFLAGS)
arts_la_LIBADD   = $(ARTS_LIBS)
arts_la_LDFLAGS  = $(common_ldflags)
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

if CONFIG_SUN
output_LTLIBRARIES += sun.la
sun_la_SOURCES  = sun.c mixer_sun.c mixer_sun.h
sun_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_WAVEOUT
output_LTLIBRARIES += waveout.la
waveout_la_SOURCES = waveout.c
waveout_la_LIBADD  = $(WAVEOUT_LIBS)
waveout_la_LDFLAGS = $(common_ldflags)
endif

if CONFIG_ROAR
output_LTLIBRARIES += roar.la
roar_la_SOURCES  = roar.c
roar_la_CFLAGS   = $(ROAR_CFLAGS)
roar_la_LIBADD   = $(ROAR_LIBS)
roar_la_LDFLAGS  = $(common_ldflags)
endif

if CONFIG_AAUDIO
output_LTLIBRARIES += aaudio.la
aaudio_la_SOURCES = aaudio.c
aaudio_la_CFLAGS  = $(AAUDIO_CFLAGS)
aaudio_la_LIBADD  = $(AAUDIO_LIBS)
aaudio_la_LDFLAGS = $(common_ldflags)
endif
