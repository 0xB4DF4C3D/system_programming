/**
 * System Programming Assignment #2-1 (proxy server)
 * @file server.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu Apr 26 10:12:44 KST 2018
 */

#include <dirent.h>      // format of directory entries
#include <errno.h>       // system error numbers
#include <fcntl.h>       // file control options
#include <libgen.h>      // definitions for pattern matching functions
#include <pwd.h>         // password structure
#include <signal.h>
#include <stdbool.h>     // for boolean type
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>      // standard symbolic constants and types

#include <openssl/sha.h> // to use SHA1
#include <arpa/inet.h>   // 

#include <sys/stat.h>    // data returned by the stat() function
#include <sys/types.h>   // data types
#include <sys/wait.h>

/**
 * Constants in proxy_cache to avoid magic numbers.
 * @see MAX_URL   : See https://stackoverflow.com/a/417184/7899226
 * @see MAN_PATH  : Not official but refer to the linux/limits.h and wikipedia.
 */
typedef enum {
    PROXY_LEN_PREFIX = 3,  ///< A length of the front hash.
    PROXY_LEN_HASH   = 41, ///< A length of the full hash(in the SHA1).

    PROXY_MAX_URL    = 2048, ///< A maximum length of an url.
    PROXY_MAX_PATH   = 4096, ///< A maximum length of a path.

    PROXY_HIT  = 8000, ///< An arbitrary magic number for the HIT case.
    PROXY_MISS = 8001, ///< An arbitrary magic number for the MISS case.

    PROXY_PORTNO = 40000
} Proxy_constants;

char *getHomeDir(char *home);
char *sha1_hash(char *input_url, char *hashed_url);
int insert_delim(char *str, size_t size_max, size_t idx, char delim);
int write_log(const char *path, const char *header, const char *body, bool time_, bool pid_);
int find_subcache(const char *path_subcache, const char *hash_full);
int find_primecache(const char *path_primecache, const char *hash_full);
int sub_process(const char *path_log, const char *path_cache, int client_fd);
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
 * A main process running a subprocess
 * @return [int] Success:EXIT_SUCCESS, Fail:EXIT_FAILURE
 */
int main_process(){
    // a pid number of process
    pid_t child_pid = 0;

    // char arrays for handling paths
    char path_home[PROXY_MAX_PATH]  = {0};
    char path_log[PROXY_MAX_PATH]   = {0};
    char path_cache[PROXY_MAX_PATH] = {0};

    // counter for number of subprocesses
    size_t count_subprocess = 0;

    // temporary buffer for misc
    char buf[BUFSIZ] = {0};

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    int socket_fd = 0, client_fd = 0;
    socklen_t len_socket = 0;
    
    // set full permission for the current process
    umask(0);

    // try getting current user's home path and concatenate cache and log paths with it
    if(getHomeDir(path_home) == NULL){
        fprintf(stderr, "[!] getHomeDir fail\n");
        return EXIT_FAILURE;
    }
    snprintf(path_cache, PROXY_MAX_PATH, "%s/cache", path_home);
    snprintf(path_log, PROXY_MAX_PATH, "%s/logfile/logfile.txt", path_home);

    if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "[!](server) can't open stream socket\n");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PROXY_PORTNO);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "[!](server) can't bind local address\n");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    listen(socket_fd, 5);
    signal(SIGCHLD, handler_child);

    while(true){
        memset(&client_addr, 0, sizeof(client_addr));
        len_socket = sizeof(client_addr);
        client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &len_socket); 
        inet_ntop(AF_INET, &client_addr.sin_addr, buf, len_socket);

        printf("[%s : %d] client was connected\n", buf, ntohs(client_addr.sin_port));

        if(client_fd < 0){
            fprintf(stderr, "[!](server) accept failed\n");
            close(client_fd);
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        child_pid = fork();
        if(child_pid == -1){
            close(client_fd);
            close(socket_fd);
            exit(EXIT_FAILURE);
        } else if (child_pid == 0){
            sub_process(path_log, path_cache, client_fd);
            printf("[%s : %d] client was disconnected\n", buf, ntohs(client_addr.sin_port));

            close(client_fd);
            exit(EXIT_SUCCESS);
        }
        close(client_fd);
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}

/**
 * A subprocess running in a child process.
 * @param path_log The path containing a logfile.
 * @param path_cache The path containing primecaches.
 * @param client_fd The client file descriptor.
 * @return [int] Success:EXIT_SUCCESS
 */
