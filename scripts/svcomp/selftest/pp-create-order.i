/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* create-order.c as a preprocessed task: the tail's pthread_create must also
 * store the ID before the thread runs */
typedef unsigned long int pthread_t;
typedef unsigned int pthread_key_t;
union pthread_attr_t { char __size[36]; long int __align; };
typedef union pthread_attr_t pthread_attr_t;
typedef union { char __size[24]; long int __align; } pthread_mutex_t;
typedef union { char __size[4]; int __align; } pthread_mutexattr_t;
typedef union { char __size[32]; long int __align; } pthread_rwlock_t;
typedef union { char __size[8]; long int __align; } pthread_rwlockattr_t;
enum
{
  PTHREAD_MUTEX_TIMED_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_NORMAL = PTHREAD_MUTEX_TIMED_NP,
  PTHREAD_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE_NP,
};

extern void __assert_fail(const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern int pthread_create (pthread_t *__restrict __newthread,
      const pthread_attr_t *__restrict __attr,
      void *(*__start_routine) (void *),
      void *__restrict __arg) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 3)));
extern int pthread_join (pthread_t __th, void **__thread_return);
extern int pthread_mutex_init (pthread_mutex_t *__mutex,
      const pthread_mutexattr_t *__mutexattr) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_mutex_lock (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_mutex_trylock (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_mutex_unlock (pthread_mutex_t *__mutex) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_mutexattr_init (pthread_mutexattr_t *__attr) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_rwlock_init (pthread_rwlock_t *__restrict __rwlock,
      const pthread_rwlockattr_t *__restrict __attr) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_rwlock_rdlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_rwlock_wrlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_rwlock_unlock (pthread_rwlock_t *__rwlock) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern int pthread_key_create (pthread_key_t *__key,
      void (*__destr_function) (void *)) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern void *pthread_getspecific (pthread_key_t __key) __attribute__ ((__nothrow__));
extern int pthread_setspecific (pthread_key_t __key, const void *__pointer) __attribute__ ((__nothrow__));
pthread_t tids[1];
int started;

void *worker(void *arg)
{
 __atomic_store_n(&started, 1, 3);
 return 0;
}

void *joiner(void *arg)
{
 while (!__atomic_load_n(&started, 2))
  ;
 pthread_join(tids[0], 0);
 return 0;
}

int main(void)
{
 pthread_t j;

 pthread_create(&j, 0, joiner, 0);
 pthread_create(&tids[0], 0, worker, 0);
 pthread_join(j, 0);
 return 0;
}
