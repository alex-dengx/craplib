// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_FILE_H__
#define __ASYNC_FILE_H__

#include "Thread.h"
#include "ActiveMsg.h"
#include "Timer.h"
#include "LogUtil.h"

class AsyncFileReader
// : public ActiveMsgDelegate
: public TimerDelegate
{
public:
    struct Delegate {
        virtual void onChunk(const AsyncFileReader&, Data&) = 0;
        virtual void onError(const AsyncFileReader&) = 0;
        virtual void onEndOfFile(const AsyncFileReader&) = 0;
        virtual ~Delegate() {};
    };
    
private:    
    Delegate            *delegate_;
    FILE                *fp_;
    Data                buffer_;
    Timer               timer_;
    int                 bsz_;
    
    virtual void onTimer(const Timer& timer) {
        if( bsz_ == 0) {
            if(feof(fp_)) {
                delegate_->onEndOfFile(*this);
            } else {
                delegate_->onError(*this);
            }
        } else {
            buffer_ = Data(buffer_, 0, bsz_);
            delegate_->onChunk(*this, buffer_);
        }
    }
public:
    AsyncFileReader(Delegate* del, const std::string& filename)
    : delegate_(del)
    , timer_(*this)
    {        
        this->open(filename);
    }

    AsyncFileReader(Delegate* del)
    : delegate_(del)
    , fp_(NULL)
    , timer_(*this)
    {        
    }
    
    ~AsyncFileReader()
    {
        if(fp_)
            fclose(fp_);
    }
    
    void open(const std::string& filename) 
    {
        if( !(fp_ = fopen(filename.c_str(), "rb")) ) {
            throw std::exception();
        }   
    }
    
    void read(int size) {        
        if(!fp_) 
            return;
        
        buffer_ = Data(size);

        bsz_ = (int)fread(buffer_.lock(), 1, size, fp_);        
        timer_.set(0);
    }
    
    operator bool() {
        return fp_!=NULL;
    }
};

#endif // __ASYNC_FILE_H__