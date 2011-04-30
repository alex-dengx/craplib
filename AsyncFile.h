// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_FILE_H__
#define __ASYNC_FILE_H__

#include <stdio.h>

#include "Thread.h"
#include "ActiveMsg.h"
#include "Timer.h"
#include "LogUtil.h"


class FileReader
: public DataInputStream
{
public:
    explicit FileReader(const std::string& filename)
    : fp(fopen(filename.c_str(), "rb"))
    {        
        if( !fp ) // This if saves us all other ifs
            throw std::exception();
    }
	Data read(int size) {
        Data buffer(size);
        int bsz = (int)fread(buffer.lock(), 1, size, fp);        
        return Data(buffer, 0, bsz);
    }
    virtual ~FileReader() {
		fclose(fp);
    }
	virtual int read(Data & data) {
		data = read(64*1024);
		return data.getSize();
	}

    int filesize() const {
        // Get file size
        off_t was = ftello(fp);
		fseeko(fp, 0 , SEEK_END);
        int result = ftello(fp); // Overflow possible
        fseeko(fp, was, SEEK_SET);
        return result;
    }

	static Data read_file(const std::string& filename) // If this fun is used, empty Data for non-existent file is ok
	{
		try {
			FileReader fr(filename);
			return fr.read(fr.filesize());
		} catch( const std::exception & ex) {
		}
		return Data();
	}
private:
    FILE                *fp;
};

class AsyncFileReader
: public Timer::Delegate
{
public:
    struct Delegate {
        virtual void onChunk(const AsyncFileReader&, Data&) = 0;
        virtual void onError(const AsyncFileReader&) = 0;
        virtual void onEndOfFile(const AsyncFileReader&) = 0;
        virtual ~Delegate() {};
    };
    
    AsyncFileReader(Delegate* del, const std::string& filename)
    : delegate(del)
    , fp(NULL)
    , timer(this)
    , fileSize(0)
    {        
        if( !(fp = fopen(filename.c_str(), "rb")) ) {
            throw std::exception();
        }   
        
        // Get file size
        fseek(fp, 0 , SEEK_END);
        fileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }
    
    ~AsyncFileReader()
    {
        if(fp)
            fclose(fp);
    }
    
    void read(int size) {        
        if(!fp) 
            return;
        if(!buffer.empty())
            return;
        
        buffer = Data(size);

        int bsz = (int)fread(buffer.lock(), 1, size, fp);        
        buffer = Data(buffer, 0, bsz);
        
        timer.set(0);
    }
    
    long filesize() const {
        return fileSize;
    }
    
private:    
    Delegate            *delegate;
    FILE                *fp;
    Data                buffer;
    Timer               timer;
    long                fileSize;
    
    virtual void onTimer(const Timer& timer) {
        if( buffer.empty() ) {
            if(feof(fp)) {
                delegate->onEndOfFile(*this);
            } else {
                delegate->onError(*this);
            }
        } else {
            delegate->onChunk(*this, buffer);
            buffer = Data();
        }
    }
};

#endif // __ASYNC_FILE_H__
