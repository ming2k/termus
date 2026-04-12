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
# C23 preferred, C11 with atomics fallback
# -----------------------------------------------------------------------
AC_MSG_CHECKING([for a C standard with atomics (prefer C23)])
c_std_ok=no
for flag in "-std=gnu2x" "-std=c2x" "-std=gnu23" "-std=c23" "-std=gnu11" "-std=c11" ""; do
  save_CFLAGS="$CFLAGS"
  test -n "$flag" && CFLAGS="$CFLAGS $flag"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <stdatomic.h>
#ifdef __STDC_NO_ATOMICS__
# error no atomics
#endif
int main(void) { _Atomic int r = ATOMIC_VAR_INIT(0); return r; }
  ]])], [c_std_ok=yes], [CFLAGS="$save_CFLAGS"])
  test "$c_std_ok" = yes && break
done
AC_MSG_RESULT([$c_std_ok])
test "$c_std_ok" = no && AC_MSG_ERROR([C11 or newer with atomics required])

# -----------------------------------------------------------------------
# Project-wide developer flags
# -----------------------------------------------------------------------
DEV_WARN_CFLAGS=
for flag in \
    -pipe -Wall -Wshadow -Wcast-align -Wpointer-arith \
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
# ncurses (prefer wide-char)
# -----------------------------------------------------------------------
PKG_CHECK_MODULES([NCURSES],[ncursesw],[have_ncursesw=yes],[have_ncursesw=no])
if test "$have_ncursesw" = no; then
  PKG_CHECK_MODULES([NCURSES],[ncurses],,
    [PKG_CHECK_MODULES([NCURSES],[curses],,
      [AC_MSG_ERROR([ncurses not found])])])
  AC_MSG_WARN([ncurses without wide character support])
fi

# Probe for working include path
save_CFLAGS="$CFLAGS"; save_LIBS="$LIBS"
CFLAGS="$CFLAGS $NCURSES_CFLAGS"; LIBS="$LIBS $NCURSES_LIBS"
ncurses_ok=no
for nflag in "" "-I/usr/include/ncurses" "-I/usr/include/ncursesw"; do
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
# -----------------------------------------------------------------------

# -- CDDB --
AC_ARG_ENABLE([cddb],[AS_HELP_STRING([--disable-cddb],[disable libcddb])],
  [enable_cddb=$enableval],[enable_cddb=auto])
have_cddb=no
test "$enable_cddb" != no &&
  PKG_CHECK_MODULES([CDDB],[libcddb],
    [have_cddb=yes;AC_DEFINE([HAVE_CDDB],[1],[Have libcddb])],
    [test "$enable_cddb" = yes && AC_MSG_ERROR([libcddb not found])])
AM_CONDITIONAL([CONFIG_CDDB],[test "$have_cddb" = yes])

# -- CDIO --
AC_ARG_ENABLE([cdio],[AS_HELP_STRING([--disable-cdio],[disable libcdio CDDA])],
  [enable_cdio=$enableval],[enable_cdio=auto])
have_cdio=no
test "$enable_cdio" != no &&
  PKG_CHECK_MODULES([CDIO],[libcdio_cdda],
    [have_cdio=yes],
    [test "$enable_cdio" = yes && AC_MSG_ERROR([libcdio_cdda not found])])
AM_CONDITIONAL([CONFIG_CDIO],[test "$have_cdio" = yes])

# -- FLAC --
AC_ARG_ENABLE([flac],[AS_HELP_STRING([--disable-flac],[disable FLAC])],
  [enable_flac=$enableval],[enable_flac=auto])
have_flac=no
test "$enable_flac" != no &&
  PKG_CHECK_MODULES([FLAC],[flac],
    [have_flac=yes;FLAC_CFLAGS=$(echo "$FLAC_CFLAGS"|sed 's|/FLAC$||')],
    [test "$enable_flac" = yes && AC_MSG_ERROR([flac not found])])
AM_CONDITIONAL([CONFIG_FLAC],[test "$have_flac" = yes])

