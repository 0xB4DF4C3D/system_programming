/**
 * System Programming Assignment #1-1 (proxy server)
 * @file proxy_cache.c
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Wed Mar 28 00:08:27 KST 2018
 * @warning Implemented for only a MISS case yet.
 */

#include <dirent.h>     // format of directory entries
#include <errno.h>      // system error numbers
#include <fcntl.h>      // file control options
#include <pwd.h>        // password structure
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
PROXY_LEN_PREFIX = 3,    ///< A length of the front hash.
PROXY_LEN_HASH = 41,     ///< A length of the full hash(in the SHA1).
PROXY_MAX_URL = 2048,    ///< A maximum length of an url.
PROXY_MAX_PATH = 4096    ///< A maximum length of a path.
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
 * Find subcache(a back part of the hashed URL)
 * @param path_subcache A const char pointer to the path containing subcaches.
 * @param hash_back A const char pointer to be used as a subcache name.
 * @todo Implement the HIT case.
 * @return [int] HIT:0, MISS:1, FAIL:-1
 */
int find_subcache(const char *path_subcache, const char *hash_back){
    struct dirent *pFile = NULL;
    DIR           *pDir  = NULL;
    FILE          *fp    = NULL;

    char test_msg[256] = "This is a test message.\n";   // just dummy data

    char   *path_fullcache      = NULL;
    size_t  path_fullcache_size = 0;

    // find the subcache while traversing the path
    pDir = opendir(path_subcache);
    for(pFile=readdir(pDir); pFile; pFile=readdir(pDir))
        if(strcmp(hash_back, pFile->d_name) == 0)
            break;
    closedir(pDir);

    if(pFile == NULL){  // for MISS case, should create a file to the path of subcache
        // make the full path of the subcache file
        path_fullcache_size = sizeof(char)*strlen(path_subcache) + sizeof(char)*strlen(hash_back) + 8; // 8 for a safety
        path_fullcache = (char*)malloc(path_fullcache_size);
        snprintf(path_fullcache, path_fullcache_size, "%s/%s", path_subcache, hash_back);

        // try creating a file which contatins a dummy data to the path of subcache
        fp = fopen(path_fullcache, "w");
        if(fp == NULL){
            printf("[!] %s : %s\n", path_fullcache, strerror(errno));
            return -1;
        }
        fwrite(test_msg, 1, sizeof(test_msg), fp);
        fclose(fp);

        free(path_fullcache);
        return EXIT_FAILURE;
    }else{              // for HIT case
        // TODO
        return EXIT_SUCCESS;
    }
}

/**
 * Find primecache(A front part of the hashed URL).
 * @param path_primecache A const char pointer to the path containing primecaches.
 * @param hash_full A const char pointer to be used as a part of primecache name.
 * @todo Implement the HIT case.
 * @return [int] HIT:0, MISS:1, FAIL:-1
 */
int find_primecache(const char *path_primecache, const char *hash_full){
    struct dirent *pFile  = NULL;
    DIR           *pDir   = NULL;
    struct stat path_stat;

    char hash_front[PROXY_LEN_PREFIX + 1]                 = {0};
    char hash_back[PROXY_LEN_HASH - PROXY_LEN_PREFIX + 1] = {0};

    char *path_subcache       = NULL;
    size_t path_subcache_size = 0;

    int result = 0;

    // divide the entire hash into two parts
    strncpy(hash_front, hash_full, PROXY_LEN_PREFIX);
    strncpy(hash_back, hash_full + PROXY_LEN_PREFIX, PROXY_LEN_HASH - PROXY_LEN_PREFIX);

    // check whether the path of primecache exist or not
    pDir = opendir(path_primecache);
    if(pDir == NULL){   // if not exist,
        // create that path
        mkdir(path_primecache, S_IRWXU | S_IRWXG | S_IRWXO);

        pDir = opendir(path_primecache); // and try opening it again
    }
    
    // make the full path of the directory which contains subcaches
    path_subcache_size = sizeof(char)*strlen(path_primecache) + PROXY_LEN_PREFIX + 8;
    path_subcache = (char*)malloc(path_subcache_size);
    snprintf(path_subcache, path_subcache_size, "%s/%s", path_primecache, hash_front);

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
                return -1;
            }
        }
    }
    closedir(pDir);
    
    // if there isn't the path of subcache, then create that path
    if(pFile == NULL)
        mkdir(path_subcache, S_IRWXU | S_IRWXG | S_IRWXO);
    
    // find the subcache in the path of the current primecache
    result = find_subcache(path_subcache, hash_back);
    free(path_subcache);
    
    return result;
}

int main(int argc, char* argv[]){

    char url_input[PROXY_MAX_URL];
    char url_hash[PROXY_LEN_HASH];
    char path_cache[PROXY_MAX_PATH];

    // set full permission for the current process.
    umask(0);

    // try getting current user's home path and concatenate a cache path
    if(getHomeDir(path_cache) == NULL){
        printf("[!] getHomeDir fail\n");
        return EXIT_FAILURE;
    }
    strcat(path_cache, "/cache/");

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
        find_primecache(path_cache, url_hash);
    }

    return EXIT_SUCCESS;
}
