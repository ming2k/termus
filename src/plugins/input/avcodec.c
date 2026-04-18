/*
 * FFmpeg-based input plugin.
 *
 * Roles:
 *   Primary backend  (ip_priority=50) for AAC/M4A/MP4 — no maintained
 *                    dedicated library exists for these containers.
 *   Fallback backend (ip_priority=10) for FLAC, Opus, Ogg Vorbis, MP3 —
 *                    used when the format-specific library is not installed.
 *
 * See docs/codecs.md for the full backend priority rationale.
 *
 * Requires: libavformat >= 59, libavcodec >= 59, libswresample >= 4
 * (FFmpeg 5.0+; Arch/Debian/Ubuntu ship FFmpeg 6+ or 7+)
 */

#include "common/debug.h"
#include "common/xmalloc.h"
#include "core/channelmap.h"
#include "core/comment.h"
#include "core/ip.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* ── internal state ────────────────────────────────────────────────────── */

struct avcodec_private {
	AVFormatContext *fmt_ctx;
	AVCodecContext *codec_ctx;
	SwrContext *swr_ctx;
	AVPacket *packet;
	AVFrame *frame;

	int audio_stream;
	int eof;      /* no more packets from demuxer     */
	int flushing; /* draining decoder after eof       */

	/* decoded PCM not yet consumed by caller */
	uint8_t *pcm_buf;
	int pcm_cap; /* allocated bytes                  */
	int pcm_len; /* bytes currently buffered         */
	int pcm_off; /* read offset into pcm_buf         */

	enum AVSampleFormat out_av_fmt;
	int out_channels;
	int out_rate;
	int out_sample_size; /* bytes per sample per channel */
};

/* ── metadata key mapping ──────────────────────────────────────────────── */

/*
 * Map FFmpeg AVDictionary key names to termus keyval names.
 * Vorbis Comment keys (FLAC/Opus/Ogg) already match termus names when
 * lowercased.  QuickTime/ID3v2 keys need explicit remapping.
 */
static const char *map_metadata_key(const char *av_key)
{
	static const struct {
		const char *from, *to;
	} tbl[] = {{"album_artist", "albumartist"},
		   {"track", "tracknumber"},
		   {"disc", "discnumber"},
		   {"sort_name", "titlesort"},
		   {"sort_artist", "artistsort"},
		   {"sort_album_artist", "albumartistsort"},
		   {"sort_album", "albumsort"},
		   {"REPLAYGAIN_TRACK_GAIN", "replaygain_track_gain"},
		   {"REPLAYGAIN_TRACK_PEAK", "replaygain_track_peak"},
		   {"REPLAYGAIN_ALBUM_GAIN", "replaygain_album_gain"},
		   {"REPLAYGAIN_ALBUM_PEAK", "replaygain_album_peak"},
		   {"MUSICBRAINZ_TRACKID", "musicbrainz_trackid"},
		   {NULL, NULL}};
	for (int i = 0; tbl[i].from; i++)
		if (strcasecmp(av_key, tbl[i].from) == 0)
			return tbl[i].to;
	return av_key; /* Vorbis Comment keys pass through as-is */
}

/* ── PCM buffer helpers ────────────────────────────────────────────────── */

static void pcm_reset(struct avcodec_private *priv)
{
	priv->pcm_len = 0;
	priv->pcm_off = 0;
}

/* Ensure pcm_buf has room for at least `need` more bytes. */
static void pcm_ensure(struct avcodec_private *priv, int need)
{
	if (priv->pcm_cap - priv->pcm_off - priv->pcm_len >= need)
		return;
	/* compact: move unread data to front */
	if (priv->pcm_len > 0 && priv->pcm_off > 0)
		memmove(priv->pcm_buf, priv->pcm_buf + priv->pcm_off,
			priv->pcm_len);
	priv->pcm_off = 0;
	if (priv->pcm_cap - priv->pcm_len < need) {
		priv->pcm_cap = priv->pcm_len + need + 65536;
		priv->pcm_buf = xrealloc(priv->pcm_buf, priv->pcm_cap);
	}
}

/* Drain up to `count` bytes from pcm_buf into `buf`. Returns bytes drained. */
static int pcm_drain(struct avcodec_private *priv, char *buf, int count)
{
	int n = priv->pcm_len < count ? priv->pcm_len : count;
	if (n <= 0)
		return 0;
	memcpy(buf, priv->pcm_buf + priv->pcm_off, n);
	priv->pcm_off += n;
	priv->pcm_len -= n;
	return n;
}

