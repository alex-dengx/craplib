// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __DATA_TESTS_H__
#define __DATA_TESTS_H__

#include "Data.h"

SUITE(Data)

// Data simple tests
////////////////////////////////////////////////////////////////////
TEST(Simple, Data) {
    Data d0;
    rassert(d0.empty()==true);
    
    Data d1(10);
    rassert(d1.empty()==false);
    
    d1.fill(0);
    rassert(d1.get_data()[0] == 0);
    d1.fill(1);
    rassert(d1.get_data()[0] == 1);
    
    rassert(d0 != d1);
    d0 = d1;
    rassert(d0 == d1);
}

TEST(Assignment, Data) {
    Data d0(10, "hello");
    Data d1(d0);
}


#endif // __MEMORY_TESTS_H__
