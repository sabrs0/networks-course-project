#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../inc/threadpool.h"
#include <signal.h>
#define PORT 8088
#define BUF_SIZE 2048
int server_active = 1;

const char *resp = "HTTP/1.0 200 OK\r\n"
"Server: webserver-c\r\n"
"Content-type: text/html\r\n\r\n"
"<html>hello, world</html>\r\n";

void handle_signal(int signum){
    switch (signum)
    {
    case SIGINT:
        server_active = 0;
        printf("\n...SIGINT RECIEVED, CLOSING SERVER...\n");
        break;
    default:
        break;
    }
}

int handle_connection(int *sockfd){
    char buf[BUF_SIZE];
    int err = read(*sockfd, buf, BUF_SIZE);
    if (err <= 0){
        printf("READ ERROR (handler)\n");
        close(*sockfd);
        free(sockfd);
        return 1;
    }
    struct sockaddr_in client_address;
    int addr_len;
    err = getsockname(*sockfd, (struct sockaddr *)&client_address, (socklen_t *)&addr_len);
    if (err != 0){
        printf("getsockname (handler)\n");
        close(*sockfd);
        free(sockfd);
        return 1;
    }
    printf("Read from [%s:%u] : %s\n", inet_ntoa(client_address.sin_addr),
    ntohs(client_address.sin_port), buf);

    err = write(*sockfd, resp, strlen(resp));
    if (err <= 0){
        printf("WRITE ERROR (handler)\n");
        close(*sockfd);
        free(sockfd);
        return 1;
    }
    close(*sockfd);
    free(sockfd);
    return 0;

}

int main(){
    //read-write

    //POOL
    ThreadPool *pool = new_threadPool();
    if (pool == NULL){
        return 1;
    }

    signal(SIGINT, handle_signal);

    // SERVER
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1){
        perror("main (socket)");
        return 1;
    }
    struct sockaddr_in host_address;
    host_address.sin_family = AF_INET;
    host_address.sin_addr.s_addr = htonl(INADDR_ANY);
    host_address.sin_port = htons(PORT);
    size_t host_addr_size = sizeof(host_address);
    
    int err = bind(socketfd, (struct sockaddr *)&host_address, host_addr_size);
    if (err != 0){
        perror("main (bind)");
        return 1;
    }

    err = listen(socketfd, SOMAXCONN);
    if (err != 0){
        perror("main (listen)");
        return 1;
    }
    while(server_active){
        int *connfd = malloc(sizeof(int));
        if (connfd == NULL){
            printf("main (malloc sockfd)\n");
            continue;
        }
        *connfd = accept(socketfd, NULL, NULL);
        if (*connfd == -1){
            perror("main (accept)");
            continue;
        }
        err = add_task(pool, new_task((void *(*)(void *))handle_connection, (void *)connfd));
        if (err != 0){
            printf("Cant add task\n");
            continue;
        }

    }
    printf("escaped server\n");
    threadPool_Destructor(pool);
    return 0;

}

// таймауты на подключение
// мьютекс один общий на разделяемый ресурс и на condition variable
// cond variables - каналы го, для блокировки до тех пор, пока не наступит событие
// pthread_join - блочим главный поток, пока не завершатся дети
// плотно обработать ошибки
//отслеживание клиентов - стр 208 стивенс
// мб вместо server_active ориентировать на дескриптор listenfd, который ставить в 0, если неактивен сервак
// клиентов 256, но вроде там в каких-то главах написано, как решить эту проблему (,,15.6)
// tcp обслуживание
//обрабатывать ошибку threadpool destructor - либо на месте паника - либо игнорим
// подумай, должны ли совпадать max_conn и client_cap
// следить за тем, чтобы len не превышала cap
// у меня 1 мьютекс на удерживание task_queue и  queue_ready. - норм ли это
// добавил 2й мьютекс для client_stuff в threadpool