// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>

#include "Data.h"
#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"

class RWSocket
: public Runnable
, public ActiveMsgDelegate
{
public:
    struct Delegate
    {
        virtual void onConnect(const RWSocket&) = 0;
        virtual void onDisconnect(const RWSocket&) = 0;
        virtual void onRead(const RWSocket&, const Data&) = 0;
        virtual void onCanWrite(const RWSocket&) = 0;
        virtual void onError(const RWSocket&) = 0;
        virtual ~Delegate() { };
    };

private:
    Delegate& delegate_;
    
    int                 sock_;
    std::string         host_, 
                        service_;
    struct addrinfo     *addr_;
    
    enum message_enum { CONNECTING, CONNECTED, CANREAD, CANWRITE, CLOSE, ONERROR };
    message_enum        message_;    

    ThreadWithMessage   t_;
    
    Data                readData_;
    bool                wantWrite_;
    
    virtual void onCall(const ActiveMsg& msg) {
        switch(message_) {
            case CONNECTED:
                delegate_.onConnect(*this);
                break;
            case CANREAD:
                delegate_.onRead(*this, readData_);
                break;
            case CANWRITE:
                delegate_.onCanWrite(*this);
                break;
            case CLOSE:
                delegate_.onDisconnect(*this);
                break;
            case ONERROR:
                delegate_.onError(*this);
                break;
            default:
                break;
        }
    }
    
public:
    RWSocket(Delegate& del, const std::string& host, const std::string& service)
    : delegate_(del)
    , sock_(0)
    , host_(host)
    , service_(service)
    , message_(CONNECTING)
    , t_(*this, *this)
    , readData_(1024)
    , wantWrite_(false)
    {
        t_.start();
    }
    
    ~RWSocket();
    
    const Data& write(const Data& bytes);    
    virtual void run();
};


class LASocket
{
    
};

#endif // __ASYNC_SOCKET_H__