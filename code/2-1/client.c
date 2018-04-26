/**
 * System Programming Assignment #2-1 (proxy server)
 * @file client.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu Apr 26 20:52:07 KST 2018
 */

#include <arpa/inet.h> // for socket programming

#include <stdbool.h>
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
    // buffer for URL inputted by user or result from server
    char buf[BUFSIZ] = {0};

    // struct for server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    // variables for socket
    int socket_fd        = 0;
    socklen_t len_socket = 0;

    // try open a stream socket
    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "[!](client) can't open stream socket\n");    
        exit(EXIT_FAILURE);
    }

	// initialize server_addr
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_port = htons(PROXY_PORTNO);

	// connect socket_fd to server_addr
    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "[!](client) can't connect\n");
        exit(EXIT_FAILURE);
    }

    // interact with the server 
    while(true){
        // input url by the user without a linebreak
        printf("input url > "); fflush(stdout);
        len_socket = read(STDIN_FILENO, buf, sizeof(buf));
        buf[strlen(buf)-1] = '\0';

        // if the url is 'bye' then break
        if(!strncmp(buf, "bye", 3)) break;

        // send the url to the server
        if(write(socket_fd, buf, strlen(buf)) > 0){
            // receive the result from the server
            if((len_socket = read(socket_fd, buf, sizeof(buf))) > 0){
                write(STDOUT_FILENO, buf, len_socket);
                write(STDOUT_FILENO, "\n", 1);
                memset(&buf, 0, sizeof(buf));
            }
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}
