// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_EPOLL_

#pragma once
#ifndef __ASYNC_SOCKET_EPOLL_H__
#define __ASYNC_SOCKET_EPOLL_H__

#include <string>
#include <deque>
#include <map>
#include <algorithm>

#include <sys/epoll.h>

#include "Data.h"
#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"
#include "StaticRefCounted.h"
#include "AsyncNetInterface.h"

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
    
    virtual void onDisconnect() = 0;
    virtual void onRead() = 0;
    virtual void onCanWrite() { /* nop */ }
    virtual void onError() = 0;
};

#define MAX_CONNECTIONS 65000

/**
 * Shared socket select thread
 */
class SocketWorker
: public Runnable
, public ActiveMsgDelegate
{
private:
    typedef std::deque<SocketImpl*>  Vec;
    CondVar             c_;

    Vec                 clients_;
    Vec                 read_, write_, err_;
    
    ThreadWithMessage   t_;
    
    enum message_enum { IDLE, ONCHANGES };
    message_enum        message_;    
    
    int                 eq_;
    
    void handleChanges();
    
public:
    SocketWorker()
    : c_(false)
    , t_(*this, *this)
    , message_(IDLE)
    , eq_( epoll_create(MAX_CONNECTIONS) )        
    { 
        t_.start();
    }
    
    ~SocketWorker()
    {
        t_.requestTermination();
        t_.waitTermination();
        close(eq_);
    }
    
    void registerSocket(SocketImpl* impl)
    {
        CondLock lock(c_);
        if(clients_.size() >= MAX_CONNECTIONS-1) {
            wLog("can't handle this client - epoll buffer is full.");
            return;
        }
         
        // Set the event filter
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
	ev.data.fd = impl->sock_;
	ev.data.ptr = impl;

	int res = epoll_ctl(eq_, EPOLL_CTL_ADD, impl->sock_, &ev);
	if(res != 0) {
        	wLog("epoll_ctl failed. couldn't add socket to epoll"); 
		return;
	}
        
        clients_.push_back(impl);
        wLog("socket attached. new clients size = %d; SOCKFD=%d", 
             clients_.size(), impl->sock_);
        
        lock.set(true); // New client available
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        read_.erase( std::remove(read_.begin(), read_.end(), impl), read_.end());
        write_.erase( std::remove(write_.begin(), write_.end(), impl), write_.end());
        err_.erase( std::remove(err_.begin(), err_.end(), impl), err_.end());

        CondLock lock(c_);
        clients_.erase( std::remove(clients_.begin(), clients_.end(), impl), clients_.end());        
        
	// automatically removed from epoll
        lock.set(true);
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg) {

        switch(message_) {
            case ONCHANGES:
                handleChanges();
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
        virtual void onDisconnect(const LASocket&) = 0;
        virtual void onNewClient(const LASocket&, Socket&) = 0;
        virtual void onError(const LASocket&) = 0;
        virtual ~Delegate() { };
    };
    
private:
    friend class SocketWorker;
    Delegate            *delegate_;

    NetworkInterface    if_;
    std::string         service_;
    struct addrinfo     *addr_;
    
    // SocketWorker delegate methods
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
    LASocket(Delegate* del, const std::string& service);
    LASocket(Delegate* del, const NetworkInterface& nif, const std::string& service);    
    virtual ~LASocket();
    
};

#endif // __ASYNC_SOCKET_EPOLL_H__
#endif // _CRAP_SOCKET_EPOLL_