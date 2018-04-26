#include <arpa/inet.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#define PORTNO 40000

void handler_child(){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(){
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    int socket_fd = 0, client_fd = 0;
    int len_out = 0;

    socklen_t len_socket = 0;

    pid_t child_pid = 0;

    char buf[BUFSIZ] = {0};

    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
       perror("[!] create socket failed");
       exit(-1);
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORTNO);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("[!] error bind server socket");
        close(socket_fd);
        exit(-1);
    }

    listen(socket_fd, 5);
    printf("[+] start listening..\n");
    signal(SIGCLD, handler_child);

    while(1){
        memset(&client_addr, 0, sizeof(client_addr));
        len_socket = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &len_socket);

        if(client_fd < 0){
            perror("[!] accept failed");
            close(socket_fd);
            exit(-1);
        }

        printf("[%d : %d] client was connected\n", client_addr.sin_addr.s_addr, client_addr.sin_port);
        child_pid = fork();
        if(child_pid == -1){
            close(client_fd);
            close(socket_fd);
            continue;
        } else if(child_pid == 0){
            while((len_out = read(client_fd, buf, BUFSIZ)) > 0){
                if(!strncmp(buf, "bye", 3)) break;

                write(STDOUT_FILENO, " - Messages : ", 15);
                write(STDOUT_FILENO, buf, len_out);
                write(client_fd, buf, len_out);
                write(STDOUT_FILENO, "\n", 1);
            }
            printf("[%d : %d] client was disconnected\n", client_addr.sin_addr.s_addr, client_addr.sin_port);
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }
    close(socket_fd);

    return 0;
}
