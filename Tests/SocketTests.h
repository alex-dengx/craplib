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
    : sock_(this, "localhost", "9090")
    {
        Data ldata(4096);   // 4k of 1s
        ldata.fill(1);
        ldata.lock()[4095] = '\n';
        std::string large((char*)ldata.get_data(), 4096);
        
        cmds_.push("first line\n");
        cmds_.push("second test\n");
        cmds_.push(large);
        
        data_ = Data((int)cmds_.front().length(), cmds_.front().c_str());
        lastCmd_ = cmds_.front();
        cmds_.pop();
                
        // Write some data
        data_ = sock_.write(data_);
    }
    
    // Delegate methods
    virtual void onDisconnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] test req socket disconnected", &sock);
        }
    }
        
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {   

            std::string str((char*)d.get_data(), d.get_size());
                
            Data tmp = Data(data_.get_size(), data_.get_data());
            data_ = Data(d.get_size() + data_.get_size());
            data_.copy(tmp.get_size(), d);
            data_.copy(0, tmp);
             
            if( *str.rbegin() != '\n' ) {
                wLog("appended %d bytes. total bytes now %d", d.get_size(), data_.get_size());
                return;
            }
            
            std::string fullStr((char*)data_.get_data(), data_.get_size());
            rassert( fullStr == lastCmd_ );
            
            if(!cmds_.empty()) {
                data_ = Data((int)cmds_.front().length(), cmds_.front().c_str());
                lastCmd_ = cmds_.front();
                cmds_.pop();   
                
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
    : sock_(this, sock)
    { }
    
    // Delegate methods
    virtual void onDisconnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] client socket disconnected", &sock);
        }
    }
    
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("[%x] Got data len %d", &sock, d.get_size());

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


typedef SharedPtr<TestReq> TestReqPtr;
std::vector<TestReqPtr> testReqVec;


class TestServer
: public LASocket::Delegate
{
private:
    typedef SharedPtr<TestClient> CliPtr;
    
    LASocket sock_;
    std::vector<CliPtr> clients_;
    
public:
    TestServer()
    : sock_(this, "9090")
    { 
        for(int i=0; i<5; ++i) 
            testReqVec.push_back( TestReqPtr( new TestReq() ) );
    }
    
    // Delegate methods    
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

typedef SharedPtr<ServerOnThread> ServerOnThreadPtr;
std::vector<ServerOnThreadPtr> servers;

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
        servers.push_back( ServerOnThreadPtr( new ServerOnThread(iface) ) );
    }
};



SUITE(Socket);

// Server socket testing - simple echo server
////////////////////////////////////////////////////////////////////
//TEST(Server, Socket) {
//    
//    RunLoop rl;
//    TestServer srv;
//    rl.run();
//    
//};

TEST(GetInterfaces, Socket) {
    
    RunLoop rl;
    GetInterfaces gif;
    rl.run();
    
};



#endif // __SOCKET_TESTS_H__