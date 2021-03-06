/*
 * Copyright 2013 Luke Dashjr
 * Copyright 2012-2013 Con Kolivas
 * Copyright 2011 Andrew Smith
 * Copyright 2011 Jeff Garzik
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>
#include <string.h>
#include <sys/time.h>

#include <curl/curl.h>
#include <jansson.h>

#include "compat.h"

#define INVALID_TIMESTAMP ((time_t)-1)

#if defined(unix) || defined(__APPLE__)
	#include <errno.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#define SOCKETTYPE int
	#define SOCKETFAIL(a) ((a) < 0)
	#define INVSOCK -1
	#define INVINETADDR -1
	#define CLOSESOCKET close

	#define SOCKERR (errno)
	#define SOCKERRMSG bfg_strerror(errno, BST_SOCKET)
	static inline bool sock_blocks(void)
	{
		return (errno == EAGAIN || errno == EWOULDBLOCK);
	}
#elif defined WIN32
	#include <ws2tcpip.h>
	#include <winsock2.h>

	#define SOCKETTYPE SOCKET
	#define SOCKETFAIL(a) ((int)(a) == SOCKET_ERROR)
	#define INVSOCK INVALID_SOCKET
	#define INVINETADDR INADDR_NONE
	#define CLOSESOCKET closesocket

	#define SOCKERR (WSAGetLastError())
	#define SOCKERRMSG bfg_strerror(WSAGetLastError(), BST_SOCKET)

	static inline bool sock_blocks(void)
	{
		return (WSAGetLastError() == WSAEWOULDBLOCK);
	}
	#ifndef SHUT_RDWR
	#define SHUT_RDWR SD_BOTH
	#endif

	#ifndef in_addr_t
	#define in_addr_t uint32_t
	#endif
#endif

#if JANSSON_MAJOR_VERSION >= 2
#define JSON_LOADS(str, err_ptr) json_loads((str), 0, (err_ptr))
#else
#define JSON_LOADS(str, err_ptr) json_loads((str), (err_ptr))
#endif
extern char *json_dumps_ANY(json_t *, size_t flags);

static inline
const char *bfg_json_obj_string(json_t *json, const char *key, const char *fail)
{
	json = json_object_get(json, key);
	if (!json)
		return fail;
	return json_string_value(json) ?: fail;
}

extern const char *__json_array_string(json_t *, unsigned int entry);

static inline
bool isCspace(int c)
{
	switch (c)
	{
		case ' ': case '\f': case '\n': case '\r': case '\t': case '\v':
			return true;
		default:
			return false;
	}
}

typedef struct timeval cgtimer_t;

struct thr_info;
struct pool;
enum dev_reason;
struct cgpu_info;

extern void json_rpc_call_async(CURL *, const char *url, const char *userpass, const char *rpc_req, bool longpoll, struct pool *pool, bool share, void *priv);
extern json_t *json_rpc_call_completed(CURL *, int rc, bool probe, int *rolltime, void *out_priv);

extern char *absolute_uri(char *uri, const char *ref);  // ref must be a root URI

extern void gen_hash(unsigned char *data, unsigned char *hash, int len);
extern void hash_data(unsigned char *out_hash, const unsigned char *data);
extern void real_block_target(unsigned char *target, const unsigned char *data);
extern bool hash_target_check(const unsigned char *hash, const unsigned char *target);
extern bool hash_target_check_v(const unsigned char *hash, const unsigned char *target);

int thr_info_create(struct thr_info *thr, pthread_attr_t *attr, void *(*start) (void *), void *arg);
void thr_info_freeze(struct thr_info *thr);
void thr_info_cancel(struct thr_info *thr);
void subtime(struct timeval *a, struct timeval *b);
void addtime(struct timeval *a, struct timeval *b);
bool time_more(struct timeval *a, struct timeval *b);
bool time_less(struct timeval *a, struct timeval *b);
void copy_time(struct timeval *dest, const struct timeval *src);
void timespec_to_val(struct timeval *val, const struct timespec *spec);
void timeval_to_spec(struct timespec *spec, const struct timeval *val);
void us_to_timeval(struct timeval *val, int64_t us);
void us_to_timespec(struct timespec *spec, int64_t us);
void ms_to_timespec(struct timespec *spec, int64_t ms);
void timeraddspec(struct timespec *a, const struct timespec *b);
void cgsleep_ms(int ms);
void cgsleep_us(int64_t us);
#define cgtimer_time(ts_start) timer_set_now(ts_start)
#define cgsleep_prepare_r(ts_start) cgtimer_time(ts_start)
void cgsleep_ms_r(cgtimer_t *ts_start, int ms);
void (*cgsleep_us_r)(cgtimer_t *ts_start, int64_t us);

static inline
int cgtimer_to_ms(cgtimer_t *cgt)
{
	return (cgt->tv_sec * 1000) + (cgt->tv_usec / 1000);
}

#define cgtimer_sub(a, b, res)  timersub(a, b, res)
double us_tdiff(struct timeval *end, struct timeval *start);
double tdiff(struct timeval *end, struct timeval *start);
bool _stratum_send(struct pool *pool, char *s, ssize_t len, bool force);
#define stratum_send(pool, s, len)  _stratum_send(pool, s, len, false)
bool sock_full(struct pool *pool);
char *recv_line(struct pool *pool);
bool parse_method(struct pool *pool, char *s);
bool extract_sockaddr(struct pool *pool, char *url);
bool auth_stratum(struct pool *pool);
bool initiate_stratum(struct pool *pool);
bool restart_stratum(struct pool *pool);
void suspend_stratum(struct pool *pool);
extern void dev_error_update(struct cgpu_info *, enum dev_reason);
void dev_error(struct cgpu_info *dev, enum dev_reason reason);
void *realloc_strcat(char *ptr, char *s);
extern char *sanestr(char *o, char *s);
void RenameThread(const char* name);

enum bfg_strerror_type {
	BST_ERRNO,
	BST_SOCKET,
	BST_LIBUSB,
};
extern const char *bfg_strerror(int, enum bfg_strerror_type);

typedef SOCKETTYPE notifier_t[2];
extern void notifier_init(notifier_t);
extern void notifier_wake(notifier_t);
extern void notifier_read(notifier_t);
extern void notifier_init_invalid(notifier_t);
extern void notifier_destroy(notifier_t);

/* Align a size_t to 4 byte boundaries for fussy arches */
static inline void align_len(size_t *len)
{
	if (*len % 4)
		*len += 4 - (*len % 4);
}


