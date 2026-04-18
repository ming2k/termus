#include <errno.h>
#include <stdatomic.h>
#include <string.h>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>

#include "common/debug.h"
#include "common/utils.h"
#include "common/xmalloc.h"
#include "core/mixer.h"
#include "core/op.h"

static struct pw_thread_loop *pw_loop;
static struct pw_stream *pw_s;

static sample_format_t pw_sf;
static int pw_frame_size;

/* ring buffer for audio data */
static char *pw_buf;
static size_t pw_buf_size;
static size_t pw_buf_rpos;	   /* read position */
static size_t pw_buf_wpos;	   /* write position */
static atomic_size_t pw_buf_avail; /* bytes available for reading */

/* volume */
static float pw_vol_l = 1.0f;
static float pw_vol_r = 1.0f;
static int mixer_notify_in;
static int mixer_notify_out;

/* paused state */
static int pw_paused;

#define PW_BUF_SIZE (64 * 1024)

#define RET_PW_ERROR(msg)                                                      \
	do {                                                                   \
		d_print("PipeWire error: %s\n", msg);                          \
		return -OP_ERROR_INTERNAL;                                     \
	} while (0)

static enum spa_audio_format convert_sample_format(sample_format_t sf)
{
	const int _signed = sf_get_signed(sf);
	const int big_endian = sf_get_bigendian(sf);
	const int sample_size = sf_get_sample_size(sf) * 8;

	if (!_signed && sample_size == 8)
		return SPA_AUDIO_FORMAT_U8;

	if (_signed) {
		switch (sample_size) {
		case 16:
			return big_endian ? SPA_AUDIO_FORMAT_S16_BE
					  : SPA_AUDIO_FORMAT_S16_LE;
		case 24:
			return big_endian ? SPA_AUDIO_FORMAT_S24_BE
					  : SPA_AUDIO_FORMAT_S24_LE;
		case 32:
			return big_endian ? SPA_AUDIO_FORMAT_S32_BE
					  : SPA_AUDIO_FORMAT_S32_LE;
		}
	}

	return SPA_AUDIO_FORMAT_UNKNOWN;
}

static enum spa_audio_channel convert_channel_position(channel_position_t p)
{
	switch (p) {
	case CHANNEL_POSITION_MONO:
		return SPA_AUDIO_CHANNEL_MONO;
	case CHANNEL_POSITION_FRONT_LEFT:
		return SPA_AUDIO_CHANNEL_FL;
	case CHANNEL_POSITION_FRONT_RIGHT:
		return SPA_AUDIO_CHANNEL_FR;
	case CHANNEL_POSITION_FRONT_CENTER:
		return SPA_AUDIO_CHANNEL_FC;
	case CHANNEL_POSITION_REAR_CENTER:
		return SPA_AUDIO_CHANNEL_RC;
	case CHANNEL_POSITION_REAR_LEFT:
		return SPA_AUDIO_CHANNEL_RL;
	case CHANNEL_POSITION_REAR_RIGHT:
		return SPA_AUDIO_CHANNEL_RR;
	case CHANNEL_POSITION_LFE:
		return SPA_AUDIO_CHANNEL_LFE;
	case CHANNEL_POSITION_FRONT_LEFT_OF_CENTER:
		return SPA_AUDIO_CHANNEL_FLC;
	case CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER:
		return SPA_AUDIO_CHANNEL_FRC;
	case CHANNEL_POSITION_SIDE_LEFT:
		return SPA_AUDIO_CHANNEL_SL;
	case CHANNEL_POSITION_SIDE_RIGHT:
		return SPA_AUDIO_CHANNEL_SR;
	case CHANNEL_POSITION_TOP_CENTER:
		return SPA_AUDIO_CHANNEL_TC;
	case CHANNEL_POSITION_TOP_FRONT_LEFT:
		return SPA_AUDIO_CHANNEL_TFL;
	case CHANNEL_POSITION_TOP_FRONT_RIGHT:
		return SPA_AUDIO_CHANNEL_TFR;
	case CHANNEL_POSITION_TOP_FRONT_CENTER:
		return SPA_AUDIO_CHANNEL_TFC;
	case CHANNEL_POSITION_TOP_REAR_LEFT:
		return SPA_AUDIO_CHANNEL_TRL;
	case CHANNEL_POSITION_TOP_REAR_RIGHT:
		return SPA_AUDIO_CHANNEL_TRR;
	case CHANNEL_POSITION_TOP_REAR_CENTER:
		return SPA_AUDIO_CHANNEL_TRC;
	default:
		return SPA_AUDIO_CHANNEL_UNKNOWN;
	}
}

