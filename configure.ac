AC_PREREQ([2.69])
AC_INIT([termus], [4.0.0], [https://github.com/termus/termus/issues])
AC_CONFIG_SRCDIR([src/app/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AM_SILENT_RULES([yes])

# -----------------------------------------------------------------------
# Programs
# -----------------------------------------------------------------------
AC_PROG_CC
AM_PROG_AR
LT_INIT([dlopen disable-static])
AC_PROG_INSTALL

AC_SYS_LARGEFILE

# -----------------------------------------------------------------------
# Version and port baked into binary
# -----------------------------------------------------------------------
AC_DEFINE_UNQUOTED([VERSION], ["$PACKAGE_VERSION"], [Package version])
AC_DEFINE([DEFAULT_PORT], [3000], [Default termus server port])

# -----------------------------------------------------------------------
# C23 required
# -----------------------------------------------------------------------
AC_MSG_CHECKING([for a C23 compiler mode])
c_std_ok=no
c_std_flag=
for flag in "-std=gnu2x" "-std=c2x" "-std=gnu23" "-std=c23"; do
  save_CFLAGS="$CFLAGS"
  test -n "$flag" && CFLAGS="$CFLAGS $flag"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <stdatomic.h>
#ifdef __STDC_NO_ATOMICS__
# error no atomics
#endif
int main(void) { _Atomic int r = 0; return r; }
  ]])], [c_std_ok=yes; c_std_flag=$flag], [CFLAGS="$save_CFLAGS"])
  test "$c_std_ok" = yes && break
done
test "$c_std_ok" = yes && AC_MSG_RESULT([$c_std_flag])
test "$c_std_ok" = no && AC_MSG_ERROR([C23 compiler mode with atomics required])

# -----------------------------------------------------------------------
# Project-wide developer flags
# -----------------------------------------------------------------------
DEV_WARN_CFLAGS=
for flag in \
    -pipe -Wall -Wshadow -Wcast-align -Wpointer-arith \
    -fanalyzer \
    -Wwrite-strings -Wundef -Wmissing-prototypes -Wredundant-decls \
    -Wextra -Wno-sign-compare -Wformat-security \
    -Wold-style-definition -Wno-pointer-sign \
    -Werror-implicit-function-declaration -Wno-unused-parameter \
    -Wno-missing-field-initializers
do
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $flag"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[int main(void){return 0;}]])],
    [DEV_WARN_CFLAGS="$DEV_WARN_CFLAGS $flag"], [])
  CFLAGS="$save_CFLAGS"
done
AC_SUBST([DEV_WARN_CFLAGS])

AC_ARG_ENABLE([warnings-as-errors],
  [AS_HELP_STRING([--enable-warnings-as-errors],[treat compiler warnings as errors (default: no)])],
  [enable_warnings_as_errors=$enableval],[enable_warnings_as_errors=no])
case "$enable_warnings_as_errors" in
  yes|no) ;;
  *) AC_MSG_ERROR([--enable-warnings-as-errors must be yes or no]);;
esac
WERROR_CFLAGS=
if test "$enable_warnings_as_errors" = yes; then
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[int main(void){return 0;}]])],
    [WERROR_CFLAGS=" -Werror"],
    [AC_MSG_ERROR([compiler does not support -Werror])])
  CFLAGS="$save_CFLAGS"
fi
AC_SUBST([WERROR_CFLAGS])

AC_ARG_ENABLE([sanitizers],
  [AS_HELP_STRING([--enable-sanitizers@<:@=LIST@:>@],[enable runtime sanitizers (default list: address,undefined)])],
  [enable_sanitizers=$enableval],[enable_sanitizers=no])
case "$enable_sanitizers" in
  yes) sanitizer_list="address,undefined" ;;
  no) sanitizer_list="" ;;
  *) sanitizer_list="$enable_sanitizers" ;;
