// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __THREAD_H__
#define __THREAD_H__

#include "Threads.h"
#include <string>

namespace crap {
    
    class Runnable
    {
    public:
        virtual void run() = 0;
        virtual ~Runnable() {}
    };
    
    class Thread 
    {	
    private:
        Threads::thread_type    t_;
        Runnable&               r_;
        
        void execute();
        
        static void* run(void*);
    public:
        explicit Thread(Runnable& r);
        virtual ~Thread();
        
        class ThreadException 
        : public std::exception 
        {
        private:
            std::string se_;
            int ev_;
            
        public:
            ThreadException(int v, std::string s)
            : se_(s)
            , ev_(v)
            { };
            
            ~ThreadException() throw() { };
            
            virtual const char* what() const throw() 
            {
                return se_.c_str();		
            }
        };
        
        void start();                   // Starts the thread
        void waitTermination();         // Join the thread
    };
} // namespace crap
using namespace crap;

#endif // __THREAD_H__
