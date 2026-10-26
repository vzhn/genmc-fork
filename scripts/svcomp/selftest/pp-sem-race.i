/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* A preprocessed task that calls sleep, strcpy, ffs and POSIX semaphores,
 * which GenMC does not provide: the generated tail defines them. A
 * semaphore of 2 guards the counter, so the two workers race on it. */
typedef unsigned long int pthread_t;
union pthread_attr_t { char __size[36]; long int __align; };
typedef union pthread_attr_t pthread_attr_t;
typedef union { char __size[16]; long int __align; } sem_t;
typedef unsigned int size_t;

extern void __assert_fail(const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
extern int pthread_create (pthread_t *__restrict __newthread,
      const pthread_attr_t *__restrict __attr,
      void *(*__start_routine) (void *),
      void *__restrict __arg) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 3)));
extern int pthread_join (pthread_t __th, void **__thread_return);
extern int sem_init (sem_t *__sem, int __pshared, unsigned int __value) __attribute__ ((__nothrow__ , __leaf__));
extern int sem_wait (sem_t *__sem);
extern int sem_post (sem_t *__sem) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned int sleep (unsigned int __seconds);
extern char *strcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int ffs (int __i) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
void reach_error() { __assert_fail("0", "pp-sem-race.c", 3, __extension__ __PRETTY_FUNCTION__); }

sem_t sem;
int counter;
char name[8];

void *worker(void *arg)
{
 sleep(1);
 sem_wait(&sem);
 counter++;
 sem_post(&sem);
 return 0;
}

int main(void)
{
 pthread_t t1, t2;

 strcpy(name, "genmc");
 if (name[4] != 'c' || name[5] != 0)
  reach_error();
 if (ffs(0) != 0 || ffs(1) != 1 || ffs(12) != 3)
  reach_error();
 sem_init(&sem, 0, 2);
 pthread_create(&t1, 0, worker, 0);
 pthread_create(&t2, 0, worker, 0);
 pthread_join(t1, 0);
 pthread_join(t2, 0);
 if (counter != 2)
  reach_error();
 return 0;
}