esac
SANITIZER_CFLAGS=
SANITIZER_LDFLAGS=
if test -n "$sanitizer_list"; then
  need_frame_pointer=yes
  save_IFS="$IFS"
  IFS=,
  for sanitizer in $sanitizer_list; do
    case "$sanitizer" in
      address|undefined) sanitizer_flag="-fsanitize=$sanitizer" ;;
      *) AC_MSG_ERROR([unsupported sanitizer '$sanitizer']) ;;
    esac
    save_CFLAGS="$CFLAGS"
    save_LDFLAGS="$LDFLAGS"
    CFLAGS="$CFLAGS $sanitizer_flag"
    LDFLAGS="$LDFLAGS $sanitizer_flag"
    AC_LINK_IFELSE([AC_LANG_SOURCE([[int main(void){return 0;}]])],
      [
        SANITIZER_CFLAGS="$SANITIZER_CFLAGS $sanitizer_flag"
        SANITIZER_LDFLAGS="$SANITIZER_LDFLAGS $sanitizer_flag"
      ],
      [AC_MSG_ERROR([compiler/linker do not support $sanitizer_flag])])
    CFLAGS="$save_CFLAGS"
    LDFLAGS="$save_LDFLAGS"
  done
  IFS="$save_IFS"
  if test "$need_frame_pointer" = yes; then
    SANITIZER_CFLAGS="$SANITIZER_CFLAGS -fno-omit-frame-pointer"
  fi
fi
AC_SUBST([SANITIZER_CFLAGS])
AC_SUBST([SANITIZER_LDFLAGS])

# -----------------------------------------------------------------------
# Endianness
# -----------------------------------------------------------------------
AC_C_BIGENDIAN([AC_DEFINE([WORDS_BIGENDIAN], [1], [Big-endian])])

# -----------------------------------------------------------------------
# Platform / compat libs
# -----------------------------------------------------------------------
AC_CANONICAL_HOST

COMPAT_LIBS=
case "$host_os" in
  solaris*|sunos*)
    AC_CHECK_LIB([socket],  [connect],     [COMPAT_LIBS="$COMPAT_LIBS -lsocket"])
    AC_CHECK_LIB([nsl],     [gethostbyname],[COMPAT_LIBS="$COMPAT_LIBS -lnsl"])
    AC_CHECK_LIB([rt],      [nanosleep],
      [COMPAT_LIBS="$COMPAT_LIBS -lrt"],
      [AC_CHECK_LIB([posix4],[nanosleep],
        [COMPAT_LIBS="$COMPAT_LIBS -lposix4"])])
    ;;
  *cygwin*)
    AC_DEFINE([CONFIG_CYGWIN], [1], [Cygwin build])
    ;;
esac
AC_SUBST([COMPAT_LIBS])

# -----------------------------------------------------------------------
# pthread
# -----------------------------------------------------------------------
PTHREAD_CFLAGS=""
PTHREAD_LIBS=""
AC_CHECK_LIB([pthread], [pthread_create],
  [PTHREAD_LIBS="-lpthread"],
  [AC_MSG_ERROR([pthreads required])])
AC_SUBST([PTHREAD_CFLAGS])
AC_SUBST([PTHREAD_LIBS])

save_CFLAGS="$CFLAGS"; save_LIBS="$LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
LIBS="$LIBS $PTHREAD_LIBS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <pthread.h>
int main(void) {
  pthread_attr_t a; struct sched_param p;
  pthread_attr_init(&a);
  pthread_attr_setschedpolicy(&a, SCHED_RR);
  p.sched_priority = sched_get_priority_max(SCHED_RR);
  pthread_attr_setschedparam(&a, &p);
  return 0;
}]])], [AC_DEFINE([REALTIME_SCHEDULING],[1],[Realtime scheduling])])
CFLAGS="$save_CFLAGS"; LIBS="$save_LIBS"

# -----------------------------------------------------------------------
# ncursesw
# -----------------------------------------------------------------------
PKG_CHECK_MODULES([NCURSES],[ncursesw],,
  [AC_MSG_ERROR([ncursesw not found])])

# Probe for working include path
save_CFLAGS="$CFLAGS"; save_LIBS="$LIBS"
CFLAGS="$CFLAGS $NCURSES_CFLAGS"; LIBS="$LIBS $NCURSES_LIBS"
ncurses_ok=no
for nflag in "" "-I/usr/include/ncursesw"; do
  CFLAGS="$save_CFLAGS $NCURSES_CFLAGS $nflag"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(__sun__)||defined(__CYGWIN__)
