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
    Data     data_;
    
public:
    TestReq()
    : sock_(*this, "localhost", "80")
    , data_(409600)
    {
        data_.fill(1);
    }
    
    // Delegate methods
    virtual void onConnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("our socket connected");
            
            // Write some data
            data_ = sock_.write(data_);
        }
    }
        
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("Got response: '%s'", d.get_data());
        }
    }
    
    virtual void onCanWrite(const RWSocket& sock)
    {
        wLog("on can write..");
        if(&sock == &sock_ && !data_.empty()) {
            data_ = sock_.write(data_);
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
    
    RunLoop rl;
    TestReq req;
    rl.run(); // Locks because 'f' is not deleted (doesn't deregister msgs)
    
};

#endif // __SOCKET_TESTS_H__