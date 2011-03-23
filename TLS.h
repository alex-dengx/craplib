// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __TLS_H__
#define __TLS_H__

#if !defined( __MACH__ )
#error This system is not MACH. Try using __thread keyword instead of ThreadLocalStorage template.
#endif

#include <pthread.h>

namespace crap {
    
    template<typename T>
    class ThreadLocalStorage 
    {
    private:
        pthread_key_t   key_;
        
    public:
        ThreadLocalStorage()
        {
            pthread_key_create(&key_, NULL);     
        }
        
        ~ThreadLocalStorage()
        {
            pthread_key_delete(key_);
        }
        
        ThreadLocalStorage& operator =(T* p)
        {
            pthread_setspecific(key_, p);
            return *this;
        }
        
        bool operator !() 
        {
            return pthread_getspecific(key_)==NULL;
        }
        
        T* const operator ->() const
        {
            return static_cast<T*>(pthread_getspecific(key_));
        }
        
        T* const get() const
        {
            return static_cast<T*>(pthread_getspecific(key_));
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __TLS_H__