/* ── open ──────────────────────────────────────────────────────────────── */

static int avcodec_open(struct input_plugin_data *ip_data)
{
	struct avcodec_private *priv;
	AVFormatContext *fmt_ctx = NULL;
	const AVCodec *codec;
	AVCodecContext *codec_ctx = NULL;
	SwrContext *swr_ctx = NULL;
	int bits, rc;

	rc = avformat_open_input(&fmt_ctx, ip_data->filename, NULL, NULL);
	if (rc < 0) {
		d_print("avformat_open_input(%s): %s\n", ip_data->filename,
			av_err2str(rc));
		return -IP_ERROR_ERRNO;
	}

	rc = avformat_find_stream_info(fmt_ctx, NULL);
	if (rc < 0) {
		d_print("avformat_find_stream_info: %s\n", av_err2str(rc));
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_FILE_FORMAT;
	}

	rc =
	    av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
	if (rc < 0) {
		d_print("no audio stream found\n");
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_UNSUPPORTED_FILE_TYPE;
	}
	int audio_stream = rc;

	codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx) {
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_INTERNAL;
	}
	rc = avcodec_parameters_to_context(
	    codec_ctx, fmt_ctx->streams[audio_stream]->codecpar);
	if (rc < 0) {
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_INTERNAL;
	}
	rc = avcodec_open2(codec_ctx, codec, NULL);
	if (rc < 0) {
		d_print("avcodec_open2: %s\n", av_err2str(rc));
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_UNSUPPORTED_FILE_TYPE;
	}

	/* choose output PCM format:
	 * 32-bit for high-res sources, 16-bit otherwise */
	int src_depth = codec_ctx->bits_per_raw_sample;
	if (src_depth <= 0)
		src_depth = av_get_bytes_per_sample(codec_ctx->sample_fmt) * 8;
	if (src_depth > 16) {
		bits = 32;
	} else {
		bits = 16;
	}
	enum AVSampleFormat out_av_fmt =
	    (bits == 32) ? AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;

	int out_channels = codec_ctx->ch_layout.nb_channels;
	if (out_channels <= 0)
		out_channels = 2;
	if (out_channels > CHANNELS_MAX)
		out_channels = CHANNELS_MAX;
	int out_rate = codec_ctx->sample_rate;

	/* set up resampler: handles planar → interleaved, sample format, and
	 * optionally channel layout conversion */
	AVChannelLayout out_layout;
	av_channel_layout_default(&out_layout, out_channels);

	rc = swr_alloc_set_opts2(&swr_ctx, &out_layout, out_av_fmt, out_rate,
				 &codec_ctx->ch_layout, codec_ctx->sample_fmt,
				 codec_ctx->sample_rate, 0, NULL);
	av_channel_layout_uninit(&out_layout);
	if (rc < 0 || !swr_ctx) {
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_INTERNAL;
	}
	rc = swr_init(swr_ctx);
	if (rc < 0) {
		swr_free(&swr_ctx);
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_INTERNAL;
	}

	priv = xnew0(struct avcodec_private, 1);
	priv->fmt_ctx = fmt_ctx;
	priv->codec_ctx = codec_ctx;
	priv->swr_ctx = swr_ctx;
	priv->packet = av_packet_alloc();
	priv->frame = av_frame_alloc();
	priv->audio_stream = audio_stream;
	priv->out_av_fmt = out_av_fmt;
	priv->out_channels = out_channels;
	priv->out_rate = out_rate;
	priv->out_sample_size = av_get_bytes_per_sample(out_av_fmt);

	if (!priv->packet || !priv->frame) {
		av_packet_free(&priv->packet);
		av_frame_free(&priv->frame);
		free(priv);
		swr_free(&swr_ctx);
		avcodec_free_context(&codec_ctx);
		avformat_close_input(&fmt_ctx);
		return -IP_ERROR_INTERNAL;
	}

	/* ip layer has the fd open; avformat opened its own — close theirs */
	if (ip_data->fd != -1) {
		close(ip_data->fd);
		ip_data->fd = -1;
	}

	ip_data->private = priv;
	ip_data->sf = sf_rate(out_rate) | sf_channels(out_channels) |
		      sf_bits(bits) | sf_signed(1) | sf_host_endian();

	if (out_channels == 1) {
		ip_data->channel_map[0] = CHANNEL_POSITION_MONO;
	} else {
		channel_map_init_stereo(ip_data->channel_map);
	}

	return 0;
}

