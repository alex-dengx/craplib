// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SOCKET_TESTS_H__
#define __SOCKET_TESTS_H__

#include "AsyncSocket.h"
#include <queue>

class TestReq 
: public RWSocket::Delegate
{
private:
    RWSocket sock_;
    Data     data_;
    std::queue<std::string> cmds_;
    
public:
    TestReq()
    : sock_(*this, "pop.mail.ru", "110")
    {
        cmds_.push("USER login@bk.ru\n");
        cmds_.push("PASS yourpass\n");
        cmds_.push("STAT\n");
        cmds_.push("EXIT\n");
        
        data_ = Data(cmds_.front().length(), cmds_.front().c_str());
        cmds_.pop();
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
    
    virtual void onDisconnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("our socket disconnected");
        }
    }
        
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("Got response: '%s'", d.get_data());
            
            if(!cmds_.empty()) {
                data_ = Data(cmds_.front().length(), cmds_.front().c_str());
                cmds_.pop();   
                
                sleep(1);
                data_ = sock_.write(data_);
            }
        }
    }
    
    virtual void onCanWrite(const RWSocket& sock)
    {
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