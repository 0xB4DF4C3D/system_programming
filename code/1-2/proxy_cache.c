/**
 * System Programming Assignment #1-2 (proxy server)
 * @file proxy_cache.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Thu Apr  5 22:41:51 KST 2018
 */

#include <dirent.h>     // format of directory entries
#include <errno.h>      // system error numbers
#include <fcntl.h>      // file control options
#include <libgen.h>     // definitions for pattern matching functions
#include <pwd.h>        // password structure
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>     // standard symbolic constants and types

#include <openssl/sha.h>// to use SHA1

#include <sys/stat.h>   // data returned by the stat() function
#include <sys/types.h>  // data types

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
    PROXY_MISS = 8001  ///< An arbitrary magic number for the MISS case.
} Proxy_constants;

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
    size_t size = sizeof(char) * strlen(str);

    // check memory overflow or invalid index
    if(size_max <= size || size_max <= idx)
        return EXIT_FAILURE;

    // insert the delimiter into the position idx in the str.
    memmove(str + idx + 1,
            str + idx,
            size - idx);
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
int write_log(const char *path, const char *header, const char *body, bool time_){
    FILE *fp        = NULL;

    // 32 is a moderately large value to save time information
    char time_str[32] = {0};

    char *msg_total       = NULL;
    size_t msg_total_size = 0;

    // a string for temporary path(the case for absence of the log path)
    char path_tmp[PROXY_MAX_PATH] = {0};

    struct tm *ltp = NULL; // for local time
    time_t now;

    // try opening the path with the append mode
    fp = fopen(path, "a+");
    if(fp == NULL){ // when the try went fail
        if(errno == ENOENT){ // if its reason is absence of the log path,

            // make the path, and try again
            strncpy(path_tmp, path, strlen(path));
            mkdir(dirname(path_tmp), S_IRWXU | S_IRWXG | S_IRWXO);       
            fp = fopen(path, "a+");
        } else {
            printf("[!] %s : %s\n", path, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    // assign new memory block to msg_total by enough space
    msg_total_size = sizeof(char) * (strlen(header) + strlen(body) + 32);
    msg_total = (char*)malloc(msg_total_size);

    // if time flag is true
    if(time_){
        // then time_str has format as belows
        time(&now);
        ltp = localtime(&now);
        strftime(time_str, 32, "-[%Y/%m/%d, %T]", ltp);
    }

    // join all parts of the message together
    snprintf(msg_total, msg_total_size, "%s%s%s\n", header, body, time_str);

    // write message to the path
    fwrite(msg_total, 1, strlen(msg_total), fp);
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
    struct dirent *pFile = NULL;
    DIR           *pDir  = NULL;

    char hash_back[PROXY_LEN_HASH - PROXY_LEN_PREFIX + 1] = {0};

    // extract the back part of the hash
    strncpy(hash_back, hash_full + PROXY_LEN_PREFIX, PROXY_LEN_HASH - PROXY_LEN_PREFIX);

    // find the subcache while traversing the path
    pDir = opendir(path_subcache);
    for(pFile=readdir(pDir); pFile; pFile=readdir(pDir))
        if(strcmp(hash_back, pFile->d_name) == 0)
            break;
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
    struct dirent *pFile  = NULL;
    DIR           *pDir   = NULL;
    struct stat path_stat;

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
                printf("[!] find_primecache fail\n");
                printf("[+] the cache directory is corrupted\n");
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

int main(int argc, char* argv[]){

    // char arrays for handling a url
    char url_input[PROXY_MAX_URL] = {0};
    char url_hash[PROXY_LEN_HASH] = {0};

    // char arrays for handling paths
    char path_home[PROXY_MAX_PATH]      = {0};
    char path_cache[PROXY_MAX_PATH]     = {0};
    char path_fullcache[PROXY_MAX_PATH] = {0};
    char path_log[PROXY_MAX_PATH]       = {0};

    // result for finding cache
    int res = 0;

    // counter for HIT and MISS cases
    size_t count_hit  = 0;
    size_t count_miss = 0;

    time_t time_start, time_end;

    // temporary buffer for misc
    char buf[BUFSIZ] = {0};

    // timer start
    time(&time_start);

    // set full permission for the current process.
    umask(0);

    // try getting current user's home path and concatenate cache and log paths
    if(getHomeDir(path_home) == NULL){
        printf("[!] getHomeDir fail\n");
        return EXIT_FAILURE;
    }
    snprintf(path_cache, PROXY_MAX_PATH, "%s/cache", path_home);
    snprintf(path_log, PROXY_MAX_PATH, "%s/logfile/logfile.txt", path_home);

    // receive an input till the input is 'bye'
    while(1){
        printf("input url> ");
        scanf("%s", url_input);

        // if input is 'bye' then break loop
        if(strcmp("bye", url_input) == 0){
            break;
        }

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
                write_log(path_log, "[Hit]", url_hash, true);
                write_log(path_log, "[Hit]", url_input, false);
                break;

            // If the result is MISS case,
            case PROXY_MISS:
                count_miss += 1;
                write_log(path_fullcache, "[TEST]", url_input, true);
                write_log(path_log, "[Miss]",url_input, true);
                break;

            default:
                break;
        }
    }

    // timer end
    time(&time_end);

    // make a string for terminating the log and write it
    snprintf(buf, BUFSIZ, " run time: %d sec. #request hit : %lu, miss : %lu", (int)difftime(time_end, time_start), count_hit, count_miss);
    write_log(path_log, "[Terminated]", buf, false);

    return EXIT_SUCCESS;
}