typedef struct bytes_t {
	uint8_t *buf;
	size_t sz;
	size_t allocsz;
} bytes_t;

#define BYTES_INIT ((bytes_t){.buf=NULL,})

static inline
void bytes_init(bytes_t *b)
{
	*b = BYTES_INIT;
}

// This can't be inline without ugly const/non-const issues
#define bytes_buf(b)  ((b)->buf)

static inline
size_t bytes_len(const bytes_t *b)
{
	return b->sz;
}

extern void _bytes_alloc_failure(size_t);

static inline
void bytes_resize(bytes_t *b, size_t newsz)
{
	b->sz = newsz;
	if (newsz <= b->allocsz)
		return;
	
	if (!b->allocsz)
		b->allocsz = 0x10;
	do {
		b->allocsz *= 2;
	} while (newsz > b->allocsz);
	b->buf = realloc(b->buf, b->allocsz);
	if (!b->buf)
		_bytes_alloc_failure(b->allocsz);
}

static inline
void bytes_append(bytes_t *b, const void *add, size_t addsz)
{
	size_t origsz = bytes_len(b);
	bytes_resize(b, origsz + addsz);
	memcpy(&bytes_buf(b)[origsz], add, addsz);
}

static inline
void bytes_cat(bytes_t *b, const bytes_t *cat)
{
	bytes_append(b, bytes_buf(cat), bytes_len(cat));
}

static inline
void bytes_cpy(bytes_t *dst, const bytes_t *src)
{
	dst->sz = src->sz;
	if (!dst->sz) {
		dst->allocsz = 0;
		dst->buf = NULL;
		return;
	}
	dst->allocsz = src->allocsz;
	size_t half;
	while (dst->sz <= (half = dst->allocsz / 2))
		dst->allocsz = half;
	dst->buf = malloc(dst->allocsz);
	memcpy(dst->buf, src->buf, dst->sz);
}

static inline
void bytes_shift(bytes_t *b, size_t shift)
{
	b->sz -= shift;
	memmove(bytes_buf(b), &bytes_buf(b)[shift], bytes_len(b));
}

static inline
void bytes_reset(bytes_t *b)
{
	b->sz = 0;
}

static inline
void bytes_nullterminate(bytes_t *b)
{
	bytes_append(b, "", 1);
	--b->sz;
}

