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
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (void *)&set, sizeof(int));
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
    
    int written = (int)send(sock_, bytes.get_data(), bytes.get_size(), MSG_NOSIGNAL);
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
    int r = (int)recv(sock_, d.lock(), 1024, MSG_NOSIGNAL);
    readData_ = Data(d, 0, r);
    return r;
}

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

LASocket::LASocket(Delegate* del, const std::string& service)
: delegate_(del)
, service_(service)
{
    if(delegate_ == NULL)
        throw std::exception();
    
    struct sockaddr_in serverAddress = {};    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(service.c_str()));
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Bind on all interfaces
    
    if( bind(sock_, (const struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        wLog("bind failed.");
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
    
    if( listen(sock_, 5) < 0 )
    {
        wLog("listen failed\n");
        return;
    }
    
    
    statics().registerSocket(this);
}


LASocket::LASocket(Delegate* del, const NetworkInterface& nif, const std::string& service)
: delegate_(del)
, if_(nif)
, service_(service)
{
    if(delegate_ == NULL)
        throw std::exception();
    
    struct sockaddr* sockAddr = NULL;
    struct sockaddr_in serverAddress = {};
    struct sockaddr_in6 serverAddress6 = {};
    
    if(nif.family() == NetworkInterface::IPv4) {
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(atoi(service.c_str()));
        serverAddress.sin_addr.s_addr = nif.addr()->sin_addr.s_addr;
        
        sockAddr = (struct sockaddr*)&serverAddress;        
        
    } else {        
        serverAddress6.sin6_family = AF_INET6;
        /*serverAddress6.sin6_len    = sizeof(serverAddress6);
        serverAddress6.sin6_port = htons(atoi(service.c_str()));
        serverAddress6.sin6_flowinfo = nif.addr6()->sin6_flowinfo;
        serverAddress6.sin6_scope_id = nif.addr6()->sin6_scope_id;
        serverAddress6.sin6_addr.__u6_addr = nif.addr6()->sin6_addr.__u6_addr;
        */
	wLog("ipv6 not supported in craplib for linux atm. sorry :)");

        sockAddr = (struct sockaddr*)&serverAddress6;        
    }
    
    if( bind(sock_, (const struct sockaddr *)sockAddr, 
             nif.family()==NetworkInterface::IPv4 ? 
             sizeof(serverAddress) : sizeof(serverAddress6)) < 0)
    {
        wLog("bind failed.");
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
    
    if( listen(sock_, 5) < 0 )
    {
        wLog("listen failed\n");
        return;
    }
    
    
    statics().registerSocket(this);
}

LASocket::~LASocket()
{
    statics().deregisterSocket(this);
    freeaddrinfo(addr_);
}


void LASocket::onRead()
{
    // Indicates there is a new client
    int cli = accept4(sock_, NULL, NULL, O_NONBLOCK);
    if(cli != -1) {
        Socket sock(cli);
        delegate_->onNewClient(*this, sock);        
    } else {
        perror("accept()");
    }
}

#endif // _CRAP_SOCKET_EPOLL_
