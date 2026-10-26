/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* A preprocessed task that declares pthread_t as int, as the
 * pthread-driver-races tasks do, and races on x */
typedef int pthread_t;
typedef int pthread_attr_t;
int pthread_create(pthread_t* thread,const pthread_attr_t* attr,
                   void *(*start_routine)(void*), void* arg);
int pthread_join(pthread_t thread, void **value_ptr);

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
