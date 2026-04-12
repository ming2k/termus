#ifndef TERMUS_PL_ENV_H
#define TERMUS_PL_ENV_H

/**
 * ABOUT
 *   This file contains functions related to the pl_env_vars configuration
 *   option, which allows the library to be relocated by specifying a list of
 *   environment variables to use instead of the base path they contain when
 *   saving library/playlist and cache paths.
 *
 * CONFIGURATION
 * - options.h / pl_env_vars
 *   The array of environment variables to substitute. These are checked in the
 *   order they are specified in, and the first matching variable is used.
 *   Note that it is safe for the configuration to be changed regardless of if
 *   termus is currently running, but termus must be restarted for this to take full
 *   effect.
 *
 * - options.c / get_pl_env_vars + set_pl_env_vars
 *   Encodes and decodes the config entry to/from a comma-separated list.
 *
 * SUBSTITUTION
 * - termus.c / save_playlist_cb
 *   Where the in-memory library/playlists are written on exit.
 *
 *   Paths are transformed into the versions with env var substitutions here to
 *   allow tracks to be relocated based on the path in the env var.
 *
 * - job.c / handle_line
 *   Where the on-disk library/playlists are parsed on startup.
 *
 *   Paths are subsituted from environment variables here as the inverse of
 *   save_playlist_cb.
 *
 * - cache.c / write_ti
 *   Where the in-memory track_info's are converted into cache entries, which
 *   are written to the on-disk cache file.
 *
 *   Paths are transformed into the versions with env var substitutions here to
 *   allow metadata (including the play count) to follow the track regardless of
 *   its real on-disk location.
 *
 * - cache.c / cache_entry_to_ti
 *   Where the on-disk cache entries are parsed into the in-memory hashtable of
 *   read/write track_info's on startup.
 *
 *   Paths are subsituted from environment variables here as the inverse of
 *   write_ti.
 *
 * FAQ
 * - Why use '\x1F' as the delimiter?
 *   1. To prevent compatibility issues if the user happened to use unusual
 *      characters in their filenames. Control characters like this one will not
 *      generally be found in filenames on any platform.
 *   2. This is the ASCII "Unit Separator" control character, which is supposed
 *      to be an (old-fashioned) invisible delimiter, which makes sense for our
 *      purposes.
 *   3. Since it is invisible, we don't need to deal with everywhere it might be
 *      printed to the UI if it is not substituted by pl_env_expand (i.e. if
 *      the env var is missing or invalid).
 *
 * - What other approaches were considered?
 *   1. Using relative paths and resolving the filenames passed to input
 *      plugins. This would have required rewriting the library/playlist and
 *      cache code to handle relative paths, and would have resulted in a high
 *      risk of regressions and taken a long time to implement.
 *   2. Wrapping the filesystem functions. This is simpler to implement than the
 *      previous option, but even more risky.
 *   3. Same as this approach, but using native OS env var syntax. This would be
 *      significantly more complex to implement, and is likely to cause
 *      compatibility issues requiring manual intervention.
 *   4. Adding a configuration option for a base directory, and only storing
 *      relative paths against that. This seems simple at first, but it's has
 *      all the disadvantages of the aforementioned approaches, is more complex
 *      to implement, makes it harder to handle configuration changes while termus
 *      is not running, and is a lot less flexible.
 *
 * - What are the advantages of this approach?
 *    1. This approach is mostly self-contained. There are very few places which
 *       this needs to hook, and the mechanism is easy to reason about.
 *    2. This approach works well with termus. It takes advantage of the way it
 *       keeps all metadata in-memory during runtime.
 *    3. This approach does not have a risk of causing incidental compatibility issues,
 *       behaviour changes, or regressions for users who do not enable this
 *       feature.
 *    4. This approach can handle configuration changes regardless of whether
 *       termus is running.
 *    5. This approach does not require modifying input/output plugins.
 *    6. This approach is os-independent, and can even handle sharing libraries
 *       with platforms using the backslash as a path separator.
 *    7. This approach does not interfere with the stream mechanism.
 *    8. This approach can handle multiple replacement paths (i.e. base
 *       folders).
 *    9. This approach results in behaviour comparable to without it.
 *
 * - What are the disadvantages of this approach?
 *   1. While the substitution works correctly and reliably, it is a somewhat
 *      hacky method. Nevertheless, it is also the least intrusive method, which
 *      is a significant advantage.
 *   2. It is not possible to implement a mechanism for fallback paths within
 *      termus for searching for individual tracks.
 *
 * - What are some potential uses of pl_env_vars?
 *   1. Syncing a $TERMUS_HOME with multiple devices with different home folders.
 *   2. Syncing a $TERMUS_HOME with multiple devices with one or more different
 *      music paths.
 *   3. The above case, but also with a path which only exists on certain
 *      devices.
 *   4. Relocating the cache/library/playlists while preserving metadata
 *      including the play count.
 */

/**
 * PL_ENV_DELIMITER surrounds env var substitutions at the beginning of paths.
 */
#define PL_ENV_DELIMITER '\x1F'

/**
 * pl_env_init initializes the environment variable cache used by pl_env_get. It
 * must be called before loading the library, playlists, or cache.
 */
void pl_env_init(void);

/**
 * pl_env_reduce checks the base path against the configured environment
 * variables, replaces the first match with a substitution, and returns a
 * malloc'd copy of the result. If there isn't any valid match or the path
 * already contains a substitution, a copy of the original path is returned
 * as-is.
 */
char *pl_env_reduce(const char *path);

/**
 * pl_env_expand returns a malloc'd copy of path, with the environment variable
 * substitution. The provided path must use forward slashes, and begin with a
 * slash or a substitution followed by a slash (which will always be true for
 * library paths within termus). If the path does not have a substitution, the
 * original path is returned. If the environment variable does not exist or is
 * invalid, the original path is also returned.
 */
char *pl_env_expand(const char *path);

/**
 * pl_env_var returns a pointer to the start of the substituted environment
 * variable name, or NULL if it is not present. If a variable is present and
 * out_length is not NULL, it is set to the length of the variable name. The
 * remainder of the path will be at pl_env_var_remainder(path, out_length).
 */
const char *pl_env_var(const char *path, int *out_length);

/**
 * pl_env_var_remainder returns a pointer to the remainder of the path after the
 * substitution. See pl_env_var.
 */
const char *pl_env_var_remainder(const char *path, int length);

/**
 * pl_env_var_len returns the length of the substituted environment variable
 * name, if present. Otherwise, it returns 0.
 */
int pl_env_var_len(const char *path);

#endif
