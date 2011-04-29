// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>
#include <deque>
#include <map>
#include <algorithm>

#include "Data.h"
#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"
#include "StaticRefCounted.h"
#include "AsyncNetInterface.h"

class SocketWorker;

/**
 * Socket base class
 */
class Socket 
{
public:
    Socket();    
    Socket(Socket& other) 
    : sock(0) 
    {
        std::swap(this->sock, other.sock);
    }
    explicit Socket(int sock);
    ~Socket();
	int get_sock()const { return sock; }
private:
    int sock;    
};

/**
 * Base socket with delegate methods
 */
class SocketImpl
{   
protected:
	Socket s;
    friend class SocketWorker;
    
public:
    SocketImpl() {}
    
    SocketImpl(Socket& sock)
    : s(sock) { }
    
    virtual ~SocketImpl() {}
    
private:
    virtual void onDisconnect() = 0;
    virtual void onCanRead() = 0;
    virtual void onCanWrite() = 0;
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
        virtual void onCanRead(const RWSocket&) = 0;
        virtual void onCanWrite(const RWSocket&) = 0;
        virtual ~Delegate() { }
    };
    
private:
    friend class SocketWorker;
    
    Delegate*           delegate;
    
    // SocketWorker delegate methods
    virtual void onDisconnect()
    {
        delegate->onDisconnect(*this);  
    }
    
    virtual void onCanRead()
    {
        delegate->onCanRead(*this);
    }
    
    virtual void onCanWrite()
    {
        delegate->onCanWrite(*this);
    }
public:
    RWSocket(Delegate* del, const std::string& host, const std::string& service);    
    RWSocket(Delegate* del, Socket& sock);    
    
    virtual ~RWSocket();
    
    int write(Data & data);
	int read(Data & data);
    
    inline int getSock() {
        return s.get_sock();
    }
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
        virtual ~Delegate() { }
    };
    
private:
    friend class SocketWorker;
    Delegate            *delegate;
    
    NetworkInterface    iface;
    std::string         service;
    
    // SocketWorker delegate methods
    virtual void onDisconnect()
    {
        delegate->onDisconnect(*this);  
    }
    
    virtual void onCanRead();
	virtual void onCanWrite() {}
public:
    LASocket(Delegate* del, const std::string& service);
    LASocket(Delegate* del, const NetworkInterface& nif, const std::string& service);    
    virtual ~LASocket();
    
};


#if defined(__MACH__)

#define _CRAP_SOCKET_KEVENT_
#include "AsyncSocketKevent.h"

#elif defined(__linux__)

#define _CRAP_SOCKET_EPOLL_
#include "AsyncSocketEpoll.h"

#else

#define _CRAP_SOCKET_SELECT_
#include "AsyncSocketSelect.h"

#endif

#endif // __ASYNC_SOCKET_H__