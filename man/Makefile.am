EXTRA_DIST = termus.txt termus-remote.txt termus-tutorial.txt

noinst_PROGRAMS = ttman
ttman_SOURCES   = ttman.c

# man pages are generated (not distributed as source)
man1_MANS = termus.1 termus-remote.1
man7_MANS = termus-tutorial.7

nodist_man1_MANS = $(man1_MANS)
nodist_man7_MANS = $(man7_MANS)
CLEANFILES       = $(man1_MANS) $(man7_MANS)

all-local: $(man1_MANS) $(man7_MANS)

termus.1: $(srcdir)/termus.txt ttman
	./ttman $(srcdir)/termus.txt $@

termus-remote.1: $(srcdir)/termus-remote.txt ttman
	./ttman $(srcdir)/termus-remote.txt $@

termus-tutorial.7: $(srcdir)/termus-tutorial.txt ttman
	./ttman $(srcdir)/termus-tutorial.txt $@