static size_t pw_buf_readable(void)
{
	return atomic_load_explicit(&pw_buf_avail, memory_order_acquire);
}

static size_t pw_buf_writable(void)
{
	return pw_buf_size -
	       atomic_load_explicit(&pw_buf_avail, memory_order_acquire);
}

static size_t pw_buf_read(char *dst, size_t count)
{
	size_t avail = pw_buf_readable();
	if (count > avail)
		count = avail;

	size_t to_end = pw_buf_size - pw_buf_rpos;
	if (count <= to_end) {
		memcpy(dst, pw_buf + pw_buf_rpos, count);
	} else {
		memcpy(dst, pw_buf + pw_buf_rpos, to_end);
		memcpy(dst + to_end, pw_buf, count - to_end);
	}
	pw_buf_rpos = (pw_buf_rpos + count) % pw_buf_size;
	atomic_fetch_sub_explicit(&pw_buf_avail, count, memory_order_release);
	return count;
}

static size_t pw_buf_write(const char *src, size_t count)
{
	size_t space = pw_buf_writable();
	if (count > space)
		count = space;

	size_t to_end = pw_buf_size - pw_buf_wpos;
	if (count <= to_end) {
		memcpy(pw_buf + pw_buf_wpos, src, count);
	} else {
		memcpy(pw_buf + pw_buf_wpos, src, to_end);
		memcpy(pw_buf, src + to_end, count - to_end);
	}
	pw_buf_wpos = (pw_buf_wpos + count) % pw_buf_size;
	atomic_fetch_add_explicit(&pw_buf_avail, count, memory_order_release);
	return count;
}

static void on_process(void *userdata)
{
	struct pw_buffer *b;
	struct spa_buffer *buf;
	void *dst;
	size_t n_frames, n_bytes, stride;

	b = pw_stream_dequeue_buffer(pw_s);
	if (!b) {
		d_print("PipeWire: out of buffers\n");
		return;
	}

	buf = b->buffer;
	dst = buf->datas[0].data;
	if (!dst)
		return;

	stride = pw_frame_size;
	n_frames = buf->datas[0].maxsize / stride;
	if (b->requested)
		n_frames = SPA_MIN((uint64_t)n_frames, b->requested);

	n_bytes = n_frames * stride;

	size_t readable = pw_buf_readable();
	if (readable < n_bytes)
		n_bytes = readable;

	/* align to frame boundary */
	n_bytes = (n_bytes / stride) * stride;
	n_frames = n_bytes / stride;

	if (n_bytes > 0) {
		pw_buf_read(dst, n_bytes);
	} else {
		/* underrun: output silence */
		memset(dst, 0, buf->datas[0].maxsize);
		n_bytes = buf->datas[0].maxsize;
		n_frames = n_bytes / stride;
	}

	buf->datas[0].chunk->offset = 0;
	buf->datas[0].chunk->stride = stride;
	buf->datas[0].chunk->size = n_bytes;

	pw_stream_queue_buffer(pw_s, b);
}

static void on_state_changed(void *userdata, enum pw_stream_state old,
			     enum pw_stream_state state, const char *error)
{
	d_print("PipeWire stream state: %s -> %s\n",
		pw_stream_state_as_string(old),
		pw_stream_state_as_string(state));

	if (state == PW_STREAM_STATE_STREAMING ||
	    state == PW_STREAM_STATE_PAUSED || state == PW_STREAM_STATE_ERROR ||
	    state == PW_STREAM_STATE_UNCONNECTED)
		pw_thread_loop_signal(pw_loop, false);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
    .state_changed = on_state_changed,
};

static int op_pw_init(void)
{
	pw_init(NULL, NULL);

	pw_loop = pw_thread_loop_new("termus-pipewire", NULL);
	if (!pw_loop)
		RET_PW_ERROR("failed to create thread loop");

	int rc = pw_thread_loop_start(pw_loop);
	if (rc < 0) {
		pw_thread_loop_destroy(pw_loop);
		pw_loop = NULL;
		RET_PW_ERROR("failed to start thread loop");
	}

	return OP_ERROR_SUCCESS;
}

