#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "threadpool.h"
#include <sys/select.h>
#define CLIENTS_CAP FD_SETSIZE // может и больше
#define MAX_CONN SOMAXCONN  

typedef struct{
    struct sockaddr_in address;
    int listenfd; // не в clients_fd

    int *clients_fd;
    int clients_cap;
    int max_client_ind;
    int max_fd;

    fd_set allset;
    int nready;

    ThreadPool *pool;

    
} Http_Server;

Http_Server *new_http_server(in_addr_t address, in_port_t port, int clients_len);
void free_http_server(Http_Server *server); //close listenfd;


int listen_and_serve(Http_Server *server);
int accept_connection(Http_Server *server);
int stop_server();