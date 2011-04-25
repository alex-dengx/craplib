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
        virtual void onChunk(const AsyncFileReader&, const Data&) = 0;
        virtual void onError(const AsyncFileReader&) = 0;
        virtual void onEndOfFile(const AsyncFileReader&) = 0;
        virtual ~Delegate() {};
    };
    
private:    
//    enum message_enum { START, CHUNK, COMPLETE };
//    message_enum        message;    
    
    Delegate            *delegate_;
    FILE                *fp_;
    Data                buffer_;
    Timer               timer_;
    int                 bsz_;
    
//    virtual void onCall(const ActiveMsg& msg) {
//        switch(message) {
//            case START:
//                break;
//                
//            case CHUNK:
//                break;
//                
//            case COMPLETE:
//                break;
//        }
//    }
    
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
        if( !(fp_ = fopen(filename.c_str(), "rb")) ) {
            throw std::exception();
        }
    }

    ~AsyncFileReader()
    {
        if(fp_)
            fclose(fp_);
    }
    
    void read(int size) {        
        buffer_ = Data(size);

        bsz_ = (int)fread(buffer_.lock(), 1, size, fp_);        
        timer_.set(0);
    }
};

#endif // __ASYNC_FILE_H__