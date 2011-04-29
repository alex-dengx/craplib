// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SHARED_PTR_H__
#define __SHARED_PTR_H__

#include "WeakPtr.h"

namespace crap {
    
    template<typename T>
    class SharedPtr 
    {
        friend class WeakPtr<T>;
        
    private:    
        WeakPtr<T>  ptr;
        
        void attach() 
        {
            Threads::interlocked_increment(&ptr.cnt->sc);
        }
        
        void detach() 
        {
            if( Threads::interlocked_decrement(&ptr.cnt->sc) == 0 ) {
                delete ptr.pObj;
                ptr.pObj = 0;
            }
        }
        
        SharedPtr(T* p, typename WeakPtr<T>::Counters * c)
        {
            ptr.pObj = p;
            ptr.cnt = c;
            attach();
        }
        
    public:
        SharedPtr()
        {  
            attach();
        }
        
        explicit SharedPtr(T* p)
        : ptr(p)
        {
            attach();
        }
        
        SharedPtr(const SharedPtr<T>& r)
        : ptr(r.ptr)
        { 
            attach();
        }
        
        ~SharedPtr() 
        {
            detach();
        }
        
        bool isTheOne()const { return ptr.cnt->sc; }
        void swap(SharedPtr<T>& other)
        {
            ptr.swap(other.ptr);
        }
        
        SharedPtr<T>& operator= (const SharedPtr<T>& r) 
        {
            SharedPtr<T> temp(r);
            swap(temp);
            return *this;
        }
        
        inline T * get() const
        { 
            return ptr.pObj; 
        }
        
        inline T * operator-> () const 
        { 
            return ptr.pObj; 
        }    
        
        inline operator bool() const 
        { 
            return ptr.pObj != 0; 
        }
		
        inline bool operator < ( const SharedPtr<T>& other ) const
        {
            return ptr.pObj < other.ptr.pObj;
        }
    };
    
    template<typename T>
	inline bool operator==(const SharedPtr<T>& a, const SharedPtr<T>& b )
	{
		return a.get() == b.get();
	}
    template<typename T>
	inline bool operator==(const SharedPtr<T>& a, T * b)
	{
		return a.get() == b;
	}
    template<typename T>
	inline bool operator==(T * a, const SharedPtr<T>& b)
	{
		return a == b.get();
	}
	

} // namespace crap
using namespace crap;

#endif // __SHARED_PTR_H__