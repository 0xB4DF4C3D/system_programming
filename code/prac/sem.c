#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

void repeat(int semid);
void p(int semid);
void v(int semid);

void repeat(int semid){
    srand((unsigned int)getpid());
    p(semid);
    printf("%d process is using tool\n", getpid());
    sleep(rand()%5);
    printf("%d processs is returning tool\n", getpid());
    v(semid);

    exit(0);
}

void p(int semid){
    struct sembuf pbuf;
    pbuf.sem_num = 0;
    pbuf.sem_op = -1;
    pbuf.sem_flg = SEM_UNDO;

    if(semop(semid, &pbuf, 1) == -1){
        perror("p: semop failed");
        exit(1);
    }
}

void v(int semid){
    struct sembuf vbuf;
    vbuf.sem_num = 0;
    vbuf.sem_op = 1;
    vbuf.sem_flg = SEM_UNDO;
    if(semop(semid, &vbuf, 1) == -1){
        perror("v : semop failed");
        exit(1);
    }
}

int main() {
    int semid, i;

    union semun{
        int val;
        struct semid_ds *buf;
        unsigned short int *array;
    } arg;

    if((semid = semget((key_t)1234, 1, IPC_CREAT|0666)) == -1){
        perror("semget failed");
        exit(1);
    }

    arg.val = 1;
    if(semctl(semid, 0, SETVAL, arg) == -1){
        perror("semctl failed");
        exit(1);
    }

    for(i=0; i<3; i++){
        if(!fork())
            repeat(semid);
    }

    sleep(10);

    if(semctl(semid, 0, IPC_RMID, arg) == -1){
        perror("semctl failed");
        exit(1);
    }

    return EXIT_SUCCESS;
}
