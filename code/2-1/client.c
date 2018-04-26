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

    // variables for address
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    socklen_t addr_len = 0;

    // a file descriptor
    int fd_socket = 0;

    // try open a stream socket
    if((fd_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "[!](client) can't open stream socket\n");    
        exit(EXIT_FAILURE);
    }

	// initialize addr_server
    addr_server.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr_server.sin_addr);
    addr_server.sin_port = htons(PROXY_PORTNO);

	// connect fd_socket to addr_server
    if(connect(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0){
        fprintf(stderr, "[!](client) can't connect\n");
        exit(EXIT_FAILURE);
    }

    // interact with the server 
    while(true){
        // input url by the user without a linebreak
        printf("input url > "); fflush(stdout);
        addr_len = read(STDIN_FILENO, buf, sizeof(buf));
        buf[strlen(buf)-1] = '\0';

        // if the url is 'bye' then break
        if(!strncmp(buf, "bye", 3)) break;

        // send the url to the server
        if(write(fd_socket, buf, strlen(buf)) > 0){
            // receive the result from the server
            if((addr_len = read(fd_socket, buf, sizeof(buf))) > 0){
                write(STDOUT_FILENO, buf, addr_len);
                write(STDOUT_FILENO, "\n", 1);
                memset(&buf, 0, sizeof(buf));
            }
        }
    }

    close(fd_socket);
    return EXIT_SUCCESS;
}
