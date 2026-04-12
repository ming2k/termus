#include "common/locale.h"

#include <string.h>

const char *charset = "ISO-8859-1";
int using_utf8 = 0;

void locale_init_charset(const char *name)
{
	if (name == NULL || name[0] == '\0')
		name = "ISO-8859-1";

	charset = name;
	using_utf8 = strcmp(charset, "UTF-8") == 0;
}
