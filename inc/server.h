#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "threadpool.h"
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include "http.h"
#include <sys/stat.h>
#include <dirent.h>

#define CLIENTS_CAP FD_SETSIZE // может и больше
#define MAX_CONN SOMAXCONN  
#define PORT 8088
#define BUF_SIZE 4096


typedef struct{
    struct sockaddr_in address;
    int listenfd; // не в clients_fd

    ThreadPool *pool;
} Web_server;

Web_server *new_Web_server(in_addr_t address, in_port_t port);
void free_Web_server(Web_server *server); //close listenfd;


int listen_and_serve(Web_server *server, int clients_cap);
int accept_connection(Web_server *server, Clients_info *clients_info);
int stop_server();