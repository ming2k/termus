#ifndef TERMUS_OPTIONS_REGISTRY_H
#define TERMUS_OPTIONS_REGISTRY_H

#include "common/list.h"

#define OPTION_MAX_SIZE 4096

typedef void (*opt_get_cb)(void *data, char *buf, size_t size);
typedef void (*opt_set_cb)(void *data, const char *buf);
typedef void (*opt_toggle_cb)(void *data);
typedef void (*opt_post_set_cb)(void *data);

enum opt_type { OPT_INT, OPT_BOOL, OPT_STR, OPT_CALLBACK };

enum {
	OPT_PROGRAM_PATH = 1 << 0,
};

struct termus_opt {
	struct list_head node;
	const char *name;
	void *data;
	void *value;
	enum opt_type type;
	union {
		struct {
			int min;
			int max;
		} i;
	} extra;
	opt_get_cb get;
	opt_set_cb set;
	opt_toggle_cb toggle;
	opt_post_set_cb post_set;
	unsigned int flags;
};

extern struct list_head option_head;
extern int nr_options;

void options_add(void);
void options_load(void);
void options_exit(void);

int source_file(const char *filename);
void option_add(const char *name, const void *data, opt_get_cb get,
		opt_set_cb set, opt_toggle_cb toggle, unsigned int flags);
void option_add_int(const char *name, int *val, int min, int max,
		    unsigned int flags);
void option_add_bool(const char *name, int *val, unsigned int flags);
void option_add_str(const char *name, char **val, unsigned int flags);

void option_add_int_full(const char *name, int *val, int min, int max,
			 opt_post_set_cb post_set, unsigned int flags);
void option_add_bool_full(const char *name, int *val, opt_post_set_cb post_set,
			  unsigned int flags);
void option_add_str_full(const char *name, char **val, opt_post_set_cb post_set,
			 unsigned int flags);

struct termus_opt *option_find(const char *name);
struct termus_opt *option_find_silent(const char *name);
void option_set(const char *name, const char *value);
int parse_enum(const char *buf, int minval, int maxval,
	       const char *const names[], int *val);

#endif