int sub_process(const char *path_log, const char *path_cache, int client_fd){

    // char arrays for handling a url
    char url_input[PROXY_MAX_URL] = {0};
    char url_hash[PROXY_LEN_HASH] = {0};

    // char arrays for fullcache
    char path_fullcache[PROXY_MAX_PATH] = {0};

    // result for finding cache
    int res = 0;

    // counter for HIT and MISS cases
    size_t count_hit  = 0;
    size_t count_miss = 0;

    // time structs for logging elapsed time
    time_t time_start = {0}, time_end = {0};

    // temporary buffer for misc
    char buf[BUFSIZ] = {0};

    int len_out = 0;

    // timer start
    time(&time_start);

    // receive inputs till the input is 'bye'
    while((len_out = read(client_fd, url_input, BUFSIZ)) > 0){

        if(!strncmp(url_input, "bye", 3)) break;

        // hash the input URL and find the cache with it
        sha1_hash(url_input, url_hash);
        res = find_primecache(path_cache, url_hash);

        // insert a slash delimiter at the 3rd index in the url_hash
        insert_delim(url_hash, PROXY_MAX_URL, 3, '/');

        // make a path for fullcache
        snprintf(path_fullcache, PROXY_MAX_PATH ,"%s/%s", path_cache, url_hash);

        switch(res){
            // If the result is HIT case,
            case PROXY_HIT:
                count_hit += 1;
                write_log(path_log, "[HIT]", url_hash, true, true);
                write_log(path_log, "[HIT]", url_input, false, false);

                write(client_fd, "HIT", 3);
                break;

                // If the result is MISS case,
            case PROXY_MISS:
                count_miss += 1;
                write_log(path_fullcache, "[TEST]", url_input, true, false);
                write_log(path_log, "[MISS]", url_input, true, true);

                write(client_fd, "MISS", 4);
                break;

            default:
                break;
        }
    }

    // timer end
    time(&time_end);

    // make a string for terminating the log and write it
    snprintf(buf, BUFSIZ, "run time: %d sec. #request hit : %lu, miss : %lu", (int)difftime(time_end, time_start), count_hit, count_miss);
    write_log(path_log, "[Terminated]", buf, false, true);

    return EXIT_SUCCESS;
}

/**
 * A given function for getting the home path.
 * @param home A char pointer that the current user's home path'll be assigned into.
 * @return The home path is also returned as a return value.
 */
char *getHomeDir(char *home){
    struct passwd *usr_info = getpwuid(getuid());   // get the user info into the usr_info
    strcpy(home, usr_info->pw_dir);                 // extract the home path

    return home;
}

/**
 * A given funtion for hashing an input URL with SHA1.
 * @param input_url A char pointer pointing an input URL to be hashed.
 * @param hashed_url A char pointer that the hashed URL'll be assigned into.
 * @return The hashed URL is also returned as a return value.
 * @warning There is no exception handling, note the size of pointers.
 */
char *sha1_hash(char *input_url, char *hashed_url){
    unsigned char hashed_160bits[20];   // since the SHA1 has a 160 bits size and a character is 8 bits
    char hashed_hex[41];                // since a hashed string is consists of hexadecimal characters
    unsigned int i;

    SHA1((unsigned char*)input_url, strlen(input_url), hashed_160bits); // do hashing

    // convert from binary format to hex
    for(i=0; i<sizeof(hashed_160bits); i++)
        sprintf(hashed_hex + i*2, "%02x", hashed_160bits[i]);

    strcpy(hashed_url, hashed_hex);

    return hashed_url;
}

/**
 * Insert a delimiter into a given string.
 * @param str A char array that a delimiter'll be inserted into.
 * @param size_max The size of str buffer.
 * @param idx The location where the delimiter to be inserted into.
 * @param delim A delimiter character.
 * @return [int] Success:EXIT_SUCCESS, Fail:EXIT_FAILURE
 */
int insert_delim(char *str, size_t size_max, size_t idx, char delim){
    // size for checking is there a room to insert a delim
    size_t size = strlen(str);

    // check buffer overflow or invalid index
    if(size_max <= size || size <= idx)
        return EXIT_FAILURE;

    // insert the delimiter into the position idx in the str.
    memmove(str + idx + 1,
            str + idx,
            size - idx + 1);
    str[idx] = delim;

    return EXIT_SUCCESS;
}

/**
 * Write a log with a specific format.
 * @param path A const char pointer pointing the path to write the log.
 * @param header The header of a log message.
 * @param body The body of a log message.
 * @param time_ If it is true, write the log with current time. otherwise, don't.
 * @return [int] Success:EXIT_SUCCESS, Fail:EXIT_FAILURE
 */
