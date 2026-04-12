#ifndef TERMUS_WORKER_H
#define TERMUS_WORKER_H

#include <stdint.h>

#define JOB_TYPE_NONE 0
#define JOB_TYPE_ANY  ~0

#define JOB_TYPE_LIB   (1 << 0)
#define JOB_TYPE_PL    (1 << 1)
#define JOB_TYPE_QUEUE (1 << 2)

#define JOB_TYPE_ADD          (1 << 16)
#define JOB_TYPE_UPDATE       (1 << 17)
#define JOB_TYPE_UPDATE_CACHE (1 << 18)
#define JOB_TYPE_DELETE       (1 << 19)

typedef int (*worker_match_cb)(uint32_t type, void *job_data, void *opaque);

void worker_init(void);
void worker_start(void);
void worker_exit(void);

void worker_add_job(uint32_t type, void (*job_cb)(void *job_data),
		void (*free_cb)(void *job_data), void *job_data);

/* NOTE: The callbacks below run in parallel with the job_cb function. Access to
 * job_data must by synchronized.
 */

void worker_remove_jobs_by_type(uint32_t pat);
void worker_remove_jobs_by_cb(worker_match_cb cb, void *opaque);

int worker_has_job(void);
int worker_has_job_by_type(uint32_t pat);
int worker_has_job_by_cb(worker_match_cb cb, void *opaque);

int worker_cancelling(void);

#endif
