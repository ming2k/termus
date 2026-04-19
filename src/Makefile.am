AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/src
AM_CFLAGS = $(DEV_WARN_CFLAGS) $(WERROR_CFLAGS) $(SANITIZER_CFLAGS)
AM_LDFLAGS = $(SANITIZER_LDFLAGS)

bin_PROGRAMS = termus termusc

# ------------------------------------------------------------
# Convenience library: files shared by both binaries
# ------------------------------------------------------------
noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	app/options_parse.c app/options_parse.h \
	app/options_store.c \
	common/file.c common/file.h \
	common/locale.c common/locale.h \
	common/misc.c common/misc.h \
	common/msg.c common/msg.h \
	common/path.c common/path.h \
	common/prog.c common/prog.h \
	common/xmalloc.c common/xmalloc.h \
	common/xstrjoin.c common/xstrjoin.h

# ------------------------------------------------------------
# termus
# ------------------------------------------------------------
termus_SOURCES = \
	app/cmdline.c app/cmdline.h \
	app/termus.c app/termus.h \
	app/options_core.c app/options_core.h app/options_core_state.c \
	app/options_core_state.h \
	app/options_defaults.c app/options_defaults.h \
	app/options_library.c app/options_library.h app/options_library_state.c \
	app/options_library_state.h \
	app/options_loader.c app/options_loader.h \
	app/options_misc.c app/options_misc.h \
	app/options_playback.c app/options_playback.h app/options_playback_state.c \
	app/options_playback_state.h \
	app/options_persist.c \
	app/options_plugins.c app/options_plugins.h \
	app/options_registry.h \
	app/options_ui_state.c \
	app/options_ui_state.h \
	app/resume.c app/resume.h \
	common/compiler.h \
	common/debug.c common/debug.h \
	common/gbuf.c common/gbuf.h \
	common/glob.c common/glob.h \
	common/iter.h \
	common/list.h \
	common/locking.c common/locking.h \
	common/mergesort.c common/mergesort.h \
	common/read_wrapper.c common/read_wrapper.h \
	common/rbtree.c common/rbtree.h \
	common/spawn.c common/spawn.h \
	common/u_collate.c common/u_collate.h \
	common/uchar.c common/uchar.h \
	common/unidecomp.h \
	common/utils.h \
	common/wcwidth_uchar.h \
	common/worker.c common/worker.h \
	core/buffer.c core/buffer.h \
	core/channelmap.c core/channelmap.h \
	core/comment.c core/comment.h \
	core/convert.c core/convert.h \
	core/cue_utils.c core/cue_utils.h \
	core/history.c core/history.h \
	core/http.c core/http.h \
	core/id3.c core/id3.h \
	core/input.c core/input.h \
	core/ip.h \
	core/keyval.c core/keyval.h \
	core/mixer.h \
	core/op.h \
	core/output.c core/output.h \
	core/pcm.c core/pcm.h \
	core/player.c core/player.h \
	core/sf.h \
	core/track.c core/track.h \
	core/track_info_cb.h \
	core/track_info.c core/track_info.h \
	ipc/server.c ipc/server.h \
	library/ape.c library/ape.h \
	library/cache.c library/cache.h \
	library/cue.c library/cue.h \
	library/editable.c library/editable.h \
	library/expr.c library/expr.h \
	library/filters.c library/filters.h \
	library/lib.c library/lib.h \
	library/load_dir.c library/load_dir.h \
	library/pl.c library/pl.h \
	library/pl_env.c library/pl_env.h \
	library/play_queue.c library/play_queue.h \
	library/search.c library/search.h \
	ui/command_mode.c ui/command_mode.h \
	ui/editable_view.c ui/editable_view.h \
	ui/format_print.c ui/format_print.h \
	ui/job.c ui/job.h \
	ui/keys.c ui/keys.h \
	ui/options_display.c ui/options_display.h \
	ui/options_hooks.c ui/options_hooks.h \
	ui/options.c \
	ui/options_ui.c ui/options_ui.h \
	ui/search_mode.c ui/search_mode.h \
	ui/tabexp.c ui/tabexp.h \
	ui/tabexp_file.c ui/tabexp_file.h \
	ui/curses_compat.h \
	ui/ui_curses.c ui/ui_curses.h \
	ui/ui_utils.c ui/ui_utils.h \
	ui/window.c ui/window.h

if CONFIG_MPRIS
termus_SOURCES += ipc/mpris.c ipc/mpris.h
endif

termus_CFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(NCURSES_CFLAGS) \
	$(ICONV_CFLAGS) \
	$(DL_CFLAGS) \
	$(LIBSYSTEMD_CFLAGS)

# Export all symbols so dlopen'd plugins can resolve termus internals
# (xmalloc, debug helpers, etc.) without duplicating them per-plugin.
termus_LDFLAGS = -Wl,--export-dynamic

termus_LDADD = \
	libcommon.la \
	$(PTHREAD_LIBS) \
	$(NCURSES_LIBS) \
	$(ICONV_LIBS) \
	$(DL_LIBS) \
	$(MATH_LIBS) \
	$(COMPAT_LIBS) \
	$(LIBSYSTEMD_LIBS)

# ------------------------------------------------------------
# termusc
# ------------------------------------------------------------
termusc_SOURCES = app/main.c

termusc_LDADD = libcommon.la $(COMPAT_LIBS)
