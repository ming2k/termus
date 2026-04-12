EXTRA_DIST = termus.txt termus-remote.txt

noinst_PROGRAMS = ttman
ttman_SOURCES   = ttman.c

# man pages are generated (not distributed as source)
nodist_man1_MANS = termus.1 termus-remote.1
CLEANFILES       = termus.1 termus-remote.1

all-local: termus.1 termus-remote.1

termus.1: $(srcdir)/termus.txt ttman
	./ttman $(srcdir)/termus.txt $@

termus-remote.1: $(srcdir)/termus-remote.txt ttman
	./ttman $(srcdir)/termus-remote.txt $@
