// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __RUN_LOOP_TESTS_H__
#define __RUN_LOOP_TESTS_H__

#include "RunLoop.h"
#include "AsyncFile.h"

#include <vector>

SUITE(RunLoop)


class Main
: public TimerDelegate
{
private:
    Timer   t1_, t2_, t3_;
    RunLoop runLoop_;
    
    void onTimer( const Timer& t ) {
        if(&t == &t1_) {
            wLog("Kill all jobs..");
            v_.clear();
            
            t1_.set(2.0);
        }
        else if(&t == &t2_) {
            wLog("Start new job!");
            
            // Add job
            v_.push_back(AFPtr( new AsyncFile("test.txt") ));
            
            t2_.set(0.2);
        }
        else if(&t == &t3_) {
            // Removing all active objects and timers from the runloop
            // will result in Runloop termination.
            t2_.cancel();
            t1_.cancel();
            v_.clear();
        }
    }
    
    typedef SharedPtr<AsyncFile> AFPtr;
    typedef std::vector<AFPtr> AFVec;
    
    AFVec   v_;
    
public:
    void run()
    {
        t1_.set(2.0);
        t2_.set(0.2);
        t3_.set(10.0);  // termination timer
        
        runLoop_.run();        
    }

    Main()
    : t1_(*this)
    , t2_(*this)
    , t3_(*this)
    { 
    }
};

// Minimal runloop usage (on main thread)
////////////////////////////////////////////////////////////////////
//TEST(Mini, RunLoop) {
//    RunLoop rl;
//    AsyncFile f("test.txt");
//    rl.run(); // Locks because 'f' is not deleted (doesn't deregister msgs)
//}


// Basic runloop usage with timers and active objects (on main thread)
////////////////////////////////////////////////////////////////////
TEST(Basic, RunLoop) {
    Main m;
    m.run();
}

#endif // __RUN_LOOP_TESTS_H__