int write_log(const char *path, const char *header, const char *body, bool time_, bool pid_){
    FILE *fp        = NULL;

    // a string for temporary path(the case for absence of the log path)
    char path_tmp[PROXY_MAX_PATH] = {0};

    char *msg_total       = NULL;
    size_t msg_total_size = 0;

    // 32 is a moderately large value to save time information
    struct tm *ltp = NULL; // for local time
    time_t now = {0};
    char str_time[32] = {0};
    char str_pid[32] = {0};

    pid_t pid_current = getpid();

    // try opening the path with the append mode
    fp = fopen(path, "a+");
    if(fp == NULL){ // when the try went fail
        if(errno == ENOENT){ // if its reason is absence of the log path,
            // make the path, and try again
            strncpy(path_tmp, path, strlen(path));
            mkdir(dirname(path_tmp), S_IRWXU | S_IRWXG | S_IRWXO);       
            fp = fopen(path, "a+");
        } else {
            fprintf(stderr, "[!] %s : %s\n", path, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    // if time flag is true
    if(time_){
        // then time_str has format as belows
        time(&now);
        ltp = localtime(&now);
        strftime(str_time, 32, " - [%Y/%m/%d, %T]", ltp);
    }

    if(pid_){
        snprintf(str_pid, 32, " ServerPID : %d | ", pid_current);
    }

    // assign new memory block to msg_total by enough space
    msg_total_size = strlen(header) + strlen(body) + strlen(str_time) + strlen(str_pid) + 2;
    msg_total = (char*)malloc(msg_total_size);

    // join all parts of the message together
    snprintf(msg_total, msg_total_size, "%s%s%s%s\n", header, str_pid, body, str_time);

    // write message to the path
    fwrite(msg_total, 1, strlen(msg_total), fp);
    fflush(fp);
    if(chmod(path, S_IRWXU | S_IRWXG | S_IRWXO) == -1){
        fprintf(stderr, "[!] chmod fail\n");

        fclose(fp);
        free(msg_total);
        return EXIT_FAILURE;
    }

    fclose(fp);
    free(msg_total);
    return EXIT_SUCCESS;
}

/**
 * Find subcache(a back part of the hashed URL)
 * @param path_subcache A const char pointer to the path containing subcaches.
 * @param hash_full A const char pointer to be used as a part of a cache.
 * @return [int] HIT:PROXY_HIT, MISS:PROXY_MISS
 */
int find_subcache(const char *path_subcache, const char *hash_full){
    char hash_back[PROXY_LEN_HASH - PROXY_LEN_PREFIX + 1] = {0};

    DIR           *pDir  = NULL;
    struct dirent *pFile = NULL;

    // extract the back part of the hash
    strncpy(hash_back, hash_full + PROXY_LEN_PREFIX, PROXY_LEN_HASH - PROXY_LEN_PREFIX);

    // find the subcache while traversing the path
    pDir = opendir(path_subcache);
    for(pFile=readdir(pDir); pFile; pFile=readdir(pDir)){
        if(strcmp(hash_back, pFile->d_name) == 0){
            break;
        }
    }
    closedir(pDir);

    if(pFile == NULL){  // for MISS case
        return PROXY_MISS;
    }else{              // for HIT case
        return PROXY_HIT;
    }
}

/**
 * Find primecache(A front part of the hashed URL).
 * @param path_primecache A const char pointer to the path containing primecaches.
 * @param hash_full A const char pointer to be used as a part of a cache.
 * @return [int] HIT:PROXY_HIT, MISS:PROXY_MISS, FAIL:EXIT_FAILURE
 */
int find_primecache(const char *path_primecache, const char *hash_full){
    DIR           *pDir   = NULL;
    struct dirent *pFile  = NULL;
    struct stat path_stat = {0};

    char hash_front[PROXY_LEN_PREFIX + 1] = {0};

    char path_subcache[PROXY_MAX_PATH] = {0};

    int result = 0;

    // extract the front of the hash
    strncpy(hash_front, hash_full, PROXY_LEN_PREFIX);

    // check whether the path of primecache exist or not
    pDir = opendir(path_primecache);
    if(pDir == NULL){   // if not exist,
        // create that path
        mkdir(path_primecache, S_IRWXU | S_IRWXG | S_IRWXO);
        // and try opening it again
        pDir = opendir(path_primecache); 
    }

    // make the full path of the directory which contains subcaches
    snprintf(path_subcache, PROXY_MAX_PATH, "%s/%s", path_primecache, hash_front);

    // find the primecache while traversing the path
    for(pFile=readdir(pDir); pFile; pFile=readdir(pDir)){
        // if the primecache was found
        if(strcmp(hash_front, pFile->d_name) == 0){
            // check whether it is a directory or not
            stat(path_subcache, &path_stat);
            if(S_ISDIR(path_stat.st_mode)){
                break;
            }else{ // if the file was regular, then something's wrong in the cache directory
                fprintf(stderr, "[!] find_primecache fail\n");
                fprintf(stderr, "[+] the cache directory is corrupted\n");
                return EXIT_FAILURE;
            }
        }
    }
    closedir(pDir);

    // if there isn't the path of subcache, then create that path
    if(pFile == NULL)
        mkdir(path_subcache, S_IRWXU | S_IRWXG | S_IRWXO);

    // find the subcache in the path of the current primecache
    result = find_subcache(path_subcache, hash_full);

    return result;
}