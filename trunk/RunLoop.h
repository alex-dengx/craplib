// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __RUN_LOOP_H__
#define __RUN_LOOP_H__

#include <deque>

#include "Condition.h"
#include "SharedPtr.h"
#include "Timer.h"
#include "Compare.h"
#include "Threads.h"
#include "TLS.h"

namespace crap {
    
    class ActiveMsg;
    
    class RunLoop
    {
    private:
        friend class ActiveMsg;
        friend class Timer;
        
        typedef std::deque<ActiveMsg*>      MsgContainer;    
        typedef std::deque<Timer*>          TimerContainer;
        
        bool            running_;
        MsgContainer    queue_;
        ActiveMsg       * processedMsg_;
        CondVar         c_;
        
        TimerContainer   timers_;        
        Threads::counter msgCnt_;
        
        static ThreadLocalStorage<RunLoop> CurrentLoop;
        
        void queue( ActiveMsg* msg ); // Called from 2nd thread
        void dequeue( ActiveMsg* msg );
        
        void queue( Timer* timer );
        void dequeue( Timer* timer );
        
        // Maintain counters
        void registerMsg( ActiveMsg* msg )
        {
            Threads::interlocked_increment(&msgCnt_);
        }
        
        void deregisterMsg( ActiveMsg* msg )
        {
            Threads::interlocked_decrement(&msgCnt_);
        }
        
    public:
        
        RunLoop()
        : c_(false)
        , running_(false)
        , processedMsg_(0)
        , msgCnt_(0)
        { 
            if(!RunLoop::CurrentLoop) {
                RunLoop::CurrentLoop = this;
            } else {
                throw std::exception();
            }
        }
        
        ~RunLoop()
        {
            RunLoop::CurrentLoop = NULL;
        }
        
        void run();
        
        void stop()
        {
            running_ = false;
        }
    };
    
} // namespace crap
using namespace crap;
    
#endif // __RUN_LOOP_H__