// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SOCKET_TESTS_H__
#define __SOCKET_TESTS_H__

#include "AsyncSocket.h"
#include "AsyncNetInterface.h"
#include <queue>
#include <string>

class TestReq 
: public RWSocket::Delegate
{
private:
    Data     data_;
    RWSocket sock_;
    std::queue<std::string> cmds_;
    std::string             lastCmd_;
    
public:
    TestReq()
    : sock_(this, "www.google.com", "80")
    {        
        cmds_.push("GET / HTTP/1.0\n\n");
        
        data_ = Data((int)cmds_.front().length(), cmds_.front().c_str());
        lastCmd_ = cmds_.front();
        cmds_.pop();
                
        // Write some data
        sock_.write(data_);
    }
    
    // Delegate methods
    virtual void onDisconnect(const RWSocket& sock) 
    {
        wLog("[%x] test req socket disconnected", &sock);
    }
        
    virtual void onCanRead(const RWSocket& sock)
    {
        int bytes = 0;
        while(0 != (bytes = sock_.read(data_))) {
            wLog("Got %d bytes from socket..", bytes);        
            wLog("DATA: %s", std::string(data_.get_data(), data_.get_data()+bytes).c_str());
        }
    }
    
    virtual void onCanWrite(const RWSocket& sock)
    {
        if(&sock == &sock_ && !data_.empty()) {
            sock_.write(data_);
        }
    }
    
};



class ServerOnThread
: public Runnable
, public LASocket::Delegate
{
private:
    NetworkInterface    if_;
    Thread              t_;
    
public:
    ServerOnThread(const NetworkInterface& nif)
    : if_(nif)
    , t_(*this)
    {
        t_.start();
    }
    
    ~ServerOnThread()
    {
        t_.waitTermination();
    }
    
    // Delegate methods
    virtual void onDisconnect(const LASocket& s) {
        wLog("0x%X: on disconnect", this);
    }
    
    virtual void onNewClient(const LASocket& s, Socket& c) {
        wLog("0x%X: on new client", this);        
    }
    
    virtual void onError(const LASocket& s) {
        wLog("0x%X: on error", this);        
    }
    
    
    virtual void run()
    {
        RunLoop runloop;
        wLog("Starting runloop for interface: %s; listening on %s", 
             if_.name().c_str(), if_.ip().c_str());
        
        LASocket sock(this, if_, "8090");
        
        runloop.run();
    }
};

class Server
: public LASocket::Delegate
{
private:
    NetworkInterface    if_;
    LASocket            sock_;
    
public:
    Server(const NetworkInterface& nif)
    : if_(nif)
    , sock_(this, if_, "8090")
    {
        wLog("Starting runloop for interface: %s; listening on %s", 
             if_.name().c_str(), if_.ip().c_str());        
    }
    
    ~Server()
    {
    }
    
    // Delegate methods
    virtual void onDisconnect(const LASocket& s) {
        wLog("0x%X: on disconnect", this);
    }
    
    virtual void onNewClient(const LASocket& s, Socket& c) {
        wLog("0x%X: on new client", this);        
    }
    
    virtual void onError(const LASocket& s) {
        wLog("0x%X: on error", this);        
    }
};

typedef SharedPtr<ServerOnThread> ServerOnThreadPtr;
typedef SharedPtr<Server> ServerPtr;
std::vector<ServerPtr> servers;

class GetInterfaces
: public NetworkInterfaces::Delegate
{
private:
    NetworkInterfaces nif_;
    
public:
    GetInterfaces()
    : nif_(this, NetworkInterface::IPv4)
    {  }
    
    // Delegate methods    
    virtual void onInterfaceDetected(const NetworkInterfaces& nif, const NetworkInterface& iface) 
    {
        wLog("Got detected network interface: %s IP: %s", 
             iface.name().c_str(), iface.ip().c_str());
        
        // Start server for that
        servers.push_back( ServerPtr( new Server(iface) ) );
    }
};



SUITE(Socket);

TEST(Request, Socket) {
    RunLoop rl;
    TestReq req;
    rl.run();
}

// Server socket testing - simple echo server
////////////////////////////////////////////////////////////////////
//TEST(Server, Socket) {
//    
//    RunLoop rl;
//    TestServer srv;
//    rl.run();
//    
//};

//TEST(GetInterfaces, Socket) {
//    
//    RunLoop rl;
//    GetInterfaces gif;
//    rl.run();
//    
//};



#endif // __SOCKET_TESTS_H__