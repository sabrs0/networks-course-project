#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "logger.h"
#define METHOD_SIZE 16
#define URL_SIZE 2048
#define CODE_SIZE 8
#define VERSION_SIZE 16
#define CONTENT_TYPE_SIZE 64
#define HEADERS_LEN 1024
#define ROOT_DIR "/home/sersab/bmstu_ubu/4_course/networks/networks-course-project/static"
#define RESPONSE_LEN 4096
extern int server_active;
/*static const char resp[] = "HTTP/1.0 200 OK\r\n"
"Server: webserver-c\r\n"
"Content-type: text/html\r\n\r\n"
"<html>hello, world</html>\r\n";*/



typedef struct{
    char method[METHOD_SIZE];
    char url[URL_SIZE];
    char version[VERSION_SIZE];
} Http_request;

typedef struct{
    char version[METHOD_SIZE];
    char code[CODE_SIZE];
    char status[VERSION_SIZE];
    char headers[HEADERS_LEN];
    
} Http_response;

void set_file_from_url(char *err_str, char *filename, char *url);

Http_request parse_request(char *request);

Http_response form_response(char *version, char *code, char *status, char *headers);

void response_to_string(char *response, Http_response resp);

int add_header(char *header_list, char *header);

char *get_content_type(char *filename);

//char *resp_http_200(Http_response resp);
void resp_http_403(char *response);
void resp_http_404(char *response);
void resp_http_405(char *response);