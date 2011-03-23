// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SOCKET_TESTS_H__
#define __SOCKET_TESTS_H__

#include "AsyncSocket.h"

class TestReq 
: public RWSocket::Delegate
{
private:
    RWSocket sock_;
    
public:
    TestReq()
    : sock_(*this, "127.0.0.1", "8080")
    {
        
    }
    
    // Delegate methods
    virtual void onConnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("our socket connected");
        }
    }
        
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("can read from sock");
        }
    }
    
    virtual void onCanWrite(const RWSocket& sock)
    {
        if(&sock == &sock_) {
            wLog("can write to sock");
            
            // bytes = sock_.write(bytes);
        }
    }
    
    virtual void onError(const RWSocket& sock) 
    {
        wLog("error on sock");
    }
    
};


SUITE(Socket);

// Socket simple tests
////////////////////////////////////////////////////////////////////
TEST(Basic, Socket) {
    
    TestReq req;
    
};

#endif // __SOCKET_TESTS_H__