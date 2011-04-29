// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "RunLoop.h"
#include "ActiveMsg.h"
#include "Condition.h"
#include "Timer.h"
#include "LogUtil.h"

ThreadLocalStorage<RunLoop> RunLoop::CurrentLoop;

void RunLoop::run()
{
    running = true;
    
    while( running ) {
        processedMsg = 0;
        if( !timers.empty() )
        {
            double ft = timers.front()->firetime();
            CondLock lock(c, true, ft + 0.001); // :)

            if(!queue.empty()) {
                processedMsg = queue.front();
                queue.pop_front();
            }
            lock.set(!queue.empty());
        }
        else
        {
            CondLock lock(c, true);
            if(!queue.empty()) {
                processedMsg = queue.front();
                queue.pop_front();
            }
            lock.set(!queue.empty());
        }
        if( processedMsg )
            processedMsg->onCall();
        if( processedMsg )
        {
            CondLock l(processedMsg->c);
            l.set(false);
        }
        while(!timers.empty())
        {
            double ct = Timing::currentTime();
            double ft = timers.front()->firetime();
            if( ct < ft )
                break;
            Timer * t = timers.front();
            timers.pop_front();
            t->process();
        }
        
        // Now when we have processed everything pending
        // check whether something new is scheduled.. if not - terminate
        if(timers.empty() && msgCnt == 0)
            running = false;
    } 
    
    wLog("Runloop terminating..");
}

void RunLoop::enqueue( ActiveMsg *msg )
{         
    CondLock lock(c);
    queue.push_back(msg); 
    lock.set(!queue.empty());
}

void RunLoop::dequeue( ActiveMsg *msg ) 
{
    if( msg == processedMsg )
        processedMsg = 0;
    
    queue.erase( std::remove(queue.begin(), 
                              queue.end(), 
                              msg), queue.end() );
}

void RunLoop::enqueue( Timer* timer ) 
{
    timers.insert( std::lower_bound(timers.begin(),
                                     timers.end(), 
                                     timer, PtrCmp<Timer>()), timer);
}

void RunLoop::dequeue( Timer *timer ) 
{
    timers.erase( std::remove(timers.begin(), 
                               timers.end(), 
                               timer ), timers.end());
}