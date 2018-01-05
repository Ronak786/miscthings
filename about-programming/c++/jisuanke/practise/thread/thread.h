#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <string>
#include <cstring>
#include <cerrno>
#include <functional>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

class Mthread {
    private:
        pthread_t threadId;  //the thread id
        void* threadResult; //the thread return result
        std::string errorMsg;    //set if error occurred
        
        /*
         * this used as pthread's create func
         * static, not have a this pointer,
         * so can be passed to pthread_create
         * You set your arg in class,
         * get your ret value in class
         */
        static void *entryFunc(void *This) {
            ((Mthread*)This)->start_routine();
            return nullptr;
        }
    public:
        Mthread() = default;
        Mthread(Mthread & ) = default;
        virtual ~Mthread() = default;
        // called by base class, can not be overrided
        void run();
        
        // this should be implemented by every sub class
        virtual void  start_routine() {
            return;
        } 

        // do the join, set void* result pointer
        void join();

        void detach();

        //should check after run, join, detach, cancel;
        bool hasError();

/*
        // input: Mmutex class ref
        void lock(Mmutex& mtx) final {}

        // input: Mmutex class ref
        void unlock(Mmutex& mtx) final{}

        void cond_wait(Mmutex& mtx, Mcond& cond) {}

        // in condition ,you should set how many expected be wakedup, so wake will call signal or broadcast
        void cond_wake(Mcond& cond){}
*/
};

/*
 * below all should be moved into a library file!!
 */
void Mthread::run() {
    /*
     * run the start_routine
     * set threadid, if error , set errorMsg
     */
    int ret = pthread_create(&threadId, nullptr, entryFunc, this);
    if (ret != 0) {
//        errorMsg = std::strerror(std::errno);
        errorMsg = std::strerror(errno);
    }
}

void Mthread::join() {
    int ret = pthread_join(threadId, nullptr);
    if (ret != 0) {
//        errorMsg = std::strerror(std::errno);
        errorMsg = std::strerror(errno);
    }
}

void Mthread::detach() {
    int ret = pthread_detach(threadId);
    if (ret != 0) {
        errorMsg = std::strerror(errno);
    }
}

bool Mthread::hasError() {
    return errorMsg != "";
}

#ifdef __cplusplus
}
#endif

#endif