# -- MAD --
AC_ARG_ENABLE([mad],[AS_HELP_STRING([--disable-mad],[disable MAD mp3 decoder])],
  [enable_mad=$enableval],[enable_mad=auto])
have_mad=no
test "$enable_mad" != no &&
  PKG_CHECK_MODULES([MAD],[mad],
    [have_mad=yes],
    [test "$enable_mad" = yes && AC_MSG_ERROR([libmad not found])])
AM_CONDITIONAL([CONFIG_MAD],[test "$have_mad" = yes])

# -- MPG123 --
AC_ARG_ENABLE([mpg123],[AS_HELP_STRING([--disable-mpg123],[disable mpg123])],
  [enable_mpg123=$enableval],[enable_mpg123=auto])
have_mpg123=no
test "$enable_mpg123" != no &&
  PKG_CHECK_MODULES([MPG123],[libmpg123],
    [have_mpg123=yes],
    [test "$enable_mpg123" = yes && AC_MSG_ERROR([libmpg123 not found])])
AM_CONDITIONAL([CONFIG_MPG123],[test "$have_mpg123" = yes])

# -- MIKMOD (off by default) --
AC_ARG_ENABLE([mikmod],[AS_HELP_STRING([--enable-mikmod],[enable libmikmod (default: no)])],
  [enable_mikmod=$enableval],[enable_mikmod=no])
have_mikmod=no
if test "$enable_mikmod" = yes; then
  PKG_CHECK_MODULES([MIKMOD],[libmikmod],[have_mikmod=yes],[
    AC_CHECK_LIB([mikmod],[MikMod_RegisterAllDrivers],
      [have_mikmod=yes;MIKMOD_LIBS="-lmikmod $PTHREAD_LIBS"],,[
$PTHREAD_LIBS])])
fi
AC_SUBST([MIKMOD_CFLAGS])
AC_SUBST([MIKMOD_LIBS])
AM_CONDITIONAL([CONFIG_MIKMOD],[test "$have_mikmod" = yes])

# -- MODPLUG --
AC_ARG_ENABLE([modplug],[AS_HELP_STRING([--disable-modplug],[disable libmodplug])],
  [enable_modplug=$enableval],[enable_modplug=auto])
have_modplug=no; modplug_api8=0
test "$enable_modplug" != no &&
  PKG_CHECK_MODULES([MODPLUG],[libmodplug],
    [have_modplug=yes
     AC_CHECK_LIB([modplug],[ModPlug_GetModuleType],[modplug_api8=1],,[$MODPLUG_LIBS])],
    [test "$enable_modplug" = yes && AC_MSG_ERROR([libmodplug not found])])
AC_DEFINE_UNQUOTED([MODPLUG_API_8],[$modplug_api8],[ModPlug API 8])
AM_CONDITIONAL([CONFIG_MODPLUG],[test "$have_modplug" = yes])

# -- BASS (off by default) --
AC_ARG_ENABLE([bass],[AS_HELP_STRING([--enable-bass],[enable libbass (default: no)])],
  [enable_bass=$enableval],[enable_bass=no])
have_bass=no; BASS_LIBS=""
if test "$enable_bass" = yes; then
  AC_CHECK_HEADER([bass.h],
    [AC_CHECK_LIB([bass],[BASS_Init],[have_bass=yes;BASS_LIBS="-lbass"])])
fi
AC_SUBST([BASS_LIBS])
AM_CONDITIONAL([CONFIG_BASS],[test "$have_bass" = yes])

# -- MPC --
AC_ARG_ENABLE([mpc],[AS_HELP_STRING([--disable-mpc],[disable Musepack])],
  [enable_mpc=$enableval],[enable_mpc=auto])
have_mpc=no; mpc_sv8=0; MPC_LIBS=""
if test "$enable_mpc" != no; then
  AC_CHECK_HEADER([mpc/mpcdec.h],[mpc_sv8=1],[AC_CHECK_HEADER([mpcdec/mpcdec.h],[])])
  AC_CHECK_LIB([mpcdec],[mpc_demux_init],
    [have_mpc=yes;MPC_LIBS="-lmpcdec -lm"],
    [test "$enable_mpc" = yes && AC_MSG_ERROR([libmpcdec not found])])
