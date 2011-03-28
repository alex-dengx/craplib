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
    : sock_(this, "pop.mail.ru", "110")
    {
        cmds_.push("USER email@bk.ru\n");
        cmds_.push("PASS pass\n");
        cmds_.push("STAT\n");
        
        data_ = Data(cmds_.front().length(), cmds_.front().c_str());
        cmds_.pop();
    }
    
    // Delegate methods
    virtual void onConnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] our socket connected", &sock);
            
            // Write some data
            data_ = sock_.write(data_);
        }
    }
    
    virtual void onDisconnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] our socket disconnected", &sock);
        }
    }
        
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("[%x] Got response: '%s'", &sock, d.get_data());
            
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
        wLog("[%x] error on sock", &sock);
    }
    
};


class TestClient 
: public RWSocket::Delegate
{
private:
    Data data_;
    RWSocket sock_;
    
public:
    TestClient(Socket& sock)
    : data_(6, "HELLO\n")
    , sock_(this, sock)
    { }
    
    // Delegate methods
    virtual void onConnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] our socket connected", &sock);            
            data_ = sock_.write(data_);
        }
    }
    
    virtual void onDisconnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] our socket disconnected", &sock);
        }
    }
    
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("[%x] Got data: '%d'", &sock, d.get_size());

            // Echo
            data_ = sock_.write(d);
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
        wLog("[%x] error on sock", &sock);
    }
    
};


class TestServer
: public LASocket::Delegate
{
private:
    typedef SharedPtr<TestClient> CliPtr;
    
    LASocket sock_;
    std::vector<CliPtr> clients_;
    
public:
    TestServer()
    : sock_(*this, "localhost", "9090")
    { }
    
    // Delegate methods
    virtual void onListening(const LASocket& sock)
    {
        wLog("[SERVER] listening...");   
    }
    
    virtual void onDisconnect(const LASocket& sock) 
    {
        wLog("[SERVER] exit");
    }
    
    virtual void onNewClient(const LASocket& sock, Socket& cli) 
    {
        clients_.push_back( CliPtr( new TestClient(cli) ) );
    }
    
    virtual void onError(const LASocket& sock) 
    {
        wLog("[SERVER] error on sock");
    }
    
};


SUITE(Socket);

// Socket simple tests
////////////////////////////////////////////////////////////////////
//TEST(Basic, Socket) {
//    
//    RunLoop rl;
//    TestReq req;
//    rl.run(); // Locks because 'f' is not deleted (doesn't deregister msgs)    
//};


// Server socket testing - simple echo server
////////////////////////////////////////////////////////////////////
TEST(Server, Socket) {
    
    RunLoop rl;
    TestServer srv;
    rl.run();
    
};


#endif // __SOCKET_TESTS_H__