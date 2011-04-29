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
        
        explicit ActiveMsg(Delegate* delegate) 
        : c_(false)
        , delegate_(delegate)
        , destroyed_(false)
        {
            if(!RunLoop::CurrentLoop) {
                throw std::exception();
            }
            
            // The runloop we are working on
            rl_ = RunLoop::CurrentLoop.get();            
            RunLoop::CurrentLoop->registerMsg( this );
        }
		bool is_destroyed()const { return destroyed_; }
        
        ~ActiveMsg() {
            RunLoop::CurrentLoop->dequeue( this );
            RunLoop::CurrentLoop->deregisterMsg( this );
        }
        
        inline void onCall()
        {
            delegate_->onCall(*this);
        }
    
    private:
        friend class RunLoop;
        friend class ActiveCall;
        friend class ThreadWithMessage;
        
        CondVar                     c_;
        Delegate                  * delegate_;
        RunLoop                   * rl_;
        bool                        destroyed_;
        
        inline void call() 
        {
            rl_->queue( this );
        }
    };
    
    class ActiveCall
    {
    private:
        ActiveMsg&  am_;
        CondLock    lock_;
        
    public:
        explicit ActiveCall(ActiveMsg & am)
        : am_(am)
        , lock_(am_.c_, 0) // 1 - between call and onCall, 0 - free or destroyed
        {
            if( am_.destroyed_ )
                throw std::exception();
        }
        void call()
        {
            lock_.set(true);
            am_.call();
        }
    };
    
    class ThreadWithMessage
    {
    private:
        Thread      thread_;
        
    public:  
        ActiveMsg   msg_;
        
        explicit ThreadWithMessage(Runnable& r, ActiveMsg::Delegate* delegate)
        : thread_(r)
        , msg_(delegate)
        { }
        
        void start()
        {
            return thread_.start();
        }
        
        void requestTermination()
        {
            CondLock lock(msg_.c_);        
            msg_.destroyed_ = true;
            lock.set(0);
        }
        
        void waitTermination()
        {
            return thread_.waitTermination();
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __ACTIVE_MSG_H__