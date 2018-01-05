#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>


void *start_routine(void *arg) {
    int k = (int)arg;
    int i;

    printf ("I am in subrouting, receiving %d\n", k);
    return (void*)20;
}

int main(void) {
    pthread_t id;
    int res, kk = 100;
    void *ret;

    res = pthread_create(&id, NULL, start_routine, (void*)kk);
    if (res) {
        perror("create thread error");
        exit(1);
    }

    res = pthread_join(id, &ret);
    if (res) {
        perror("join thread error");
        exit(1);
    }

    printf("I get result from thread %d\n", (int)ret);
    return 0;
}





