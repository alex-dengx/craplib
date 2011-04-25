// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSocket.h"

#if defined(_CRAP_SOCKET_KEVENT_)
#include "AsyncSocketKevent.cpp"

#elif defined(_CRAP_SOCKET_EPOLL_)
#include "AsyncSocketEpoll.cpp"

#elif defined(_CRAP_SOCKET_SELECT_)
#include "AsyncSocketSelect.cpp"

#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/event.h>
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
#ifndef __linux__
    setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
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
    
#ifdef __linux__
    int written = (int)send(sock_, bytes.get_data(), bytes.get_size(), MSG_NOSIGNAL);
#else
    int written = (int)send(sock_, bytes.get_data(), bytes.get_size(), 0);
#endif
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
#ifdef __linux__
    int r = (int)recv(sock_, d.lock(), 1024, MSG_NOSIGNAL);
#else
    int r = (int)read(sock_, d.lock(), 1024);
#endif
    readData_ = Data(d, 0, r);
    return r;
}

LASocket::LASocket(Delegate* del, const std::string& service)
: delegate_(del)
, service_(service)
{
    if(delegate_ == NULL)
        throw std::exception();
    
    struct sockaddr_in serverAddress = {};    
    serverAddress.sin_family = AF_INET;
#ifndef __linux__
    serverAddress.sin_len    = sizeof(serverAddress);
#endif
    serverAddress.sin_port = htons(atoi(service.c_str()));
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Bind on all interfaces
    
    int set = 1;
    setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
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
#ifndef __linux__
        serverAddress.sin_len    = sizeof(serverAddress);
#endif
        serverAddress.sin_port = htons(atoi(service.c_str()));
        serverAddress.sin_addr.s_addr = nif.addr()->sin_addr.s_addr;
        
        sockAddr = (struct sockaddr*)&serverAddress;        
        
    } else {        
        serverAddress6.sin6_family = AF_INET6;
#ifndef __linux__
        serverAddress6.sin6_len    = sizeof(serverAddress6);
        serverAddress6.sin6_port = htons(atoi(service.c_str()));
        serverAddress6.sin6_flowinfo = nif.addr6()->sin6_flowinfo;
        serverAddress6.sin6_scope_id = nif.addr6()->sin6_scope_id;
        serverAddress6.sin6_addr.__u6_addr = nif.addr6()->sin6_addr.__u6_addr;
#else
#error ipv6 not yet fully supported for linux
#endif
        sockAddr = (struct sockaddr*)&serverAddress6;        
    }
    
    int set = 1;
    setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
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
#ifndef __linux__
    int cli = accept(sock_, NULL, NULL);
#else
    int cli = accept4(sock_, NULL, NULL, O_NONBLOCK);
#endif
    if(cli != -1) {
        Socket sock(cli);
        delegate_->onNewClient(*this, sock);        
    } else {
        perror("accept()");
    }
}
