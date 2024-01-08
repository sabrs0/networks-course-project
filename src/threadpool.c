#include "../inc/threadpool.h"
// QUEUE STUFF
Task_t new_task(void *func(void *), void *arg){
    Task_t task;
    task.arg = arg;
    task.task_func = func;
    return task;
}
int add_task(ThreadPool *pool, Task_t task){
    pthread_mutex_lock(&pool->mutex);
    Task_node *new_task = malloc(sizeof(Task_node));
    if (new_task == NULL){
        pthread_mutex_unlock(&pool->mutex);
        return MALLOC_ERR;
    }
    new_task->task = task;
    new_task->next_task = NULL;
    if (pool->task_queue->q_head == NULL){
        pool->task_queue->q_tail = new_task;
        pool->task_queue->q_head = new_task;
    }else{
        pool->task_queue->q_tail->next_task = new_task;
        pool->task_queue->q_tail = new_task;
    }
    pool->task_queue->task_queue_len ++;
    pthread_mutex_unlock(&pool->mutex);
    pthread_cond_signal(&pool->queue_ready);
    return OK;
}

Task_node *pop_task_node(ThreadPool *pool){
    //pthread_mutex_lock(&pool->mutex);
    Task_node *first_task = pool->task_queue->q_head;
    if (first_task == NULL){
        //pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }
    pool->task_queue->q_head = pool->task_queue->q_head->next_task;
    pool->task_queue->task_queue_len --;
    //pthread_mutex_unlock(&pool->mutex);
    return first_task;
}
void free_task_queue(ThreadPool *pool){
    Task_node *cur_elem = pool->task_queue->q_head;
    if (cur_elem == NULL){
        pool->task_queue->q_tail = NULL;
        free(pool->task_queue);
        return;
    }
    while(cur_elem != NULL){
        Task_node *next_elem = cur_elem->next_task;
        free(cur_elem);
        cur_elem = next_elem;
    }
    pool->task_queue->q_head = NULL;
    pool->task_queue->q_tail = NULL;
    free(pool->task_queue);
}

Task_Queue * new_task_queue(){
    Task_Queue * task_queue = malloc(sizeof(Task_Queue));
    if (task_queue == NULL){
        return NULL;
    }
    task_queue->task_queue_len = 0;
    task_queue->q_head = NULL;
    task_queue->q_tail = NULL;
    return task_queue;
}

// THREADPOOL STUFF
void *do_task(void *arg){
    ThreadPool *pool = (ThreadPool *)arg;
    printf("START DOING TASK %lx\n", pthread_self());
    while(pool->active){
        pthread_mutex_lock(&pool->mutex);
        while(pool->task_queue->task_queue_len == 0){
            //printf("WAITING %lx\n", pthread_self());
            pthread_cond_wait(&pool->queue_ready, &pool->mutex);
            break;
        }
        //printf("NOT WAITING ANYMORE %lx\n", pthread_self());
        Task_node *task_node = pop_task_node(pool);
        if (task_node == NULL){
         //по идее значит, что мы вызвали деструктор пула
            printf("NULL ESCAPE %lx\n", pthread_self());
            pthread_mutex_unlock(&pool->mutex);
            return NULL; //?????
        }
        Task_t task = task_node->task;
        pthread_mutex_unlock(&pool->mutex);
        //printf("before task by %lx\n", pthread_self());
        task.task_func(task.arg);
        printf("task finished %lx\n\n", pthread_self());
    }
    printf("INACTIVE %lx\n", pthread_self());
    return NULL;
}
void free_threadpool(ThreadPool *pool){
    free_task_queue(pool);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->queue_ready);
    free(pool);
}
ThreadPool *new_threadPool(){
    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (pool == NULL){
        printf("tpool malloc (tpool constructor)\n");
        return NULL;
    }
    if (pthread_mutex_init(&pool->mutex, NULL) != 0){
        //не изменяют errno

        return NULL;
    }
    if (pthread_cond_init(&pool->queue_ready, NULL) != 0){
        //не изменяют errno
        return NULL;
    }

    pool->task_queue = new_task_queue();
    if (pool->task_queue == NULL){
        printf("task queue malloc (tpool constructor)\n");
        pthread_cond_destroy(&pool->queue_ready);
        pthread_mutex_destroy(&pool->mutex);
        free(pool);
        return NULL;
    }
    pool->active = 1;
    pthread_mutex_lock(&pool->mutex);
    for(int i = 0; i < THREAD_COUNT; i ++){
        int err = pthread_create(&pool->threads[i], NULL, do_task, pool);
        if (err != 0){
            //
            free_threadpool(pool);
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
    }
    
    pthread_mutex_unlock(&pool->mutex);
    return pool;
}

int threadPool_Destructor(ThreadPool *pool){
    pool->active = 0;
    printf("try broadcasting all threads to wake up\n");
    pthread_cond_broadcast(&pool->queue_ready);
    printf("broadcasted\n");
    for (int i = 0; i < THREAD_COUNT; i ++){
        int err = pthread_join(pool->threads[i], NULL);
        if (err != 0){
            
            //freeeee
            //опасная ошибка
            printf("threadpool(join)\n");
            return THREAD_JOIN_ERR;
        }
    }
    free_threadpool(pool);
    return 0;

}