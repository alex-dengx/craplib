// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

//#pragma once
//#ifndef __EXCLUSIVE_GUARD_H__
//#define __EXCLUSIVE_GUARD_H__
//
//#include "Mutex.h"
//
//template<typename T>
//class ExclusiveGuard 
//{
//private:
//    T		data_;
//    Mutex	m_;			
//    
//public:
//    ExclusiveGuard() 
//    { };
//    
//    ExclusiveGuard(const T& d)
//    : data_(d) 
//    { };
//    
//    class Locker 
//    : private Mutex 
//    {
//    private:
//        ExclusiveGuard<T>& b_;
//        
//    public:
//        Locker(ExclusiveGuard<T>& pl) 
//        : b_(pl) 
//        {
//            b_.m_.lock(); 
//        }
//        
//        ~Locker() 
//        { 
//            b_.m_.unLock(); 
//        }
//        
//        T& operator *() { return b_.data_; }
//        T* operator ->() { return &b_.data_; }
//    };
//    
//    T get() 
//    {
//        Locker lk(*this);
//        return data_; // Copy data out
//    }
//    
//    void set(const T& d) 
//    {
//        Locker lk(*this);
//        data_ = d;
//    }
//};
//
//#endif // __EXCLUSIVE_GUARD_H__
