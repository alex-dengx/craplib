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
 
        {
            // Wait till we have some clients
            CondLock lock(c_, true);
        }

        struct epoll_event eqEvents_[1024];
	int n = epoll_wait(eq_, eqEvents_, 1024, -1);
	
        if(n == 0) {
            continue;
        }
        else if(n < 0) { 
            wLog("Err on epoll");
            return;
        }
        
        ActiveCall c(t_.msg_);        
        message_ = ONCHANGES;

        for(int i=0; i<n; ++i) {

            if(eqEvents_[i].events & EPOLLERR || eqEvents_[i].events & EPOLLHUP) { 
                err_.push_back( static_cast<SocketImpl*>(eqEvents_[i].data.ptr) );
                continue;
            }
	    else if(eqEvents_[i].events & EPOLLIN || eqEvents_[i].events & EPOLLPRI) {

                    read_.push_back( static_cast<SocketImpl*>(eqEvents_[i].data.ptr) );
            }
	    else if(eqEvents_[i].events & EPOLLOUT) {

                    write_.push_back( static_cast<SocketImpl*>(eqEvents_[i].data.ptr) );
            }                                        
        }   
        
        wLog("epoll sizes n=%d; read=%d; write=%d; err=%d", n, read_.size(), write_.size(), err_.size());
        c.call();
    }
}

void SocketWorker::handleChanges()
{
    /*
     * Process all read, write and error changes
     */
    while( !read_.empty() ) {
        SocketImpl* cur = read_.front();
        read_.pop_front();
        if(find(clients_.begin(), clients_.end(), cur ) == clients_.end())
            continue;
        
        if( cur->isListening() ) {
            cur->onRead(); // new client
            
        } else {
            
            // Try to actually read
            int bytes = (int)cur->readData();
            if(bytes > 0) {
                cur->onRead();
            } else if(bytes == 0) {
                cur->onDisconnect();                
                deregisterSocket(cur);
            }
            else {
                cur->onError();                
                deregisterSocket(cur);
            }
        }        
    }
    
    while( !write_.empty() ) {
        SocketImpl* cur = write_.front();
        write_.pop_front();
        if(find(clients_.begin(), clients_.end(), cur ) == clients_.end())
            continue;
        
        if( cur->wantWrite() ) {
            cur->onCanWrite();
        }
    }
    
    while( !err_.empty() ) {
        SocketImpl* cur = err_.front();
        err_.pop_front();        
        if(find(clients_.begin(), clients_.end(), cur ) == clients_.end())
            continue;

        cur->onError();
    }    
}

#endif // _CRAP_SOCKET_EPOLL_
