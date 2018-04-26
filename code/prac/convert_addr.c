#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

int main(){

    char buf[BUFSIZ] = {0};
    in_addr_t ip = 0;
    struct sockaddr_in sock; memset(&sock, 0, sizeof(sock));

    while(1){
        printf("Input addr : ");
        fgets(buf, BUFSIZ, stdin);
        buf[strlen(buf)-1] = '\0';

        if(!strcmp(buf, "end")) break;

        ip = inet_addr(buf);
        inet_aton(buf, &sock.sin_addr);
        printf("You inputted         : %s\n", buf);
        printf("Convert to in_addr_t : %u\n", ip);
        printf("sock                 : %u\n", sock.sin_addr.s_addr);

        strncpy(buf, "I AM CLEAR", BUFSIZ);
        printf("Now buf is           : %s\n", buf);
        

        printf("inet_ntoa(sock)      : %s\n", inet_ntoa(sock.sin_addr));

    }
    return 0;
}
