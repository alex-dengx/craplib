// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>

#include "Data.h"
#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"

class RWSocket
: public Runnable
, public ActiveMsgDelegate
{
public:
    struct Delegate
    {
        virtual void onConnect(const RWSocket&) = 0;
        virtual void onRead(const RWSocket&, const Data&) = 0;
        virtual void onCanWrite(const RWSocket&) = 0;
        virtual void onError(const RWSocket&) = 0;
        virtual ~Delegate() { };
    };

private:
    Delegate& delegate_;
    
    int                 sock_;
    std::string         host_, 
                        port_;
    struct addrinfo     *addr_;
    
    enum message_enum { CONNECTING, CONNECTED, CANREAD, CANWRITE, CLOSE };
    message_enum        message_;    

    ThreadWithMessage   t_;
    
    Data                readData_;
    bool                wantWrite_;
    
    virtual void onCall(const ActiveMsg& msg) {
        switch(message_) {
            case CONNECTED:
                delegate_.onConnect(*this);
                break;
            case CANREAD:
                delegate_.onRead(*this, readData_);
                break;
            case CANWRITE:
                delegate_.onCanWrite(*this);
                break;
            default:
                wLog("--");
                break;
        }
    }
    
public:
    RWSocket(Delegate& del, const std::string& host, const std::string& port)
    : delegate_(del)
    , sock_(0)
    , host_(host)
    , port_(port)
    , message_(CONNECTING)
    , t_(*this, *this)
    , readData_(1024)
    , wantWrite_(false)
    {
        t_.start();
    }
    
    ~RWSocket()
    {
        t_.requestTermination();
        t_.waitTermination();

        close(sock_);
        freeaddrinfo(addr_);
    }

    
    const Data& write(const Data& bytes)
    {
        wantWrite_ = false;
        
        size_t written = send(sock_, bytes.get_data(), bytes.get_size(), 0);
        wLog("written %u bytes from %u.", written, bytes.get_size());
        
        if(bytes.get_size()-written > 0)
            wantWrite_ = true;
        
        return Data(bytes, written, bytes.get_size()-written);
    }
    
    virtual void run() 
    {
        {
            // Connect on socket
            {
                
                ActiveCall call(t_.msg_);
                message_ = CONNECTED;
                
                struct addrinfo hints = {};
                
                hints.ai_family = AF_INET6;
                hints.ai_protocol = IPPROTO_TCP;
                hints.ai_socktype = SOCK_STREAM;
                
                // TODO: what if we use IP instead of hostname?
                int errcode = getaddrinfo (host_.c_str(), port_.c_str(), &hints, &addr_);
                if (errcode != 0)
                {
                    wLog("getaddrinfo failed");
                    return;
                }
                
                sock_ = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                int set = 1;
                errcode = setsockopt(sock_, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
                errcode = connect(sock_, addr_->ai_addr, addr_->ai_addrlen);
                if ( errcode < 0)
                {
                    wLog("connect failed, will get error on write");
                    return;
                }            
                
                call.call();
            }
            
            // Make non-blocking
            int opts;            
            opts = fcntl(sock_,F_GETFL);
            if (opts < 0) {
                perror("fcntl(F_GETFL)");
                return;
            }
            opts = (opts | O_NONBLOCK);
            if (fcntl(sock_,F_SETFL,opts) < 0) {
                perror("fcntl(F_SETFL)");
                return;
            }
            
            
            while(1) {
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
                
                if( FD_ISSET(sock_, &read_fd) ) {

                    ActiveCall c(t_.msg_);
                    message_ = CANREAD;

                    // Read data
                    readData_.fill(0);
                    size_t bytes = read(sock_, readData_.lock(), 1024);
                    if (bytes <= 0)
                        return; // Connection closed                    
                    
                    c.call();                    
                }
                
                if( FD_ISSET(sock_, &write_fd) && wantWrite_ ) {
                    // Can write
                    ActiveCall c(t_.msg_);
                    message_ = CANWRITE;                                        
                    c.call();
                }
                
                if( FD_ISSET(sock_, &err_fd) ) {
                    wLog("hren.");
                    return;
                }
            }
        }
    }
    
};


class LASocket
{
    
};

#endif // __ASYNC_SOCKET_H__