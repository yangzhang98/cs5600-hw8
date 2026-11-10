#include <stdio.h>
#include <unistd.h>
#include "common_threads.h"

// If done correctly, each child should print their "before" message
// before either prints their "after" message. Test by adding sleep(1)
// calls in various locations.

sem_t s1, s2;   // s1: child 1 has arrived, s2: child 2 has arrived

void *child_1(void *arg) {
    sleep(1);                       // child 1 arrives late
    printf("child 1: before\n");
    Sem_post(&s1);                  // tell child 2 I'm here
    Sem_wait(&s2);                  // wait until child 2 is here
    printf("child 1: after\n");
    return NULL;
}

void *child_2(void *arg) {
    printf("child 2: before\n");
    Sem_post(&s2);                  // tell child 1 I'm here
    Sem_wait(&s1);                  // wait until child 1 is here
    printf("child 2: after\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    printf("parent: begin\n");
    Sem_init(&s1, 0);
    Sem_init(&s2, 0);
    Pthread_create(&p1, NULL, child_1, NULL);
    Pthread_create(&p2, NULL, child_2, NULL);
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("parent: end\n");
    return 0;
}
