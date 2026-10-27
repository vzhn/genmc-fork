/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* A preprocessed task with key destructors: a thread that returns and one
 * that calls pthread_exit each run the destructor of their non-NULL value,
 * once; a NULL value and a key without a destructor run nothing. The tail
 * ignored destructors. */
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
extern void pthread_exit (void *__retval) __attribute__ ((__noreturn__));
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
void reach_error() { __assert_fail("0", "pp-key-dtors.c", 3, __extension__ __PRETTY_FUNCTION__); }

pthread_key_t key, plain;
int calls;
int seen[3];

void dtor(void *v)
{
 __atomic_fetch_add(&calls, 1, 0);
 if (*(int *) v != 0)
  reach_error();
 *(int *) v = 1;
}

void *returns(void *arg)
{
 pthread_setspecific(key, arg);
 pthread_setspecific(plain, arg);
 return 0;
}

void *exits(void *arg)
{
 pthread_setspecific(key, arg);
 pthread_exit(0);
}

void *keeps_null(void *arg)
{
 pthread_setspecific(key, arg);
 pthread_setspecific(key, 0);
 return 0;
}

int main(void)
{
 pthread_t t1, t2, t3;

 pthread_key_create(&key, dtor);
 pthread_key_create(&plain, 0);
 pthread_create(&t1, 0, returns, &seen[0]);
 pthread_create(&t2, 0, exits, &seen[1]);
 pthread_create(&t3, 0, keeps_null, &seen[2]);
 pthread_join(t1, 0);
 pthread_join(t2, 0);
 pthread_join(t3, 0);
 if (calls != 2 || !seen[0] || !seen[1] || seen[2])
  reach_error();
 return 0;
}
