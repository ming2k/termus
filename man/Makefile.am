EXTRA_DIST = termus.txt termusc.txt

noinst_PROGRAMS = ttman
ttman_SOURCES   = ttman.c

# man pages are generated (not distributed as source)
nodist_man1_MANS = termus.1 termusc.1
CLEANFILES       = termus.1 termusc.1

all-local: termus.1 termusc.1

termus.1: $(srcdir)/termus.txt ttman
	./ttman $(srcdir)/termus.txt $@

termusc.1: $(srcdir)/termusc.txt ttman
	./ttman $(srcdir)/termusc.txt $@