static int op_pw_exit(void)
{
	if (pw_loop) {
		pw_thread_loop_stop(pw_loop);
		pw_thread_loop_destroy(pw_loop);
		pw_loop = NULL;
	}

	pw_deinit();

	return OP_ERROR_SUCCESS;
}

static int op_pw_open(sample_format_t sf, const channel_position_t *cmap)
{
	enum spa_audio_format fmt;
	uint8_t buffer[1024];
	struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const struct spa_pod *params[1];
	struct spa_audio_info_raw info = {0};
	int i, channels;

	fmt = convert_sample_format(sf);
	if (fmt == SPA_AUDIO_FORMAT_UNKNOWN)
		return -OP_ERROR_SAMPLE_FORMAT;

	channels = sf_get_channels(sf);
	pw_sf = sf;
	pw_frame_size = sf_get_frame_size(sf);

	info.format = fmt;
	info.rate = sf_get_rate(sf);
	info.channels = channels;

	if (cmap && channel_map_valid(cmap)) {
		for (i = 0; i < channels; i++)
			info.position[i] = convert_channel_position(cmap[i]);
	} else {
		if (channels == 1) {
			info.position[0] = SPA_AUDIO_CHANNEL_MONO;
		} else if (channels == 2) {
			info.position[0] = SPA_AUDIO_CHANNEL_FL;
			info.position[1] = SPA_AUDIO_CHANNEL_FR;
		}
	}

	/* allocate ring buffer */
	pw_buf_size = PW_BUF_SIZE;
	pw_buf = xmalloc(pw_buf_size);
	pw_buf_rpos = 0;
	pw_buf_wpos = 0;
	atomic_store_explicit(&pw_buf_avail, 0, memory_order_release);
	pw_paused = 0;

	pw_thread_loop_lock(pw_loop);

	struct pw_properties *props = pw_properties_new(
	    PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback",
	    PW_KEY_MEDIA_ROLE, "Music", PW_KEY_APP_NAME, "C* Music Player",
	    PW_KEY_APP_ID, "termus", PW_KEY_NODE_NAME, "termus", NULL);

	pw_s = pw_stream_new_simple(pw_thread_loop_get_loop(pw_loop),
				    "termus-playback", props, &stream_events,
				    NULL);

	if (!pw_s) {
		pw_thread_loop_unlock(pw_loop);
		free(pw_buf);
		pw_buf = NULL;
		RET_PW_ERROR("failed to create stream");
	}

	params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

	int rc = pw_stream_connect(pw_s, PW_DIRECTION_OUTPUT, PW_ID_ANY,
				   PW_STREAM_FLAG_AUTOCONNECT |
				       PW_STREAM_FLAG_MAP_BUFFERS |
				       PW_STREAM_FLAG_RT_PROCESS,
				   params, 1);

	if (rc < 0) {
		pw_stream_destroy(pw_s);
		pw_s = NULL;
		pw_thread_loop_unlock(pw_loop);
		free(pw_buf);
		pw_buf = NULL;
		RET_PW_ERROR("failed to connect stream");
	}

	/* wait for stream to be ready */
	for (;;) {
		enum pw_stream_state state = pw_stream_get_state(pw_s, NULL);
		if (state == PW_STREAM_STATE_STREAMING ||
		    state == PW_STREAM_STATE_PAUSED)
			break;
		if (state == PW_STREAM_STATE_ERROR ||
		    state == PW_STREAM_STATE_UNCONNECTED) {
			pw_stream_destroy(pw_s);
			pw_s = NULL;
			pw_thread_loop_unlock(pw_loop);
			free(pw_buf);
			pw_buf = NULL;
			RET_PW_ERROR("stream failed to connect");
		}
		pw_thread_loop_wait(pw_loop);
	}

	pw_thread_loop_unlock(pw_loop);

	return OP_ERROR_SUCCESS;
}

static int op_pw_close(void)
{
	pw_thread_loop_lock(pw_loop);

	if (pw_s) {
		pw_stream_destroy(pw_s);
		pw_s = NULL;
	}

	pw_thread_loop_unlock(pw_loop);

	free(pw_buf);
	pw_buf = NULL;
	pw_buf_rpos = 0;
	pw_buf_wpos = 0;
	atomic_store_explicit(&pw_buf_avail, 0, memory_order_release);

	return OP_ERROR_SUCCESS;
}

