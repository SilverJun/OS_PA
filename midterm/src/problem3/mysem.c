/*TODO*/

#include <pthread.h>

typedef struct {
	pthread_mutex_t lock;
	int max_count;
	int curr_count;
	pthread_cond_t post;
} mysem_t ;


void
mysem_init (mysem_t * sem, int init)
{
	//TODO
	pthread_mutex_init(&sem->lock, 0x0);
	sem->max_count = init;
	sem->curr_count = 0;
}

void
mysem_post (mysem_t * sem)
{
	//TODO
	pthread_mutex_lock(&sem->lock);
	sem->curr_count--;
	pthread_mutex_unlock(&sem->lock);
	pthread_cond_signal(&sem->post);
}

void
mysem_wait (mysem_t * sem)
{
	//TODO
	pthread_mutex_lock(&sem->lock);

	while (sem->curr_count >= sem->max_count)
	{
		pthread_cond_wait(&sem->post, &sem->lock);
	}
	sem->curr_count++;
	pthread_mutex_unlock(&sem->lock);
}
