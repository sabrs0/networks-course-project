#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#define THREAD_COUNT 4

#define MALLOC_ERR 1
#define THREAD_JOIN_ERR 2
#define OK 0

typedef struct {
    void* (*task_func)(void *);
    void *arg;
} Task_t;
typedef struct node{
    Task_t task;
    struct node *next_task;
} Task_node;
typedef struct{
    Task_node *q_tail ;
    Task_node * q_head;
    int task_queue_len;
} Task_Queue;


typedef struct {
    pthread_t threads[THREAD_COUNT];
    
    pthread_mutex_t mutex;
    pthread_cond_t queue_ready;
    
    Task_Queue *task_queue;

    int active;
} ThreadPool;
Task_t new_task(void *func(void *), void *arg);
int add_task(ThreadPool *pool, Task_t task);
Task_node *pop_task_node(ThreadPool *pool);
void free_task_queue(ThreadPool *pool);

ThreadPool *new_threadPool();
int threadPool_Destructor(ThreadPool *pool);
void *do_task(void *arg);



// очередь задач пусть будет - туда будет закидывать коннекты