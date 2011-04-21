#include "Data.h"
#include <string.h>
#include <algorithm>

static int check_position(int pos, int size)
{
	if( pos < 0 || pos > size )
		throw std::exception(); // TODO - out of range
	return pos;
}

Data::Impl::Impl(int size)
: data( new unsigned char[size] )
, size( size )
{ }

Data::Impl::~Impl()
{
	delete data;
}

Data::Data()
: impl( new Impl(0) )
, position( 0 )
, size( 0 )
{ }

Data::Data(int size)
: impl( new Impl(size) ) 
, position( 0 ) 
, size( size )
{ }

Data::Data(int size, const void * data)
: impl( new Impl(size) )
, position( 0 )
, size( size )
{
	memmove(impl->data, data, size);
}

Data::Data(const Data & data)
: impl( data.impl )
, position( data.position )
, size( data.size )
{ }

Data::Data(const Data & data, int pos, int si) 
: impl( data.impl )
, position( data.position + check_position(pos, data.size) )
, size( std::min(si, data.size - pos) )
{ }

const Data & Data::operator=(const Data & data)
{
	Data temp(data);
	swap(temp);
	return *this;
}

void Data::swap(Data & other)
{
	impl.swap(other.impl);
	std::swap(position, other.position);
	std::swap(size, other.size);
}

Data::~Data()
{ }

unsigned char * Data::lock()
{
	if( !impl.isTheOne() )
	{
		Data da( size, get_data() );
		swap(da);
	}
	return impl->data;
}

void Data::fill(int value)
{
	memset( lock(), value, size );
}

int Data::copy(int pos, const Data & other)
{
	pos = check_position(pos, size);
	int si = std::min( size - pos, other.size );
	unsigned char * dst = lock();
	memmove( dst + pos, other.get_data(), si );
	return si;
}

/*const Data & Data::operator+=(const Data & other)
{
	Data result( get_size() + other.get_size() );
	result.copy(0, *this);
	result.copy(get_size(), other);
	swap(result);
	return *this;
}*/

int Data::compare(const Data & other)const
{
	if( size == other.size )
		return memcmp(get_data(), other.get_data(), size);
	return size - other.size;
}
