#include "app/options_core.h"
#include "app/options_core_state.h"
#include "app/options_defaults.h"
#include "app/options_library.h"
#include "app/options_loader.h"
#include "app/options_misc.h"
#include "app/options_playback.h"
#include "app/options_plugins.h"
#include "app/options_registry.h"
#include "ui/options_display.h"
#include "ui/options_ui.h"
#include "ui/ui.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void options_add(void)
{
	options_add_plugin_options();
	options_add_core_options();
	options_add_playback_options();
	options_add_library_options();
	options_add_misc_options();
	options_add_display_options();
}

void options_load(void)
{
	options_display_load_defaults();
	options_library_load_defaults();
	options_load_autosave_or_default_rc();
	options_load_optional_rc();
	options_ui_apply_ascii_clipped_text_fallback(
	    using_utf8, options_display_default_clipped_text());
}
