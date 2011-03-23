// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "Threads.h"
#include "Timing.h"

#ifdef __MACH__

#include <sys/time.h>
#include <math.h>
#include <errno.h>

// Implementation is GCC-specific
namespace crap {
namespace Threads {
    /* Interlocks */
	counter interlocked_increment(counter* c) {
        return __sync_add_and_fetch(c, 1);
    }
    
	counter interlocked_decrement(counter* c) {
        return __sync_sub_and_fetch(c, 1);
    }
    
    /* Thread */
    int createThread(thread_type& t, thread_attr_type* a, void *(*r)(void*), void* s) {
        return pthread_create(&t, a, r, s);
    }

    void exitThread(void* a) {
        pthread_exit(a);
    }
    
    void cancelThread(thread_type& t) {
        pthread_cancel(t);
    }
    
    void killThread(thread_type& t) {
        pthread_kill(t,3);
    }
    
    int joinThread(thread_type& t, void** a) {
        return pthread_join(t, a);
    }
    
    /* Mutex */
    int createMutex(mutex_type& m, const mutex_attr_type* a) {
        return pthread_mutex_init(&m, a);
    }
    
    int destroyMutex(mutex_type& m) {
        return pthread_mutex_destroy(&m);
    }
    
    int lock(mutex_type& m) {
        return pthread_mutex_lock(&m);
    }
    
    int unLock(mutex_type& m) {
        return pthread_mutex_unlock(&m);
    }    
    
    /* Condition */
    int createCondition(condition_type& c, const condition_attr_type* a) {
        return pthread_cond_init(&c, a);
    }
    
    int destroyCondition(condition_type& c) {
        return pthread_cond_destroy(&c);
    }
    
    void wait(condition_type& c, mutex_type& m) {
        pthread_cond_wait(&c, &m);
    }
    
    bool wait(condition_type& c, mutex_type& m, double time) {
        long sec = floor(time);
        long nano = (time-sec)*1E9;
        struct timespec ts = { sec, nano };

        return pthread_cond_timedwait(&c, &m, &ts) == ETIMEDOUT;
    }
    
    int signal(condition_type& c) {
        return pthread_cond_signal(&c);
    }
    
    int broadcast(condition_type& c) {
        return pthread_cond_broadcast(&c);
    }
} // namespace Threads
} // namespace crap
using namespace crap;

namespace crap {
namespace Timing {
    
    double currentTime()
    {
        struct timeval start;        
        gettimeofday(&start, NULL);
        return double(start.tv_sec) + start.tv_usec * 1E-6;
    }
} // namespace Timing
} // namespace crap
using namespace crap;


#endif // __POSIX__