// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSocket.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>

Socket::Socket()
: sock_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{ }

Socket::Socket(int sock)
: sock_(sock)
{ }

Socket::~Socket()
{
    if(sock_!=0) {
        close(sock_);
    }
}


RWSocket::RWSocket(Delegate* del, Socket& sock) 
: SocketImpl(sock)
, delegate_(del)
, addr_(NULL)
{
    statics().registerSocket(this);
//    onConnect();
}

RWSocket::RWSocket(Delegate* del, const std::string& host, const std::string& service)
: delegate_(del)
, host_(host)
, service_(service)
, addr_(NULL)
{
    struct addrinfo hints = {};
    
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_V4MAPPED;
    
    // TODO: what if we use IP instead of hostname?
    int errcode = getaddrinfo (host_.c_str(), service_.c_str(), &hints, &addr_);
    if (errcode != 0)
    {
        wLog("getaddrinfo failed");
        return;
    }
    
    int set = 1;
    errcode = setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    errcode = connect(sock_, addr_->ai_addr, addr_->ai_addrlen);
    if ( errcode < 0)
    {
        wLog("connect failed, will get error on write");
        return;
    }          
    
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
    
    statics().registerSocket(this);
//    onConnect();
}

RWSocket::~RWSocket()
{
    statics().deregisterSocket(this);
    if(addr_!=NULL)
        freeaddrinfo(addr_);
}

const Data RWSocket::write(const Data& bytes)
{
    wantWrite_ = false;
    
    int written = (int)send(sock_, bytes.get_data(), bytes.get_size(), 0);
    if(written < 0)
        return bytes; // Nothing is written - most likely socket is dead
    
    if(bytes.get_size()-written > 0)
        wantWrite_ = true;
    
    return Data(bytes, written, bytes.get_size()-written);
}

size_t RWSocket::readData() 
{
    // Read data
    Data d(1024);
    int r = (int)read(sock_, d.lock(), 1024);
    readData_ = Data(d, 0, r);
    return r;
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
            // CondLock lock(c_);
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
            
            // CondLock lock(c_);
            for(Container::iterator it = clients_.begin(); it != clients_.end(); ) {
                                
                if( FD_ISSET(it->first, &read_fd) ) {                                      
                    ActiveCall c(t_.msg_);
                    curSock = it->second;

                    size_t bytes = curSock->readData();
                    if(bytes <= 0 && !curSock->isListening()) {
                        message_ = ONCLOSE;
                        clients_.erase(it++); // Deregister
                        
                        c.call();                                       
                        continue;
                    }
                    else {
                        message_ = ONREAD;
                        c.call();            
                    }        
                }
                
                if( FD_ISSET(it->first, &write_fd) ) {
                    if( it->second->wantWrite() ) {
                        ActiveCall c(t_.msg_);
                        curSock = it->second;
                        message_ = CANWRITE;                                        
                        c.call();
                    }
                }
                
                if( FD_ISSET(it->first, &err_fd) ) {
                    ActiveCall c(t_.msg_);
                    message_ = ONERROR;                                        
                    curSock = it->second;
                    c.call();
                }
                
                ++it;
            }            
        }
    }
}


LASocket::LASocket(Delegate* del, const std::string& host, const std::string& service)
: delegate_(del)
, host_(host)
, service_(service)
{
    if(delegate_ == NULL)
        throw std::exception();
    
    struct addrinfo hints = {};
    
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_V4MAPPED;
    
    int errcode = getaddrinfo (host_.c_str(), service_.c_str(), &hints, &addr_);
    if (errcode != 0)
    {
        wLog("getaddrinfo failed");
        return;
    }
    
    int set = 1;
    errcode = setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    errcode = bind(sock_, addr_->ai_addr, addr_->ai_addrlen);
    if ( errcode < 0)
    {
        wLog("bind failed");
        return;
    }          
    
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
    
    errcode = listen(sock_, 512);
    if ( errcode < 0)
    {
        wLog("listen failed.");
        return;
    }
    
    statics().registerSocket(this);
//    onConnect();
}

LASocket::~LASocket()
{
    statics().deregisterSocket(this);
    freeaddrinfo(addr_);
}


void LASocket::onRead()
{
    // Indicates there is a new client
    int cli = accept(sock_, NULL, NULL);
    if(cli != -1) {
        Socket sock(cli);
        delegate_->onNewClient(*this, sock);        
    }
}

