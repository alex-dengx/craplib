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
    : fp_(NULL)
    , fileSize_(0)
    {        
        if( !(fp_ = fopen(filename.c_str(), "rb")) ) {
            throw std::exception();
        }   
        
        // Get file size
        fseek(fp_, 0 , SEEK_END);
        fileSize_ = ftell(fp_);
        fseek(fp_, 0, SEEK_SET);
    }
    
    ~FileReader()
    {
        if(fp_)
            fclose(fp_);
    }
    
    Data read(int size) {        
        if(!fp_) 
            return Data(0);
        
        Data buffer(size);
        int bsz = (int)fread(buffer.lock(), 1, size, fp_);        
        return Data(buffer, 0, bsz);
    }
    
    long filesize() const {
        return fileSize_;
    }
    
private:    
    FILE                *fp_;
    long                fileSize_;    
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
    : delegate_(del)
    , fp_(NULL)
    , timer_(this)
    , fileSize_(0)
    {        
        if( !(fp_ = fopen(filename.c_str(), "rb")) ) {
            throw std::exception();
        }   
        
        // Get file size
        fseek(fp_, 0 , SEEK_END);
        fileSize_ = ftell(fp_);
        fseek(fp_, 0, SEEK_SET);
    }
    
    ~AsyncFileReader()
    {
        if(fp_)
            fclose(fp_);
    }
    
    void read(int size) {        
        if(!fp_) 
            return;
        if(!buffer_.empty())
            return;
        
        buffer_ = Data(size);

        int bsz = (int)fread(buffer_.lock(), 1, size, fp_);        
        buffer_ = Data(buffer_, 0, bsz);
        
        timer_.set(0);
    }
    
    long filesize() const {
        return fileSize_;
    }
    
private:    
    Delegate            *delegate_;
    FILE                *fp_;
    Data                buffer_;
    Timer               timer_;
    long                fileSize_;
    
    virtual void onTimer(const Timer& timer) {
        if( buffer_.empty() ) {
            if(feof(fp_)) {
                delegate_->onEndOfFile(*this);
            } else {
                delegate_->onError(*this);
            }
        } else {
            delegate_->onChunk(*this, buffer_);
            buffer_ = Data();
        }
    }
};

#endif // __ASYNC_FILE_H__