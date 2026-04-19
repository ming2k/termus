BUILDDIR ?= build

ifeq ($(wildcard $(BUILDDIR)/Makefile),)
$(error Build directory '$(BUILDDIR)' is not configured. Run: mkdir -p $(BUILDDIR) && cd $(BUILDDIR) && ../configure)
endif

.PHONY: $(or $(MAKECMDGOALS),all)
$(or $(MAKECMDGOALS),all):
	$(MAKE) -C $(BUILDDIR) $@
