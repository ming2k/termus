#include "common/msg.h"
#include <stdio.h>
#include <stdarg.h>

static void default_handler(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static int default_query_handler(const char *format, ...)
{
	return 0;
}

msg_handler_t info_handler = default_handler;
msg_handler_t error_handler = default_handler;
query_handler_t yes_no_query_handler = default_query_handler;

void info_msg(const char *format, ...)
{
	va_list ap;
	char buf[1024];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	info_handler("%s", buf);
}

void error_msg(const char *format, ...)
{
	va_list ap;
	char buf[1024];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	error_handler("%s", buf);
}

int yes_no_query(const char *format, ...)
{
	va_list ap;
	char buf[1024];

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	return yes_no_query_handler("%s", buf);
}
