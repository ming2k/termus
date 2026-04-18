#ifndef TERMUS_GBUF_H
#define TERMUS_GBUF_H

#include "common/compiler.h"
#include "common/uchar.h"

#include <stdarg.h>
#include <stddef.h> /* size_t */

struct gbuf {
	char *buffer;
	size_t alloc;
	size_t len;
};

extern char gbuf_empty_buffer[];

#define GBUF(name) struct gbuf name = {gbuf_empty_buffer, 0, 0}

static inline void gbuf_clear(struct gbuf *buf)
{
	buf->len = 0;
	buf->buffer[0] = 0;
}

static inline size_t gbuf_avail(struct gbuf *buf)
{
	if (buf->alloc)
		return buf->alloc - buf->len - 1;
	return 0;
}

void gbuf_grow(struct gbuf *buf, size_t more);
void gbuf_used(struct gbuf *buf, size_t used);
void gbuf_free(struct gbuf *buf);
void gbuf_add_ch(struct gbuf *buf, char ch);
void gbuf_add_uchar(struct gbuf *buf, uchar u);
void gbuf_add_bytes(struct gbuf *buf, const void *data, size_t len);
void gbuf_add_str(struct gbuf *buf, const char *str);
void gbuf_addf(struct gbuf *buf, const char *fmt, ...) TERMUS_FORMAT(2, 3);
void gbuf_vaddf(struct gbuf *buf, const char *fmt, va_list ap);
void gbuf_set(struct gbuf *buf, int c, size_t count);
char *gbuf_steal(struct gbuf *buf);

#endif
