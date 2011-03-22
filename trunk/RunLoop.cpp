// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "RunLoop.h"
#include "ActiveMsg.h"
#include "Condition.h"
#include "Timer.h"

ThreadLocalStorage<RunLoop> RunLoop::CurrentLoop;

void RunLoop::run()
{
    running_ = true;
    
    while( running_ ) {
        processedMsg_ = 0;
        if( !timers_.empty() )
        {
            double ft = timers_.front()->firetime();
            CondLock lock(c_, true, ft + 0.001); // :)

            if(!queue_.empty()) {
                processedMsg_ = queue_.front();
                queue_.pop_front();
            }
            lock.set(!queue_.empty());
        }
        else
        {
            CondLock lock(c_, true);
            if(!queue_.empty()) {
                processedMsg_ = queue_.front();
                queue_.pop_front();
            }
            lock.set(!queue_.empty());
        }
        if( processedMsg_ )
            processedMsg_->onCall();
        if( processedMsg_ )
        {
            CondLock l(processedMsg_->c_);
            l.set(false);
        }
        while(!timers_.empty())
        {
            double ct = Timing::currentTime();
            double ft = timers_.front()->firetime();
            if( ct < ft )
                break;
            Timer * t = timers_.front();
            timers_.pop_front();
            t->process();
        }
    } 
}

void RunLoop::queue( ActiveMsg *msg )
{         
    CondLock lock(c_);
    queue_.push_back(msg); 
    lock.set(!queue_.empty());
}

void RunLoop::dequeue( ActiveMsg *msg ) 
{
    if( msg == processedMsg_ )
        processedMsg_ = 0;
    
    queue_.erase( std::remove(queue_.begin(), 
                              queue_.end(), 
                              msg), queue_.end() );
}

void RunLoop::queue( Timer* timer ) 
{
    timers_.insert( std::lower_bound(timers_.begin(),
                                     timers_.end(), 
                                     timer, PtrCmp<Timer>()), timer);
}

void RunLoop::dequeue( Timer *timer ) 
{
    timers_.erase( std::remove(timers_.begin(), 
                               timers_.end(), 
                               timer ), timers_.end());
}