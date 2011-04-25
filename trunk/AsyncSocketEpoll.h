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

#endif // __ASYNC_SOCKET_EPOLL_H__
#endif // _CRAP_SOCKET_EPOLL_
