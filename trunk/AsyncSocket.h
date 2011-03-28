// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "Data.h"
#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"
#include "StaticRefCounted.h"

class RWSocket;
class LASocket;
class SocketWorker;

/**
 * Socket base class
 */
class Socket 
{
    friend class RWSocket;
    
protected:
    int sock_;

public:    
    Socket();    
    Socket(Socket& other) 
    : sock_(0) 
    {
        std::swap(this->sock_, other.sock_);
    }
    explicit Socket(int sock);
    virtual ~Socket();
};

/**
 * Base socket with delegate methods
 */
class SocketImpl
: public Socket
{   
protected:
    friend class SocketWorker;

public:
    SocketImpl() {};
    
    SocketImpl(Socket& sock)
    : Socket(sock) { };
    
    virtual ~SocketImpl() {};
    
private:
    virtual size_t readData() { /* nop */ return 0; }
    virtual bool wantWrite() const { /* nop */ return false; }
    virtual bool isListening() const { return false; }
    
//    virtual void onConnect() = 0;
    virtual void onDisconnect() = 0;
    virtual void onRead() = 0;
    virtual void onCanWrite() { /* nop */ }
    virtual void onError() = 0;
};

/**
 * Shared socket select thread
 */
class SocketWorker
: public Runnable
, public ActiveMsgDelegate
{
private:
    typedef std::map<int, SocketImpl*>   Container;
    typedef std::pair<int, SocketImpl*>  Pair;
    CondVar                              c_;
    Container                            clients_;
    
    ThreadWithMessage                    t_;
    
    enum message_enum { IDLE, ONREAD, CANWRITE, ONCLOSE, ONERROR };
    message_enum        message_;    
    
    SocketImpl          *curSock;
    
public:
    SocketWorker()
    : c_(false)
    , t_(*this, *this)
    , message_(IDLE)
    { 
        t_.start();
    }
    
    ~SocketWorker()
    {
        t_.requestTermination();
        t_.waitTermination();
    }
    
    void registerSocket(SocketImpl* impl)
    {
        CondLock lock(c_);
        clients_.insert( Pair(impl->sock_, impl) );
        lock.set(true); // New client available
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        CondLock lock(c_);
        clients_.erase(impl->sock_);
        lock.set(!clients_.empty());
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg) {

        switch(message_) {
            case ONREAD:   
                curSock->onRead();
                break;
                
            case CANWRITE:                
                curSock->onCanWrite();
                break;
                
            case ONCLOSE:     
                curSock->onDisconnect();
                break;
                
            case ONERROR:
                curSock->onError();
                break;
                
            default:
                break;
        }
    }
    
    virtual void run();
};

/**
 * Asynchronous read-write socket implementation
 */
class RWSocket
: private SocketImpl
, private StaticRefCounted<SocketWorker>
{
public:
    struct Delegate
    {
//        virtual void onConnect(const RWSocket&) = 0;
        virtual void onDisconnect(const RWSocket&) = 0;
        virtual void onRead(const RWSocket&, const Data&) = 0;
        virtual void onCanWrite(const RWSocket&) = 0;
        virtual void onError(const RWSocket&) = 0;
        virtual ~Delegate() { };
    };
    
private:
    friend class SocketWorker;
    
    Delegate*           delegate_;
    
    std::string         host_, 
                        service_;
    struct addrinfo     *addr_;
    
    Data                readData_;
    bool                wantWrite_;
    
    virtual bool wantWrite() const { return wantWrite_; }
    virtual size_t readData();
    
    // SocketWorker delegate methods
    virtual void onDisconnect()
    {
        delegate_->onDisconnect(*this);  
    }
    
    virtual void onRead()
    {
        delegate_->onRead(*this, readData_);
    }
    
    virtual void onCanWrite()
    {
        delegate_->onCanWrite(*this);
    }
    
    virtual void onError()
    {
        delegate_->onError(*this);
    }
    
public:
    RWSocket(Delegate* del, const std::string& host, const std::string& service);    
    RWSocket(Delegate* del, Socket& sock);    
    
    virtual ~RWSocket();
    
    const Data write(const Data& bytes);    
};


/**
 * Asynchronous listen-accept (aka Server) socket implementation
 */
class LASocket
: private SocketImpl
, private StaticRefCounted<SocketWorker>
{
public:
    struct Delegate
    {
//        virtual void onListening(const LASocket&) = 0;
        virtual void onDisconnect(const LASocket&) = 0;
        virtual void onNewClient(const LASocket&, Socket&) = 0;
        virtual void onError(const LASocket&) = 0;
        virtual ~Delegate() { };
    };
    
private:
    friend class SocketWorker;
    Delegate            *delegate_;

    std::string         host_, 
    service_;
    struct addrinfo     *addr_;
    
    // SocketWorker delegate methods
//    virtual void onConnect() 
//    {
//        // Indicates we are ready to accept connections
//        delegate_->onListening(*this);
//    }
    
    virtual void onDisconnect()
    {
        delegate_->onDisconnect(*this);  
    }
    
    virtual void onRead();
    
    virtual void onError()
    {
        delegate_->onError(*this);
    }
    
    virtual bool isListening() const 
    {
        return true; 
    }
    
public:
    LASocket(Delegate* del, const std::string& host, const std::string& service);    
    virtual ~LASocket();
    
};

#endif // __ASYNC_SOCKET_H__