#include "../inc/server.h"
void *handle_connection(void *arg){

}
void free_http_server(Http_Server *server){
    for (int i = 0; i < server->clients_cap; i ++){
        if (server->clients_fd[i] != -1){
            close(server->clients_fd[i]);
        }
    }
    close(server->listenfd);
    threadPool_Destructor(server->pool);
    free(server->clients_fd);
    free(server);
}

Http_Server *new_http_server(in_addr_t address, in_port_t port, int clients_cap){
    Http_Server *server = malloc(sizeof(Http_Server));
    if (server == NULL){
        printf("new server (malloc)\n");
        return NULL;
    }
    server->address.sin_addr.s_addr = htonl(address);
    server->address.sin_port = htons(port);
    server->address.sin_family = AF_INET;

    server->pool = new_threadPool();
    if (server->pool == NULL){
        free(server);
        printf("new server (pool)\n");
        return NULL;
    }
    
    server->max_client_ind = -1;

    server->clients_cap = clients_cap;
    server->clients_fd = malloc(sizeof(int) * clients_cap);
    if (server->clients_fd == NULL){
        threadPool_Destructor(server->pool); //may cause the error
        free(server);
        printf("new server malloc(client)\n");
        return NULL;
    }
    for (int i = 0; i < server->clients_cap; i ++){
        server->clients_fd[i] = -1;
    }


    server->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listenfd == -1){
        threadPool_Destructor(server->pool); //may cause the error
        free(server->clients_fd);
        free(server);
    }

    int err = bind(server->listenfd, (struct sockaddr *)&server->address, sizeof(server->address));
    if (err != 0){
        free_http_server(server);
        return NULL;
    }
    return server;    

}

int listen_and_serve(Http_Server *server){
    int err = listen(server->listenfd, MAX_CONN);
    if (err != 0){
        printf("listen_and_serve (listen)\n");
        return -1;
    }
    server->max_fd = server->listenfd;
    FD_ZERO(&server->allset);
    FD_SET(server->listenfd,  &server->allset);

}

int accept_connection(Http_Server *server){
    fd_set rset = server->allset;

    int nready = select(server->max_fd + 1, &rset, NULL, NULL, NULL);

    if (FD_ISSET(server->listenfd, &rset)){
        int client_fd = accept(server->listenfd, NULL, NULL);
        if (client_fd  < 0){
            printf("server (accept)\n");
            //continue;
            return -1;
        }
        for (int i = 0; i < server->clients_cap; i ++){
            if (server->clients_fd[i] == -1){
                if (i > server->max_client_ind){
                    server->max_client_ind = i;
                }
                if (i > server->max_fd){
                    server->max_fd = i;
                }
                server->clients_fd[i] = client_fd;
                FD_SET(client_fd, &server->allset);
                //FD_SET(client_fd, &rset); ////????????
                break;
            }
            if (i == (server->clients_cap - 1)){
                printf("too many clients\n");
                return -1;
                /*server->clients_cap += CLIENTS_CAP;
                server->clients_fd = realloc(server->clients_fd, sizeof(int) * server->clients_cap);
                if (server->clients_fd == NULL){
                    printf("accept connection (realloc)\n");
                    return -1;
                }*/
            }
        }
        if (--nready <= 0){
            printf("no ready for read descriptors\n");
            return 0;
        }
    }
    for (int i = 0; i < server->max_client_ind; i ++){
        if (server->clients_fd[i] == -1){
            continue;
        }
        if (FD_ISSET(server->clients_fd[i], &rset)){
            add_task(server->pool, new_task(handle_connection, (void *)1));//туть
        }
        if (--nready < 0){

        }
    }
}