static int op_pw_drop(void)
{
	pw_thread_loop_lock(pw_loop);

	pw_buf_rpos = 0;
	pw_buf_wpos = 0;
	atomic_store_explicit(&pw_buf_avail, 0, memory_order_release);

	if (pw_s)
		pw_stream_flush(pw_s, false);

	pw_thread_loop_unlock(pw_loop);

	return OP_ERROR_SUCCESS;
}

static int op_pw_write(const char *buf, int count)
{
	pw_thread_loop_lock(pw_loop);

	size_t written = pw_buf_write(buf, count);

	pw_thread_loop_unlock(pw_loop);

	return written;
}

static int op_pw_buffer_space(void)
{
	pw_thread_loop_lock(pw_loop);

	size_t space = pw_buf_writable();

	pw_thread_loop_unlock(pw_loop);

	return space;
}

static int op_pw_pause(void)
{
	pw_thread_loop_lock(pw_loop);

	if (pw_s)
		pw_stream_set_active(pw_s, false);
	pw_paused = 1;

	pw_thread_loop_unlock(pw_loop);

	return OP_ERROR_SUCCESS;
}

static int op_pw_unpause(void)
{
	pw_thread_loop_lock(pw_loop);

	if (pw_s)
		pw_stream_set_active(pw_s, true);
	pw_paused = 0;

	pw_thread_loop_unlock(pw_loop);

	return OP_ERROR_SUCCESS;
}

/* mixer */

static int op_pw_mixer_init(void)
{
	init_pipes(&mixer_notify_out, &mixer_notify_in);
	return OP_ERROR_SUCCESS;
}

static int op_pw_mixer_exit(void)
{
	close(mixer_notify_out);
	close(mixer_notify_in);
	return OP_ERROR_SUCCESS;
}

static int op_pw_mixer_open(int *volume_max)
{
	*volume_max = 100;
	return OP_ERROR_SUCCESS;
}

static int op_pw_mixer_close(void) { return OP_ERROR_SUCCESS; }

static int op_pw_mixer_get_fds(int what, int *fds)
{
	switch (what) {
	case MIXER_FDS_VOLUME:
		fds[0] = mixer_notify_out;
		return 1;
	default:
		return 0;
	}
}

static int op_pw_mixer_set_volume(int l, int r)
{
	if (!pw_s)
		return -OP_ERROR_NOT_OPEN;

	pw_vol_l = (float)l / 100.0f;
	pw_vol_r = (float)r / 100.0f;

	float values[2] = {pw_vol_l, pw_vol_r};
	int channels = sf_get_channels(pw_sf);

	pw_thread_loop_lock(pw_loop);

	if (channels <= 1)
		pw_stream_set_control(pw_s, SPA_PROP_channelVolumes, 1, values,
				      0);
	else
		pw_stream_set_control(pw_s, SPA_PROP_channelVolumes, 2, values,
				      0);

	pw_thread_loop_unlock(pw_loop);

	return OP_ERROR_SUCCESS;
}

static int op_pw_mixer_get_volume(int *l, int *r)
{
	clear_pipe(mixer_notify_out, -1);

	*l = (int)(pw_vol_l * 100.0f + 0.5f);
	*r = (int)(pw_vol_r * 100.0f + 0.5f);

	return OP_ERROR_SUCCESS;
}

const struct output_plugin_ops op_pcm_ops = {
    .init = op_pw_init,
    .exit = op_pw_exit,
    .open = op_pw_open,
    .close = op_pw_close,
    .drop = op_pw_drop,
    .write = op_pw_write,
    .buffer_space = op_pw_buffer_space,
    .pause = op_pw_pause,
    .unpause = op_pw_unpause,
};

const struct mixer_plugin_ops op_mixer_ops = {
    .init = op_pw_mixer_init,
    .exit = op_pw_mixer_exit,
    .open = op_pw_mixer_open,
    .close = op_pw_mixer_close,
    .get_fds.abi_2 = op_pw_mixer_get_fds,
    .set_volume = op_pw_mixer_set_volume,
    .get_volume = op_pw_mixer_get_volume,
};

const struct output_plugin_opt op_pcm_options[] = {
    {NULL},
};

const struct mixer_plugin_opt op_mixer_options[] = {
    {NULL},
};

const int op_priority = -3;
const unsigned op_abi_version = OP_ABI_VERSION;
