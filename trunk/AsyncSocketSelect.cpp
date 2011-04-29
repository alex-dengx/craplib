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
            CondLock lock(c, true);
            lock.set(!clients.empty());
        
            FD_ZERO(&readfd);
            FD_ZERO(&writefd);
            FD_ZERO(&errfd);
        
            for(Container::iterator it = clients.begin(); it != clients.end(); ++it) {
                FD_SET((*it)->s.getSock(), &readfd);
                FD_SET((*it)->s.getSock(), &writefd);
                FD_SET((*it)->s.getSock(), &errfd);                
                
                maxSock = maxSock > (*it)->s.getSock() ? maxSock:(*it)->s.getSock(); // Achtung :)                
            }
        }
        
        // Wait for read/write access                        
        int changes = select(maxSock+1, &readfd, &writefd, &errfd, NULL);
        if(changes < 0) {
            // Couldn't handle this connection
            perror("select()");
            continue;
        }
        
        // If not timeout
        if(changes > 0) {
            ActiveCall c(t.msg);
            message = ONCHANGES;
            
            // TODO: swap instead of FD_COPY to speedup?
            FD_COPY(&readfd, &readfd_copy);
            FD_COPY(&writefd, &writefd_copy);
            FD_COPY(&errfd, &errfd_copy);
            
            c.call();
        }
    }
}

void SocketWorker::handleChanges()
{
    for(Container::iterator it = clients.begin(); it != clients.end(); ++it) {        
        if( FD_ISSET((*it)->s.getSock(), &readfd_copy) ) {                                      
            read.push_back(*it);
        }
                                
        if( FD_ISSET((*it)->s.getSock(), &writefd_copy) ) {
            write.push_back(*it);
        }
        
        if( FD_ISSET((*it)->s.getSock(), &errfd_copy) ) {
            err.push_back(*it);
        }            
    }   
    
    /*
     * Process all read, write and error changes
     */
    while( !read.empty() ) {
        SocketImpl* cur = read.front();
        read.pop_front();        
        cur->onCanRead(); // new client
    }
    
    while( !write.empty() ) {
        SocketImpl* cur = write.front();
        write.pop_front();
        cur->onCanWrite();
    }
    
    while( !err.empty() ) {
        SocketImpl* cur = err.front();
        err.pop_front();        
        cur->onDisconnect();
    }    
}

#endif // _CRAP_SOCKET_SELECT_