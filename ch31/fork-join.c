#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

sem_t s; 

void *child(void *arg) {
    sleep(1);               // make sure parent really waits
    printf("child\n");
    Sem_post(&s);           // signal: child is done
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    printf("parent: begin\n");
    Sem_init(&s, 0);        // 0: nothing to give away at the start
    Pthread_create(&p, NULL, child, NULL);
    Sem_wait(&s);           // wait for child
    printf("parent: end\n");
    return 0;
}
