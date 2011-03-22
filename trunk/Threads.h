// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once 
#ifndef __THREADS_H__
#define __THREADS_H__

#ifdef __MACH__
#include <pthread.h>
#endif

namespace Threads
{

#ifdef __MACH__ // Or just POSIX
	typedef unsigned            counter;
    typedef pthread_t           thread_type;
    typedef pthread_attr_t      thread_attr_type;
    typedef pthread_mutex_t     mutex_type;
    typedef pthread_mutexattr_t mutex_attr_type;
    typedef pthread_cond_t      condition_type;
    typedef pthread_condattr_t  condition_attr_type;
    
#define __TLS__ // __thread
#endif
    
	counter interlocked_increment(counter* c);
	counter interlocked_decrement(counter* c);
    
    int createThread(thread_type&, thread_attr_type*, void *(*)(void*), void*);
    void exitThread(void*);
    void cancelThread(thread_type& t);
    void killThread(thread_type& t);
    int joinThread(thread_type&, void**);
    
    int createMutex(mutex_type& m, const mutex_attr_type* a);
    int destroyMutex(mutex_type& m);
    int tryLock(mutex_type& m);
    int lock(mutex_type& m);
    int unLock(mutex_type& m);
    
    int createCondition(condition_type& c, const condition_attr_type*);
    int destroyCondition(condition_type& c);
    void wait(condition_type& c, mutex_type& m);
    bool wait(condition_type& c, mutex_type& m, double time); // true if time reached
    int signal(condition_type& c);
    int broadcast(condition_type& c);
}

#endif // __THREADS_H__