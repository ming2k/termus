# Codec, Container, and Metadata Reference

This document is the authoritative reference for termus's audio decoding
architecture. It covers supported formats, how each format's container and
metadata are handled, and how decoder backends are selected and prioritised.

---

## Backend architecture

termus uses a runtime-loadable plugin system. Each input plugin exports a
priority value (`ip_priority`). When a file is opened, all plugins that declare
support for that extension are tried in **descending priority order** — the
plugin with the highest `ip_priority` wins.

```
ip_priority = 50   primary / format-specific library (preferred)
ip_priority = 10   FFmpeg fallback (used when primary is absent)
ip_priority ≤ 0    disabled / never selected
```

A format is supported if **at least one** backend for it is present at runtime.
The build system enforces this: configure fails unless either the format-specific
library or the FFmpeg backend is detected.

### Why multiple backends?

Each format has a preferred backend — usually the library authored by the same
group that defined the format. These are narrow, well-tested, and handle
format-specific edge cases (e.g. FLAC seek tables, Opus R128 gain) correctly.

FFmpeg (`libavformat` + `libavcodec`) is an excellent general-purpose decoder
that covers every supported format. It serves two roles:

1. **Primary backend** for formats that have no maintained dedicated library
   (currently: AAC / M4A / MP4, where `mp4v2` is unmaintained and `faad2` is
   aging).

2. **Fallback backend** for all other formats, activated when the preferred
   library is not installed.

---

## Format × container × metadata × backend matrix

### FLAC

| Property | Value |
|----------|-------|
| Container | FLAC native (`.flac`) |
| Codec | Free Lossless Audio Codec |
| Metadata | **Vorbis Comments** |
| Primary backend | `flac.c` via **libFLAC** — `ip_priority = 50` |
| Fallback backend | `avcodec.c` via **FFmpeg** — `ip_priority = 10` |
| Build default | libFLAC required; FFmpeg optional |

libFLAC is the reference implementation: it handles SEEKTABLE blocks, MD5
verification, and all METADATA_BLOCK types accurately. Prefer it over FFmpeg
when available.

---

### Opus

| Property | Value |
|----------|-------|
| Container | Ogg (`.opus`) |
| Codec | Opus (RFC 6716) |
| Metadata | **Vorbis Comments** |
| Primary backend | `opus.c` via **libopusfile** — `ip_priority = 50` |
| Fallback backend | `avcodec.c` via **FFmpeg** — `ip_priority = 10` |
| Build default | libopusfile required; FFmpeg optional |

libopusfile is the reference Opus decoder. It applies the Opus header
`output_gain` field automatically (RFC 7845 §5.2.1) and handles chained
Ogg streams correctly.

---

### Ogg Vorbis

| Property | Value |
|----------|-------|
| Container | Ogg (`.ogg`, `.oga`) |
| Codec | Vorbis |
| Metadata | **Vorbis Comments** |
| Primary backend | `vorbis.c` via **libvorbisfile** — `ip_priority = 50` |
| Fallback backend | `avcodec.c` via **FFmpeg** — `ip_priority = 10` |
| Build default | libvorbisfile auto-detected; FFmpeg optional |

Ogg Vorbis is a widely-held legacy format. Opus is the recommended successor
for new content.

---

### AAC in M4A / MP4 container

| Property | Value |
|----------|-------|
| Container | MPEG-4 (`.m4a`, `.mp4`, `.m4b`) |
| Codec | AAC-LC / HE-AAC (MPEG-4 Part 3) |
| Metadata | **QuickTime / iTunes atom tags** |
| Primary backend | `avcodec.c` via **FFmpeg** — `ip_priority = 50` |
| Fallback backend | none |
| Build default | FFmpeg required for this format |

There is no maintained dedicated library for the M4A/MP4 container:
`mp4v2` was last meaningfully released in 2012 and is absent from most
distribution repositories; `faad2` handles raw AAC bitstreams but not the
container. FFmpeg (`libavformat`) is the correct choice here: it demuxes the
MP4 container, reads QuickTime atom metadata, and decodes AAC via
`libavcodec`.

---

### Raw AAC bitstream

| Property | Value |
|----------|-------|
| Container | none / ADTS (`.aac`) |
| Codec | AAC |
| Metadata | **ID3v2** |
| Primary backend | `avcodec.c` via **FFmpeg** — `ip_priority = 50` |
| Fallback backend | none |
| Build default | FFmpeg required for this format |

