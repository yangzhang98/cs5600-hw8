#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

//
// No-starve mutex using semaphores (Morris's algorithm, from Downey's
// "Little Book of Semaphores" 4.3).
//
// Idea: two "waiting rooms". Threads gather in room 1. When room 1 is
// empty, the whole batch moves to room 2 and goes through the critical
// section one by one. While a batch is being served, turnstile t1 stays
// closed, so new arrivals wait in room 1 for the NEXT batch. So a thread
// can be passed by at most one batch -> bounded waiting -> no starvation.
//

typedef struct __ns_mutex_t {
    int room1;          // # threads in waiting room 1
    int room2;          // # threads in waiting room 2
    sem_t mutex;        // protects room1
    sem_t t1;           // turnstile into room 2 (open = 1)
    sem_t t2;           // turnstile into critical section (closed = 0)
} ns_mutex_t;

void ns_mutex_init(ns_mutex_t *m) {
    m->room1 = 0;
    m->room2 = 0;
    Sem_init(&m->mutex, 1);
    Sem_init(&m->t1, 1);
    Sem_init(&m->t2, 0);
}

void ns_mutex_acquire(ns_mutex_t *m) {
    Sem_wait(&m->mutex);
    m->room1++;
    Sem_post(&m->mutex);

    Sem_wait(&m->t1);           // enter room 2 (room2 is protected by t1)
    m->room2++;
    Sem_wait(&m->mutex);
    m->room1--;
    if (m->room1 == 0) {        // last of this batch: close t1, open t2
        Sem_post(&m->mutex);
        Sem_post(&m->t2);
    } else {                    // let the next one in room 1 through
        Sem_post(&m->mutex);
        Sem_post(&m->t1);
    }

    Sem_wait(&m->t2);           // go into the critical section
    m->room2--;                 // (room2 is protected by t2 here)
}

void ns_mutex_release(ns_mutex_t *m) {
    // braces needed: Sem_post is a macro that already ends in ';'
    if (m->room2 == 0) {        // batch done: reopen t1 for next batch
        Sem_post(&m->t1);
    } else {                    // pass to next thread in room 2
        Sem_post(&m->t2);
    }
}

//
// Test: N threads each grab the lock `loops` times as fast as possible.
// We count how many OTHER acquisitions happen between a thread asking for
// the lock and getting it ("bypasses"). If the lock does not starve
// anyone, the worst case stays small (about N), no matter how long we run.
//

ns_mutex_t m;
int loops;
int num_threads;
long total = 0;                 // total acquisitions so far (in crit sect)

typedef struct { int id; long max_bypass; long count; } tinfo_t;

void *worker(void *arg) {
    tinfo_t *t = (tinfo_t *) arg;
    for (int i = 0; i < loops; i++) {
        long ticket = __atomic_load_n(&total, __ATOMIC_SEQ_CST);
        ns_mutex_acquire(&m);
        long bypass = total - ticket;       // how many got in before me
        if (bypass > t->max_bypass) t->max_bypass = bypass;
        total++;
        t->count++;
        ns_mutex_release(&m);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    num_threads = (argc > 1) ? atoi(argv[1]) : 4;
    loops       = (argc > 2) ? atoi(argv[2]) : 100000;

    printf("parent: begin (%d threads x %d loops)\n", num_threads, loops);
    ns_mutex_init(&m);

    pthread_t p[num_threads];
    tinfo_t t[num_threads];
    for (int i = 0; i < num_threads; i++) {
        t[i].id = i; t[i].max_bypass = 0; t[i].count = 0;
        Pthread_create(&p[i], NULL, worker, &t[i]);
    }
    for (int i = 0; i < num_threads; i++)
        Pthread_join(p[i], NULL);

    for (int i = 0; i < num_threads; i++)
        printf("thread %d: acquired %ld times, worst wait = %ld other acquisitions\n",
               t[i].id, t[i].count, t[i].max_bypass);
    printf("parent: end (total = %ld, expected %ld)\n",
           total, (long) num_threads * loops);
    return 0;
}
