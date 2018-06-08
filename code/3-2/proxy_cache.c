/**
 * System Programming Assignment #3-2 (proxy server)
 * @file proxy_cache.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu May 31 12:14:38 KST 2018
 */

#include "proxy_util.h"

int sub_process(const char *path_cache, const char *path_log, int fd_client,
                struct sockaddr_in addr_client);
int main_process();

// time variables
time_t time_start = {0};
time_t time_end   = {0};

int semid = 0;

// counter for number of subprocesses
size_t count_subprocess = 0;

/**
 * A handler for handling the SIGCHLD signal.
 */
void handler_child(){
    while(waitpid(-1, NULL, WNOHANG) > 0); // clean up child processes
}

/**
 * A handler for handling the SIGALRM signal to solve timeout.
 */
void handler_alram(){
    printf("No Response\n");
    abort();
}

/**
 * A handler for handling the SIGINT signal to write terminated log.
 */
void handler_int(){
    
    char buf[BUFSIZ] = {0};

    // get log path
    char path_log[PROXY_MAX_PATH] = {0};
    getHomeDir(path_log);
    strncat(path_log, "/logfile/logfile.txt", PROXY_MAX_PATH);

    // timer end
    time(&time_end);

    // make a string for terminating the log and write it
    snprintf(buf, BUFSIZ, "run time: %d sec. #sub process: %lu", (int)difftime(time_end, time_start), count_subprocess); 
    write_log(path_log, "**SERVER** [Terminated]", buf, false, false);

    exit(EXIT_SUCCESS);
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

    // timer start
    time(&time_start);

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

    // call an appropriate handler when corresponding occurs
    signal(SIGCHLD, handler_child);
    signal(SIGALRM, handler_alram);
    signal(SIGINT, handler_int);

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
        count_subprocess += 1;

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
int sub_process(const char *path_cache, const char *path_log, int fd_client,
                struct sockaddr_in addr_client){

    // a buffer for containing request header
    char buf[BUFSIZ] = {0};

    // hash for url
    char hash_url[PROXY_LEN_HASH] = {0};

    // char arrays to be contained with parsed results
    char parsed_url[PROXY_MAX_URL] = {0};

    // a char array pointing full cache path
    char path_fullcache[PROXY_MAX_PATH] = {0};

    // result value for finding cache
    int res = 0;

    // a char array for containing response information
    char response[BUFSIZ] = {0};

    // a file descriptor for reading a cache file
    int fd_cache = 0;
    int len      = 0;

    // prevent duplicated terminated log
    signal(SIGINT, SIG_DFL);

    // intercept a request from the client 
    read(fd_client, buf, BUFSIZ);

    // extract the url part from buf that was intercepted earlier
    request_parse(buf, parsed_url);

    if(parsed_url == NULL){ // if the request is not GET method then parsed_url'll be NULL
        return EXIT_SUCCESS;
    }

    // hash the input URL and find the cache with it
    sha1_hash(parsed_url, hash_url);
    res = find_primecache(path_cache, hash_url);

    // insert a slash delimiter at the 3rd index in the url_hash
    insert_delim(hash_url, PROXY_MAX_URL, 3, '/');

    // make a path for fullcache
    snprintf(path_fullcache, PROXY_MAX_PATH ,"%s/%s", path_cache, hash_url);

    switch(res){
        // if the result is HIT case,
        case PROXY_HIT:
            // nothing to do, just log it
            write_log(path_log, "[HIT]", hash_url, true, false);
            write_log(path_log, "[HIT]", parsed_url, false, false);
            break;

        // if the result is MISS case,
        case PROXY_MISS:
            // set the timer for timeout
            alarm(10);

            // request to the url and save its response into path_fullcache
            request_dump(buf, parsed_url, path_fullcache);
            // log it
            write_log(path_log, "[MISS]", parsed_url, true, false);
            break;

        default:
            break;
    }

    // read from the cache whether it is HIT or MISS
    fd_cache = open(path_fullcache, O_RDONLY);
    while((len = read(fd_cache, response, sizeof(response))) > 0){
        // and send its content to the client
        write(fd_client, response, len);
        memset(response, 0, sizeof(response));
    }
    close(fd_cache);

    return EXIT_SUCCESS;
}
