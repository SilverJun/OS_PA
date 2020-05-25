#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#include <limits.h>
#include <string.h>


/// tsp
int** m; // adj mat
int N = 0; // tsp NxN행렬

/// mutex lock이 필요한 변수들.
pthread_mutex_t min_path_update; // min path를 업데이트하기 위한 뮤텍스
int* min_path;
int min_dist = INT_MAX;
unsigned long long checked_count = 0;
///


typedef struct {
    // Here thread info which need to be lock below.
    pthread_t tid;
    pthread_mutex_t lock;
    unsigned int subtask_count; // 현재 쓰레드에서 진행 완료한 서브 테스크 수
    unsigned long long checked_count; // 현재 서브테스크에서 확인한 route 수
} consumer_t;

///
//pthread_mutex_t consumer_update; // 메인에서 컨슈머 쓰레드 갯수를 지울 때 그것을 위한 뮤텍스 락
pthread_t producer_tid;
consumer_t consumer_arr[8]; // maximum 8 threads.
int thread_count = 0; // provided consumer thread count.
///

typedef struct {
    int* path;
    int* checked;
    int dist; // need to init by 0.

    consumer_t* owner;
} task_t;

typedef struct {
	sem_t filled;
	sem_t empty;
	pthread_mutex_t lock;
	task_t** item; //1d array
	int capacity;
	int num; 
	int front;
	int rear;
} bounded_buffer;

bounded_buffer* task_queue = NULL;

void bounded_buffer_init(bounded_buffer * buf, int capacity) {
	sem_init(&(buf->filled), 0, 0);
	sem_init(&(buf->empty), 0, capacity);
	pthread_mutex_init(&(buf->lock), NULL);
	buf->capacity = capacity;
	buf->item = (task_t**)calloc(capacity, sizeof(task_t*));
	buf->num = 0;
	buf->front = 0;
	buf->rear = 0;
}

void bounded_buffer_queue(bounded_buffer * buf, task_t* t) {
	sem_wait(&(buf->empty));
	pthread_mutex_lock(&(buf->lock));
		buf->item[buf->rear] = t;
		buf->rear = (buf->rear + 1) % buf->capacity;
		buf->num += 1;
        // printf("current queue size is: %d\n", buf->num);
	pthread_mutex_unlock(&(buf->lock));
	sem_post(&(buf->filled));
}

task_t* bounded_buffer_dequeue(bounded_buffer* buf) {
	task_t* r = NULL;
	sem_wait(&(buf->filled));
	pthread_mutex_lock(&(buf->lock));
		r = buf->item[buf->front];
		buf->front = (buf->front + 1) % buf->capacity;
		buf->num -= 1;
        // printf("current queue size is: %d\n", buf->num);
	pthread_mutex_unlock(&(buf->lock));
	sem_post(&(buf->empty));
	return r;
}

task_t* create_task() {
    task_t* task = (task_t*)malloc(sizeof(task_t));
    task->checked = (int*)malloc(sizeof(int)*N);
    memset(task->checked, 0, sizeof(int)*N);
    task->path = (int*)malloc(sizeof(int)*N);
    memset(task->path, 0, sizeof(int)*N);
    task->dist = 0;
    task->owner = NULL;
    return task;
}

void task_release(task_t* task) {
    free(task->path);
    free(task->checked);
    free(task);
}

/**
 * \brief Copy src task data to dst task data.
 * dst should call task_init before task_copy!
 */
void task_copy(task_t* dst, const task_t* src) {
    memcpy(dst->path, src->path, N*sizeof(int));
    memcpy(dst->checked, src->checked, N*sizeof(int));
    dst->dist = src->dist;
    dst->owner = src->owner;
}

// consumer_t* find_consumer(pthread_t tid) {
//     pthread_mutex_lock(&consumer_update);
//     for (int i = 0; i < thread_count; i++)
//     {
//         if (consumer_arr[i].tid == tid) {
//             pthread_mutex_unlock(&consumer_update);
//             return &consumer_arr[i];
//         }
//     }
//     pthread_mutex_unlock(&consumer_update);
//     return NULL;
// }

