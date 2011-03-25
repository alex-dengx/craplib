// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __STATIC_REF_COUNTED_H__
#define __STATIC_REF_COUNTED_H__

namespace crap {
    
    template<class T> 
    class StaticRefCounted
    {
    private:
        static int ref_counter;
        static T * sta;
        
    public:
        static T & statics()
        {
            return *sta;
        }
        
        StaticRefCounted()
        {
            if( ++ref_counter == 1 )
            {
                sta = new T;
            }
        }
        
        StaticRefCounted(const StaticRefCounted & src)
        {
            ++ref_counter;
        }
        
        ~StaticRefCounted()
        {
            if( --ref_counter == 0 )
            {
                delete sta;
                sta = 0;
            }
        }
    };
    
    
    template<class T>
    int StaticRefCounted<T>::ref_counter = 0;
    
    template<class T>
    T * StaticRefCounted<T>::sta = 0;
    
}
using namespace crap;
    
#endif // __STATIC_REF_COUNTED_H__