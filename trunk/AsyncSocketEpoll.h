// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_EPOLL_

#pragma once
#ifndef __ASYNC_SOCKET_EPOLL_H__
#define __ASYNC_SOCKET_EPOLL_H__

#include <string>
#include <deque>
#include <map>
#include <set>
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
, public ActiveMsg::Delegate
{
private:
    typedef std::set<SocketImpl*>    Clients;
    typedef std::deque<SocketImpl*>  Vec;

    Clients             clients;
    Vec                 read, write, err;
    
    ThreadWithMessage   t;
    
    enum message_enum { IDLE, ONCHANGES };
    message_enum        message;    
    
    int                 eq;
    
public:
    SocketWorker()
    : t(*this, this)
    , message(IDLE)
    , eq( epoll_create(MAX_CONNECTIONS) )        
    { 
        t.start();
    }
    
    ~SocketWorker()
    {
        t.requestTermination();
        t.waitTermination();
        close(eq);
    }
    
    void registerSocket(SocketImpl* impl)
    {
        if(clients.size() >= MAX_CONNECTIONS-1) {
            wLog("can't handle this client - epoll buffer is full.");
            return;
        }
         
        // Set the event filter
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
	ev.data.fd = impl->s.getSock();
	ev.data.ptr = impl;

	int res = epoll_ctl(eq, EPOLL_CTL_ADD, impl->s.getSock(), &ev);
	if(res != 0) {
        	wLog("epoll_ctl failed. couldn't add socket to epoll"); 
		return;
	}
        
        clients.insert(impl);
        wLog("socket attached. new clients size = %d; SOCKFD=%d", 
             clients.size(), impl->s.getSock());
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        clients.erase( impl );
        // automatically removed from epoll
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg);

    virtual void run();
};

#endif // __ASYNC_SOCKET_EPOLL_H__
#endif // _CRAP_SOCKET_EPOLL_