#include <termios.h>
#include <ncurses.h>
#else
#include <curses.h>
#endif
int main(void){initscr();endwin();return 0;}]])],
    [ncurses_ok=yes; NCURSES_CFLAGS="$NCURSES_CFLAGS $nflag"; break])
done
CFLAGS="$save_CFLAGS"; LIBS="$save_LIBS"
test "$ncurses_ok" = no && AC_MSG_ERROR([ncurses not usable])

# Optional ncurses features
save_LIBS="$LIBS"; LIBS="$LIBS $NCURSES_LIBS"
AC_CHECK_FUNC([resizeterm],
  [AC_DEFINE([HAVE_RESIZETERM],[1],[have resizeterm()])])
AC_CHECK_FUNC([use_default_colors],
  [AC_DEFINE([HAVE_USE_DEFAULT_COLORS],[1],[have use_default_colors()])])
LIBS="$save_LIBS"

save_CFLAGS="$CFLAGS"; CFLAGS="$CFLAGS $NCURSES_CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <curses.h>
int main(void){unsigned long x=A_ITALIC;return !!x;}]])],
  [AC_DEFINE([HAVE_ITALIC],[1],[A_ITALIC available])])
CFLAGS="$save_CFLAGS"

# -----------------------------------------------------------------------
# dl, math
# -----------------------------------------------------------------------
AC_CHECK_LIB([dl],[dlopen],[DL_LIBS="-ldl"],[DL_LIBS=""])
AC_SUBST([DL_LIBS])
AC_CHECK_LIB([m],[sin],[MATH_LIBS="-lm"],[MATH_LIBS=""])
AC_SUBST([MATH_LIBS])

# -----------------------------------------------------------------------
# iconv
# -----------------------------------------------------------------------
ICONV_CFLAGS=""
ICONV_LIBS=""
AC_CHECK_FUNC([iconv_open],[have_iconv=yes],[
  AC_CHECK_LIB([iconv],[iconv_open],
    [have_iconv=yes; ICONV_LIBS="-liconv"],[have_iconv=no])])
if test "$have_iconv" = yes; then
  AC_DEFINE([HAVE_ICONV],[1],[Have iconv])
fi
AC_SUBST([ICONV_CFLAGS])
AC_SUBST([ICONV_LIBS])

# -----------------------------------------------------------------------
# wcwidth, byteswap.h, strdup/strndup
# -----------------------------------------------------------------------
AC_MSG_CHECKING([for uchar-compatible wcwidth])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#define _XOPEN_SOURCE 700
#include <wchar.h>
extern char check[1/!(4-sizeof(wchar_t))];
int main(void){(void)wcwidth('a');return 0;}]])],
  [wcwidth_ok=yes], [wcwidth_ok=no])
AC_MSG_RESULT([$wcwidth_ok])
AS_IF([test "$wcwidth_ok" = yes],
  [AC_DEFINE([HAVE_WCWIDTH],[1],[uchar-compatible wcwidth])])

AC_CHECK_HEADER([byteswap.h],
  [AC_DEFINE([HAVE_BYTESWAP_H],[1],[Have byteswap.h])])
AC_CHECK_FUNCS([strdup strndup])

# -----------------------------------------------------------------------
# Debug level
# -----------------------------------------------------------------------
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug=N],[debug level 0-2 (default 1)])],
  [debug_level=$enableval],[debug_level=1])
case "$debug_level" in
  0|1|2);;
  *) AC_MSG_ERROR([--enable-debug must be 0, 1 or 2]);;
esac
AC_DEFINE_UNQUOTED([DEBUG],[$debug_level],[Debug level])

# -----------------------------------------------------------------------
# Paths baked into binary (expand before config.h generation)
# -----------------------------------------------------------------------
test "x$prefix"      = xNONE && prefix="$ac_default_prefix"
test "x$exec_prefix" = xNONE && exec_prefix="$prefix"
eval "datarootdir=\"$datarootdir\""
eval "datadir_val=\"$datadir\""
eval "libdir_val=\"$libdir\""
AC_DEFINE_UNQUOTED([DATADIR],["$datadir_val"],[Data dir])
AC_DEFINE_UNQUOTED([LIBDIR], ["$libdir_val"], [Lib dir])

