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
: data( new unsigned char[size] ) // Uninitialized to save time
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
		Data da( size, getData() );
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
	memmove( dst + pos, other.getData(), si );
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
		return memcmp(getData(), other.getData(), size);
	return size - other.size;
}

void DataStreamBridge::move_data(DataInputStream & sin, DataOutputStream & sout)
{
	while(true)
	{
		if( buffer.empty() )
			if( sin.read(buffer) == 0 )
				break; // Was nothing to write, and nothing have been read
		sout.write(buffer);
		if( !buffer.empty() )
			break;
	}
}

void DataDeque::write(const Data & data)
{
	m_data.push_back( data );
	size += data.getSize();
}

void DataDeque::read(Data & data, int max_size)
{
	if( m_data.empty() )
	{
		data = Data();
		return;
	}
	Data & fr = m_data.front();
	if( fr.getSize() <= max_size )
	{
		data = fr;
		m_data.pop_front();
		size -= data.getSize();
		return;
	}
	data = Data(fr, 0, max_size);
	fr = Data(fr, max_size, fr.getSize() - max_size);
	size -= max_size;	
}

void DataDeque::read(Data & data)
{
	if( m_data.empty() )
	{
		data = Data();
		return;
	}
	data = m_data.front();
	m_data.pop_front();
	size -= data.getSize();
}
