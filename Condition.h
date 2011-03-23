// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __CONDITION_H__
#define __CONDITION_H__

#include "Mutex.h"
#include "Threads.h"

namespace crap {
    
    /**
     * Condition variable
     */
    class CondVar 
    {
    private:
        Mutex                    m_;
        Threads::condition_type  c_;
        bool                     d_;
        
        friend class CondLock;
        CondVar(const CondVar& other);
        
    public:
        explicit CondVar(bool d)
        : d_(d)
        {
            Threads::createCondition(c_, NULL);
        }
        
        ~CondVar()
        {
            Threads::destroyCondition(c_);
        }
    };
    
    /**
     * Locks on condition
     */
    class CondLock
    {
    private:
        CondVar&    c_;
        Lock        lock_;
        
        
        inline void wait(bool val)
        {
            while(c_.d_ != val) {
                Threads::wait(c_.c_, c_.m_.m_);
            }
        }
        
        inline void wait(bool val, double time)
        {
            while(c_.d_ != val) {
                if( Threads::wait(c_.c_, c_.m_.m_, time) )
                    return;
            }
        }
        
    public:
        CondLock(CondVar& cvar, bool val)
        : c_(cvar)
        , lock_(c_.m_)
        {  
            wait(val);
        }
        
        CondLock(CondVar& cvar, bool val, double time)
        : c_(cvar)
        , lock_(c_.m_)
        {  
            wait(val, time);
        }
        
        CondLock(CondVar& cvar) 
        : c_(cvar)
        , lock_(c_.m_)
        { }
        
        inline void set(bool val) 
        { 
            c_.d_ = val;
            Threads::signal(c_.c_);     
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __CONDITION_H__