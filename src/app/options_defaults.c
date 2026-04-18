#include "app/options_defaults.h"
#include "common/misc.h"
#include "common/xstrjoin.h"

char *options_get_default_rc_path(void)
{
	return xstrjoin(termus_data_dir, "/rc");
}
