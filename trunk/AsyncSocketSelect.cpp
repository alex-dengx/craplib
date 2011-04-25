// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_SELECT_

#include "AsyncSocketSelect.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

void SocketWorker::run() 
{
    while( true ) {
 
        int maxSock = 0;
        
        {
            // Wait till we have some clients
            CondLock lock(c_, true);
            lock.set(!clients_.empty());
        
            FD_ZERO(&read_fd);
            FD_ZERO(&write_fd);
            FD_ZERO(&err_fd);
        
            for(Container::iterator it = clients_.begin(); it != clients_.end(); ++it) {
                FD_SET((*it)->sock_, &read_fd);
                FD_SET((*it)->sock_, &write_fd);
                FD_SET((*it)->sock_, &err_fd);                
                
                maxSock = maxSock > (*it)->sock_ ? maxSock:(*it)->sock_; // Achtung :)                
            }
        }
        
        // Wait for read/write access                        
        int changes = select(maxSock+1, &read_fd, &write_fd, &err_fd, NULL);
        if(changes < 0) {
            // Couldn't handle this connection
            perror("select()");
            continue;
        }
        
        // If not timeout
        if(changes > 0) {
            ActiveCall c(t_.msg_);
            message_ = ONCHANGES;
            
            // TODO: swap instead of FD_COPY to speedup?
            FD_COPY(&read_fd, &read_fd_copy);
            FD_COPY(&write_fd, &write_fd_copy);
            FD_COPY(&err_fd, &err_fd_copy);
            
            c.call();
        }
    }
}

void SocketWorker::handleChanges()
{
    for(Container::iterator it = clients_.begin(); it != clients_.end(); ++it) {        
        if( FD_ISSET((*it)->sock_, &read_fd_copy) ) {                                      
            read_.push_back(*it);
        }
                                
        if( FD_ISSET((*it)->sock_, &write_fd_copy) ) {
            write_.push_back(*it);
        }
        
        if( FD_ISSET((*it)->sock_, &err_fd_copy) ) {
            err_.push_back(*it);
        }            
    }   
    
    /*
     * Process all read, write and error changes
     */
    while( !read_.empty() ) {
        SocketImpl* cur = read_.front();
        read_.pop_front();
        
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
        
        if( cur->wantWrite() ) {
            cur->onCanWrite();
        }
    }
    
    while( !err_.empty() ) {
        SocketImpl* cur = err_.front();
        err_.pop_front();
        
        cur->onError();
    }    
}

#endif // _CRAP_SOCKET_SELECT_