fi
AC_DEFINE_UNQUOTED([MPC_SV8],[$mpc_sv8],[MPC SV8 API])
AC_SUBST([MPC_LIBS])
AM_CONDITIONAL([CONFIG_MPC],[test "$have_mpc" = yes])

# -- TREMOR (affects vorbis) --
AC_ARG_ENABLE([tremor],[AS_HELP_STRING([--enable-tremor],[use Tremor Ogg/Vorbis (default: no)])],
  [enable_tremor=$enableval],[enable_tremor=no])
test "$enable_tremor" = yes && AC_DEFINE([CONFIG_TREMOR],[1],[Use Tremor])
AM_CONDITIONAL([CONFIG_TREMOR],[test "$enable_tremor" = yes])

# -- VORBIS --
AC_ARG_ENABLE([vorbis],[AS_HELP_STRING([--disable-vorbis],[disable Ogg/Vorbis])],
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

# -- OPUS --
AC_ARG_ENABLE([opus],[AS_HELP_STRING([--disable-opus],[disable Opus])],
  [enable_opus=$enableval],[enable_opus=auto])
have_opus=no
test "$enable_opus" != no &&
  PKG_CHECK_MODULES([OPUS],[opusfile],[have_opus=yes],
    [test "$enable_opus" = yes && AC_MSG_ERROR([opusfile not found])])
AM_CONDITIONAL([CONFIG_OPUS],[test "$have_opus" = yes])

# -- WAVPACK --
AC_ARG_ENABLE([wavpack],[AS_HELP_STRING([--disable-wavpack],[disable WavPack])],
  [enable_wavpack=$enableval],[enable_wavpack=auto])
have_wavpack=no
test "$enable_wavpack" != no &&
  PKG_CHECK_MODULES([WAVPACK],[wavpack >= 4.40],[have_wavpack=yes],
    [test "$enable_wavpack" = yes && AC_MSG_ERROR([wavpack >= 4.40 not found])])
AM_CONDITIONAL([CONFIG_WAVPACK],[test "$have_wavpack" = yes])

# -- MP4 --
AC_ARG_ENABLE([mp4],[AS_HELP_STRING([--disable-mp4],[disable MPEG-4 AAC])],
  [enable_mp4=$enableval],[enable_mp4=auto])
have_mp4=no; use_mpeg4ip=0
if test "$enable_mp4" != no; then
  PKG_CHECK_MODULES([MP4],[mp4v2 faad2],
    [have_mp4=yes
     AC_CHECK_HEADER([mp4v2/mp4v2.h],,[use_mpeg4ip=1;
       AC_CHECK_HEADER([mp4.h],,[have_mp4=no])])],
    [test "$enable_mp4" = yes && AC_MSG_ERROR([mp4v2 + faad2 not found])])
fi
AC_DEFINE_UNQUOTED([USE_MPEG4IP],[$use_mpeg4ip],[Use mpeg4ip API])
AM_CONDITIONAL([CONFIG_MP4],[test "$have_mp4" = yes])

# -- AAC --
AC_ARG_ENABLE([aac],[AS_HELP_STRING([--disable-aac],[disable AAC])],
  [enable_aac=$enableval],[enable_aac=auto])
have_aac=no
if test "$enable_aac" != no; then
  PKG_CHECK_MODULES([AAC],[faad2],
    [AC_CHECK_HEADER([neaacdec.h],[have_aac=yes],,[CFLAGS="$AAC_CFLAGS"])],
    [test "$enable_aac" = yes && AC_MSG_ERROR([faad2 not found])])
fi
AM_CONDITIONAL([CONFIG_AAC],[test "$have_aac" = yes])

# -- FFMPEG --
AC_ARG_ENABLE([ffmpeg],[AS_HELP_STRING([--disable-ffmpeg],[disable FFMPEG])],
  [enable_ffmpeg=$enableval],[enable_ffmpeg=auto])
have_ffmpeg=no
if test "$enable_ffmpeg" != no; then
  PKG_CHECK_MODULES([FFMPEG],[libavformat libavcodec libswresample libavutil],
    [ok=yes
     save_CFLAGS="$CFLAGS"; CFLAGS="$CFLAGS $FFMPEG_CFLAGS"
     AC_CHECK_HEADER([libavformat/avformat.h],   ,[ok=no])
     AC_CHECK_HEADER([libavcodec/avcodec.h],     ,[ok=no])
     AC_CHECK_HEADER([libswresample/swresample.h],[ok=no])
     AC_CHECK_HEADER([libavutil/avutil.h],        ,[ok=no])
     CFLAGS="$save_CFLAGS"
     if test "$ok" = yes; then
       have_ffmpeg=yes
       AC_DEFINE([HAVE_FFMPEG_AVCODEC_H],[1],[Have ffmpeg headers])
     fi],
    [test "$enable_ffmpeg" = yes && AC_MSG_ERROR([ffmpeg not found])])
fi
AM_CONDITIONAL([CONFIG_FFMPEG],[test "$have_ffmpeg" = yes])

# -- VTX --
AC_ARG_ENABLE([vtx],[AS_HELP_STRING([--disable-vtx],[disable VTX/ayemu])],
  [enable_vtx=$enableval],[enable_vtx=auto])
have_vtx=no; VTX_LIBS=""
test "$enable_vtx" != no &&
  AC_CHECK_HEADER([ayemu.h],
    [AC_CHECK_LIB([ayemu],[ayemu_init],
      [have_vtx=yes;VTX_LIBS="-layemu"],
      [test "$enable_vtx" = yes && AC_MSG_ERROR([libayemu not found])])])
AC_SUBST([VTX_LIBS])
AM_CONDITIONAL([CONFIG_VTX],[test "$have_vtx" = yes])

# -- CUE / WAV (no external deps) --
AC_ARG_ENABLE([cue],[AS_HELP_STRING([--disable-cue],[disable CUE sheet support])],
  [enable_cue=$enableval],[enable_cue=yes])
AM_CONDITIONAL([CONFIG_CUE],[test "$enable_cue" = yes])

AC_ARG_ENABLE([wav],[AS_HELP_STRING([--disable-wav],[disable WAV support])],
  [enable_wav=$enableval],[enable_wav=yes])
AM_CONDITIONAL([CONFIG_WAV],[test "$enable_wav" = yes])

# -- USE_FALLBACK_IP --
AC_ARG_ENABLE([fallback-ip],
  [AS_HELP_STRING([--enable-fallback-ip],[use FFMPEG for unknown formats])],
  [test "$enableval" = yes &&
     AC_DEFINE([USE_FALLBACK_IP],[1],[FFMPEG fallback input plugin])])

# -----------------------------------------------------------------------
# Output plugins
# -----------------------------------------------------------------------

# -- PIPEWIRE --
AC_ARG_ENABLE([pipewire],[AS_HELP_STRING([--disable-pipewire],[disable PipeWire])],
  [enable_pipewire=$enableval],[enable_pipewire=auto])
have_pipewire=no
test "$enable_pipewire" != no &&
  PKG_CHECK_MODULES([PIPEWIRE],[libpipewire-0.3 >= 0.3.0],[have_pipewire=yes],
    [test "$enable_pipewire" = yes && AC_MSG_ERROR([libpipewire-0.3 not found])])
AM_CONDITIONAL([CONFIG_PIPEWIRE],[test "$have_pipewire" = yes])

# -- ALSA --
AC_ARG_ENABLE([alsa],[AS_HELP_STRING([--disable-alsa],[disable ALSA])],
  [enable_alsa=$enableval],[enable_alsa=auto])
have_alsa=no
test "$enable_alsa" != no &&
  PKG_CHECK_MODULES([ALSA],[alsa >= 1.0.11],[have_alsa=yes],
    [test "$enable_alsa" = yes && AC_MSG_ERROR([ALSA >= 1.0.11 not found])])
AM_CONDITIONAL([CONFIG_ALSA],[test "$have_alsa" = yes])

# -- JACK --
AC_ARG_ENABLE([jack],[AS_HELP_STRING([--disable-jack],[disable JACK])],
  [enable_jack=$enableval],[enable_jack=auto])
have_jack=no
test "$enable_jack" != no &&
  PKG_CHECK_MODULES([JACK],[jack],[have_jack=yes],
    [test "$enable_jack" = yes && AC_MSG_ERROR([JACK not found])])
AM_CONDITIONAL([CONFIG_JACK],[test "$have_jack" = yes])

# -- SAMPLERATE (JACK resampling) --
AC_ARG_ENABLE([samplerate],[AS_HELP_STRING([--disable-samplerate],[disable libsamplerate for JACK])],
  [enable_samplerate=$enableval],[enable_samplerate=auto])
have_samplerate=no
test "$enable_samplerate" != no &&
  PKG_CHECK_MODULES([SAMPLERATE],[samplerate],
    [have_samplerate=yes;AC_DEFINE([HAVE_SAMPLERATE],[1],[Have libsamplerate])],
    [test "$enable_samplerate" = yes && AC_MSG_ERROR([libsamplerate not found])])
AM_CONDITIONAL([CONFIG_SAMPLERATE],[test "$have_samplerate" = yes])

# -- AO --
AC_ARG_ENABLE([ao],[AS_HELP_STRING([--disable-ao],[disable libao])],
  [enable_ao=$enableval],[enable_ao=auto])
have_ao=no
test "$enable_ao" != no &&
  PKG_CHECK_MODULES([AO],[ao],[have_ao=yes],
    [test "$enable_ao" = yes && AC_MSG_ERROR([libao not found])])
AM_CONDITIONAL([CONFIG_AO],[test "$have_ao" = yes])

# -- COREAUDIO (macOS only) --
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

# -- ARTS --
AC_ARG_ENABLE([arts],[AS_HELP_STRING([--disable-arts],[disable ARTS])],
  [enable_arts=$enableval],[enable_arts=auto])
have_arts=no; ARTS_CFLAGS=""; ARTS_LIBS=""
if test "$enable_arts" != no; then
  AC_PATH_PROG([ARTSC_CONFIG],[artsc-config])
  if test -n "$ARTSC_CONFIG"; then
    ARTS_CFLAGS=$($ARTSC_CONFIG --cflags 2>/dev/null)
    ARTS_LIBS=$($ARTSC_CONFIG --libs 2>/dev/null)
    have_arts=yes
  fi
fi
AC_SUBST([ARTS_CFLAGS])
AC_SUBST([ARTS_LIBS])
AM_CONDITIONAL([CONFIG_ARTS],[test "$have_arts" = yes])

# -- OSS --
AC_ARG_ENABLE([oss],[AS_HELP_STRING([--disable-oss],[disable OSS])],
  [enable_oss=$enableval],[enable_oss=auto])
have_oss=no; OSS_LIBS=""
if test "$enable_oss" != no; then
  case "$host_os" in
    linux*|*freebsd*)
      AC_CHECK_HEADER([sys/soundcard.h],[have_oss=yes]);;
    *bsd*)
      AC_CHECK_LIB([ossaudio],[main],
        [have_oss=yes;OSS_LIBS="-lossaudio"]);;
  esac
