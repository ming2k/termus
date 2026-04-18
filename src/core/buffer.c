#include "core/buffer.h"
#include "common/debug.h"
#include "common/locking.h"
#include "common/xmalloc.h"

#include <sys/time.h>

/*
 * chunk can be accessed by either consumer OR producer, not both at same time
 * -> no need to lock
 */
struct chunk {
	char data[CHUNK_SIZE];

	/* index to data, first filled byte */
	unsigned int l;

	/* index to data, last filled byte + 1
	 *
	 * there are h - l bytes available (filled)
	 */
	unsigned int h : 31;

	/* if chunk is marked filled it can only be accessed by consumer
	 * otherwise only producer is allowed to access the chunk
	 */
	unsigned int filled : 1;
};

unsigned int buffer_nr_chunks;

static pthread_mutex_t buffer_mutex = TERMUS_MUTEX_INITIALIZER;
static pthread_cond_t buffer_not_empty = TERMUS_COND_INITIALIZER;
static pthread_cond_t buffer_not_full = TERMUS_COND_INITIALIZER;
static struct chunk *buffer_chunks = NULL;
static unsigned int buffer_ridx;
static unsigned int buffer_widx;

void buffer_init(void)
{
	free(buffer_chunks);
	buffer_chunks = xnew(struct chunk, buffer_nr_chunks);
	buffer_reset();
}

void buffer_free(void) { free(buffer_chunks); }

/*
 * @pos: returned pointer to available data
 *
 * Returns number of bytes available at @pos
 *
 * After reading bytes mark them consumed calling buffer_consume().
 */
int buffer_get_rpos(char **pos)
{
	struct chunk *c;
	int size = 0;

	termus_mutex_lock(&buffer_mutex);
	c = &buffer_chunks[buffer_ridx];
	if (c->filled) {
		size = c->h - c->l;
		*pos = c->data + c->l;
	}
	termus_mutex_unlock(&buffer_mutex);

	return size;
}

/*
 * @pos: pointer to buffer position where data can be written
 *
 * Returns number of bytes can be written to @pos.  If the return value is
 * non-zero it is guaranteed to be >= 1024.
 *
 * After writing bytes mark them filled calling buffer_fill().
 */
int buffer_get_wpos(char **pos)
{
	struct chunk *c;
	int size = 0;

	termus_mutex_lock(&buffer_mutex);
	c = &buffer_chunks[buffer_widx];
	if (!c->filled) {
		size = CHUNK_SIZE - c->h;
		*pos = c->data + c->h;
	}
	termus_mutex_unlock(&buffer_mutex);

	return size;
}

void buffer_consume(int count)
{
	struct chunk *c;

	BUG_ON(count < 0);
	termus_mutex_lock(&buffer_mutex);
	c = &buffer_chunks[buffer_ridx];
	BUG_ON(!c->filled);
	c->l += count;
	if (c->l == c->h) {
		c->l = 0;
		c->h = 0;
		c->filled = 0;
		buffer_ridx++;
		buffer_ridx %= buffer_nr_chunks;
		pthread_cond_signal(&buffer_not_full);
	}
	termus_mutex_unlock(&buffer_mutex);
}

/* chunk is marked filled if free bytes < 1024 or count == 0 */
int buffer_fill(int count)
{
	struct chunk *c;
	int filled = 0;

	termus_mutex_lock(&buffer_mutex);
	c = &buffer_chunks[buffer_widx];
	BUG_ON(c->filled);
	c->h += count;

	if (CHUNK_SIZE - c->h < 1024 || (count == 0 && c->h > 0)) {
		c->filled = 1;
		buffer_widx++;
		buffer_widx %= buffer_nr_chunks;
		filled = 1;
		pthread_cond_signal(&buffer_not_empty);
	}

	termus_mutex_unlock(&buffer_mutex);
	return filled;
}

void buffer_reset(void)
{
	int i;

	termus_mutex_lock(&buffer_mutex);
	buffer_ridx = 0;
	buffer_widx = 0;
	for (i = 0; i < buffer_nr_chunks; i++) {
		buffer_chunks[i].l = 0;
		buffer_chunks[i].h = 0;
		buffer_chunks[i].filled = 0;
	}
	/* wake up any thread waiting on buffer state */
	pthread_cond_broadcast(&buffer_not_empty);
	pthread_cond_broadcast(&buffer_not_full);
	termus_mutex_unlock(&buffer_mutex);
}

int buffer_get_filled_chunks(void)
{
	int c;

	termus_mutex_lock(&buffer_mutex);
	if (buffer_ridx < buffer_widx) {
		/*
		 * |__##########____|
		 *    r         w
		 *
		 * |############____|
		 *  r           w
		 */
		c = buffer_widx - buffer_ridx;
	} else if (buffer_ridx > buffer_widx) {
		/*
		 * |#######______###|
		 *         w     r
		 *
		 * |_____________###|
		 *  w            r
		 */
		c = buffer_nr_chunks - buffer_ridx + buffer_widx;
	} else {
		/*
		 * |################|
		 *     r
		 *     w
		 *
		 * |________________|
		 *     r
		 *     w
		 */
		if (buffer_chunks[buffer_ridx].filled) {
			c = buffer_nr_chunks;
		} else {
			c = 0;
		}
	}
	termus_mutex_unlock(&buffer_mutex);
	return c;
}

/*
 * Wait until the buffer has data to read, with a timeout.
 * Called by the consumer when the buffer is empty (possible underrun).
 */
void buffer_wait_fill(void)
{
	struct timespec ts;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000 + 50 * 1000000L; /* 50ms timeout */
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}

	termus_mutex_lock(&buffer_mutex);
	if (!buffer_chunks[buffer_ridx].filled)
		pthread_cond_timedwait(&buffer_not_empty, &buffer_mutex, &ts);
	termus_mutex_unlock(&buffer_mutex);
}

/*
 * Wait until the buffer has space to write, with a timeout.
 * Called by the producer when the buffer is full.
 */
void buffer_wait_drain(void)
{
	struct timespec ts;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000 + 50 * 1000000L; /* 50ms timeout */
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}

	termus_mutex_lock(&buffer_mutex);
	if (buffer_chunks[buffer_widx].filled)
		pthread_cond_timedwait(&buffer_not_full, &buffer_mutex, &ts);
	termus_mutex_unlock(&buffer_mutex);
}
