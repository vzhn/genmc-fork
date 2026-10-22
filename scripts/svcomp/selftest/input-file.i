/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* input-file.c, preprocessed: the task's input file. svcomp-run.py must run
 * this, not the .c, which does not build. reach_error() is reachable: the
 * increments are an atomic load and store each, so one can be lost */
typedef unsigned long int pthread_t;

union pthread_attr_t
{
  char __size[36];
  long int __align;
};
typedef union pthread_attr_t pthread_attr_t;

extern void __assert_fail(const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "input-file.c", 3, __extension__ __PRETTY_FUNCTION__); }
extern int pthread_create (pthread_t *__restrict __newthread,
      const pthread_attr_t *__restrict __attr,
      void *(*__start_routine) (void *),
      void *__restrict __arg) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 3)));
extern int pthread_join (pthread_t __th, void **__thread_return);

int x;

void *worker(void *arg)
{
 __atomic_store_n(&x, __atomic_load_n(&x, 5) + 1, 5);
 return 0;
}

int main(void)
{
 pthread_t t;

 pthread_create(&t, 0, worker, 0);
 __atomic_store_n(&x, __atomic_load_n(&x, 5) + 1, 5);
 pthread_join(t, 0);
 if (__atomic_load_n(&x, 5) != 2)
  reach_error();
 return 0;
}
