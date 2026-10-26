/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* A preprocessed task in which main writes x right after creating the
 * thread that writes it too: a race. GenMC finds it as main's store is
 * added, and prints that store without a name or location; it stores the
 * new thread's ID (2), like pthread_create's own store, but it is the
 * task's and must not be labelled CREATE-RACE */
typedef unsigned long int pthread_t;
union pthread_attr_t { char __size[36]; long int __align; };
typedef union pthread_attr_t pthread_attr_t;
extern int pthread_create (pthread_t *__restrict __newthread,
      const pthread_attr_t *__restrict __attr,
      void *(*__start_routine) (void *),
      void *__restrict __arg) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 3)));
extern int pthread_join (pthread_t __th, void **__thread_return);

int x;

void *worker(void *arg)
{
 x = 1;
 return 0;
}

int main(void)
{
 pthread_t t;

 pthread_create(&t, 0, worker, 0);
 x = 2;
 pthread_join(t, 0);
 return 0;
}
