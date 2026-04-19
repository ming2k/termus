#ifndef TERMUS_LOCKING_H
#define TERMUS_LOCKING_H

#include <pthread.h>
#include <stdatomic.h>

struct fifo_mutex {
	struct fifo_waiter *_Atomic tail;
	struct fifo_waiter *head;
	pthread_mutex_t mutex;
};

extern pthread_t main_thread;

#define TERMUS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define TERMUS_COND_INITIALIZER PTHREAD_COND_INITIALIZER
#define TERMUS_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER

#define FIFO_MUTEX_INITIALIZER                                                 \
	{                                                                      \
	    .mutex = PTHREAD_MUTEX_INITIALIZER,                                \
	    .tail = NULL,                                                      \
	}

void termus_mutex_lock(pthread_mutex_t *mutex);
void termus_mutex_unlock(pthread_mutex_t *mutex);
void termus_rwlock_rdlock(pthread_rwlock_t *lock);
void termus_rwlock_wrlock(pthread_rwlock_t *lock);
void termus_rwlock_unlock(pthread_rwlock_t *lock);

void fifo_mutex_lock(struct fifo_mutex *fifo);
void fifo_mutex_unlock(struct fifo_mutex *fifo);
void fifo_mutex_yield(struct fifo_mutex *fifo);

#endif
