#include "app/options_plugins.h"
#include "core/input.h"
#include "core/output.h"

const char *options_get_current_output_plugin(void) { return op_get_current(); }

void options_add_plugin_options(void)
{
	ip_add_options();
	op_add_options();
}
