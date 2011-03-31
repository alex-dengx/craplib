// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>
#include <deque>
#include <map>
#include <algorithm>

#include <sys/event.h>

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
    
    struct EventHolder {
        int           sock;
        struct kevent event;
        
        EventHolder(int s, const struct kevent& ev)
        : sock(s)
        , event(ev) 
        {};

        operator struct kevent* () 
        {
            return &event;
        }
        
        bool operator == (int s) const 
        {
            return sock == s;
        }
    };
    
    typedef std::map<int, SocketImpl*>   Container;
    typedef std::deque<SocketImpl*>      Vec;
    CondVar                              c_;

    Container                            clients_;
    Vec                                  read_, write_, err_;
    
    ThreadWithMessage                    t_;
    
    enum message_enum { IDLE, ONCHANGES };
    message_enum        message_;    
    
    int                             kq_;
    std::deque<EventHolder>         kqChangeList_;
    struct kevent                   kqEvents_[MAX_CONNECTIONS];
    
    void handleChanges();
    
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
        if(clients_.size() >= MAX_CONNECTIONS-1) {
            wLog("can't handle this client - kqueue buffer is full.");
            return;
        }
        
        // Set the event filter
        struct kevent changeLst;
        EV_SET(&changeLst, impl->sock_, EVFILT_READ | EVFILT_WRITE, EV_ADD, 0, 0, 0);
        kqChangeList_.push_back( EventHolder(impl->sock_, changeLst) );
        
        clients_.insert( std::pair<int, SocketImpl*>(impl->sock_, impl) );
        lock.set(true); // New client available
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        CondLock lock(c_);
        
        clients_.erase( impl->sock_ );
        read_.erase( std::remove(read_.begin(), read_.end(), impl), read_.end());
        write_.erase( std::remove(write_.begin(), write_.end(), impl), write_.end());
        err_.erase( std::remove(err_.begin(), err_.end(), impl), err_.end());
        
        kqChangeList_.erase( std::remove(kqChangeList_.begin(), 
                                         kqChangeList_.end(), 
                                         impl->sock_),
                            kqChangeList_.end());
        
        lock.set(!clients_.empty());
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

    std::string         host_, 
    service_;
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
    LASocket(Delegate* del, const std::string& host, const std::string& service);    
    virtual ~LASocket();
    
};

#endif // __ASYNC_SOCKET_H__