// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSocket.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>

RWSocket::~RWSocket()
{
    t_.requestTermination();
    t_.waitTermination();
    
    close(sock_);
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
            
            sock_ = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
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
        }
        
        // Make non-blocking         
        int opts = fcntl(sock_,F_GETFL);
        if (opts < 0) {
            perror("fcntl(F_GETFL)");
            ActiveCall call(t_.msg_);
            message_ = ONERROR;            
            call.call();
            return;
        }
        opts = (opts | O_NONBLOCK);
        if (fcntl(sock_,F_SETFL,opts) < 0) {
            perror("fcntl(F_SETFL)");
            ActiveCall call(t_.msg_);
            message_ = ONERROR;            
            call.call();
            return;
        }
        
        while( true ) {
            
            // Wait for read/write access
            fd_set read_fd;
            FD_ZERO(&read_fd);
            FD_SET(sock_, &read_fd);
            
            fd_set write_fd;
            FD_ZERO(&write_fd);
            FD_SET(sock_, &write_fd);
            
            fd_set err_fd;
            FD_ZERO(&err_fd);
            FD_SET(sock_, &err_fd);
            
            struct timeval timeout;
            timeout.tv_sec = 3;
            timeout.tv_usec = 0;
            
            int changes = select(sock_+1, &read_fd, &write_fd, &err_fd, &timeout);
            
            // If not timeout
            if(changes > 0) {
                
                if( FD_ISSET(sock_, &read_fd) ) {
                    
                    ActiveCall c(t_.msg_);
                    message_ = CANREAD;
                    
                    // Read data
                    readData_.fill(0);
                    size_t bytes = read(sock_, readData_.lock(), 1024);
                    if (bytes <= 0) {
                        message_ = CLOSE;
                        c.call();
                        return; // Connection closed                    
                    }
                    
                    c.call();                    
                }
                
                if( FD_ISSET(sock_, &write_fd) && wantWrite_ ) {
                    // Can write
                    ActiveCall c(t_.msg_);
                    message_ = CANWRITE;                                        
                    c.call();
                }
                
                if( FD_ISSET(sock_, &err_fd) ) {
                    ActiveCall c(t_.msg_);
                    message_ = ONERROR;                                        
                    c.call();
                    
                    return;
                }
            }
        }
    }
}