/* ── close ─────────────────────────────────────────────────────────────── */

static int avcodec_close(struct input_plugin_data *ip_data)
{
	struct avcodec_private *priv = ip_data->private;

	av_packet_free(&priv->packet);
	av_frame_free(&priv->frame);
	swr_free(&priv->swr_ctx);
	avcodec_free_context(&priv->codec_ctx);
	avformat_close_input(&priv->fmt_ctx);
	free(priv->pcm_buf);
	free(priv);
	ip_data->private = NULL;
	return 0;
}

/* ── read ──────────────────────────────────────────────────────────────── */

/* Decode one frame, convert to interleaved PCM, append to priv->pcm_buf.
 * Returns 0 on success, 1 on EOF, negative on error. */
static int decode_frame(struct avcodec_private *priv)
{
	for (;;) {
		int rc = avcodec_receive_frame(priv->codec_ctx, priv->frame);
		if (rc == 0) {
			/* got a frame — convert it */
			int max_out =
			    swr_get_out_samples(priv->swr_ctx,
						priv->frame->nb_samples) +
			    64;
			int out_bytes = max_out * priv->out_channels *
					priv->out_sample_size;
			pcm_ensure(priv, out_bytes);

			uint8_t *out =
			    priv->pcm_buf + priv->pcm_off + priv->pcm_len;
			int converted =
			    swr_convert(priv->swr_ctx, &out, max_out,
					(const uint8_t **)priv->frame->data,
					priv->frame->nb_samples);
			av_frame_unref(priv->frame);
			if (converted < 0)
				return -IP_ERROR_FILE_FORMAT;
			priv->pcm_len += converted * priv->out_channels *
					 priv->out_sample_size;
			return 0;
		}

		if (rc == AVERROR(EAGAIN)) {
			/* decoder needs more input */
			if (priv->eof) {
				if (!priv->flushing) {
					avcodec_send_packet(priv->codec_ctx,
							    NULL);
					priv->flushing = 1;
				} else {
					return 1; /* truly drained */
				}
				continue;
			}

			/* read the next audio packet */
			do {
				av_packet_unref(priv->packet);
				int prc =
				    av_read_frame(priv->fmt_ctx, priv->packet);
				if (prc == AVERROR_EOF) {
					priv->eof = 1;
					goto send_packet;
				}
				if (prc < 0)
					return -IP_ERROR_FILE_FORMAT;
			} while (priv->packet->stream_index !=
				 priv->audio_stream);

		send_packet:
			avcodec_send_packet(priv->codec_ctx,
					    priv->eof ? NULL : priv->packet);
			av_packet_unref(priv->packet);
			continue;
		}

		if (rc == AVERROR_EOF)
			return 1;

		return -IP_ERROR_FILE_FORMAT;
	}
}

static int avcodec_read(struct input_plugin_data *ip_data, char *buffer,
			int count)
{
	struct avcodec_private *priv = ip_data->private;

	/* drain any leftover from a previous decode */
	int drained = pcm_drain(priv, buffer, count);
	if (drained > 0)
		return drained;

	/* need more decoded audio */
	int rc = decode_frame(priv);
	if (rc < 0)
		return rc;
	if (rc == 1)
		return 0; /* EOF */

	return pcm_drain(priv, buffer, count);
}

/* ── seek ──────────────────────────────────────────────────────────────── */

static int avcodec_seek(struct input_plugin_data *ip_data, double offset)
{
	struct avcodec_private *priv = ip_data->private;
	AVStream *st = priv->fmt_ctx->streams[priv->audio_stream];

	int64_t pts = av_rescale_q((int64_t)(offset * AV_TIME_BASE),
				   AV_TIME_BASE_Q, st->time_base);

	int rc = av_seek_frame(priv->fmt_ctx, priv->audio_stream, pts,
			       AVSEEK_FLAG_BACKWARD);
	if (rc < 0) {
		d_print("av_seek_frame: %s\n", av_err2str(rc));
		return -IP_ERROR_INTERNAL;
	}

	avcodec_flush_buffers(priv->codec_ctx);
	swr_convert(priv->swr_ctx, NULL, 0, NULL, 0); /* flush swr state */
	pcm_reset(priv);
	priv->eof = 0;
	priv->flushing = 0;
	return 0;
}