# -----------------------------------------------------------------------
# Input plugins
#
# Decoder backends — see docs/codecs.md for the full rationale.
#
# Each supported format requires at least one backend.  Format-specific
# libraries run at ip_priority=50 (preferred); the FFmpeg backend runs at
# ip_priority=10 (fallback).  AAC/M4A have no maintained dedicated library,
# so FFmpeg is their primary and only backend.
#
# Format coverage is validated after all checks (configure errors if a
# primary format ends up with no backend at all).
# -----------------------------------------------------------------------

# -- FFmpeg decoder backend (libavformat + libavcodec + libswresample) --
# Required for AAC/M4A; optional fallback for FLAC/Opus/Vorbis/MP3.
# Minimum version: FFmpeg 5.0 / libavformat 59 / libswresample 4.
AC_ARG_ENABLE([avcodec],
  [AS_HELP_STRING([--disable-avcodec],[disable FFmpeg decoder backend])],
  [enable_avcodec=$enableval],[enable_avcodec=auto])
have_avcodec=no
if test "$enable_avcodec" != no; then
  PKG_CHECK_MODULES([AVCODEC],
    [libavformat >= 59 libavcodec >= 59 libswresample >= 4 libavutil],
    [have_avcodec=yes],
    [test "$enable_avcodec" = yes &&
       AC_MSG_ERROR([FFmpeg >= 5.0 not found; install libavformat-dev libavcodec-dev libswresample-dev])])
fi
AM_CONDITIONAL([CONFIG_AVCODEC],[test "$have_avcodec" = yes])

# -- FLAC — primary: libFLAC; fallback: FFmpeg --
AC_ARG_ENABLE([flac],[AS_HELP_STRING([--disable-flac],[disable libFLAC (FFmpeg fallback will be used if available)])],
  [enable_flac=$enableval],[enable_flac=yes])
have_flac=no
test "$enable_flac" != no &&
  PKG_CHECK_MODULES([FLAC],[flac],
    [have_flac=yes;FLAC_CFLAGS=$(echo "$FLAC_CFLAGS"|sed 's|/FLAC$||')],
    [test "$enable_flac" = yes && AC_MSG_ERROR([flac not found; install libflac-dev or --disable-flac])])
AM_CONDITIONAL([CONFIG_FLAC],[test "$have_flac" = yes])

# -- OPUS — primary: libopusfile; fallback: FFmpeg --
AC_ARG_ENABLE([opus],[AS_HELP_STRING([--disable-opus],[disable libopusfile (FFmpeg fallback will be used if available)])],
  [enable_opus=$enableval],[enable_opus=yes])
have_opus=no
test "$enable_opus" != no &&
  PKG_CHECK_MODULES([OPUS],[opusfile],
    [have_opus=yes],
    [test "$enable_opus" = yes && AC_MSG_ERROR([opusfile not found; install libopusfile-dev or --disable-opus])])
AM_CONDITIONAL([CONFIG_OPUS],[test "$have_opus" = yes])

# -- TREMOR (fixed-point Vorbis for embedded/ARM) --
AC_ARG_ENABLE([tremor],[AS_HELP_STRING([--enable-tremor],[use Tremor fixed-point Ogg/Vorbis (default: no)])],
  [enable_tremor=$enableval],[enable_tremor=no])
test "$enable_tremor" = yes && AC_DEFINE([CONFIG_TREMOR],[1],[Use Tremor])
AM_CONDITIONAL([CONFIG_TREMOR],[test "$enable_tremor" = yes])

# -- OGG VORBIS — primary: libvorbisfile; fallback: FFmpeg --
AC_ARG_ENABLE([vorbis],[AS_HELP_STRING([--disable-vorbis],[disable libvorbisfile (FFmpeg fallback will be used)])],
  [enable_vorbis=$enableval],[enable_vorbis=auto])
have_vorbis=no
if test "$enable_vorbis" != no; then
  if test "$enable_tremor" = yes; then
    PKG_CHECK_MODULES([VORBIS],[vorbisidec],[have_vorbis=yes],
      [test "$enable_vorbis" = yes && AC_MSG_ERROR([vorbisidec not found])])
  else
    PKG_CHECK_MODULES([VORBIS],[vorbisfile],[have_vorbis=yes],
      [test "$enable_vorbis" = yes && AC_MSG_ERROR([vorbisfile not found])])
  fi
