#include "../inc/http.h"
Http_request parse_request(char *request_buf){
    Http_request req;
    strcpy(req.method, "");
    strcpy(req.url, "");
    strcpy(req.version, "");
    if (sscanf(request_buf, "%s %s %s", req.method, req.url, req.version) != 3){
        log_message(WARNING_LEVEL,"parse request (sscanf)\n");
        return req;
    }

    return req; 
}
void set_file_from_url(char *err_str, char *filename, char *url){
    char real_path[PATH_MAX];
    char resolved_path[PATH_MAX];
    if (sprintf(real_path, "%s%s",ROOT_DIR, url) < 0){
        log_message(WARNING_LEVEL,"get file from url(snprintf)\n");
        resp_http_403(err_str);
        return ;
    }
    if (realpath(real_path, resolved_path) == NULL){
        log_message(WARNING_LEVEL,"get file from url(realpath) 404\n");
        resp_http_404(err_str);
        return ;
    }
    if (strncmp(resolved_path, ROOT_DIR, strlen(ROOT_DIR)) != 0){
        log_message(WARNING_LEVEL,"get file from url (strncmp) 403\n");
        resp_http_403(err_str);
        return ;
    }
    strcpy(err_str, "OK");
    strcpy(filename, resolved_path);
}

char *get_content_type(char *filename){
    char file_ext[16];
    strcpy(file_ext, strrchr(filename, '.'));
    if (file_ext != NULL){
            if (strcmp(file_ext, ".html") == 0){
                return "text/html";
            }else if (strcmp(file_ext, ".css") == 0){
                return "text/html";
            }else if (strcmp(file_ext, ".js") == 0){
                return "application/javascript";
            }else if (strcmp(file_ext, ".png") == 0){
                return "image/png";
            }else if (strcmp(file_ext, ".jpg") == 0){
                return "image/jpeg";
            }else if (strcmp(file_ext, ".jpeg") == 0){
                return "image/jpeg";
            }else if (strcmp(file_ext, ".swf") == 0){
                return "application/x-shockwave-flash";
            }else if (strcmp(file_ext, ".gif") == 0){
                return "image/gif";
            }else if (strcmp(file_ext, ".pdf") == 0){
                return "application/pdf";
            }else{
                log_message(ERROR_LEVEL,"Unknown file extension\n");
                return "";
            }
    }
    log_message(WARNING_LEVEL,"get content type (strrchr)\n");
    return "";
}
Http_response form_response(char *version, char *code, char *status, char *headers){
    Http_response resp;
    strcpy(resp.code, code);
    strcpy(resp.version, version);
    strcpy(resp.status, status);
    strcpy(resp.headers, headers);
    return resp;
}
int add_header(char *header_list, char *header){
    if ((strlen(header_list) + strlen(header) + strlen("\r\n")) >= HEADERS_LEN){
        return -1;
    }
    strcat(header_list, "Content-Type: ");  
    strcat(header_list, header);
    strcat(header_list, "\r\n");
    return 0;
}
void response_to_string(char *response, Http_response resp){
    strcpy(response, resp.version);
    strcat(response, " ");
    strcat(response, resp.code);
    strcat(response, " ");
    strcat(response, resp.status);
    strcat(response, "\r\n");
    strcat(response, resp.headers);
    strcat(response, "\r\n");
    log_message(DEBUG_LEVEL,"\n\t\t RESPONSE STR IS:\n\t\t%s\n\n", response);
}
void resp_http_403(char *response){
    Http_response resp;
    strcpy(resp.code, "403");
    strcpy(resp.headers, "Content_type: text/html");
    strcpy(resp.status, "Forbidden");
    strcpy(resp.version, "HTTP/1.1");
    response_to_string(response, resp);
}
void resp_http_404(char *response){
    Http_response resp;
    strcpy(resp.code, "404");
    strcpy(resp.headers, "Content_type: text/html");
    strcpy(resp.status, "Not Found");
    strcpy(resp.version, "HTTP/1.1");
    response_to_string(response, resp);
}
void resp_http_405(char *response){
    Http_response resp;
    strcpy(resp.code, "405");
    strcpy(resp.headers, "Content_type: text/html");
    strcpy(resp.status, "Unknown request");
    strcpy(resp.version, "HTTP/1.1");
    response_to_string(response, resp);
}