// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __TIMER_H__
#define __TIMER_H__

#include "Timing.h"

namespace crap {
    
    class RunLoop;
    
    /**
     * A generic timer
     */
    class Timer
    {
    public:
        
        struct Delegate
        {
            virtual void onTimer(const Timer& timer) = 0;
            virtual ~Delegate() {};
        };
        
        Timer(Delegate* del)
        : fire( 0 )
        , delegate(del)
        { }
        
        ~Timer();
        
        void set(double time);   // Schedule this timer on the RunLoop
        void cancel();
        
        bool operator < ( const Timer& other ) const
        {
            return fire < other.fire;
        }
        
    private:
        friend class RunLoop;
        
        double          fire;
        Delegate        * delegate;
        
        void process(); // Called by RunLoop    
        
        double firetime() const 
        {
            return fire;
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __TIMER_H__