fi
AM_CONDITIONAL([CONFIG_VORBIS],[test "$have_vorbis" = yes])

# -- MP3 (legacy) — primary: libmpg123; fallback: FFmpeg --
AC_ARG_ENABLE([mpg123],[AS_HELP_STRING([--disable-mpg123],[disable libmpg123 MP3 backend])],
  [enable_mpg123=$enableval],[enable_mpg123=auto])
have_mpg123=no
test "$enable_mpg123" != no &&
  PKG_CHECK_MODULES([MPG123],[libmpg123],
    [have_mpg123=yes],
    [test "$enable_mpg123" = yes && AC_MSG_ERROR([libmpg123 not found])])
AM_CONDITIONAL([CONFIG_MPG123],[test "$have_mpg123" = yes])

# -- CUE sheet playlist --
AC_ARG_ENABLE([cue],[AS_HELP_STRING([--disable-cue],[disable CUE sheet support])],
  [enable_cue=$enableval],[enable_cue=yes])
AM_CONDITIONAL([CONFIG_CUE],[test "$enable_cue" = yes])

# ── Format coverage validation ───────────────────────────────────────────
# Each primary format must have at least one backend available.

# FLAC: libFLAC or FFmpeg
if test "$have_flac" = no && test "$have_avcodec" = no; then
  AC_MSG_ERROR([No FLAC backend: install libflac-dev or libavformat-dev (FFmpeg)])
fi

# Opus: libopusfile or FFmpeg
if test "$have_opus" = no && test "$have_avcodec" = no; then
  AC_MSG_ERROR([No Opus backend: install libopusfile-dev or libavformat-dev (FFmpeg)])
fi

# AAC / M4A: FFmpeg only (no maintained dedicated library)
if test "$have_avcodec" = no; then
  AC_MSG_WARN([AAC/M4A not supported: install libavformat-dev (FFmpeg >= 5.0)])
fi

# -----------------------------------------------------------------------
# Output plugins
#
# Supported outputs by platform:
#   Linux:   PipeWire (primary), ALSA (fallback / raw)
#   macOS:   CoreAudio
#   FreeBSD: OSS (native)
#   OpenBSD: sndio (native)
# -----------------------------------------------------------------------

# -- PIPEWIRE (Linux primary) --
AC_ARG_ENABLE([pipewire],[AS_HELP_STRING([--disable-pipewire],[disable PipeWire])],
  [enable_pipewire=$enableval],[enable_pipewire=auto])
have_pipewire=no
test "$enable_pipewire" != no &&
  PKG_CHECK_MODULES([PIPEWIRE],[libpipewire-0.3 >= 0.3.0],[have_pipewire=yes],
    [test "$enable_pipewire" = yes && AC_MSG_ERROR([libpipewire-0.3 not found])])
AM_CONDITIONAL([CONFIG_PIPEWIRE],[test "$have_pipewire" = yes])

# -- ALSA (Linux fallback) --
AC_ARG_ENABLE([alsa],[AS_HELP_STRING([--disable-alsa],[disable ALSA])],
  [enable_alsa=$enableval],[enable_alsa=auto])
have_alsa=no
test "$enable_alsa" != no &&
  PKG_CHECK_MODULES([ALSA],[alsa >= 1.0.11],[have_alsa=yes],
    [test "$enable_alsa" = yes && AC_MSG_ERROR([ALSA >= 1.0.11 not found])])
AM_CONDITIONAL([CONFIG_ALSA],[test "$have_alsa" = yes])

# -- COREAUDIO (macOS native) --
have_coreaudio=no; COREAUDIO_LIBS=""
case "$host_os" in
  darwin*)
    AC_MSG_CHECKING([for CoreAudio framework])
    save_LIBS="$LIBS"
    LIBS="$LIBS -framework CoreAudio -framework AudioUnit"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[]],[[]])],
      [have_coreaudio=yes;COREAUDIO_LIBS="-framework CoreAudio -framework AudioUnit"],
      [])
    LIBS="$save_LIBS"
    AC_MSG_RESULT([$have_coreaudio])
    ;;
esac
AC_SUBST([COREAUDIO_LIBS])
AM_CONDITIONAL([CONFIG_COREAUDIO],[test "$have_coreaudio" = yes])

