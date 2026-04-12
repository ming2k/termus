#ifndef TERMUS_JOB_H
#define TERMUS_JOB_H

#include "app/termus.h"
#include "common/worker.h"

struct add_data {
	enum file_type type;
	char *name;
	add_ti_cb add;
	void *opaque;
	unsigned int force : 1;
};

struct update_data {
	size_t size;
	size_t used;
	struct track_info **ti;
	unsigned int force : 1;
};

struct update_cache_data {
	unsigned int force : 1;
};

struct pl_delete_data {
	struct playlist *pl;
	void (*cb)(struct playlist *);
};

extern int job_fd;

void job_init(void);
void job_exit(void);
void job_schedule_add(int type, struct add_data *data);
void job_schedule_update(struct update_data *data);
void job_schedule_update_cache(int type, struct update_cache_data *data);
void job_schedule_pl_delete(struct pl_delete_data *data);
void job_handle(void);

#endif
