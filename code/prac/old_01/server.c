#include <netinet/in.h>

#include <stdio.h>
#include <string.h>     // bzero(), ...
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h> // STDOUT_FILENO, ...

#define BUFSIZE 1024
#define PORTNO  40000

int main(){
    struct sockaddr_in server_addr, client_addr;
    int socket_fd, client_fd;
    socklen_t len, len_out;
    char buf[BUFSIZE];


    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        printf("Server: Can't open stream socket.\n");
        return 1;
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORTNO);

    if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        printf("Server: Can't bind local address.\n");
        return 1;
    }
    
    printf("[+] start listening..\n");
    listen(socket_fd, 5);
    while(1){
        len = sizeof(client_addr);
        client_fd = accept(socket_fd,
                        (struct sockaddr *)&client_addr,
                        &len
                        );

        if(client_fd < 0){
            printf("Server: accept failed.\n");
            return 1;
        }

        printf("[%d:%d] client was connected.\n",
                client_addr.sin_addr.s_addr, client_addr.sin_port);
        while((len_out = read(client_fd, buf, BUFSIZE)) > 0){
            write(STDOUT_FILENO, " - Messages : ", 15);
            write(STDOUT_FILENO, buf, len_out);
            write(client_fd, buf, len_out);
            write(STDOUT_FILENO, "\n", 1);
        }
        printf("[%d:%d] client was disconnected.\n",
                client_addr.sin_addr.s_addr, client_addr.sin_port);
        close(client_fd);
    }
    close(socket_fd);
    return 0;
}
