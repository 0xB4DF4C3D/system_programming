#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#define BUFSIZE 1024
#define PORTNO  40000

int main(){
    int socket_fd;
    socklen_t len;
    struct sockaddr_in server_addr;
    char haddr[] = "127.0.0.1";
    char buf[BUFSIZE];

    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        printf("can't create socket.\n");
        return -1;
    }

    bzero(buf, sizeof(buf));
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(haddr);
    server_addr.sin_port = htons(PORTNO);

    if(connect(socket_fd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0){
        printf("can't connect.\n");
        return -1;
    }

    write(STDOUT_FILENO, "input url > ", 12);
    while((len = read(STDIN_FILENO, buf, sizeof(buf))) > 0){

        if(!strncmp(buf, "bye", 3)) break;

        buf[strlen(buf)-1] = '\0';
        if(write(socket_fd, buf, strlen(buf)) > 0){
            if((len = read(socket_fd, buf, sizeof(buf))) > 0){
                write(STDOUT_FILENO, buf, len);
                write(STDOUT_FILENO, "\n", 1);
                bzero(buf, sizeof(buf));
            }
        }
        write(STDOUT_FILENO, "input url > ", 12);
    }
    close(socket_fd);
    return 0;
}
