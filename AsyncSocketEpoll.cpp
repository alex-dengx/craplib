// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_EPOLL_

#include "AsyncSocketEpoll.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>

void SocketWorker::run() 
{
    //    timespec ts;
    //    ts.tv_sec = 0;
    //    ts.tv_nsec = 1000;
    
    while( true ) {
        
        struct epoll_event eqEvents[1024];
        int n = epoll_wait(eq, eqEvents, 1024, -1);
        
        if(n == 0) {
            continue;
        }
        else if(n < 0) { 
            wLog("Err on epoll");
            return;
        }
        
        ActiveCall c(t.msg);        
        message = ONCHANGES;
        
        for(int i=0; i<n; ++i) {
            
            if(eqEvents[i].events & EPOLLERR || eqEvents[i].events & EPOLLHUP) { 
                err.push_back( static_cast<SocketImpl*>(eqEvents[i].data.ptr) );
                continue;
            }
            else if(eqEvents[i].events & EPOLLIN || eqEvents[i].events & EPOLLPRI) {
                
                read.push_back( static_cast<SocketImpl*>(eqEvents[i].data.ptr) );
            }
            else if(eqEvents[i].events & EPOLLOUT) {
                
                write.push_back( static_cast<SocketImpl*>(eqEvents[i].data.ptr) );
            }                                        
        }   
        
        wLog("epoll sizes n=%d; read=%d; write=%d; err=%d", n, read.size(), write.size(), err.size());
        c.call();
    }
}

void SocketWorker::onCall(const ActiveMsg& msg)
{
    /*
     * Process all read, write and error changes
     */
    while( !err.empty() ) {
        SocketImpl* cur = err.front();
        err.pop_front();     
        if( clients.find(cur) != clients.end() )
            cur->onDisconnect();
        deregisterSocket(cur);        
    }    
    
    while( !read.empty() ) {
        SocketImpl* cur = read.front();
        read.pop_front();
        if( clients.find(cur) != clients.end() )
            cur->onCanRead();
    }
    
    while( !write.empty() ) {
        SocketImpl* cur = write.front();
        write.pop_front();
        if( clients.find(cur) != clients.end() )
            cur->onCanWrite();
    }
}

#endif // _CRAP_SOCKET_EPOLL_
