#ifndef TERMUS_OPTIONS_CORE_STATE_H
#define TERMUS_OPTIONS_CORE_STATE_H

extern char *output_plugin;
extern int scroll_offset;
extern int rewind_offset;

extern char *id3_default_charset;
extern char *icecast_default_charset;
extern char **pl_env_vars;

const char *options_get_server_password(void);
void options_set_server_password(const char *password);

#endif
