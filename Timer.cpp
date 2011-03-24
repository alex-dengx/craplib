// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "Timer.h"
#include "RunLoop.h"
#include <iostream>

Timer::~Timer()
{
    if( RunLoop::CurrentLoop.get() != NULL )
        cancel();
}

void Timer::cancel()
{
    RunLoop::CurrentLoop->dequeue( this );
}

void Timer::set(double time)   // Put this timer on the RunLoop
{
    cancel();
    
    if(!RunLoop::CurrentLoop) {
        throw std::exception();
    }
    
    fire_ = Timing::currentTime() + time;
    RunLoop::CurrentLoop->queue( this );
}

void Timer::process() 
{
    delegate_.onTimer(*this);
}