fi
AC_SUBST([OSS_LIBS])
AM_CONDITIONAL([CONFIG_OSS],[test "$have_oss" = yes])

# -- SNDIO --
AC_ARG_ENABLE([sndio],[AS_HELP_STRING([--disable-sndio],[disable sndio])],
  [enable_sndio=$enableval],[enable_sndio=auto])
have_sndio=no; SNDIO_LIBS=""
test "$enable_sndio" != no &&
  AC_CHECK_LIB([sndio],[sio_open],
    [have_sndio=yes;SNDIO_LIBS="-lsndio"],
    [test "$enable_sndio" = yes && AC_MSG_ERROR([libsndio not found])])
AC_SUBST([SNDIO_LIBS])
AM_CONDITIONAL([CONFIG_SNDIO],[test "$have_sndio" = yes])

# -- SUN --
have_sun=no
AC_CHECK_HEADER([sys/audioio.h],[have_sun=yes])
AM_CONDITIONAL([CONFIG_SUN],[test "$have_sun" = yes])

# -- WAVEOUT (Cygwin) --
have_waveout=no; WAVEOUT_LIBS=""
case "$host_os" in
  *cygwin*)
    AC_CHECK_LIB([winmm],[waveOutOpen],
      [have_waveout=yes;WAVEOUT_LIBS="-lwinmm"]);;
