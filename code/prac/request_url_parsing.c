#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define PORTNO 39999

int main(){
    struct sockaddr_in addr_server, addr_client;
    memset(&addr_server, 0, sizeof(addr_server));
    memset(&addr_client, 0, sizeof(addr_client));

    int fd_socket = 0, fd_client = 0;
    int len_out = 0, opt = 1;
    socklen_t len = 0;

    if((fd_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Server : Can't open stream socket\n");
        return 1;
    }
    setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(PORTNO);

    if(bind(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0){
        fprintf(stderr, "Server : Can't bind local address\n");
        return 1;
    }

    listen(fd_socket, 5);
    while(1){
        struct in_addr inet_addr_client; memset(&inet_addr_client, 0, sizeof(inet_addr_client));

        char buf[BUFSIZ] = {0};

        char response_header[BUFSIZ] = {0};
        char response_message[BUFSIZ] = {0};

        char tmp[BUFSIZ] = {0};
        char method[20] = {0};
        char url[BUFSIZ] = {0};

        char *tok = NULL;

        len = sizeof(addr_client);
        fd_client = accept(fd_socket, (struct sockaddr *)&addr_client, &len);
        if(fd_client < 0){
            fprintf(stderr, "Server : accept failed\n");
            return 1;
        }
        inet_addr_client.s_addr = addr_client.sin_addr.s_addr;

        printf("[%s : %d] client was connected\n", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port));
        read(fd_client, buf, BUFSIZ);
        strncpy(tmp, buf, BUFSIZ);
        puts("======================================================");
        printf("Request from [%s : %d]\n", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port));
        puts(buf);
        puts("======================================================\n");

        tok = strtok(tmp, " ");
        strcpy(method, tok);
        if(strcmp(method, "GET") == 0){
            tok = strtok(NULL, " ");
            strcpy(url, tok);
        }
        snprintf(response_message, BUFSIZ,
                "<h1>RESPONSE</h1><br>"
                "Hello %s:%d<br>"
                "%s", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port), url);
        snprintf(response_header, BUFSIZ,
                "HTTP/1.0 200 OK\r\n"
                "Server:2018 simple web server\r\n"
                "Content-length:%lu\r\n"
                "Content-type:text/html\r\n\r\n", strlen(response_message));

        write(fd_client, response_header, strlen(response_header));
        write(fd_client, response_message, strlen(response_message));

        printf("[%s : %d] client was disconnected\n", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port));
        close(fd_client);
 
    }
    close(fd_socket);
    return 0;
}
