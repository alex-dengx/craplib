// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __WEAK_PTR_H__
#define __WEAK_PTR_H__

#include "Threads.h"
#include <algorithm>

namespace crap {
    
    template <typename T>
    class SharedPtr;
    
    template <typename T>
    class WeakPtr 
    {
    private:
        friend class SharedPtr<T>;
        
        // Shared data
        struct Counters {
            Threads::counter    sc;	
            Threads::counter    wc;	
        };
        
        static Counters forNulls;
        
        T          * pObj;		// Managed by SharedPtr
        Counters   * cnt;       // Managed by WeakPtr
        
        void attach() 
        {
            Threads::interlocked_increment(&cnt->wc);
        }
        
        void detach() 
        {
            if( Threads::interlocked_decrement(&cnt->wc) == 0 ) {            
                delete cnt;
            }
        }
        
        // Used by SharedPtr only
        WeakPtr(T* p)
        : pObj(p)
        , cnt( new Counters )
        {
            cnt->sc = 0;
            cnt->wc = 0;        
            attach();
        }
        
    public:
        WeakPtr() 
        : pObj(NULL)
        , cnt(&forNulls)
        {
            attach();
        }
        
        WeakPtr(const WeakPtr<T>& r)
        : pObj(r.pObj)
        , cnt(r.cnt)
        { 
            attach();
        }
        void swap(WeakPtr<T>& other)
        {
            std::swap(pObj, other.pObj);
            std::swap(cnt, other.cnt);
        }    
        WeakPtr<T>& operator= (const WeakPtr<T>& r) 
        {
            WeakPtr<T> temp(r);
            swap(temp);
            return *this;
        }
        
        WeakPtr(SharedPtr<T>& p)
        : pObj(p.ptr.pObj)
        , cnt(p.ptr.cnt)
        {
            attach();
        }
        
        ~WeakPtr() 
        {
            detach();
        }
        
        SharedPtr<T> lock() 
        { 
            Threads::counter c = Threads::interlocked_increment(&cnt->sc);
            SharedPtr<T> result;
            if( c != 1 )
                result = SharedPtr<T>(pObj, cnt);
            Threads::interlocked_decrement(&cnt->sc);
            
            return result; 
        }
    };
    
    template<typename T>
    typename WeakPtr<T>::Counters WeakPtr<T>::forNulls = {1,1};
    
} // namespace crap
using namespace crap;


#endif // __WEAK_PTR_H__