static inline
void bytes_free(bytes_t *b)
{
	free(b->buf);
	b->sz = b->allocsz = 0;
}


static inline
void set_maxfd(int *p_maxfd, int fd)
{
	if (fd > *p_maxfd)
		*p_maxfd = fd;
}


static inline
void timer_unset(struct timeval *tvp)
{
	tvp->tv_sec = -1;
}

static inline
bool timer_isset(const struct timeval *tvp)
{
	return tvp->tv_sec != -1;
}

extern void (*timer_set_now)(struct timeval *);
#define cgtime(tvp)  timer_set_now(tvp)

#define TIMEVAL_USECS(usecs)  (  \
	(struct timeval){  \
		.tv_sec = (usecs) / 1000000,  \
		.tv_usec = (usecs) % 1000000,  \
	}  \
)

#define timer_set_delay(tvp_timer, tvp_now, usecs)  do {  \
	struct timeval tv_add = TIMEVAL_USECS(usecs);  \
	timeradd(&tv_add, tvp_now, tvp_timer);  \
} while(0)

#define timer_set_delay_from_now(tvp_timer, usecs)  do {  \
	struct timeval tv_now;  \
	timer_set_now(&tv_now);  \
	timer_set_delay(tvp_timer, &tv_now, usecs);  \
} while(0)

static inline
const struct timeval *_bfg_nullisnow(const struct timeval *tvp, struct timeval *tvp_buf)
{
	if (tvp)
		return tvp;
	cgtime(tvp_buf);
	return tvp_buf;
}

static inline
long timer_elapsed_us(const struct timeval *tvp_timer, const struct timeval *tvp_now)
{
	struct timeval tv;
	const struct timeval *_tvp_now = _bfg_nullisnow(tvp_now, &tv);
	timersub(_tvp_now, tvp_timer, &tv);
	return ((long)tv.tv_sec * 1000000) + tv.tv_usec;
}

static inline
int timer_elapsed(const struct timeval *tvp_timer, const struct timeval *tvp_now)
{
	struct timeval tv;
	const struct timeval *_tvp_now = _bfg_nullisnow(tvp_now, &tv);
	timersub(_tvp_now, tvp_timer, &tv);
	return tv.tv_sec;
}

static inline
bool timer_passed(const struct timeval *tvp_timer, const struct timeval *tvp_now)
{
	if (!timer_isset(tvp_timer))
		return false;
	
	struct timeval tv;
	const struct timeval *_tvp_now = _bfg_nullisnow(tvp_now, &tv);
	
	return timercmp(tvp_timer, _tvp_now, <);
}

#if defined(WIN32) && !defined(HAVE_POOR_GETTIMEOFDAY)
#define HAVE_POOR_GETTIMEOFDAY
#endif

#ifdef HAVE_POOR_GETTIMEOFDAY
extern void bfg_gettimeofday(struct timeval *);
#else
#define bfg_gettimeofday(out)  gettimeofday(out, NULL)
#endif

static inline
void reduce_timeout_to(struct timeval *tvp_timeout, struct timeval *tvp_time)
{
	if (!timer_isset(tvp_time))
		return;
	if ((!timer_isset(tvp_timeout)) || timercmp(tvp_time, tvp_timeout, <))
		*tvp_timeout = *tvp_time;
}

static inline
struct timeval *select_timeout(struct timeval *tvp_timeout, struct timeval *tvp_now)
{
	if (!timer_isset(tvp_timeout))
		return NULL;
	
	if (timercmp(tvp_timeout, tvp_now, <))
		timerclear(tvp_timeout);
	else
		timersub(tvp_timeout, tvp_now, tvp_timeout);
	
	return tvp_timeout;
}


#define _SNP2(fn, ...)  do{  \
        int __n42 = fn(s, sz, __VA_ARGS__);  \
        s += __n42;  \
        sz = (sz <= __n42) ? 0 : (sz - __n42);  \
        rv += __n42;  \
}while(0)

#define _SNP(...)  _SNP2(snprintf, __VA_ARGS__)


#define RUNONCE(rv)  do {  \
	static bool _runonce = false;  \
	if (_runonce)  \
		return rv;  \
	_runonce = true;  \
} while(0)


static inline
char *maybe_strdup(const char *s)
{
	return s ? strdup(s) : NULL;
}

static inline
void maybe_strdup_if_null(const char **p, const char *s)
{
	if (!*p)
		*p = maybe_strdup(s);
}


extern void run_cmd(const char *cmd);


#endif /* __UTIL_H__ */
