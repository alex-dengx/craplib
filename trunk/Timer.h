// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __TIMER_H__
#define __TIMER_H__

#include "Timing.h"

namespace crap {
    
    class RunLoop;
    
    class Timer;
    class TimerDelegate
    {
        friend class Timer;
        virtual void onTimer(const Timer& timer) = 0;
    };
    
    /**
     * A generic timer
     */
    class Timer
    {
    private:
        friend class RunLoop;
        
        double          fire_;
        TimerDelegate&  delegate_;
        
        void process(); // Called by RunLoop    
        
        double firetime() const 
        {
            return fire_;
        }
    public:
        
        Timer(TimerDelegate& delegate)
        : fire_( 0 )
        , delegate_(delegate)
        { }
        
        ~Timer();
        
        void set(double time);   // Schedule this timer on the RunLoop
        void cancel();
        
        bool operator < ( const Timer& other ) const
        {
            return fire_ < other.fire_;
        }
    };
    
} // namespace crap
using namespace crap;

#endif // __TIMER_H__