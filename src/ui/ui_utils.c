#include "ui/ui_utils.h"
#include "common/gbuf.h"
#include "common/uchar.h"
#include "common/utils.h"
#include "ui/options_ui.h"

#include <string.h>

static int gbuf_mark_clipped_text(struct gbuf *buf)
{
	int buf_width = u_str_width(buf->buffer);
	int clipped_mark_len =
	    min_u(u_str_width(clipped_text_internal), buf_width);
	int skip = buf_width - clipped_mark_len;
	buf->len = u_skip_chars(buf->buffer, &skip, false);
	gbuf_grow(buf, strlen(clipped_text_internal));
	gbuf_used(buf, u_copy_chars(buf->buffer + buf->len,
				    clipped_text_internal, &clipped_mark_len));
	return skip;
}

void gbuf_add_ustr(struct gbuf *buf, const char *src, int *width)
{
	int src_bytes = u_str_print_size(src) - 1;
	gbuf_grow(buf, src_bytes);
	size_t copy_bytes = u_copy_chars(buf->buffer + buf->len, src, width);
	gbuf_used(buf, copy_bytes);
	if (copy_bytes != src_bytes) {
		gbuf_set(buf, ' ', *width);
		*width = gbuf_mark_clipped_text(buf);
	}
}
