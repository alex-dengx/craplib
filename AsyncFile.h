// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_FILE_H__
#define __ASYNC_FILE_H__

#include "Thread.h"
#include "ActiveMsg.h"
#include "Timer.h"
#include "LogUtil.h"


class FileReader
{
public:
    FileReader(const std::string& filename)
    : fp(NULL)
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
    
    ~FileReader()
    {
        if(fp)
            fclose(fp);
    }
    
    Data read(int size) {        
        if(!fp) 
            return Data(0);
        
        Data buffer(size);
        int bsz = (int)fread(buffer.lock(), 1, size, fp);        
        return Data(buffer, 0, bsz);
    }
    
    long filesize() const {
        return fileSize;
    }
    
private:    
    FILE                *fp;
    long                fileSize;    
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