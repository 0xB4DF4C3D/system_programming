#include <stdio.h>
#include <time.h>

int main(void){

    time_t now;
    struct tm *ltp, *gtp;

    time(&now);
    ltp = localtime(&now);

    printf("ctime(local)     : %s\n", ctime(&now));
    printf("localtime(local) : %02d:%02d:%02d\n", ltp->tm_hour, ltp->tm_min, ltp->tm_sec);

    gtp = gmtime(&now);
    printf("asctime(GMT)     : %s\n", asctime(gtp));
    printf("gmtime (GMT)     : %02d:%02d:%02d\n", gtp->tm_hour, gtp->tm_min, gtp->tm_sec);
    printf("mktime           : %d\n", (int)mktime(gtp));

    return 0;
}
