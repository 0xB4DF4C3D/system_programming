/**
 * System Programming Assignment #2-1 (proxy server)
 * @file client.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu Apr 26 20:52:07 KST 2018
 */

#include <arpa/inet.h> // for socket programming

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // standard symbolic constants and types

#include <sys/types.h> // data types

/**
 * Constant values for files relevanting proxy to avoid magic number.
 */
typedef enum {
    PROXY_PORTNO = 38078 ///< A given port number for me
} Proxy_constants;

int main(){
    char buf[BUFSIZ] = {0};

    char haddr[] = "127.0.0.1";

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    int socket_fd        = 0;
    socklen_t len_socket = 0;

    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "[!](client) can't open stream socket\n");    
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(haddr);
    server_addr.sin_port = htons(PROXY_PORTNO);

    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "[!](client) can't connect\n");
        exit(EXIT_FAILURE);
    }

    printf("input url > ");
    while((len_socket = read(STDIN_FILENO, buf, sizeof(buf))) > 0){
        buf[strlen(buf)-1] = '\0';

        if(!strncmp(buf, "bye", 3)) break;

        if(write(socket_fd, buf, strlen(buf)) > 0){
            if((len_socket = read(socket_fd, buf, sizeof(buf))) > 0){
                printf("%s\n", buf);
                memset(&buf, 0, sizeof(buf));
            }
        }
        printf("input url > ");
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}
