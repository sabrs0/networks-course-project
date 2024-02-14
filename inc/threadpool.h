#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "logger.h"
#define THREAD_COUNT 1024

#define MALLOC_ERR 1
#define THREAD_JOIN_ERR 2
#define OK 0

typedef struct {
    void* (*task_func)(void *);
    void *arg;
} Task_t;

typedef struct{
    int *clients_fd;
    //int *clients_handled_fd;
    int clients_fd_cap;
    fd_set clients_fdset;
    int max_client_ind;
    int max_client_fd;
    pthread_rwlock_t clients_RWmutex;

} Clients_info;


typedef struct{
    Clients_info *clients_info;
    int cur_client;
    int cur_client_ind;

} Conn_info;

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

    //Clients_info *clients_info;
} ThreadPool;

void free_conn_info(Conn_info *conn_info);
Conn_info *new_conn_info(Clients_info *clients_info, int cur_client,int cur_client_ind);

Clients_info *new_clients_info(int clients_cap);
void free_clients_info(Clients_info *clients_info);
void init_clients_info(Clients_info *clients_info, int listen_fd);
int add_new_client(Clients_info *clients_info, int newClient_fd);
void disconnect_client(Conn_info *conn_info);
void print_client_info(Clients_info *clients_info);

Task_t new_task(void *func(void *), void *arg);
int add_task(ThreadPool *pool, Task_t task);
Task_node *pop_task_node(ThreadPool *pool);
void free_task_queue(ThreadPool *pool);

ThreadPool *new_threadPool();
int threadPool_Destructor(ThreadPool *pool);
void *do_task(void *arg);



// очередь задач пусть будет - туда будет закидывать коннекты