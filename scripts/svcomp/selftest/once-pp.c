/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* once.c as a preprocessed task, which declares the pthread API itself:
 * svcomp-run.py runs it with svcomp-shim-pp.h and defines pthread_create,
 * pthread_join, pthread_once and __assert_fail in a generated tail */
typedef unsigned long int pthread_t;

union pthread_attr_t
{
  char __size[36];
  long int __align;
};
typedef union pthread_attr_t pthread_attr_t;
typedef int pthread_once_t;

extern void __assert_fail(const char *__assertion, const char *__file,
      unsigned int __line, const char *__function)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "once-pp.c", 21, __extension__ __PRETTY_FUNCTION__); }
extern int pthread_create (pthread_t *__restrict __newthread,
      const pthread_attr_t *__restrict __attr,
      void *(*__start_routine) (void *),
      void *__restrict __arg) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 3)));
extern int pthread_join (pthread_t __th, void **__thread_return);
extern int pthread_once (pthread_once_t *__once_control,
      void (*__init_routine) (void)) __attribute__ ((__nonnull__ (1, 2)));

pthread_once_t once = 0;
int inits, data;

void init(void)
{
	inits++;
	data = 42;
}

void *worker(void *arg)
{
	pthread_once(&once, init);
	if (data != 42)
		reach_error();
	return 0;
}

int main(void)
{
	pthread_t t[2];

	for (int i = 0; i < 2; i++)
		pthread_create(&t[i], 0, worker, 0);
	pthread_once(&once, init);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], 0);
	if (inits != 1)
		reach_error();
	return 0;
}