Bare `.aac` files carry ADTS framing and may have an ID3v2 tag prepended.
FFmpeg detects ADTS and reads ID3v2 metadata automatically.

---

### MP3 (legacy)

| Property | Value |
|----------|-------|
| Container | MPEG-1 Audio Layer III (`.mp3`) |
| Codec | MP3 |
| Metadata | **ID3v2 only** |
| Primary backend | `mpg123.c` via **libmpg123** — `ip_priority = 50` |
| Fallback backend | `avcodec.c` via **FFmpeg** — `ip_priority = 10` |
| Build default | libmpg123 auto-detected; FFmpeg optional |

MP3 is supported for playback of existing collections. It is not a recommended
format for new content. ID3v1 (128-byte fixed-length footer) is **not read**.

libmpg123 provides the most accurate gapless playback and ID3v2 handling for
MP3; prefer it when available.

---

### CUE sheets

| Property | Value |
|----------|-------|
| Container | CUE playlist file (`.cue`) |
| Codec | delegates to the underlying audio file's plugin |
| Metadata | CUE sheet INDEX/TRACK fields |
| Backend | `cue.c` (no external library) — `ip_priority = 50` |
| Build default | enabled |

CUE sheets describe track boundaries within a single audio file. Commonly
used with FLAC album rips where one `.flac` file plus a `.cue` sheet
represents a complete album.

---

## Metadata standards

### Vorbis Comments (FLAC, Opus, Ogg Vorbis)

UTF-8 key=value pairs embedded in the bitstream. Keys are case-insensitive;
termus lowercases them on read.

Standard keys recognised by termus:

```
title           album           artist          albumartist
date            originaldate    genre           tracknumber
discnumber      composer        conductor       lyricist
remixer         label           publisher       subtitle
comment         compilation     bpm             media
artistsort      albumartistsort albumsort
replaygain_track_gain           replaygain_track_peak
replaygain_album_gain           replaygain_album_peak
musicbrainz_trackid
```

### QuickTime / iTunes atom tags (M4A / MP4)

Atom-based metadata embedded in `udta`/`ilst` boxes. FFmpeg maps the common
atoms to standard string keys (`title`, `artist`, `album`, `date`, `genre`,
`track`, `disc`, `album_artist`, …). termus remaps these to its internal key
names on read.

### ID3v2 (MP3, raw AAC)

ID3v2.3 and ID3v2.4 text frames. Standard frames (`TIT2`, `TPE1`, `TALB`,
`TDRC`, `TCON`, `TRCK`, `TPOS`, `TPE2`, …) and `TXXX` frames for ReplayGain
and MusicBrainz IDs.

**ID3v1 is not read.** The 128-byte fixed-length ID3v1 footer is an obsolete
format; termus's ID3 reader no longer contains code to parse it.

---

## Formats explicitly out of scope

| Format | Reason |
|--------|--------|
| WAV / AIFF | Uncompressed; minimal metadata; use FLAC |
| WavPack | Niche lossless; FLAC is the standard |
| Musepack (MPC) | Obsolete lossy format |
| CDDA | Optical media is out of scope |
| MOD / MikMod / ModPlug | Tracker formats; different use-case domain |
| VTX / chiptune | Different use-case domain |
| BASS | Proprietary, non-free |
| DSD | Not currently supported |

---

## Build-time format coverage

configure checks that at least one backend is available for each primary
format and errors with a clear message if not:

```
FLAC:    libFLAC  OR  FFmpeg  (--disable-flac / --disable-avcodec)
Opus:    libopusfile  OR  FFmpeg
Vorbis:  libvorbisfile  OR  FFmpeg   (soft dependency)
AAC/M4A: FFmpeg  (required — no alternative)
MP3:     libmpg123  OR  FFmpeg   (soft dependency)
```

To inspect what was detected, run `../configure` and read the summary at the
end, or check `config.h` for `HAVE_AVCODEC`, `HAVE_FLAC`, etc.

---

## Adding a new backend

1. Create `src/plugins/input/<name>.c` implementing `struct input_plugin_ops`.
2. Declare `ip_extensions[]`, `ip_mime_types[]`, and `ip_priority`.
   - Use `ip_priority = 50` if this is the primary backend for a format.
   - Use `ip_priority = 10` if this is a fallback.
3. Add detection in `configure.ac` and a build rule in
   `src/plugins/input/Makefile.am`.
4. Update this document.
