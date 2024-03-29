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
#include <netdb.h>
#include <fcntl.h>

Socket::Socket()
: sock(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{ }

Socket::Socket(int s)
: sock(s)
{ }

Socket::~Socket()
{
    if(sock != 0) {
        close(sock);
    }
}


RWSocket::RWSocket(Delegate* del, Socket& sock) 
: SocketImpl(sock)
, delegate(del)
{
    statics().registerSocket(this);
}

RWSocket::RWSocket(Delegate* del, const std::string& host, const std::string& service)
: delegate(del)
{
    struct addrinfo *addr;
    struct addrinfo hints = {};
    
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_V4MAPPED;
    
    // TODO: what if we use IP instead of hostname?
    int errcode = getaddrinfo (host.c_str(), service.c_str(), &hints, &addr);
    if (errcode != 0)
    {
        wLog("getaddrinfo failed");
        return;
    }
    
    int set = 1;
#ifndef __linux__
    setsockopt(s.getSock(), SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
#endif
    setsockopt(s.getSock(), SOL_SOCKET, SO_REUSEADDR, (void *)&set, sizeof(int));
    errcode = connect(s.getSock(), addr->ai_addr, addr->ai_addrlen);
    freeaddrinfo(addr);

    if ( errcode < 0)
    {
        wLog("connect failed, will get error on write");
        return;
    }          
    
    // Make non-blocking         
    int opts = fcntl(s.getSock(),F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(s.getSock(),F_SETFL,opts) < 0) {
        perror("fcntl(F_SETFL)");
        return;
    }  
    
    statics().registerSocket(this);
}

RWSocket::~RWSocket()
{
    statics().deregisterSocket(this);
}

int RWSocket::write(Data & data)
{
#ifdef __linux__
    int written = (int)send(s.getSock(), data.getData(), data.getSize(), MSG_NOSIGNAL);
#else
    int written = (int)send(s.getSock(), data.getData(), data.getSize(), 0);
#endif
	if(written < 0) // Nothing is written - most likely socket is dead
		return 0;
	data = Data(data, written, data.getSize() - written);
	return written;
}

int RWSocket::read(Data & data) 
{
    // Read data
    Data d(65536); // Hrissan - this effectively allocs 64k per read (event if nothing to read) - lots of memory!
#ifdef __linux__
    int r = (int)recv(s.getSock(), d.lock(), d.getSize(), MSG_NOSIGNAL);
#else
    int r = (int)::read(s.getSock(), d.lock(), d.getSize());
#endif
	if(r < 0) // Nothing is read - most likely socket is dead
	{
		data = Data();
		return 0;
	}
    data = Data(d, 0, r);
    return r;
}

LASocket::LASocket(Delegate* del, const std::string& service)
: delegate(del)
, service(service)
{
    if(delegate == NULL)
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
    setsockopt(s.getSock(), SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    if( bind(s.getSock(), (const struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        wLog("bind failed.");
        return;
    }
    
    // Make non-blocking         
    int opts = fcntl(s.getSock(),F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(s.getSock(),F_SETFL,opts) < 0) {
        perror("fcntl(F_SETFL)");
        return;
    }  
    
    if( listen(s.getSock(), 1024) < 0 )
    {
        wLog("listen failed\n");
        return;
    }
    
    
    statics().registerSocket(this);
}


LASocket::LASocket(Delegate* del, const NetworkInterface& nif, const std::string& service)
: delegate(del)
, iface(nif)
, service(service)
{
    if(delegate == NULL)
        throw std::exception();
    
    struct sockaddr* sockAddr = NULL;
    struct sockaddr_in serverAddress = {};
    struct sockaddr_in6 serverAddress6 = {};
    
    if(nif.family == NetworkInterface::IPv4) {
        serverAddress.sin_family = AF_INET;
#ifndef __linux__
        serverAddress.sin_len    = sizeof(serverAddress);
#endif
        serverAddress.sin_port = htons(atoi(service.c_str()));
        serverAddress.sin_addr.s_addr = nif.addr.sin_addr.s_addr;
        
        sockAddr = (struct sockaddr*)&serverAddress;        
        
    } else {        
        serverAddress6.sin6_family = AF_INET6;
#ifndef __linux__
        serverAddress6.sin6_len    = sizeof(serverAddress6);
        serverAddress6.sin6_port = htons(atoi(service.c_str()));
        serverAddress6.sin6_flowinfo = nif.addr6.sin6_flowinfo;
        serverAddress6.sin6_scope_id = nif.addr6.sin6_scope_id;
        serverAddress6.sin6_addr.__u6_addr = nif.addr6.sin6_addr.__u6_addr;
#else
#warning ipv6 not yet fully supported for linux
#endif
        sockAddr = (struct sockaddr*)&serverAddress6;        
    }
    
    int set = 1;
#ifndef __linux__
    setsockopt(s.getSock(), SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    
    if( bind(s.getSock(), (const struct sockaddr *)sockAddr, 
             nif.family == NetworkInterface::IPv4 ? 
             sizeof(serverAddress) : sizeof(serverAddress6)) < 0)
    {
        wLog("bind failed.");
        return;
    }
    
    // Make non-blocking         
    int opts = fcntl(s.getSock(), F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(s.getSock(), F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return;
    }  
    
    if( listen(s.getSock(), 5) < 0 )
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
	while(true) // Hrissan: we should accept ALL clients, not only the first one
	{
#ifndef __linux__
		int cli = accept(s.getSock(), NULL, NULL);
#else
		int cli = accept4(s.getSock(), NULL, NULL, O_NONBLOCK);
#endif
		if(cli == -1)
			break; // No more accepted sockets
		Socket sock(cli);
        wLog("[0x%04X] LASocket::onCanRead created Socket [%d]", this, cli);
        
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
        
//        linger set_linger;
//        set_linger.l_onoff = 0;
////        set_linger.l_linger = 1;
//        setsockopt(cli, SOL_SOCKET, SO_LINGER, (char *)&set_linger,
//                   sizeof(set_linger));        
        
        delegate->onNewClient(*this, sock);        
	}
}
