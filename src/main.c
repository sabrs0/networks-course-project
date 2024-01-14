#include "../inc/server.h"
#include <signal.h>
//int server_active;

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

int main(){
    //signal(SIGINT, handle_signal);

    Web_server *server = new_Web_server(INADDR_ANY, PORT);
    if (server == NULL){
        return 1;
    }
    int err = listen_and_serve(server, CLIENTS_CAP);
    printf("escaped server\n");
    free_Web_server(server);
    return 0;

}

// таймауты на подключение, обработку запросов, мьютексы, треды, все все все.
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
// нужно ли реально добавлять rset помимо allset  ????????
//при больших файлах выставлять заголовок Accept-Ranges
// выставить размеры буферов для сокетов
// если размер файла для отправки больше, чем размер буфера сокета - то выставить хедеры и отправить по частям