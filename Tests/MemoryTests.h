// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __MEMORY_TESTS_H__
#define __MEMORY_TESTS_H__

#include <vector>
#include "SharedPtr.h"

struct MemoryTester 
{
    static bool alive;
    
    MemoryTester()
    {
        MemoryTester::alive = true;
    }
    
    ~MemoryTester() 
    {
        MemoryTester::alive = false;
    }
};
bool MemoryTester::alive;

SUITE(Memory);

// SharedPtr simple tests
////////////////////////////////////////////////////////////////////
TEST(SharedPtr, Memory) {
    
    {
        SharedPtr<MemoryTester> p( new MemoryTester() );
        rassert(MemoryTester::alive == true);
    }
    
    rassert(MemoryTester::alive == false);
};

// WeakPtr simple tests
////////////////////////////////////////////////////////////////////
TEST(WeakPtr, Memory) {
    
    typedef WeakPtr<MemoryTester> MTWeakPtr;    
    std::vector<MTWeakPtr> wpv;
    
    {
        SharedPtr<MemoryTester> p( new MemoryTester() );
        wpv.push_back( MTWeakPtr( p ) );
        
        rassert(MemoryTester::alive == true);        
        rassert(wpv.front().lock() == true);
        rassert(MemoryTester::alive == true);
    }

    rassert(wpv.front().lock() == false);
    rassert(MemoryTester::alive == false);
};

#endif // __MEMORY_TESTS_H__