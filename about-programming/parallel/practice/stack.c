#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void *routine(void *arg) {
    printf ("I am the routine\n");
    return (void*)20;
}

int main() {
    size_t siz;
    void *stackaddr;
    void *newstackaddr = malloc(8192*4096);
    size_t newsiz = 8192*4096;
    pthread_t pid;
    pthread_attr_t attr;
    int ret;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_attr_getstack(&attr, &stackaddr, &siz);
    printf("the stack size got is %d, addr %p\n", siz, stackaddr);
    pthread_attr_setstack(&attr, newstackaddr, newsiz);
    pthread_attr_getstack(&attr, &stackaddr, &siz);
    printf("the stack size got is %d, addr %p\n", siz, stackaddr);
    printf("sysconf pagesize %ld\n", sysconf(_SC_PAGESIZE));
    if (pthread_create(&pid, &attr, routine, NULL) != 0) {
        perror("some error");
        exit(0);
    }
    pthread_attr_destroy(&attr);
    printf("waiting...\n");

    printf("detaching... %d\n", pthread_self());
    if (pthread_detach(pthread_self())) {
        perror("detach fail");
    } else {
        perror("detach success");
    }

    if (pthread_join(pid, (void**)&ret) != 0) {
        perror("join error");
        exit(0);
    }
    printf("join success, ret: %d\n",(int)ret);
    return 0;
}


