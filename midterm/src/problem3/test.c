#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
	pthread_mutex_t lock;
	int max_count;
	int curr_count;
	pthread_cond_t post;
} mysem_t ;


void
mysem_init (mysem_t * sem, int init);

void
mysem_post (mysem_t * sem);

void
mysem_wait (mysem_t * sem);

mysem_t sem;

void* test(void* ptr) {
    printf("thread sem wait!\n");
    mysem_wait(&sem);
    printf("thread sem wait done!\n");
    mysem_post(&sem);
}

int main() {
    mysem_init(&sem, 1);

    pthread_t th1, th2, th3;
    pthread_create(&th1, 0x0, test, 0x0);
    pthread_create(&th2, 0x0, test, 0x0);
    pthread_create(&th3, 0x0, test, 0x0);

    printf("main thread post\n");

    pthread_join(th1, 0x0);
    pthread_join(th2, 0x0);
    pthread_join(th3, 0x0);

    printf("done.\n");

    return 0;
}