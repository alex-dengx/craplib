// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once 
#ifndef __DATA_H__
#define __DATA_H__

#include "SharedPtr.h"
#include <list>

namespace crap {
    
    class Data
    {
        struct Impl // TODO replace with SimpleArray
        {
            unsigned char * const data;
            const int size;
            
            Impl(int size);
            ~Impl();
        };
        
        SharedPtr<Impl> impl;
        int position;
        int size;
        
    public:
        Data();
        explicit Data(int size);
        explicit Data(int size, const void * data);
        Data(const Data & data);
        Data(const Data & data, int pos, int si); // throw on wrong position, truncate size
        const Data & operator=(const Data & data);
        void swap(Data & other);
        ~Data();
        
        const unsigned char * get_data()const
        {
            return impl->data + position;
        }
        
        unsigned char * lock();
        
        int get_size()const
        {
            return size;
        }
        
        bool empty()const
        {
            return size == 0;
        }
        
        void fill(int value);
        int copy(int pos, const Data & other); // throw on wrong position, truncate size, return copied size
        //	const Data & operator+=(const Data & other);
        
        int compare(const Data & other)const;
        bool operator==(const Data & other)const{return compare( other ) == 0;}
        bool operator!=(const Data & other)const{return compare( other ) != 0;}
        bool operator<(const Data & other)const{return compare( other ) < 0;}
        bool operator>(const Data & other)const{return compare( other ) > 0;}
        bool operator<=(const Data & other)const{return compare( other ) <= 0;}
        bool operator>=(const Data & other)const{return compare( other ) >= 0;}
    };
    
	class DataDeque
	{
	public:
		DataDeque():size(0) {}
		void write(const Data & data);
		void read(Data & data, int max_size);
		void read(Data & data);
		int get_size()const { return size; }
	private:
		std::list<Data> m_data;
		int size;
//		unsigned char * m_buffer;
//		int m_free_bytes;
	};

} // namespace crap
using namespace crap;

#endif // __DATA_H__