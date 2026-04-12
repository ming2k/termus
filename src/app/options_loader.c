#include "app/options_loader.h"
#include "app/options_registry.h"
#include "app/options_defaults.h"
#include "common/misc.h"
#include "common/msg.h"
#include "common/prog.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void options_load_autosave_or_default_rc(void)
{
	char filename[512];

	snprintf(filename, sizeof(filename), "%s/autosave", termus_state_dir);
	if (source_file(filename) == -1) {
		char *default_rc = options_get_default_rc_path();

		if (errno != ENOENT)
			error_msg("loading %s: %s", filename, strerror(errno));

		if (source_file(default_rc) == -1)
			die_errno("loading %s", default_rc);

		free(default_rc);
	}
}

void options_load_optional_rc(void)
{
	char filename[512];

	snprintf(filename, sizeof(filename), "%s/rc", termus_config_dir);
	if (source_file(filename) == -1 && errno != ENOENT)
		error_msg("loading %s: %s", filename, strerror(errno));
}
