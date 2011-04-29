// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_SELECT_

#pragma once
#ifndef __ASYNC_SOCKET_SELECT_H__
#define __ASYNC_SOCKET_SELECT_H__

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

class RWSocket;
class LASocket;

/**
 * Shared socket select thread
 */
class SocketWorker
: public Runnable
, public ActiveMsg::Delegate
{
private:
    typedef std::deque<SocketImpl*>      Container;
    CondVar                              c;

    Container                            clients;
    Container                            read, write, err;
    
    ThreadWithMessage                    t;
    
    enum message_enum { IDLE, ONCHANGES };
    message_enum        message;    
    
    fd_set              readfd;
    fd_set              writefd;
    fd_set              errfd;

    fd_set              readfd_copy;
    fd_set              writefd_copy;
    fd_set              errfd_copy;
    
    void handleChanges();
    
public:
    SocketWorker()
    : c(false)
    , t(*this, this)
    , message(IDLE)
    { 
        t.start();
    }
    
    ~SocketWorker()
    {
        t.requestTermination();
        t.waitTermination();
    }
    
    void registerSocket(SocketImpl* impl)
    {
        CondLock lock(c);
        if(impl->s.getSock() >= FD_SETSIZE) {
            wLog("can't handle this client - select is full.");
            return;
        }
        clients.push_back( impl );
        lock.set(true); // New client available
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        CondLock lock(c);
        
        clients.erase( std::remove(clients.begin(), clients.end(), impl), clients.end());
        read.erase( std::remove(read.begin(), read.end(), impl), read.end());
        write.erase( std::remove(write.begin(), write.end(), impl), write.end());
        err.erase( std::remove(err.begin(), err.end(), impl), err.end());
        
        lock.set(!clients.empty());
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg) {

        switch(message) {
            case ONCHANGES:
                handleChanges();
                break;
                
            default:
                break;
        }
    }

    virtual void run();
};

#endif // __ASYNC_SOCKET_SELECT_H__
#endif // _CRAP_SOCKET_SELECT_