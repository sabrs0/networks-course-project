#include "../inc/threadpool.h"
//CONN_STUFF
void free_conn_info(Conn_info *conn_info){
    free(conn_info);
}
Conn_info *new_conn_info(Clients_info *clients_info, int cur_client,int cur_client_ind){
    Conn_info *conn_info = malloc(sizeof(Conn_info));
    if (conn_info == NULL){
        log_message(ERROR_LEVEL, "new conn info (malloc)\n");//printf("new conn info (malloc)\n");
        return NULL;
    }
    conn_info->clients_info = clients_info;
    conn_info->cur_client = cur_client;
    conn_info->cur_client_ind = cur_client_ind;
    return conn_info;
}
//CLIENTS STUFF
void disconnect_client(Conn_info *conn_info){
    char log_buf[LOG_BUF];
    log_message(DEBUG_LEVEL, "disconnecting client №%d\n", conn_info->cur_client);
    pthread_rwlock_wrlock(&conn_info->clients_info->clients_RWmutex);
    close(conn_info->cur_client);
    FD_CLR(conn_info->cur_client, &(conn_info->clients_info->clients_fdset));
    conn_info->clients_info->clients_fd[conn_info->cur_client_ind] = -1;
    pthread_rwlock_unlock(&conn_info->clients_info->clients_RWmutex);
    free_conn_info(conn_info);
}
Clients_info *new_clients_info(int clients_cap){
    Clients_info *clients_info = malloc(sizeof(Clients_info));
    if (clients_info == NULL){
        log_message(ERROR_LEVEL, "new clients info (malloc)\n");
        return NULL;
    }
    clients_info->clients_fd_cap = clients_cap;
    clients_info->clients_fd = malloc(sizeof(int) * clients_cap);
    if (clients_info->clients_fd == NULL){
        log_message(ERROR_LEVEL, "new clients info  fd(malloc)\n");
        free(clients_info);
        return NULL;
    }
    
    pthread_rwlock_init (&clients_info->clients_RWmutex, NULL);
    return clients_info;
}
void free_clients_info(Clients_info *clients_info){
    free(clients_info->clients_fd);
    pthread_rwlock_destroy (&clients_info->clients_RWmutex);
    free(clients_info);

}

void init_clients_info(Clients_info *clients_info, int listen_fd){
    for (int i = 0; i < clients_info->clients_fd_cap; i ++){
        clients_info->clients_fd[i] = -1;
    }
    clients_info->max_client_fd = listen_fd;
    clients_info->max_client_ind = -1;
    FD_ZERO(&clients_info->clients_fdset);
    FD_SET(listen_fd,  &clients_info->clients_fdset);
}
int add_new_client(Clients_info *clients_info, int newClient_fd ){
    for (int i = 0; i < clients_info->clients_fd_cap; i ++){
        if (clients_info->clients_fd[i] == -1){
            if (i > clients_info->max_client_ind){
                clients_info->max_client_ind = i;
            }
            if (newClient_fd > clients_info->max_client_fd){
                clients_info->max_client_fd = newClient_fd;
            }
            clients_info->clients_fd[i] = newClient_fd;
            FD_SET(newClient_fd, &clients_info->clients_fdset);
            //FD_SET(client_fd, &rset); ////????????
            break;
        }
        if (i == (clients_info->clients_fd_cap - 1)){
            log_message(ERROR_LEVEL,"too many clients\n");
            return -1;
        }
    }
    
    return 0;
}
void print_client_info(Clients_info *clients_info){
    /*printf("\nCLIENTS INFO:\n");
    printf("\tmax_fd = %d\n", clients_info->max_client_fd);
    printf("\tmax_fd index = %d\n", clients_info->max_client_ind);
    printf("\tmax_fd index = %d\n\t", clients_info->max_client_ind);*/
    for (int i = 0; i < clients_info->clients_fd_cap; i ++){
        printf("%d ", clients_info->clients_fd[i]);
    }
    printf("\n");
}
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
    log_message(DEBUG_LEVEL, "START DOING TASK %lx\n", pthread_self());
    while(pool->active){
        pthread_mutex_lock(&pool->mutex);
        while(pool->task_queue->task_queue_len == 0){
            log_message(DEBUG_LEVEL, "WAITING %lx\n", pthread_self());
            pthread_cond_wait(&pool->queue_ready, &pool->mutex);
            break;
        }
        //printf("NOT WAITING ANYMORE %lx\n", pthread_self());
        Task_node *task_node = pop_task_node(pool);
        if (task_node == NULL){
         //по идее значит, что мы вызвали деструктор пула
            //printf("NULL ESCAPE %lx\n", pthread_self());
            pthread_mutex_unlock(&pool->mutex);
            return NULL; //?????
        }
        Task_t task = task_node->task;
        pthread_mutex_unlock(&pool->mutex);
        //printf("before task by %lx\n", pthread_self());
        task.task_func(task.arg);
        log_message(DEBUG_LEVEL, "task finished %lx\n\n", pthread_self());
    }
    log_message(DEBUG_LEVEL, "INACTIVE %lx\n", pthread_self());
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
        log_message(ERROR_LEVEL, "tpool malloc (tpool constructor)\n");
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
        log_message(ERROR_LEVEL, "task queue malloc (tpool constructor)\n");
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
    log_message(DEBUG_LEVEL, "try broadcasting all threads to wake up\n");
    pthread_cond_broadcast(&pool->queue_ready);
    log_message(DEBUG_LEVEL, "broadcasted\n");
    for (int i = 0; i < THREAD_COUNT; i ++){
        int err = pthread_join(pool->threads[i], NULL);
        if (err != 0){
            //printf("threadpool(join)\n");
            return THREAD_JOIN_ERR;
        }
    }
    free_threadpool(pool);
    return 0;

}