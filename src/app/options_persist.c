#include "app/options_registry.h"
#include "app/options_ui_state.h"
#include "common/misc.h"
#include "common/prog.h"
#include "library/filters.h"
#include "ui/keys.h"

#include <stdio.h>
#include <string.h>

static void options_persist_write_values(FILE *file)
{
	struct termus_opt *opt;
	int save_colorscheme = options_get_colorscheme()[0] != '\0';

	list_for_each_entry(opt, &option_head, node) {
		char buf[OPTION_MAX_SIZE];

		if (save_colorscheme && strncmp(opt->name, "color_", 6) == 0)
			continue;

		if (opt->get == NULL)
			continue;

		buf[0] = 0;
		opt->get(opt->data, buf, OPTION_MAX_SIZE);
		fprintf(file, "set %s=%s\n", opt->name, buf);
	}
}

static void options_persist_write_bindings(FILE *file)
{
	for (int i = 0; i < NR_CTXS; i++) {
		struct binding *binding = key_bindings[i];

		while (binding) {
			fprintf(file, "bind %s %s %s\n",
					key_context_names[i],
					binding->key->name,
					binding->cmd);
			binding = binding->next;
		}
	}
}

static void options_persist_write_filters(FILE *file)
{
	struct filter_entry *filter;

	list_for_each_entry(filter, &filters_head, node)
		fprintf(file, "fset %s=%s\n", filter->name, filter->filter);

	fprintf(file, "factivate");
	list_for_each_entry(filter, &filters_head, node) {
		switch (filter->act_stat) {
		case FS_YES:
			fprintf(file, " %s", filter->name);
			break;
		case FS_NO:
			fprintf(file, " !%s", filter->name);
			break;
		}
	}
	fprintf(file, "\n");
}

void options_exit(void)
{
	char filename_tmp[512];
	char filename[512];
	FILE *file;

	snprintf(filename_tmp, sizeof(filename_tmp), "%s/autosave.tmp", termus_state_dir);
	file = fopen(filename_tmp, "w");
	if (file == NULL) {
		warn_errno("creating %s", filename_tmp);
		return;
	}

	options_persist_write_values(file);
	options_persist_write_bindings(file);
	options_persist_write_filters(file);

	fclose(file);

	snprintf(filename, sizeof(filename), "%s/autosave", termus_state_dir);
	if (rename(filename_tmp, filename))
		warn_errno("renaming %s to %s", filename_tmp, filename);
}