esac
AC_SUBST([WAVEOUT_LIBS])
AM_CONDITIONAL([CONFIG_WAVEOUT],[test "$have_waveout" = yes])

# -- ROAR --
AC_ARG_ENABLE([roar],[AS_HELP_STRING([--disable-roar],[disable RoarAudio])],
  [enable_roar=$enableval],[enable_roar=auto])
have_roar=no
test "$enable_roar" != no &&
  PKG_CHECK_MODULES([ROAR],[libroar >= 0.4.5],[have_roar=yes],
    [test "$enable_roar" = yes && AC_MSG_ERROR([libroar >= 0.4.5 not found])])
AM_CONDITIONAL([CONFIG_ROAR],[test "$have_roar" = yes])

# -- AAUDIO (Android) --
have_aaudio=no; AAUDIO_CFLAGS=""; AAUDIO_LIBS=""
AC_CHECK_HEADER([aaudio/AAudio.h],
  [AC_CHECK_LIB([aaudio],[AAudio_createStreamBuilder],
    [have_aaudio=yes
     AAUDIO_CFLAGS="-D__ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__ -Werror=unguarded-availability"
     AAUDIO_LIBS="-Wl,--no-as-needed -laaudio"])])
AC_SUBST([AAUDIO_CFLAGS])
AC_SUBST([AAUDIO_LIBS])
AM_CONDITIONAL([CONFIG_AAUDIO],[test "$have_aaudio" = yes])

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
# DISCID (only useful when CDIO is on)
# -----------------------------------------------------------------------
AC_ARG_ENABLE([discid],[AS_HELP_STRING([--disable-discid],[disable libdiscid])],
  [enable_discid=$enableval],[enable_discid=auto])