# -- OSS (FreeBSD native) --
AC_ARG_ENABLE([oss],[AS_HELP_STRING([--disable-oss],[disable OSS])],
  [enable_oss=$enableval],[enable_oss=auto])
have_oss=no; OSS_LIBS=""
if test "$enable_oss" != no; then
  case "$host_os" in
    *freebsd*)
      AC_CHECK_HEADER([sys/soundcard.h],[have_oss=yes]);;
    *bsd*)
      AC_CHECK_LIB([ossaudio],[main],
        [have_oss=yes;OSS_LIBS="-lossaudio"]);;
  esac
fi
AC_SUBST([OSS_LIBS])
AM_CONDITIONAL([CONFIG_OSS],[test "$have_oss" = yes])

# -- SNDIO (OpenBSD native) --
AC_ARG_ENABLE([sndio],[AS_HELP_STRING([--disable-sndio],[disable sndio])],
  [enable_sndio=$enableval],[enable_sndio=auto])
have_sndio=no; SNDIO_LIBS=""
test "$enable_sndio" != no &&
  AC_CHECK_LIB([sndio],[sio_open],
    [have_sndio=yes;SNDIO_LIBS="-lsndio"],
    [test "$enable_sndio" = yes && AC_MSG_ERROR([libsndio not found])])
AC_SUBST([SNDIO_LIBS])
AM_CONDITIONAL([CONFIG_SNDIO],[test "$have_sndio" = yes])

# -----------------------------------------------------------------------
# MPRIS (libsystemd / libelogind / basu)
# -----------------------------------------------------------------------
AC_ARG_ENABLE([mpris],[AS_HELP_STRING([--disable-mpris],[disable MPRIS])],
  [enable_mpris=$enableval],[enable_mpris=auto])
have_mpris=no
if test "$enable_mpris" != no; then
  PKG_CHECK_MODULES([LIBSYSTEMD],[libsystemd],[have_mpris=yes],[
    PKG_CHECK_MODULES([LIBSYSTEMD],[libelogind >= 239.3],[have_mpris=yes],[
      PKG_CHECK_MODULES([LIBSYSTEMD],[basu],
        [have_mpris=yes;CFLAGS="$CFLAGS -DCONFIG_MPRIS_BASU"],[
        test "$enable_mpris" = yes && AC_MSG_ERROR([libsystemd/libelogind/basu not found])
      ])])])
fi
test "$have_mpris" = yes && AC_DEFINE([CONFIG_MPRIS],[1],[MPRIS support])
AM_CONDITIONAL([CONFIG_MPRIS],[test "$have_mpris" = yes])


# -----------------------------------------------------------------------
# Output
# -----------------------------------------------------------------------
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/plugins/input/Makefile
  src/plugins/output/Makefile
  tests/Makefile
  man/Makefile
  data/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([
=== termus build configuration ===

  Input backends  (P=primary ip_priority=50  F=fallback ip_priority=10)
  ┌──────────┬──────────────────────┬───────────────────────────┐
  │ Format   │ Primary backend      │ FFmpeg fallback           │
  ├──────────┼──────────────────────┼───────────────────────────┤
  │ FLAC     │ libFLAC: $have_flac          │ avcodec: $have_avcodec                   │
  │ Opus     │ libopusfile: $have_opus      │ avcodec: $have_avcodec                   │
  │ Vorbis   │ libvorbisfile: $have_vorbis  │ avcodec: $have_avcodec  (Tremor: $enable_tremor)  │
  │ AAC/M4A  │ (none — FFmpeg only) │ avcodec: $have_avcodec  [primary]        │
  │ MP3      │ libmpg123: $have_mpg123      │ avcodec: $have_avcodec  [legacy]         │
  └──────────┴──────────────────────┴───────────────────────────┘
    CUE sheets: $enable_cue

  Output plugins:
    PipeWire   $have_pipewire   (Linux primary)
    ALSA       $have_alsa       (Linux fallback / raw)
    CoreAudio  $have_coreaudio  (macOS)
    OSS        $have_oss        (FreeBSD)
    sndio      $have_sndio      (OpenBSD)

  Features:
    MPRIS      $have_mpris
    Debug      $debug_level
    Werror     $enable_warnings_as_errors
    Sanitizers ${sanitizer_list:- none}
])
