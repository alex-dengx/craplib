// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include <iostream>
#include <vector>

using namespace std;

#include "SharedPtr.h"
#include "RunLoop.h"
#include "ActiveMsg.h"
#include "AsyncFile.h"

class Main
: public Runnable
, public TimerDelegate
{
private:
    Timer   t1_, t2_;
    
    void onTimer( const Timer& t ) {
        if(&t == &t1_) {
            std::cout << "Kill all jobs..\n";
            v_.clear();
            
            t1_.set(2.0);
        }
        else if(&t == &t2_) {
            std::cout << "Start new job!\n";

            // Add job
            v_.push_back(AFPtr( new AsyncFile("test.txt") ));
            
            t2_.set(0.2);
        }
    }
    
    typedef SharedPtr<AsyncFile> AFPtr;
    typedef vector<AFPtr> AFVec;

    AFVec   v_;
    Thread  thrd_;
    
protected:
    virtual void run()
    {
        main();
    }
    
public:
    
    Main()
    : t1_(*this)
    , t2_(*this)
    , thrd_(*this)
    { 
    }
    
    void main()
    {      
        RunLoop mainLoop_; 
        
        t1_.set(2.0);
        t2_.set(0.5);
        
        mainLoop_.run();        
    }
    
    void start()
    {
        thrd_.start();
    }
};

int main (int argc, const char * argv[])
{
    
#define LOOPS 10
    Main m[LOOPS];
    for(int i=0; i<LOOPS; ++i) {
         m[i].start();   
    }
    
    Main m2;
    m2.main();
}