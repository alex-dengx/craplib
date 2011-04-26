// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_KEVENT_

#pragma once
#ifndef __ASYNC_SOCKET_KEVENT_H__
#define __ASYNC_SOCKET_KEVENT_H__

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
#include "AsyncNetInterface.h"

class RWSocket;
class LASocket;

//#define MAX_CONNECTIONS 65000

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

//    Vec                 clients_;
    Vec                 read_, write_, err_;

    ThreadWithMessage   t_;
    
//    enum message_enum { IDLE, ONCHANGES };
//    message_enum        message_;    

    int                 kq_;

public:
    SocketWorker()
    : c_(false)
    , t_(*this, *this)
//    , message_(IDLE)
    , kq_( kqueue() )        
    { 
        t_.start();
    }
    
    ~SocketWorker()
    {
        t_.requestTermination();
        t_.waitTermination();
        close(kq_);
    }
    
    void registerSocket(SocketImpl* impl)
    {
        CondLock lock(c_);
//        if(clients_.size() >= MAX_CONNECTIONS-1) {
//            wLog("can't handle this client - kqueue buffer is full.");
//            return;
//        }
        
//        clients_.push_back(impl);
        
        // Set the event filter
        struct kevent changeLst;
        EV_SET(&changeLst, impl->s.get_sock(), EVFILT_READ, EV_ADD, 0, 0, impl);
        kevent(kq_, &changeLst, 1, 0, 0, NULL);                
        EV_SET(&changeLst, impl->s.get_sock(), EVFILT_WRITE, EV_ADD, 0, 0, impl);
        kevent(kq_, &changeLst, 1, 0, 0, NULL);        

        wLog("socket attached. new clients size = hren; SOCKFD=%d", impl->s.get_sock());
        
        lock.set(true); // New client available
    }
    
    void deregisterSocket(SocketImpl* impl)
    {
        read_.erase( std::remove(read_.begin(), read_.end(), impl), read_.end());
        write_.erase( std::remove(write_.begin(), write_.end(), impl), write_.end());
        err_.erase( std::remove(err_.begin(), err_.end(), impl), err_.end());

        CondLock lock(c_);
//        clients_.erase( std::remove(clients_.begin(), clients_.end(), impl), clients_.end());        
                
        struct kevent changeLst;
        EV_SET(&changeLst, impl->s.get_sock(), EVFILT_READ, EV_DELETE, 0, 0, 0);
        kevent(kq_, &changeLst, 1, 0, 0, NULL);        
        EV_SET(&changeLst, impl->s.get_sock(), EVFILT_WRITE, EV_DELETE, 0, 0, 0);
        kevent(kq_, &changeLst, 1, 0, 0, NULL);        
        
        lock.set(true);
    }
        
    // Processed on main thread
    virtual void onCall(const ActiveMsg& msg);
    virtual void run();
};


#endif // __ASYNC_SOCKET_KEVENT_H__
#endif // _CRAP_SOCKET_KEVENT_