// visit city]
void tsp(task_t* task, int i) {
    //printf("visit %d\n", i);
    if (i == N) {
        //printf("this thread id: %ld \n", task->owner->tid);
        pthread_mutex_lock(&(task->owner->lock));
        task->owner->checked_count++;
        pthread_mutex_unlock(&(task->owner->lock));

        task->dist += m[task->path[i-1]][0];
    
        // update data
        pthread_mutex_lock(&min_path_update);
        checked_count++;
        if (task->dist < min_dist) {
            min_dist = task->dist;
            //printf("Consumer find new shortest path! dist:%d, path:{", min_dist);
            for (int k = 0; k < N; k++) {
                min_path[k] = task->path[k];
                //printf("%d, ", min_path[k]);
            }
            //printf("}\n");
        }
        pthread_mutex_unlock(&min_path_update);
        //
        task->dist -= m[task->path[i-1]][0];
        return;
    }
    for (int j = 0; j < N; j++) {
        //printf("visit j: %d\n", j);
        //printf("checked[j]: %d", checked[j]);
        if (task->checked[j]==1) continue;
        task->checked[j] = 1;
        task->path[i] = j;

        task->dist += m[task->path[i-1]][j];

        if (N - 11 == i+1) { // here to add to bounded buffer
            printf("Producer> try to enqueue\n");
            task_t* copy = create_task();
            task_copy(copy, task);

            bounded_buffer_queue(task_queue, copy);
            printf("Producer> enqueue done\n");
        }
        else {
            tsp(task, i+1);
        }

        task->checked[j] = 0;
        task->dist -= m[task->path[i-1]][j];
    }
}

void print_result() {
    pthread_mutex_lock(&min_path_update);
    printf("\nThe best route: <");
    for (int i = 0; i < N; i++) {
        printf("%d ", min_path[i]);
    }
    printf("0>\n");
    
    printf("The best minimum distance is %d\n", min_dist);
    printf("Total checked count is %llu\n", checked_count);
    pthread_mutex_unlock(&min_path_update);
}

void* Producer(void* param) {
    task_t* t = create_task();
    // go!
    t->path[0] = 0;
    t->checked[0] = 1;
    tsp(t, 1);
    t->checked[0] = 0;

    printf("Producer> all tasks are spawned\n");
    // while (active != 0) usleep(100); // wait until finish all child processes.
}

void* Consumer(void* param) {
    consumer_t* data = (consumer_t*)param;

    pthread_mutex_init(&data->lock, 0x0);
    data->subtask_count = 0;
    data->checked_count = 0;

    printf("Consumer[%ld]> start consumer\n", data->tid);
    while (1) { // 모든 작업이 끝날 때 까지.
        // bounded buffer dequeue.
        task_t* t = bounded_buffer_dequeue(task_queue);
        printf("Consumer[%ld]> now got one task!\n", data->tid);
        t->owner = data;
        tsp(t, N - 11); // 11! 이 되는 지점부터.
        printf("Consumer[%ld]> done task!\n", data->tid);
        task_release(t);

        pthread_mutex_lock(&data->lock);
        data->subtask_count++;
        pthread_mutex_unlock(&data->lock);
    }
}

void init(char* filename) { // memory allocation and initialization value.
    FILE* fp = fopen(filename, "r");
    int temp = 0;
    
    min_path = (int*)malloc(sizeof(int)*N);
    memset(min_path, 0, sizeof(int)*N);

    m = (int**)malloc(sizeof(int*)*N);
    for (int i = 0; i < N; i++) {
        m[i] = (int*)malloc(sizeof(int)*N);
		for (int j = 0; j < N; j++) {
			fscanf(fp, "%d", &temp);
			m[i][j] = temp;
		}
	}
	fclose(fp);

    // mutex initialization 
    pthread_mutex_init(&min_path_update, 0x0);
    task_queue = (bounded_buffer*)malloc(sizeof(bounded_buffer));
    bounded_buffer_init(task_queue, thread_count);
}

void release() {
    for (int i = 0; i < N; i++) {
        free(m[i]);
    }
    free(m);
    free(min_path);
}

int main(int argc, char* argv[]) {
    sscanf(argv[2], "%d", &thread_count);
	FILE* fp = fopen(argv[1], "r");
    //sscanf(argv[1], "gr%d.tsp", &len); // this is some trick

    if (!fp) {
        printf("file open error\n");
        exit(1);
    }
    
    while (!feof(fp)) { // check len
        char buf[256]={0};
        fgets(buf, 256, fp);
        if (strlen(buf)) N++;
    }
    fclose(fp);

    init(argv[1]);

    printf("main> creating producer thread\n");
    // spawn producer
    pthread_create(&producer_tid, 0x0, Producer, 0x0);
    //pthread_detach(producer_tid);

    printf("main> creating consumer thread\n");
    // spawn consumer
    for (int i = 0; i < thread_count; i++)
    {
        pthread_create(&consumer_arr[i].tid, 0x0, Consumer, (void*)&consumer_arr[i]);
        //pthread_detach(consumer_arr[i].tid);
    }

    pthread_join(producer_tid, 0x0);
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(consumer_arr[i].tid, 0x0);
    }
    
    // while end
    // cui 처리.

    // while (1) // TODO : 종료조건(프로듀서에서 모든 테스크를 큐에 넣음 + 큐가 empty가 됨)
    // {
    //     char cmd[128] = {0};
    //     printf("> ");
    //     scanf("%s", cmd);

    //     // parse command
        

    // }
    

    return 0;
}