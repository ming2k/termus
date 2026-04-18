#include "app/options_parse.h"
#include "app/options_registry.h"
#include "common/gbuf.h"
#include "common/msg.h"
#include "common/utils.h"

#include <strings.h>

const char *const options_bool_names[] = {"false", "true", NULL};

int options_parse_bool(const char *buf, int *val)
{
	return parse_enum(buf, 0, 1, options_bool_names, val);
}

int parse_enum(const char *buf, int minval, int maxval,
	       const char *const names[], int *val)
{
	long int tmp;
	int i;
	GBUF(names_buf);

	if (str_to_int(buf, &tmp) == 0) {
		if (tmp < minval || tmp > maxval)
			goto err;
		*val = tmp;
		return 1;
	}

	for (i = 0; names[i]; i++) {
		if (strcasecmp(buf, names[i]) == 0) {
			*val = i + minval;
			return 1;
		}
	}
err:
	for (i = 0; names[i]; i++) {
		if (i)
			gbuf_add_str(&names_buf, ", ");
		gbuf_add_str(&names_buf, names[i]);
	}
	error_msg("expected [%d..%d] or [%s]", minval, maxval,
		  names_buf.buffer);
	gbuf_free(&names_buf);
	return 0;
}
