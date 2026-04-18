AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)/src
AM_CFLAGS = $(DEV_WARN_CFLAGS) $(WERROR_CFLAGS) $(SANITIZER_CFLAGS)
AM_LDFLAGS = $(SANITIZER_LDFLAGS)

TESTS_ENVIRONMENT = LC_ALL=C; export LC_ALL;

check_PROGRAMS = test-locale test-options test-path test-player-logic
TESTS = $(check_PROGRAMS)

test_player_logic_SOURCES = test_player_logic.c
test_player_logic_LDADD = $(top_builddir)/src/libcommon.la

test_locale_SOURCES = test_locale.c
test_locale_LDADD = $(top_builddir)/src/libcommon.la

test_options_SOURCES = test_options.c
test_options_LDADD = $(top_builddir)/src/libcommon.la

test_path_SOURCES = test_path.c
test_path_LDADD = $(top_builddir)/src/libcommon.la
