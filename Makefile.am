ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src src/plugins/input src/plugins/output tests man data

EXTRA_DIST = \
	AUTHORS \
	CHANGELOG.md \
	COPYING \
	README.md \
	docs/README.md \
	docs/architecture.md \
	docs/contributing.md \
	docs/development.md \
	docs/project-structure.md \
	docs/runtime.md \
	contrib

DISTCLEANFILES = config.h

# ---------------------------------------------------------------------------
# Developer convenience: run termus directly from the build tree
#
#   make run
#
# Stages plugin .so files (built by libtool into .libs/) into a local
# directory tree that mirrors the installed layout, then launches termus
# pointing at it.
#
# `make run` launches against a persistent dev-home at $(builddir)/dev-home so
# configuration survives across invocations. Use `make run-clean` to start from
# a completely empty throwaway home (useful for testing first-run behaviour).
# Use `make run-user` to run against the caller's normal config directory.
# ---------------------------------------------------------------------------

stage_dir   = $(abs_builddir)/stage
input_libs  = $(abs_builddir)/src/plugins/input/.libs
output_libs = $(abs_builddir)/src/plugins/output/.libs
data_dir    = $(abs_srcdir)/data

dev_home    = $(abs_builddir)/dev-home

.PHONY: stage run run-clean run-user

stage: all
	$(MKDIR_P) $(stage_dir)/input $(stage_dir)/output
	@for f in $(input_libs)/*.so; do \
	    test -f "$$f" && $(LN_S) -f "$$f" $(stage_dir)/input/ 2>/dev/null || true; \
	done
	@for f in $(output_libs)/*.so; do \
	    test -f "$$f" && $(LN_S) -f "$$f" $(stage_dir)/output/ 2>/dev/null || true; \
	done

run: stage
	$(MKDIR_P) $(dev_home)
	TERMUS_HOME=$(dev_home) \
	TERMUS_SOCKET=$(dev_home)/socket \
	TERMUS_LIB_DIR=$(stage_dir) \
	TERMUS_DATA_DIR=$(data_dir) \
	$(abs_builddir)/src/termus

run-clean: stage
	@tmp_home=$$(mktemp -d "$(abs_builddir)/run-home.XXXXXX") && \
	trap 'rm -rf "$$tmp_home"' EXIT INT TERM && \
	TERMUS_HOME="$$tmp_home" \
	TERMUS_SOCKET="$$tmp_home/socket" \
	TERMUS_LIB_DIR=$(stage_dir) \
	TERMUS_DATA_DIR=$(data_dir) \
	$(abs_builddir)/src/termus

run-user: stage
	TERMUS_LIB_DIR=$(stage_dir) TERMUS_DATA_DIR=$(data_dir) $(abs_builddir)/src/termus
