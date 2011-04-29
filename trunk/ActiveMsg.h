// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ACTIVE_MSG_H__
#define __ACTIVE_MSG_H__

#include "RunLoop.h"
#include "Condition.h"
#include "Thread.h"

namespace crap {
    
    class ActiveMsg 
    {
    public:
        
        struct Delegate
        {            
            virtual void onCall(const ActiveMsg& msg) = 0;
            virtual ~Delegate() {};
        };
        
        explicit ActiveMsg(Delegate* del) 
        : c(false)
        , delegate(del)
        , destroyed(false)
        {
            if(!RunLoop::CurrentLoop) {
                throw std::exception();
            }
            
            // The runloop we are working on
            rl = RunLoop::CurrentLoop.get();            
            RunLoop::CurrentLoop->registerMsg( this );
        }
		bool is_destroyed() const { return destroyed; }
        
        ~ActiveMsg() {
            RunLoop::CurrentLoop->dequeue( this );
            RunLoop::CurrentLoop->deregisterMsg( this );
        }
        
        inline void onCall()
        {
            delegate->onCall(*this);
        }
    
    private:
        friend class RunLoop;
        friend class ActiveCall;
        friend class ThreadWithMessage;
        
        CondVar                     c;
        Delegate                  * delegate;
        RunLoop                   * rl;
        bool                        destroyed;
        
        inline void call() 
        {
            rl->enqueue( this );
        }
    };
    
    class ActiveCall
    {
    private:
        ActiveMsg&  am;
        CondLock    lock;
        
    public:
        explicit ActiveCall(ActiveMsg & am)
        : am(am)
        , lock(am.c, 0) // 1 - between call and onCall, 0 - free or destroyed
        {
            if( am.destroyed )
                throw std::exception();
        }
        void call()
        {
            lock.set(true);
            am.call();
        }
    };
    
    class ThreadWithMessage
    {
    private:
        Thread      thread;
        
    public:  
        ActiveMsg   msg;
        
        explicit ThreadWithMessage(Runnable& r, ActiveMsg::Delegate* delegate)
        : thread(r)
        , msg(delegate)
        { }
        
        void start()
        {
            return thread.start();
        }
        
        void requestTermination()
        {
            CondLock lock(msg.c);
            msg.destroyed = true;
            lock.set(0);
        }
        
        void waitTermination()
        {
            return thread.waitTermination();
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __ACTIVE_MSG_H__