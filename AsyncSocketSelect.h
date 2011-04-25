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
, public ActiveMsgDelegate
{
private:
    typedef std::deque<SocketImpl*>      Container;
    CondVar                              c_;

    Container                            clients_;
    Container                            read_, write_, err_;
    
    ThreadWithMessage                    t_;
    
    enum message_enum { IDLE, ONCHANGES };
    message_enum        message_;    
    
    fd_set              read_fd;
    fd_set              write_fd;
    fd_set              err_fd;

    fd_set              read_fd_copy;
    fd_set              write_fd_copy;
    fd_set              err_fd_copy;
    
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
        if(impl->sock_ >= FD_SETSIZE) {
            wLog("can't handle this client - select is full.");
            return;
        }
        clients_.push_back( impl );
        lock.set(true); // New client available
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        CondLock lock(c_);
        
        clients_.erase( std::remove(clients_.begin(), clients_.end(), impl), clients_.end());
        read_.erase( std::remove(read_.begin(), read_.end(), impl), read_.end());
        write_.erase( std::remove(write_.begin(), write_.end(), impl), write_.end());
        err_.erase( std::remove(err_.begin(), err_.end(), impl), err_.end());
        
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

#endif // __ASYNC_SOCKET_SELECT_H__
#endif // _CRAP_SOCKET_SELECT_