have_discid=no; DISCID_CFLAGS=""; DISCID_LIBS=""
if test "$have_cdio" = yes && test "$enable_discid" != no; then
  PKG_CHECK_MODULES([DISCID],[libdiscid],
    [have_discid=yes;AC_DEFINE([HAVE_DISCID],[1],[Have libdiscid])],
    [test "$enable_discid" = yes && AC_MSG_ERROR([libdiscid not found])])
fi
AM_CONDITIONAL([CONFIG_DISCID],[test "$have_discid" = yes])

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
  Input plugins:
    CDIO:      $have_cdio   (CDDB: $have_cddb, DiscID: $have_discid)
    FLAC:      $have_flac
    MAD:       $have_mad
    MPG123:    $have_mpg123
    MikMod:    $have_mikmod
    ModPlug:   $have_modplug
    BASS:      $have_bass
    MPC:       $have_mpc
    Vorbis:    $have_vorbis  (Tremor: $enable_tremor)
    Opus:      $have_opus
    WavPack:   $have_wavpack
    MP4:       $have_mp4
    AAC:       $have_aac
    FFMPEG:    $have_ffmpeg
    CUE:       $enable_cue
    WAV:       $enable_wav
    VTX:       $have_vtx
  Output plugins:
    PipeWire:  $have_pipewire
    ALSA:      $have_alsa
    JACK:      $have_jack
    AO:        $have_ao
    CoreAudio: $have_coreaudio
    ARTS:      $have_arts
    OSS:       $have_oss
    Sndio:     $have_sndio
    Sun:       $have_sun
    WaveOut:   $have_waveout
    RoarAudio: $have_roar
    AAudio:    $have_aaudio
  Features:
    MPRIS:     $have_mpris
    Debug:     $debug_level
    Werror:    $enable_warnings_as_errors
    Sanitizers:${sanitizer_list:- none}
])
