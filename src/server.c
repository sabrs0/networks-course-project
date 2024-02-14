#include "../inc/server.h"
int server_active;

const char resp[] = "HTTP/1.1 200 OK\r\n"
"Server: webserver-c\r\n"
"Content-type: text/html\r\n"
"Content-length: 18\r\n"
"Connection: Keep-Alive\r\n\r\n"
"<html>hello, world</html>\r\n";

int send_dir(int client_fd, char *fname, char *url){
    DIR *fd = opendir(fname);
    if (fd == NULL){
        log_message(WARNING_LEVEL,"send dir (opendir)\n");
        return -1;
    }
    struct dirent *dir;
    Http_response resp;
    strcpy(resp.code, "200");
    strcpy(resp.version, "HTTP/1.1");
    strcpy(resp.status, "OK");
    resp.headers[0] = '\0';
    add_header(resp.headers, "text/html");
    char resp_buf[RESPONSE_LEN];
    response_to_string(resp_buf, resp);
    int err = write(client_fd, resp_buf, strlen(resp_buf));
    if (err <= 0){
        log_message(WARNING_LEVEL, "send dir (write)\n");
        closedir(fd);
        return -1;
    }
    char body_buf[BUF_SIZE];
    sprintf(body_buf, "<html><head><title>Dir %s</title></head><body><h1>Dir %s</h1><ul>", url, url);
    while((dir = readdir(fd)) != NULL){
        if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0)){
            continue;
        }
        
        char full_fname[FILENAME_MAX];
        sprintf(full_fname, "%s/%s", fname, dir->d_name);
        struct stat f_stat;
        if (stat(full_fname, &f_stat) == 0 && S_ISDIR(f_stat.st_mode)){
            sprintf(body_buf + strlen(body_buf), "<li><a href=\"%s%s/\">%s/</a></li>", url, dir->d_name, dir->d_name);
        }else{
            sprintf(body_buf + strlen(body_buf), "<li><a href=\"%s%s\">%s</a></li>", url, dir->d_name, dir->d_name);
        }
    }
    closedir(fd);
    sprintf(body_buf + strlen(body_buf), "</ul></body></html>");
    err = write(client_fd, body_buf, strlen(body_buf));
    if (err <= 0){
        log_message(WARNING_LEVEL, "send dir (write)\n");
        return -1;
    }
    return 0;

}

long get_file_size(char *filename){
    struct stat f_stat;
    if (stat(filename, &f_stat) < 0){
        return -1;
    }
    
    return f_stat.st_size;
}


int send_file(int client_fd, char *fname){
    Http_response resp;
    strcpy(resp.code, "200");
    strcpy(resp.version, "HTTP/1.1");
    strcpy(resp.status, "OK");
    resp.headers[0] = '\0';
    
    long file_size = get_file_size(fname);
    
    char content_type[128];
    strcpy(content_type, get_content_type(fname));
    if (strcmp(content_type, "") == 0){
        log_message(WARNING_LEVEL, "content type trouble\n");
        add_header(resp.headers, "text/html");
        
        char resp_buf[RESPONSE_LEN];
        response_to_string(resp_buf, resp);
        write(client_fd, resp_buf, strlen(resp_buf));
        /*if (err <= 0){
            log_message(DEBUG_LEVEL, "send dir (write)\n");
            return -1;
        }*/
        char *body_buf = "<html>Unknown content-type</html>\r\n";
        write(client_fd, body_buf, strlen(body_buf));
        /*if (err <= 0){
            printf("send dir (write)\n");
            return -1;
        }*/
    }
    add_header(resp.headers, content_type);
    char resp_buf[RESPONSE_LEN];
    response_to_string(resp_buf, resp);
    int err = write(client_fd, resp_buf, strlen(resp_buf));
    if (err <= 0){
            log_message(WARNING_LEVEL,"send dir (write)\n");
            return -1;
    }
    FILE *f = fopen(fname, "r");
    if (f == NULL){
        char err_str[128];
        resp_http_404(err_str);
        err = write(client_fd, err_str, strlen(err_str));
        if (err <= 0){
            log_message(WARNING_LEVEL,"send dir (write)\n");
            return -1;
        }
    }
    char body_buf[BUF_SIZE];
    int bytes_read;
    while((bytes_read = fread(body_buf, 1, BUF_SIZE, f)) > 0){
        err = write(client_fd, body_buf, bytes_read);
        if (err <= 0){
            log_message(WARNING_LEVEL,"send dir (write)\n");
            return -1;
        }
    }
    fclose(f);
    return 0;

}


