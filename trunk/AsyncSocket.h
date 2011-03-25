// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>
#include <map>

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
class SocketImpl
{   
protected:
    friend class SocketWorker;
    int sock_;

public:    
    SocketImpl();    
    virtual ~SocketImpl();

private:
    virtual void onChanges(bool read, bool write, bool err) = 0;    
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
    
    enum message_enum { IDLE, CANREAD, CANWRITE, ONCLOSE, ONERROR };
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
        wLog("dereg sock");
        CondLock lock(c_);
        clients_.erase(impl->sock_);
        lock.set(!clients_.empty());
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg) {

        // FIXME: this if shouldn't be necesarry
        if(curSock)
            
        switch(message_) {
            case CANREAD:                
                curSock->onChanges(true, false, false);
                break;
                
            case CANWRITE:                
                curSock->onChanges(false, true, false);
                break;
                
            case ONCLOSE:     
                // TODO: this can't happen?
                wLog("ON CLOSE");
                break;
                
            case ONERROR:
                curSock->onChanges(false, false, true);
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
, public Runnable
, public ActiveMsgDelegate
, private StaticRefCounted<SocketWorker>
{
public:
    struct Delegate
    {
        virtual void onConnect(const RWSocket&) = 0;
        virtual void onDisconnect(const RWSocket&) = 0;
        virtual void onRead(const RWSocket&, const Data&) = 0;
        virtual void onCanWrite(const RWSocket&) = 0;
        virtual void onError(const RWSocket&) = 0;
        virtual ~Delegate() { };
    };
    
private:
    Delegate& delegate_;
    
    std::string         host_, 
                        service_;
    struct addrinfo     *addr_;
    
    enum message_enum { CONNECTING, CONNECTED, ONREAD, ONWRITE, CLOSE, ONERROR };
    message_enum        message_;    

    ThreadWithMessage   t_;
    
    Data                readData_;
    bool                wantWrite_;
    CondVar             jobOrTermination_;
    
    bool                readFlag_, writeFlag_, errFlag_;
    
    virtual void onChanges(bool read, bool write, bool err)
    {
        readFlag_ = read;
        writeFlag_ = write;
        errFlag_ = err;
        
        CondLock lock(jobOrTermination_);
        lock.set(true);
    }
    
    virtual void onCall(const ActiveMsg& msg) {
        switch(message_) {
            case CONNECTED:
                delegate_.onConnect(*this);
                break;
            case ONREAD:
                delegate_.onRead(*this, readData_);
                break;
            case ONWRITE:
                delegate_.onCanWrite(*this);
                break;
            case CLOSE:
                delegate_.onDisconnect(*this);
                break;
            case ONERROR:
                delegate_.onError(*this);
                break;
            default:
                break;
        }
    }
    
public:
    RWSocket(Delegate& del, const std::string& host, const std::string& service)
    : delegate_(del)
    , host_(host)
    , service_(service)
    , message_(CONNECTING)
    , t_(*this, *this)
    , readData_(1024)
    , jobOrTermination_(false)
    {
        statics().registerSocket(this);
        t_.start();
    }
    
    virtual ~RWSocket();
    
    const Data& write(const Data& bytes);    
    virtual void run();
};


/**
 * Asynchronous listen-accept (aka Server) socket implementation
 */
class LASocket
{
    
};

#endif // __ASYNC_SOCKET_H__