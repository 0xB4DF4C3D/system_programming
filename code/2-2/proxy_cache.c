/**
 * System Programming Assignment #2-2 (proxy server)
 * @file proxy_cache.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu May  3 00:15:43 KST 2018
 */

#include "proxy_util.h"

int sub_process(const char *path_cache, const char *path_log, int fd_client, struct sockaddr_in addr_client);
int main_process();

/**
 * A handler for handling the SIGCHLD signal.
 */
void handler_child(){
    while(waitpid(-1, NULL, WNOHANG) > 0); // clean up child processes
}

int main(int argc, char* argv[]){

    //start a main process
    main_process();

    return EXIT_SUCCESS;
}

/**
 * A main process running a subprocess.
 * It also handles stream between server and client.
 * @return [int] Success:EXIT_SUCCESS, Fail:EXIT_FAILURE
 */
int main_process(){

    // variables for address
    socklen_t addr_len = 0;
    struct sockaddr_in addr_server, addr_client;
    memset(&addr_server, 0, sizeof(addr_server));
    memset(&addr_client, 0, sizeof(addr_client));

    // temporary buffer for misc
    char buf[BUFSIZ] = {0};

    // counter for number of subprocesses
    size_t count_subprocess = 0;

    // file descriptors
    int fd_socket = 0, fd_client = 0;

    socklen_t len = 0;
    
    // an option value for socket
    int opt = 1;

    // char arrays for handling paths
    char path_cache[PROXY_MAX_PATH] = {0};
    char path_home[PROXY_MAX_PATH]  = {0};
    char path_log[PROXY_MAX_PATH] = {0};

    // a pid number of process
    pid_t pid_child = 0;

    // set full permission for the current process
    umask(0);

    // try getting current user's home path and concatenate cache and log paths with it
    if(getHomeDir(path_home) == NULL){
        fprintf(stderr, "[!] getHomeDir fail\n");
        return EXIT_FAILURE;
    }
    snprintf(path_cache, PROXY_MAX_PATH, "%s/cache", path_home);
    snprintf(path_log, PROXY_MAX_PATH, "%s/logfile/logfile.txt", path_home);

    // try open a stream socket
    if((fd_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "[!](server) can't open stream socket\n");
        return EXIT_FAILURE;
    }
    setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // initialize addr_server
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PROXY_PORTNO);
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind fd_socket to addr_server
    if(bind(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0){
        fprintf(stderr, "[!](server) can't bind local address\n");
        close(fd_socket);
        return EXIT_FAILURE;
    }

    // listen for connections on a socket
    listen(fd_socket, 5);

    // call handler_child when SIGCHLD occurs
    signal(SIGCHLD, handler_child);

    // interact with the client
    while(true){
        // receive data for the client address
        memset(&addr_client, 0, sizeof(addr_client));
        addr_len = sizeof(addr_client);
        fd_client = accept(fd_socket, (struct sockaddr *)&addr_client, &addr_len); 
        inet_ntop(AF_INET, &addr_client.sin_addr, buf, addr_len);
        if(fd_client < 0){
            fprintf(stderr, "[!](server) accept failed\n");
            close(fd_client);
            close(fd_socket);
            return EXIT_FAILURE;
        }

        // fork a process
        pid_child = fork();
        if(pid_child == -1){
            close(fd_client);
            close(fd_socket);
            return EXIT_FAILURE;
        } else if (pid_child == 0){
            // call the sub_process
            sub_process(path_cache, path_log, fd_client, addr_client);

            close(fd_client);
            return EXIT_SUCCESS;
        }
        // in a parent process
        close(fd_client);
    }

    close(fd_socket);
    return EXIT_SUCCESS;
}

/**
 * A subprocess running in a child process.
 * It handles caches practically.
 * @param path_cache The path containing primecaches.
 * @param path_log The path containing a logfile
 * @param fd_client The client file descriptor.
 * @param addr_client The address struct for the client
 * @return [int] Success:EXIT_SUCCESS
 */
int sub_process(const char *path_cache, const char *path_log, int fd_client, struct sockaddr_in addr_client){

    // a buffer for containing request header
    char buf[BUFSIZ] = {0};

    // counts for hit&miss
    size_t count_hit  = 0;
    size_t count_miss = 0;
    
    // hash for url
    char hash_url[PROXY_LEN_HASH] = {0};
    
    // in_addr structure for checking ip address
    struct in_addr inet_addr_client; memset(&inet_addr_client, 0, sizeof(inet_addr_client));

    // char arrays to be contained with parsed results
    char parsed_method[20]         = {0};
    char parsed_url[PROXY_MAX_URL] = {0};

    // a char array pointing full cache path
    char path_fullcache[PROXY_MAX_PATH] = {0};

    // result value for finding cache
    int res = 0;

    // char arrays for containing response information
    char response_header[BUFSIZ]  = {0};
    char response_message[BUFSIZ] = {0};

    // timer variables
    time_t time_start = {0};
    time_t time_end   = {0};

    // timer start
    time(&time_start);


    inet_addr_client.s_addr = addr_client.sin_addr.s_addr;

    puts("======================================================");
    printf("[%s : %d] client was connected\n", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port));

    read(fd_client, buf, BUFSIZ);

    printf("Request from [%s : %d]\n", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port));
    puts(buf);

    parse_request(buf, parsed_url, parsed_method); 

    // hash the input URL and find the cache with it
    sha1_hash(parsed_url, hash_url);
    res = find_primecache(path_cache, hash_url);

    // insert a slash delimiter at the 3rd index in the url_hash
    insert_delim(hash_url, PROXY_MAX_URL, 3, '/');

    // make a path for fullcache
    snprintf(path_fullcache, PROXY_MAX_PATH ,"%s/%s", path_cache, hash_url);

    switch(res){
        // If the result is HIT case,
        case PROXY_HIT:
            count_hit += 1;
            write_log(path_log, "[HIT]", hash_url, true, true);
            write_log(path_log, "[HIT]", parsed_url, false, false);

            strncpy(response_message, "Hit", BUFSIZ);
            break;

            // If the result is MISS case,
        case PROXY_MISS:
            count_miss += 1;
            write_log(path_fullcache, "[TEST]", parsed_url, true, false);
            write_log(path_log, "[MISS]", parsed_url, true, true);

            write_log(path_fullcache, "[TEST]", parsed_url, true, false); 
            strncpy(response_message, "Miss", BUFSIZ);
            break;

        default:
            break;
    }

    snprintf(response_header, BUFSIZ,
            "HTTP/1.0 200 OK\r\n"
            "Server:2018 simple web server\r\n"
            "Content-length:%lu\r\n"
            "Content-type:text/html\r\n\r\n", strlen(response_message));

    write(fd_client, response_header, strlen(response_header));
    write(fd_client, response_message, strlen(response_message));

    printf("[%s : %d] client was disconnected\n", inet_ntoa(inet_addr_client), ntohs(addr_client.sin_port));
    puts("======================================================\n");

    // timer end
    time(&time_end);

    // make a string for terminating the log and write it
    snprintf(buf, BUFSIZ, "run time: %d sec. #request hit : %lu, miss : %lu", (int)difftime(time_end, time_start), count_hit, count_miss);
    write_log(path_log, "[Terminated]", buf, false, true);

    return EXIT_SUCCESS;
}
