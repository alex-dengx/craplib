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
, addr_(0)
{
    statics().registerSocket(this);
}

RWSocket::RWSocket(Delegate* del, const std::string& host, const std::string& service)
: delegate_(del)
, addr_(0)
{
    struct addrinfo hints = {};
    
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_V4MAPPED;
    
    // TODO: what if we use IP instead of hostname?
    int errcode = getaddrinfo (host.c_str(), service.c_str(), &hints, &addr_);
    if (errcode != 0)
    {
        wLog("getaddrinfo failed");
        return;
    }
    
    int set = 1;
#ifndef __linux__
    setsockopt(s.get_sock(), SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
    linger set_linger;
    set_linger.l_onoff = 0;
    setsockopt(s.get_sock(), SOL_SOCKET, SO_LINGER, (char *)&set_linger,
               sizeof(set_linger));            
#endif
    setsockopt(s.get_sock(), SOL_SOCKET, SO_REUSEADDR, (void *)&set, sizeof(int));
    errcode = connect(s.get_sock(), addr_->ai_addr, addr_->ai_addrlen);
    if ( errcode < 0)
    {
        wLog("connect failed, will get error on write");
        return;
    }          
    
    // Make non-blocking         
    int opts = fcntl(s.get_sock(),F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(s.get_sock(),F_SETFL,opts) < 0) {
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

int RWSocket::write(Data & data)
{
#ifdef __linux__
    int written = (int)send(s.get_sock(), data.get_data(), data.get_size(), MSG_NOSIGNAL);
#else
    int written = (int)send(s.get_sock(), data.get_data(), data.get_size(), 0);
#endif
	if(written < 0) // Nothing is written - most likely socket is dead
		return 0;
	data = Data(data, written, data.get_size());
	return written;
}

int RWSocket::read(Data & data) 
{
    // Read data
    Data d(65536);
#ifdef __linux__
    int r = (int)recv(s.get_sock(), d.lock(), d.get_size(), MSG_NOSIGNAL);
#else
    int r = (int)::read(s.get_sock(), d.lock(), d.get_size());
#endif
	if(r < 0) // Nothing is read - most likely socket is dead
		return 0;
    data = Data(d, 0, r);
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
    
#ifndef __linux__
    int set = 1;
    setsockopt(s.get_sock(), SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    if( bind(s.get_sock(), (const struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        wLog("bind failed.");
        return;
    }
    
    // Make non-blocking         
    int opts = fcntl(s.get_sock(),F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(s.get_sock(),F_SETFL,opts) < 0) {
        perror("fcntl(F_SETFL)");
        return;
    }  
    
    if( listen(s.get_sock(), 5) < 0 )
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
    setsockopt(s.get_sock(), SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
    if( bind(s.get_sock(), (const struct sockaddr *)sockAddr, 
             nif.family()==NetworkInterface::IPv4 ? 
             sizeof(serverAddress) : sizeof(serverAddress6)) < 0)
    {
        wLog("bind failed.");
        return;
    }
    
    // Make non-blocking         
    int opts = fcntl(s.get_sock(), F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(s.get_sock(), F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return;
    }  
    
    if( listen(s.get_sock(), 5) < 0 )
    {
        wLog("listen failed\n");
        return;
    }
    
    
    statics().registerSocket(this);
}

LASocket::~LASocket()
{
    statics().deregisterSocket(this);
}


void LASocket::onCanRead()
{
    // Indicates there is a new client
#ifndef __linux__
    int cli = accept(s.get_sock(), NULL, NULL);
#else
    int cli = accept4(s.get_sock(), NULL, NULL, O_NONBLOCK);
#endif
    if(cli != -1) {
        Socket sock(cli);
        
#ifndef __linux__
        int set = 1;
        setsockopt(cli, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
      
        // Make non-blocking         
        int opts = fcntl(cli, F_GETFL);
        if (opts < 0) {
            perror("fcntl(F_GETFL)");
            return;
        }
        opts = (opts | O_NONBLOCK);
        if (fcntl(cli, F_SETFL, opts) < 0) {
            perror("fcntl(F_SETFL)");
            return;
        }  
        
        linger set_linger;
        set_linger.l_onoff = 0;
//        set_linger.l_linger = 1;
        setsockopt(cli, SOL_SOCKET, SO_LINGER, (char *)&set_linger,
                   sizeof(set_linger));        
        
        delegate_->onNewClient(*this, sock);        
    } else {
        perror("accept()");
    }
}
