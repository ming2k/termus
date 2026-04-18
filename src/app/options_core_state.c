#include "app/options_core_state.h"

#include "common/xmalloc.h"

#include <stddef.h>

char *output_plugin = NULL;
static char *server_password;
int scroll_offset = 2;
int rewind_offset = 5;

char *id3_default_charset = NULL;
char *icecast_default_charset = NULL;
char **pl_env_vars;

const char *options_get_server_password(void) { return server_password; }

void options_set_server_password(const char *password)
{
	free(server_password);
	server_password = password ? xstrdup(password) : NULL;
}