/* ── metadata ──────────────────────────────────────────────────────────── */

static void add_dict(struct growing_keyvals *c, AVDictionary *dict)
{
	if (!dict)
		return;
	const AVDictionaryEntry *e = NULL;
	while ((e = av_dict_get(dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
		const char *key = map_metadata_key(e->key);
		if (key)
			comments_add_const(c, key, e->value);
	}
}

static int avcodec_read_comments(struct input_plugin_data *ip_data,
				 struct keyval **comments)
{
	struct avcodec_private *priv = ip_data->private;
	GROWING_KEYVALS(c);

	/* container-level tags (covers QuickTime, ID3, Vorbis Comments in Ogg)
	 */
	add_dict(&c, priv->fmt_ctx->metadata);

	/* stream-level tags (some formats put tags on the audio stream) */
	if (priv->audio_stream >= 0)
		add_dict(&c,
			 priv->fmt_ctx->streams[priv->audio_stream]->metadata);

	keyvals_terminate(&c);
	*comments = c.keyvals;
	return 0;
}

/* ── info ──────────────────────────────────────────────────────────────── */

static int avcodec_duration(struct input_plugin_data *ip_data)
{
	struct avcodec_private *priv = ip_data->private;
	int64_t dur = priv->fmt_ctx->duration;
	if (dur == AV_NOPTS_VALUE)
		return -IP_ERROR_FUNCTION_NOT_SUPPORTED;
	return (int)(dur / AV_TIME_BASE);
}

static long avcodec_bitrate(struct input_plugin_data *ip_data)
{
	struct avcodec_private *priv = ip_data->private;
	int64_t br = priv->fmt_ctx->bit_rate;
	if (br <= 0)
		br = priv->codec_ctx->bit_rate;
	return br > 0 ? (long)br : -IP_ERROR_FUNCTION_NOT_SUPPORTED;
}

static long avcodec_bitrate_current(struct input_plugin_data *ip_data)
{
	return avcodec_bitrate(ip_data);
}

static char *avcodec_codec(struct input_plugin_data *ip_data)
{
	struct avcodec_private *priv = ip_data->private;
	const char *name = avcodec_get_name(priv->codec_ctx->codec_id);
	return name ? xstrdup(name) : NULL;
}

static char *avcodec_codec_profile(struct input_plugin_data *ip_data)
{
	struct avcodec_private *priv = ip_data->private;
	int profile = priv->codec_ctx->profile;
#ifdef AV_PROFILE_UNKNOWN
	if (profile == AV_PROFILE_UNKNOWN)
		return NULL;
#else
	if (profile == FF_PROFILE_UNKNOWN)
		return NULL;
#endif
	const char *p = av_get_profile_name(priv->codec_ctx->codec, profile);
	return p ? xstrdup(p) : NULL;
}

/* ── plugin registration ───────────────────────────────────────────────── */

const struct input_plugin_ops ip_ops = {
    .open = avcodec_open,
    .close = avcodec_close,
    .read = avcodec_read,
    .seek = avcodec_seek,
    .read_comments = avcodec_read_comments,
    .duration = avcodec_duration,
    .bitrate = avcodec_bitrate,
    .bitrate_current = avcodec_bitrate_current,
    .codec = avcodec_codec,
    .codec_profile = avcodec_codec_profile,
};

/*
 * ip_priority = 50 for formats with no dedicated library (AAC/M4A/MP4).
 * ip_priority = 10 for all others — format-specific plugins win at 50.
 *
 * The value is overridable at runtime via:
 *   set input.avcodec.priority <N>
 */
const int ip_priority = 10;

const char *const ip_extensions[] = {
    /* primary: no maintained dedicated library */
    "m4a", "mp4", "m4b", "aac",
    /* fallback: format-specific plugin preferred */
    "flac", "opus", "ogg", "oga", "mp3", NULL};

const char *const ip_mime_types[] = {"audio/mp4", "audio/x-m4a", "audio/aac",
				     "audio/x-aac", NULL};

const struct input_plugin_opt ip_options[] = {{NULL}};
const unsigned ip_abi_version = IP_ABI_VERSION;
