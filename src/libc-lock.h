#ifndef LIBC_LOCK_H
#define LIBC_LOCK_H

#include <pthread.h>

typedef pthread_mutex_t __libc_lock_t;

#if __LT_SPINLOCK_INIT == 0
#define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS pthread_mutex_t NAME;
#else
#define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS pthread_mutex_t NAME = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Lock the named lock variable.  */
#define __libc_lock_lock(NAME) pthread_mutex_lock (&(NAME));

/* Unlock the named lock variable.  */
#define __libc_lock_unlock(NAME) pthread_mutex_unlock (&(NAME));

#else
#error included from wrong place
#endif	/* bits/libc-lock.h */
