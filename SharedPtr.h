// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SHARED_PTR_H__
#define __SHARED_PTR_H__

#include "WeakPtr.h"

template<typename T>
class SharedPtr 
{
    friend class WeakPtr<T>;
    
private:    
    WeakPtr<T>  ptr_;
	
	void attach() 
    {
		Threads::interlocked_increment(&ptr_.cnt_->sc_);
    }
	
	void detach() 
    {
        if( Threads::interlocked_decrement(&ptr_.cnt_->sc_) == 0 ) {
            delete ptr_.pObj_;
            ptr_.pObj_ = 0;
        }
    }
    
    SharedPtr(T* p, typename WeakPtr<T>::Counters * c)
    {
        ptr_.pObj_ = p;
        ptr_.cnt_ = c;
        attach();
    }
    
public:
    SharedPtr()
    {  
        attach();
    }
    
    SharedPtr(T* p)
    : ptr_(p)
    {
        attach();
    }
    
    SharedPtr(const SharedPtr<T>& r)
    : ptr_(r.ptr_)
    { 
        attach();
    }
    
    ~SharedPtr() 
    {
        detach();
    }
    
    SharedPtr<T>& operator= (const SharedPtr<T>& r) 
    {
        SharedPtr<T> temp(r);
        std::swap(ptr_, temp.ptr_);
        return *this;
    }
    
    inline const T& get() const
    { 
        return *ptr_.pObj_; 
    }
    
    inline T* const operator-> () const 
    { 
        return ptr_.pObj_; 
    }    
    
    inline operator bool() const 
    { 
        return ptr_.pObj_ != 0; 
    }
    
    inline bool operator < ( const SharedPtr<T>& other ) const
    {
        return ptr_.pObj_ < other.ptr_.pObj_;
    }
};

#endif // __SHARED_PTR_H__