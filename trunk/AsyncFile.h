// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_FILE_H__
#define __ASYNC_FILE_H__

#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"

#include <fstream>

class AsyncFile 
: public Runnable
, public ActiveMsgDelegate
{
private:    
    std::ifstream   stream_;
  
    enum message_enum { START, CHUNK, COMPLETE };
    message_enum        message;    
    std::string         chunk_; // 5 chars on iteration dude! :-)
        
    ThreadWithMessage   t_;
    
    virtual void onCall(const ActiveMsg& msg) {
        switch(message) {
            case START:
                wLog("Starting to read file..");
                break;
                
            case CHUNK:
//                std::cout << this << " :: Written chunk = '" << chunk_.size() << "'\n";
                break;
                
            case COMPLETE:
                wLog("Complete!");
                break;
        }
    }
public:
    AsyncFile(const std::string& filename)
    : stream_(filename.c_str())
    , t_(*this, *this)
    {
        if(!stream_.is_open())
            throw std::exception();
     
        t_.start();
    }

    ~AsyncFile()
    {
        t_.requestTermination();
        t_.waitTermination();
    }

    virtual void run() 
    {
        {
            ActiveCall call(t_.msg_);
            message = START;
            ///...
            call.call();
        }

        // FIXME: looks like a very shitty way of reading data :-/
        char data[5];
        while( !stream_.eof() ) {
            std::streamsize r = stream_.readsome(data, 5);
            
            if(r==0)
                break;
            
            ActiveCall call(t_.msg_);
            message = CHUNK;
            
            chunk_.assign(data, r);          

            call.call(); // Tell the runloop we want to be called now
        } 

        // Nop
        {
            ActiveCall call(t_.msg_);
            message = COMPLETE;
            call.call();
        }
    }
};

#endif // __ASYNC_FILE_H__