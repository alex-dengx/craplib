// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once 
#ifndef __NOCOPY_H__
#define __NOCOPY_H__

namespace crap
{

/** 
 * This class is used to clearly specify to compiler and reader, that another
 * class has no copy semantics.
 * class MyType : private Nocopy. 
 */
class Nocopy
{
	const Nocopy & operator=(const Nocopy & );
	Nocopy(const Nocopy & );    
    
public:    
	Nocopy() {}	
    ~Nocopy() {}
};

} //namespace crap
using namespace crap;

#endif //__NOCOPY_H__