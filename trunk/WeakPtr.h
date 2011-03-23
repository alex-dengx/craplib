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
            Threads::counter    sc_;	
            Threads::counter    wc_;	
        };
        
        static Counters forNulls;
        
        T          * pObj_;		// Managed by SharedPtr
        Counters   * cnt_;      // Managed by WeakPtr
        
        void attach() 
        {
            Threads::interlocked_increment(&cnt_->wc_);
        }
        
        void detach() 
        {
            if( Threads::interlocked_decrement(&cnt_->wc_) == 0 ) {            
                delete cnt_;
            }
        }
        
        // Used by SharedPtr only
        WeakPtr(T* p)
        : pObj_(p)
        , cnt_( new Counters )
        {
            cnt_->sc_ = 0;
            cnt_->wc_ = 0;        
            attach();
        }
        
    public:
        WeakPtr() 
        : pObj_(NULL)
        , cnt_(&forNulls)
        {
            attach();
        }
        
        WeakPtr(const WeakPtr<T>& r)
        : pObj_(r.pObj_)
        , cnt_(r.cnt_)
        { 
            attach();
        }
        void swap(WeakPtr<T>& other)
        {
            std::swap(pObj_, other.pObj_);
            std::swap(cnt_, other.cnt_);
        }    
        WeakPtr<T>& operator= (const WeakPtr<T>& r) 
        {
            WeakPtr<T> temp(r);
            swap(temp);
            return *this;
        }
        
        WeakPtr(SharedPtr<T>& p)
        : pObj_(p.ptr_.pObj_)
        , cnt_(p.ptr_.cnt_)
        {
            attach();
        }
        
        ~WeakPtr() 
        {
            detach();
        }
        
        SharedPtr<T> lock() 
        { 
            Threads::counter c = Threads::interlocked_increment(&cnt_->sc_);
            SharedPtr<T> result;
            if( c != 1 )
                result = SharedPtr<T>(pObj_, cnt_);
            Threads::interlocked_decrement(&cnt_->sc_);
            
            return result; 
        }
    };
    
    template<typename T>
    typename WeakPtr<T>::Counters WeakPtr<T>::forNulls = {1,1};
    
} // namespace crap
using namespace crap;


#endif // __WEAK_PTR_H__