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
        
        TimerContainer  timers_;
        
        static ThreadLocalStorage<RunLoop> CurrentLoop;
        
        void queue( ActiveMsg* msg ); // Called from 2nd thread
        void dequeue( ActiveMsg* msg );
        
        void queue( Timer* timer );
        void dequeue( Timer* timer );
        
    public:
        
        RunLoop()
        : c_(false)
        , running_(false)
        , processedMsg_(0)
        { 
            if(!RunLoop::CurrentLoop) {
                RunLoop::CurrentLoop = this;
            } else {
                throw std::exception();
            }
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