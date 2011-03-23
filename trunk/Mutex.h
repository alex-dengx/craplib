// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __MUTEX_H__
#define __MUTEX_H__ 

#include "Threads.h"

namespace crap {
    
    class Lock;
    class CondLock;
    
    /**
     * The mutex resource
     */
    class Mutex
    {
    private:
        Threads::mutex_type     m_;
        
        friend class Lock;
        friend class CondLock;
        
    public:
        Mutex()
        {
            Threads::createMutex(m_, NULL);
        }
        
        ~Mutex()
        {
            Threads::destroyMutex(m_);
        }
    };
    
    /**
     * Locks the Mutex resource
     */
    class Lock
    {
    private:
        Mutex& m_;
        
    public:
        Lock(Mutex& m)
        : m_(m)
        {
            Threads::lock(m_.m_);
        }
        
        ~Lock()
        {
            Threads::unLock(m_.m_);
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __MUTEX_H__
