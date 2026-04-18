#include "app/commands.h"
#include "app/options_parse.h"
#include "app/options_registry.h"
#include "common/file.h"
#include "common/misc.h"
#include "common/msg.h"
#include "common/utils.h"
#include "common/xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LIST_HEAD(option_head);
int nr_options = 0;

static void generic_get_int(void *data, char *buf, size_t size)
{
	const struct termus_opt *opt = data;
	int *val = opt->value;
	snprintf(buf, size, "%d", *val);
}

static void generic_set_int(void *data, const char *buf)
{
	struct termus_opt *opt = data;
	int *val = opt->value;
	long int tmp;

	if (str_to_int(buf, &tmp) == -1 || tmp < opt->extra.i.min ||
	    tmp > opt->extra.i.max) {
		error_msg("integer in range %d..%d expected", opt->extra.i.min,
			  opt->extra.i.max);
		return;
	}
	*val = (int)tmp;
	if (opt->post_set)
		opt->post_set(opt->value);
}

static void generic_get_bool(void *data, char *buf, size_t size)
{
	const struct termus_opt *opt = data;
	int *val = opt->value;
	strscpy(buf, *val ? "true" : "false", size);
}

static void generic_set_bool(void *data, const char *buf)
{
	struct termus_opt *opt = data;
	int *val = opt->value;
	int tmp;

	if (options_parse_bool(buf, &tmp)) {
		*val = tmp;
		if (opt->post_set)
			opt->post_set(opt->value);
	}
}

static void generic_toggle_bool(void *data)
{
	struct termus_opt *opt = data;
	int *val = opt->value;
	*val = !*val;
	if (opt->post_set)
		opt->post_set(opt->value);
}

static void generic_get_str(void *data, char *buf, size_t size)
{
	const struct termus_opt *opt = data;
	char **val = opt->value;
	if (*val)
		strscpy(buf, *val, size);
	else
		buf[0] = 0;
}

#if defined(__GNUC__) && __GNUC__ >= 10
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
#endif
static void generic_set_str(void *data, const char *buf)
{
	struct termus_opt *opt = data;
	char **val = opt->value;

	free(*val);
	*val = xstrdup(buf);
	if (opt->post_set)
		opt->post_set(opt->value);
}
#if defined(__GNUC__) && __GNUC__ >= 10
#pragma GCC diagnostic pop
#endif

static void option_insert(struct termus_opt *opt)
{
	struct list_head *item;

	item = option_head.next;
	while (item != &option_head) {
		struct termus_opt *o =
		    container_of(item, struct termus_opt, node);

		if (strcmp(opt->name, o->name) < 0)
			break;
		item = item->next;
	}

	/* add before item */
	list_add_tail(&opt->node, item);
	nr_options++;
}

void option_add(const char *name, const void *data, opt_get_cb get,
		opt_set_cb set, opt_toggle_cb toggle, unsigned int flags)
{
	struct termus_opt *opt = xnew(struct termus_opt, 1);

	opt->name = name;
	opt->data = (void *)data;
	opt->value = NULL;
	opt->type = OPT_CALLBACK;
	opt->get = get;
	opt->set = set;
	opt->toggle = toggle;
	opt->post_set = NULL;
	opt->flags = flags;

	option_insert(opt);
}

void option_add_int_full(const char *name, int *val, int min, int max,
			 opt_post_set_cb post_set, unsigned int flags)
{
	struct termus_opt *opt = xnew(struct termus_opt, 1);

	opt->name = name;
	opt->data = opt;
	opt->value = val;
	opt->type = OPT_INT;
	opt->extra.i.min = min;
	opt->extra.i.max = max;
	opt->get = generic_get_int;
	opt->set = generic_set_int;
	opt->toggle = NULL;
	opt->post_set = post_set;
	opt->flags = flags;

	option_insert(opt);
}

void option_add_int(const char *name, int *val, int min, int max,
		    unsigned int flags)
{
	option_add_int_full(name, val, min, max, NULL, flags);
}

void option_add_bool_full(const char *name, int *val, opt_post_set_cb post_set,
			  unsigned int flags)
{
	struct termus_opt *opt = xnew(struct termus_opt, 1);

	opt->name = name;
	opt->data = opt;
	opt->value = val;
	opt->type = OPT_BOOL;
	opt->get = generic_get_bool;
	opt->set = generic_set_bool;
	opt->toggle = generic_toggle_bool;
	opt->post_set = post_set;
	opt->flags = flags;

	option_insert(opt);
}

void option_add_bool(const char *name, int *val, unsigned int flags)
{
	option_add_bool_full(name, val, NULL, flags);
}

void option_add_str_full(const char *name, char **val, opt_post_set_cb post_set,
			 unsigned int flags)
{
	struct termus_opt *opt = xnew(struct termus_opt, 1);

	opt->name = name;
	opt->data = opt;
	opt->value = val;
	opt->type = OPT_STR;
	opt->get = generic_get_str;
	opt->set = generic_set_str;
	opt->toggle = NULL;
	opt->post_set = post_set;
	opt->flags = flags;

	option_insert(opt);
}

void option_add_str(const char *name, char **val, unsigned int flags)
{
	option_add_str_full(name, val, NULL, flags);
}

struct termus_opt *option_find(const char *name)
{
	struct termus_opt *opt = option_find_silent(name);

	if (opt == NULL)
		error_msg("no such option %s", name);
	return opt;
}

struct termus_opt *option_find_silent(const char *name)
{
	struct termus_opt *opt;

	list_for_each_entry(opt, &option_head, node)
	{
		if (strcmp(name, opt->name) == 0)
			return opt;
	}
	return NULL;
}

void option_set(const char *name, const char *value)
{
	struct termus_opt *opt = option_find(name);

	if (opt)
		opt->set(opt->data, value);
}

static int handle_line(void *data, const char *line)
{
	run_command(line);
	return 0;
}

int source_file(const char *filename)
{
	return file_for_each_line(filename, handle_line, NULL);
}