int handle_connection(void *arg){
    Conn_info *conn_info = (Conn_info *)arg;

    char buf[BUF_SIZE];
    log_message(DEBUG_LEVEL,"\t BEFORE READ from %d\n", conn_info->cur_client);
    int err = read(conn_info->cur_client, buf, BUF_SIZE);
    if (err <= 0){
        log_message(WARNING_LEVEL,"READ NOTHING of %d (handler)\n", conn_info->cur_client);
        disconnect_client(conn_info);
        //free(sockfd);
        return -1;
    }
    log_message(DEBUG_LEVEL,"\t AFTER READ from %d: \n%s\n", conn_info->cur_client, buf);
    
    Http_request req = parse_request(buf);
    char err_str[256];
    if (strcmp(req.method, "GET")  && strcmp(req.method, "HEAD")){
        log_message(INFO_LEVEL,"UNKNOWN REQUEST: %s is not GET\n", req.method);
        resp_http_405(err_str);
        err = write(conn_info->cur_client, err_str, strlen(err_str));
        if (err <= 0){
            log_message(WARNING_LEVEL,"WRITE ERROR (handler)\n");
            disconnect_client(conn_info);
            return 1;
        }
        disconnect_client(conn_info);
        return 0;    
    }
    char filepath[PATH_MAX];
    if (strcmp(req.url, "/") == 0){
        strcat(req.url, "index.html");
    }
    set_file_from_url(err_str, filepath, req.url);
    if (strcmp(err_str, "OK")){
        log_message(WARNING_LEVEL,"\t\t\tBAD URL\t\t\t\n");
        err = write(conn_info->cur_client, err_str, strlen(err_str));
        if (err <= 0){
            log_message(WARNING_LEVEL,"WRITE ERROR (handler)\n");
            disconnect_client(conn_info);
            return 1;
        }
        disconnect_client(conn_info);
        return 0;
    }
    
    struct stat path_stat;
    // если урл - директория
    if (stat(filepath, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        if (req.url[strlen(req.url) - 1] != '/'){ 
            strcat(req.url, "/"); 
        } 
        log_message(DEBUG_LEVEL,"BEFORE DIR SEND\n");
        err = send_dir(conn_info->cur_client, filepath, req.url);
        if (err <= 0){
            log_message(WARNING_LEVEL,"WRITE ERROR (handler)\n");
            disconnect_client(conn_info);
            return 1;
        }
        log_message(DEBUG_LEVEL,"AFTER DIR SEND\n");
        disconnect_client(conn_info);
        return 0;
    }
    log_message(DEBUG_LEVEL,"BEFORE FILE SEND\n");
    err = send_file(conn_info->cur_client, filepath);
    if (err <= 0){
        log_message(WARNING_LEVEL,"WRITE ERROR (handler)\n");
        disconnect_client(conn_info);
        return 1;
    }
    log_message(DEBUG_LEVEL,"AFTER FILE SEND\n");
    disconnect_client(conn_info);
    log_message(INFO_LEVEL,"Successfully handled\n");
    return 0;
}
void free_Web_server(Web_server *server){
    close(server->listenfd);
    threadPool_Destructor(server->pool);
    free(server);
}
int init_logger(){
    FILE *f = fopen(LOG_FILE, "w");
    if (f == NULL){
        printf("Cant open log file\n");
        return -1;
    }
    fclose(f);
    return 0;
}
Web_server *new_Web_server(in_addr_t address, in_port_t port){
    if (init_logger() != 0){
        return NULL;
    }
    Web_server *server = malloc(sizeof(Web_server));
    if (server == NULL){
        log_message(ERROR_LEVEL,"new server (malloc)\n");
        return NULL;
    }
    server->address.sin_addr.s_addr = htonl(address);
    server->address.sin_port = htons(port);
    server->address.sin_family = AF_INET;

    server->pool = new_threadPool();
    if (server->pool == NULL){
        free(server);
        log_message(ERROR_LEVEL,"new server (pool)\n");
        return NULL;
    }
    
    


    server->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listenfd == -1){
        log_message(ERROR_LEVEL,"new server (socket)\n");
        threadPool_Destructor(server->pool); // мб error
        free(server);
        return NULL;
    }

    int err = bind(server->listenfd, (struct sockaddr *)&server->address, sizeof(server->address));
    if (err != 0){
        log_message(ERROR_LEVEL,"new server (bind)\n");
        free_Web_server(server);
        return NULL;
    }
    return server;    

}
int accept_connection(Web_server *server, Clients_info *clients_info){
    // на всякий проверим ошибки, хотя возможно их можно игнорировать
        int client_fd = accept(server->listenfd, NULL, NULL);
        if (client_fd  < 0){
            log_message(WARNING_LEVEL,"server (accept)\n");
            //continue;
            return 1;
        }
        int buf_size = BUF_SIZE;
        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(BUF_SIZE))){
            log_message(DEBUG_LEVEL,"accept connection (setsockopt rcv)\n");
            return -1;
        }
        if (setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(BUF_SIZE))){
            log_message(DEBUG_LEVEL,"accept connection (setsockopt snd)\n");
            return -1;
        }
        pthread_rwlock_wrlock(&clients_info->clients_RWmutex);
        if (add_new_client(clients_info, client_fd) != 0){
            pthread_rwlock_unlock(&clients_info->clients_RWmutex);
            return -1;
        }    
        pthread_rwlock_unlock(&clients_info->clients_RWmutex);
        log_message(INFO_LEVEL,"connection accepted from %d\n", client_fd);
        return 0;
}
int listen_and_serve(Web_server *server, int clients_cap){
    server_active = 1;
    int err = listen(server->listenfd, MAX_CONN);
    if (err != 0){
        log_message(ERROR_LEVEL,"listen_and_serve (listen)\n");
        return -1;
    }
    Clients_info *clients_info = new_clients_info(clients_cap);
    if (clients_info == NULL){
        log_message(ERROR_LEVEL,"listen_and_serve (clients_info)\n");
        return -1;
    }
    init_clients_info(clients_info, server->listenfd);

    while(server_active){
        //делаем это, так как если дескриптор не готов на чтение - то будет сброшен, но это же не значит, что он отключен,
        //поэтому копируем
        fd_set rset = clients_info->clients_fdset;
        log_message(DEBUG_LEVEL,"\nBLOCKED ON SELECT\n");
        int nready = select(clients_info->max_client_fd + 1, &rset, 
                                                    NULL, NULL, NULL);
        // это значит, что accept сработает без блокировки 
        // но есть один нюанс (см. стивенс 6.3 стр 196 самый низ пункт 3)
        log_message(DEBUG_LEVEL,"nready is %d\nSELECT UNBLOCKED\n", nready);
        if (FD_ISSET(server->listenfd, &rset)){
            log_message(DEBUG_LEVEL,"attempt to accept connection\n");
            int err = accept_connection(server, clients_info);
            if (err != 0){
                log_message(ERROR_LEVEL,"listen and serve (accept)\n");
                free_clients_info(clients_info);
                return -1;
            }
            if (--nready <= 0){
                log_message(DEBUG_LEVEL,"continuing\n");
                continue;
            }
        }else{
            log_message(DEBUG_LEVEL,"no new connections\n");
        }

        //print_client_info(clients_info);
        for (int i = 0; i <= clients_info->max_client_ind; i ++){
            pthread_rwlock_rdlock(&clients_info->clients_RWmutex);    
            
            int cur_client_fd = clients_info->clients_fd[i];
            log_message(DEBUG_LEVEL,"cur ind %d\n", i);
            if (cur_client_fd != -1){
                if (FD_ISSET(cur_client_fd, &rset)){
                    log_message(DEBUG_LEVEL,"%d is set\n", cur_client_fd);
                    Conn_info *conn_info = new_conn_info(clients_info, cur_client_fd, i);
                    if (conn_info == NULL){
                        log_message(ERROR_LEVEL,"listen and serve (new conn info)\n");
                        pthread_rwlock_unlock(&clients_info->clients_RWmutex);
                        free_clients_info(clients_info);
                        return -1;
                    }
                    //clients_info->clients_fd[i] = -1;
                    FD_CLR(cur_client_fd, &clients_info->clients_fdset);
                    add_task(server->pool, new_task((void * (*)(void *))handle_connection, (void *)conn_info));
                    if (--nready <= 0){
                        pthread_rwlock_unlock(&clients_info->clients_RWmutex);
                        break;
                    }
                }
                
            }
            pthread_rwlock_unlock(&clients_info->clients_RWmutex);
        }
    }
    free_clients_info(clients_info);
    return 0;


}


/*
ошибки:
    при подключении второго клиента - происходит
    при отключении клиента - происходит дичь
    не получается выйти по сигналу
*/