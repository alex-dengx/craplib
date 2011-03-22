// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __PTR_CMP_H__
#define __PTR_CMP_H__

template<typename T>
struct PtrCmp
{
    bool operator()(const T* a, const T* b) const 
    {
        return *a<*b;
    }
};

#endif // __PTR_CMP_H__