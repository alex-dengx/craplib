// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_KEVENT_

#pragma once
#ifndef __ASYNC_SOCKET_KEVENT_H__
#define __ASYNC_SOCKET_KEVENT_H__

#include <deque>
#include <set>

#include <sys/event.h>

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
    typedef std::set<SocketImpl*>    Clients;
    typedef std::deque<SocketImpl*>  Vec;

    Clients             clients;
    Vec                 read, write, err;
    ThreadWithMessage   t;
    
    int                 kq;

public:
    SocketWorker()
    : t(*this, this)
    , kq( kqueue() )        
    { 
        t.start();
    }
    
    ~SocketWorker()
    {
        t.requestTermination();
        t.waitTermination();
        close(kq);
    }
    
    void registerSocket(SocketImpl* impl)
    {
        // Set the event filter
        struct kevent changeLst;
        EV_SET(&changeLst, impl->s.getSock(), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, impl);
        kevent(kq, &changeLst, 1, 0, 0, NULL);                
        EV_SET(&changeLst, impl->s.getSock(), EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, impl);
        kevent(kq, &changeLst, 1, 0, 0, NULL);        
        
        clients.insert(impl);
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        struct kevent changeLst;
        EV_SET(&changeLst, impl->s.getSock(), EVFILT_READ, EV_DELETE, 0, 0, 0);
        kevent(kq, &changeLst, 1, 0, 0, NULL);        
        EV_SET(&changeLst, impl->s.getSock(), EVFILT_WRITE, EV_DELETE, 0, 0, 0);
        kevent(kq, &changeLst, 1, 0, 0, NULL);        
        
        clients.erase(impl);
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg);
    virtual void run();
};


#endif // __ASYNC_SOCKET_KEVENT_H__
#endif // _CRAP_SOCKET_KEVENT_
