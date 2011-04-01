// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSocketBSD.h"

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
    // TODO: move to member buffer
    Data d(1024);
    int r = (int)read(sock_, d.lock(), 1024);
    readData_ = Data(d, 0, r);
    return r;
}

void SocketWorker::run() 
{
    while( true ) {
 
        int csz = 0;
        {
            // Wait till we have some clients
            CondLock lock(c_, true);
            csz = (int)clients_.size();
        }

        struct kevent kqEvents_[csz];
        int n = kevent(kq_, 0, 0, kqEvents_, csz, NULL);        
        if(n <= 0) { 
            wLog("ERROR ON KEVENT");
            continue;
        }
        
        ActiveCall c(t_.msg_);        
        message_ = ONCHANGES;

        for(int i=0; i<n; ++i) {

            if(kqEvents_[i].flags & EV_ERROR) { 
                err_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                continue;
            }
                   
            switch(kqEvents_[i].filter) {

                case EVFILT_READ: {
                    if(find(clients_.begin(), clients_.end(), static_cast<SocketImpl*>(kqEvents_[i].udata) ) != clients_.end()) 
                        read_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                    break;
                }
                case EVFILT_WRITE: {
                    if(find(clients_.begin(), clients_.end(), static_cast<SocketImpl*>(kqEvents_[i].udata) ) != clients_.end())
                        write_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                    break;
                }
            }                                        
        }   
        
          wLog("kevent sizes n=%d; read=%d; write=%d; err=%d", n, read_.size(), write_.size(), err_.size());
        
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

LASocket::LASocket(Delegate* del, const std::string& host, const std::string& service)
: delegate_(del)
, host_(host)
, service_(service)
{
    if(delegate_ == NULL)
        throw std::exception();
    
    struct sockaddr_in serverAddress = {};    
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_len    = sizeof(serverAddress);
    serverAddress.sin_port = htons(atoi(service.c_str()));
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
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
    
    if( listen(sock_, 512) < 0 )
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
    int cli = accept(sock_, NULL, NULL);
    if(cli != -1) {
        Socket sock(cli);
        delegate_->onNewClient(*this, sock);        
    } else {
        perror("accept()");
    }
}

