/**
 * System Programming Assignment #3-1 (proxy server)
 * @file proxy_util.h
 * @author name       : Jung Dong Ho
 * @n      email      : dongho971220@gmail.com
 * @n      student id : 2016722092
 * @date Tue May 29 15:12:20 KST 2018
 */

#include <arpa/inet.h>   // for socket programming 
#include <openssl/sha.h> // to use SHA1

#include <dirent.h>      // format of directory entries
#include <errno.h>       // system error numbers
#include <fcntl.h>       // file control options
#include <libgen.h>      // definitions for pattern matching functions
#include <netdb.h>
#include <pwd.h>         // password structure
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>     // for boolean type
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>      // standard symbolic constants and types

#include <sys/stat.h>    // data returned by the stat() function
#include <sys/types.h>   // data types
#include <sys/wait.h>

/**
 * Constant values for files relevanting proxy to avoid magic number.
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

    PROXY_PORTNO      = 38078,  ///< A given port number for me.
    PROXY_HTTP_PORTNO = 80 ///< The port number for HTTP
} Proxy_constants;

char *getHomeDir(char *home);
char *getIPAddr(const char *addr);
char *sha1_hash(char *input_url, char *hashed_url);
int find_primecache(const char *path_primecache, const char *hash_full);
int find_subcache(const char *path_subcache, const char *hash_full);
int insert_delim(char *str, size_t size_max, size_t idx, char delim);
int request_dump(const char *buf, const char *url, const char *filepath);
int request_parse(const char *buf, char *url);
int write_log(const char *path, const char *header, const char *body, bool time_, bool pid_);
