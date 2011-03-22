// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "Thread.h"
#include <errno.h>

using namespace std;

Thread::Thread(Runnable& r)
: r_(r), t_(0)
{ }

Thread::~Thread()
{ 
//    assert( t_ == 0 );
}

void* Thread::run(void* arg) {
	Thread *thread = (Thread*)arg;
	thread->execute();
	return NULL;
}

void Thread::execute() {    
    try {        
        r_.run();
    }
    catch(...)
    {
        Threads::exitThread(NULL);
        throw;
    }
}

void Thread::start() {
    if( t_ != 0 )
        throw new ThreadException(0, "must wait before new start.");

	// Create new thread and invoke the run method inside the new context
	int ret = Threads::createThread(t_, NULL, Thread::run, this);	

	switch(ret) {
		case EAGAIN:
				throw new ThreadException(ret, "The system lacked the necessary resources to create another thread, \n \
											 	or the system-imposed limit on the total number of threads in a process would be exceeded.");
			break;
		case EINVAL:	/* Should never happen to us */
				throw new ThreadException(ret, "The value specified by attr is invalid.");
			break;
		case EPERM:
				throw new ThreadException(ret, "The caller does not have appropriate pormission to set the required \
											 	scheduling parameters or scheduling policy.");	
			break;
	}
}

void Thread::waitTermination() 
{
    Threads::joinThread(t_, NULL);
    t_ = 0;
}

