// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSocket.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>

SocketImpl::SocketImpl()
: sock_(socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP))
{ }

SocketImpl::~SocketImpl()
{
    close(sock_);
}

RWSocket::~RWSocket()
{
    statics().deregisterSocket(this);

    t_.requestTermination();
    t_.waitTermination();
 
    freeaddrinfo(addr_);
}

const Data& RWSocket::write(const Data& bytes)
{
    wantWrite_ = false;
    
    int written = send(sock_, bytes.get_data(), bytes.get_size(), 0);
    wLog("written %u bytes from %u.", written, bytes.get_size());
    
    if(bytes.get_size()-written > 0)
        wantWrite_ = true;
    
    return Data(bytes, written, bytes.get_size()-written);
}


void SocketWorker::run() 
{
    while( true ) {
 
        {
            // Wait till we have some clients
            CondLock lock(c_, true);
            lock.set(!clients_.empty());
        }
        
        fd_set read_fd;
        fd_set write_fd;
        fd_set err_fd;
        
        FD_ZERO(&read_fd);
        FD_ZERO(&write_fd);
        FD_ZERO(&err_fd);
        
        int maxSock = 0;
        
        {
            CondLock lock(c_);
            for(Container::iterator it = clients_.begin(); it != clients_.end(); ++it) {
                FD_SET(it->first, &read_fd);
                FD_SET(it->first, &write_fd);
                FD_SET(it->first, &err_fd);                
                
                maxSock = maxSock>it->first?maxSock:it->first; // Achtung :)
            }
        }
        
        // Wait for read/write access                        
        int changes = select(maxSock+1, &read_fd, &write_fd, &err_fd, NULL);
        
        // If not timeout
        if(changes > 0) {
            
            CondLock lock(c_);
            for(Container::iterator it = clients_.begin(); it != clients_.end(); ++it) {
                
                curSock = it->second;
                
                if( FD_ISSET(it->first, &read_fd) ) {                            
                    ActiveCall c(t_.msg_);
                    message_ = CANREAD;
                    c.call();                    
                }
                
                if( FD_ISSET(it->first, &write_fd) ) {
                    ActiveCall c(t_.msg_);
                    message_ = CANWRITE;                                        
                    c.call();
                }
                
                if( FD_ISSET(it->first, &err_fd) ) {
                    ActiveCall c(t_.msg_);
                    message_ = ONERROR;                                        
                    c.call();
                }
                
                curSock = NULL;
            }            
        }
    }
}

void RWSocket::run() 
{
    {
        // Connect on socket
        {                        
            struct addrinfo hints = {};
            
            hints.ai_family = AF_INET6;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_socktype = SOCK_STREAM;
            
            // TODO: what if we use IP instead of hostname?
            int errcode = getaddrinfo (host_.c_str(), service_.c_str(), &hints, &addr_);
            if (errcode != 0)
            {
                wLog("getaddrinfo failed");
                ActiveCall call(t_.msg_);
                message_ = ONERROR;            
                call.call();
                return;
            }
            
            int set = 1;
            errcode = setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
            errcode = connect(sock_, addr_->ai_addr, addr_->ai_addrlen);
            if ( errcode < 0)
            {
                wLog("connect failed, will get error on write");
                ActiveCall call(t_.msg_);
                message_ = ONERROR;            
                call.call();
                return;
            }          
            
            ActiveCall call(t_.msg_);
            message_ = CONNECTED;            
            call.call();
            
            // Make non-blocking         
            int opts = fcntl(sock_,F_GETFL);
            if (opts < 0) {
                perror("fcntl(F_GETFL)");
                return;
            }
            opts = (opts | O_NONBLOCK);
            if (fcntl(sock_,F_SETFL,opts) < 0) {
                perror("fcntl(F_SETFL)");
                return;
            }            
        }
                
        while(1) {
            CondLock lock(jobOrTermination_, true);    
            
            if( readFlag_ ) {
                // Read data
                readData_.fill(0);
                size_t bytes = read(sock_, readData_.lock(), 1024);
                if (bytes <= 0) {
                    ActiveCall call(t_.msg_);
                    message_ = CLOSE;            
 
                    statics().deregisterSocket(this);                    
                    call.call();
                    
                    return; // Connection closed                    
                }
                
                readFlag_ = false;
                
                ActiveCall call(t_.msg_);
                message_ = ONREAD;            
                call.call();
                
            }
            else if( writeFlag_ && wantWrite_ ) {

                writeFlag_ = false;

                ActiveCall call(t_.msg_);
                message_ = ONWRITE;            
                call.call();
            }
            
            lock.set(false);
        }
    }
}