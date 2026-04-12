#include "core/ip.h"
#include "core/id3.h"
#include "library/ape.h"
#include "common/xmalloc.h"
#include "common/read_wrapper.h"
#include "common/debug.h"
#include "core/comment.h"

#include <mpg123.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

struct mpg123_private {
	mpg123_handle *handle;
};

static ssize_t mpg123_read_func(void *datasource, void *buffer, size_t count)
{
	struct input_plugin_data *ip_data = datasource;

	return read_wrapper(ip_data, buffer, count);
}

static off_t mpg123_lseek_func(void *datasource, off_t offset, int whence)
{
	struct input_plugin_data *ip_data = datasource;

	return lseek(ip_data->fd, offset, whence);
}

static void mpg123_cleanup_func(void *datasource)
{
	struct input_plugin_data *ip_data = datasource;

	if (ip_data->fd != -1) {
		close(ip_data->fd);
		ip_data->fd = -1;
	}
}

static int mpg123_ip_open(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv;
	mpg123_handle *handle;
	int err;
	long rate;
	int channels, encoding;

	/* mpg123_init() is safe to call multiple times */
	if (mpg123_init() != MPG123_OK)
		return -IP_ERROR_INTERNAL;

	handle = mpg123_new(NULL, &err);
	if (!handle) {
		d_print("mpg123_new failed: %s\n", mpg123_plain_strerror(err));
		return -IP_ERROR_INTERNAL;
	}

	/* force 16-bit signed output */
	mpg123_param(handle, MPG123_FLAGS, MPG123_QUIET, 0);
	mpg123_format_none(handle);
	mpg123_format(handle, 44100, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	mpg123_format(handle, 48000, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	mpg123_format(handle, 32000, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	mpg123_format(handle, 22050, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	mpg123_format(handle, 16000, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	mpg123_format(handle, 11025, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	mpg123_format(handle, 8000, MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);

	if (mpg123_replace_reader_handle(handle, mpg123_read_func,
			mpg123_lseek_func, mpg123_cleanup_func) != MPG123_OK) {
		d_print("mpg123_replace_reader_handle failed\n");
		mpg123_delete(handle);
		return -IP_ERROR_INTERNAL;
	}

	if (mpg123_open_handle(handle, ip_data) != MPG123_OK) {
		d_print("mpg123_open_handle failed: %s\n", mpg123_strerror(handle));
		mpg123_delete(handle);
		return -IP_ERROR_FILE_FORMAT;
	}

	if (mpg123_getformat(handle, &rate, &channels, &encoding) != MPG123_OK) {
		d_print("mpg123_getformat failed: %s\n", mpg123_strerror(handle));
		mpg123_close(handle);
		mpg123_delete(handle);
		return -IP_ERROR_FILE_FORMAT;
	}

	/* lock the format so it doesn't change mid-stream */
	mpg123_format_none(handle);
	mpg123_format(handle, rate, channels, encoding);

	priv = xnew(struct mpg123_private, 1);
	priv->handle = handle;
	ip_data->private = priv;

	ip_data->sf = sf_rate(rate) | sf_channels(channels) |
		sf_bits(16) | sf_signed(1);
	channel_map_init_waveex(channels, 0, ip_data->channel_map);
	return 0;
}

static int mpg123_ip_close(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv = ip_data->private;

	mpg123_close(priv->handle);
	mpg123_delete(priv->handle);
	free(priv);
	ip_data->fd = -1;
	ip_data->private = NULL;
	return 0;
}

static int mpg123_ip_read(struct input_plugin_data *ip_data, char *buffer, int count)
{
	struct mpg123_private *priv = ip_data->private;
	size_t done = 0;
	int err;

	err = mpg123_read(priv->handle, (unsigned char *)buffer, count, &done);
	switch (err) {
	case MPG123_OK:
	case MPG123_DONE:
	case MPG123_NEW_FORMAT:
		return (int)done;
	default:
		d_print("mpg123_read error: %s\n", mpg123_strerror(priv->handle));
		return -1;
	}
}

static int mpg123_ip_seek(struct input_plugin_data *ip_data, double offset)
{
	struct mpg123_private *priv = ip_data->private;
	long rate;
	int channels, encoding;
	off_t sample_offset;

	if (mpg123_getformat(priv->handle, &rate, &channels, &encoding) != MPG123_OK)
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;

	sample_offset = (off_t)(offset * rate);
	if (mpg123_seek(priv->handle, sample_offset, SEEK_SET) < 0) {
		d_print("mpg123_seek failed: %s\n", mpg123_strerror(priv->handle));
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;
	}
	return 0;
}

static int mpg123_ip_read_comments(struct input_plugin_data *ip_data,
		struct keyval **comments)
{
	struct id3tag id3;
	int fd, rc, save, i;
	APETAG(ape);
	GROWING_KEYVALS(c);

	fd = open(ip_data->filename, O_RDONLY);
	if (fd == -1)
		return -1;

	d_print("filename: %s\n", ip_data->filename);

	id3_init(&id3);
	rc = id3_read_tags(&id3, fd, ID3_V1 | ID3_V2);
	save = errno;
	close(fd);
	errno = save;
	if (rc) {
		if (rc == -1) {
			d_print("error: %s\n", strerror(errno));
			return -1;
		}
		d_print("corrupted tag?\n");
		goto next;
	}

	for (i = 0; i < NUM_ID3_KEYS; i++) {
		char *val = id3_get_comment(&id3, i);
		if (val)
			comments_add(&c, id3_key_names[i], val);
	}

next:
	id3_free(&id3);

	rc = ape_read_tags(&ape, ip_data->fd, 0);
	if (rc < 0)
		goto out;

	for (i = 0; i < rc; i++) {
		char *k, *v;
		k = ape_get_comment(&ape, &v);
		if (!k)
			break;
		comments_add(&c, k, v);
		free(k);
	}

out:
	ape_free(&ape);

	keyvals_terminate(&c);
	*comments = c.keyvals;
	return 0;
}

static int mpg123_ip_duration(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv = ip_data->private;
	off_t length;
	long rate;
	int channels, encoding;

	if (mpg123_getformat(priv->handle, &rate, &channels, &encoding) != MPG123_OK)
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;

	length = mpg123_length(priv->handle);
	if (length == MPG123_ERR || rate == 0)
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;

	return (int)(length / rate);
}

static long mpg123_ip_bitrate(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv = ip_data->private;
	struct mpg123_frameinfo fi;

	if (mpg123_info(priv->handle, &fi) != MPG123_OK)
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;

	return fi.bitrate * 1000;
}

static long mpg123_ip_current_bitrate(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv = ip_data->private;
	struct mpg123_frameinfo fi;

	if (mpg123_info(priv->handle, &fi) != MPG123_OK)
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;

	return fi.bitrate * 1000;
}

static char *mpg123_ip_codec(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv = ip_data->private;
	struct mpg123_frameinfo fi;

	if (mpg123_info(priv->handle, &fi) != MPG123_OK)
		return NULL;

	switch (fi.layer) {
	case 3:
		return xstrdup("mp3");
	case 2:
		return xstrdup("mp2");
	case 1:
		return xstrdup("mp1");
	}
	return NULL;
}

static char *mpg123_ip_codec_profile(struct input_plugin_data *ip_data)
{
	struct mpg123_private *priv = ip_data->private;
	struct mpg123_frameinfo fi;

	if (mpg123_info(priv->handle, &fi) != MPG123_OK)
		return NULL;

	switch (fi.vbr) {
	case MPG123_VBR:
		return xstrdup("VBR");
	case MPG123_ABR:
		return xstrdup("ABR");
	default:
		return xstrdup("CBR");
	}
}

const struct input_plugin_ops ip_ops = {
	.open = mpg123_ip_open,
	.close = mpg123_ip_close,
	.read = mpg123_ip_read,
	.seek = mpg123_ip_seek,
	.read_comments = mpg123_ip_read_comments,
	.duration = mpg123_ip_duration,
	.bitrate = mpg123_ip_bitrate,
	.bitrate_current = mpg123_ip_current_bitrate,
	.codec = mpg123_ip_codec,
	.codec_profile = mpg123_ip_codec_profile
};

const int ip_priority = 50;
const char * const ip_extensions[] = { "mp3", "mp2", NULL };
const char * const ip_mime_types[] = {
	"audio/mpeg", "audio/x-mp3", "audio/x-mpeg", NULL
};
const struct input_plugin_opt ip_options[] = { { NULL } };
const unsigned ip_abi_version = IP_ABI_VERSION;
