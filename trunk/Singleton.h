// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

namespace crap {
    
    /**
     * A minimalistic singleton template
     */
    template<class T>
    class Singleton {
    private:
        Singleton()	{ };
        ~Singleton() { };
        
    public:
        static T& instance() 
        { 
            static T instance;
            return instance; 
        };
    };
    
} // namespace crap
using namespace crap;

#endif // __SINGLETON_H__