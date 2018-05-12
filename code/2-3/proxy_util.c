/**
 * System Programming Assignment #2-3 (proxy server)
 * @file proxy_util.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu May 10 14:03:41 KST 2018
 */

#include "proxy_util.h"

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
 * @param time_flag If it is true, write the log with current time. otherwise, don't.
 * @param pid_flag Append PID information into the log when it is set.
 * @return [int] Success:EXIT_SUCCESS, Fail:EXIT_FAILURE
 */
int write_log(const char *path, const char *header, const char *body, bool time_flag, bool pid_flag){
    FILE *fp        = NULL;

    char *msg_total       = NULL;
    size_t msg_total_size = 0;

    // a string for temporary path(the case for absence of the log path)
    char path_tmp[PROXY_MAX_PATH] = {0};

    pid_t pid_current = getpid();

    char str_pid[32]  = {0};
    char str_time[32] = {0};

    // 32 is a moderately large value to save time information
    struct tm *ltp = NULL; // for local time
    time_t now = {0};

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
    if(time_flag){
        // then time_str has format as belows
        time(&now);
        ltp = localtime(&now);
        strftime(str_time, 32, "-[%Y/%m/%d, %T]", ltp);
    }

    // if pid flag is true
    if(pid_flag){
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
    char hash_front[PROXY_LEN_PREFIX + 1] = {0};

    DIR *pDir                          = NULL;
    char path_subcache[PROXY_MAX_PATH] = {0};
    struct dirent *pFile               = NULL;
    struct stat path_stat              = {0};

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

/**
 * Parse a request from a client to extract the url.
 * @param buf A buffer containing request
 * @param url A char array to contain extracted url, it'll be "NULL" when method isn't GET
 * @return [int] SUCCESS:EXIT_SUCCESS, FAIL:EXIT_FAILURE
 */
int request_parse(const char *buf, char *url){

    // temporary vars for parsing
    char tmp[BUFSIZ] = {0};
    char *tok        = NULL;

    // copy contents of origin
    strncpy(tmp, buf, BUFSIZ);

    // extract a method part
    tok = strtok(tmp, " ");

    // if the method is GET then extract an url part
    if(strcmp(tok, "GET") == 0){
        tok = strtok(NULL, " ");
        strcpy(url, tok);
    } else { // else then url be "NULL"
        strcpy(url, "NULL");
    }

    return EXIT_SUCCESS;
}

/**
 * Return IP address of host name.
 * @param addr The URL of the host
 * @return [char *] SUCCESS:statically allocated char array ,FAIL:NULL
 */
char *getIPAddr(const char *addr){
    char *haddr          = NULL;
    int len              = strlen(addr);
    struct hostent *hent = NULL;

    if((hent = (struct hostent*)gethostbyname(addr)) != NULL){
        haddr = inet_ntoa(*((struct in_addr *)hent->h_addr_list[0]));
    }

    // The string is returned in a statically allocated buffer, 
    // which subsequent calls will overwrite.
    return haddr;
}

/**
 * Request to the url, and write the result into the filepath
 * @param buf A buffer containing response
 * @param url A char array containing the url
 * @param filepath A char array containing the filepath
 * @return [int] SUCCESS:EXIT_SUCCESS, FAIL:EXIT_FAILURE
 */
int request_dump(const char *buf, const char *url, const char *filepath){

    // variables for requesting to the url
    struct sockaddr_in addr_server = {0};
    int fd_socket = 0;

    // a file descriptor for writing response content
    int fd_cache = 0;
    int len = 0;

    // temporary buffers
    char tmp[BUFSIZ] = {0};
    char *tok = NULL;

    // get a file descriptor at filepath to write with full permission
    fd_cache = open(filepath, O_CREAT | O_TRUNC | O_WRONLY, 0777);

    // open a socket as TCP
    if((fd_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        close(fd_socket);
        return EXIT_FAILURE;
    }

    // equivalent to extract hostname
    strncpy(tmp, url, BUFSIZ);
    tok = strtok(tmp, "/");
    tok = strtok(NULL, "/");

    if(tok != NULL){
        // initialize addr_server
        addr_server.sin_family = AF_INET;
        addr_server.sin_addr.s_addr = inet_addr(getIPAddr(tok)); 
        addr_server.sin_port = htons(PROXY_HTTP_PORTNO);

        // connect fd_socket to addr_server
        if(connect(fd_socket, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0){
            close(fd_socket);
            return EXIT_FAILURE;
        }

        // send the buf which is request message
        write(fd_socket, buf, strlen(buf));

        // read the response and write it into fd_cache
        while((len = read(fd_socket, tmp, sizeof(tmp))) > 0){
            write(fd_cache, tmp, len);
        }
    }
    // close file descriptors
    close(fd_socket);
    close(fd_cache);

    return EXIT_SUCCESS;
}
