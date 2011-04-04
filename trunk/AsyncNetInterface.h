// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_NET_INTERFACE_H__
#define __ASYNC_NET_INTERFACE_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ifaddrs.h>
#include <exception>
#include <string>

#include "ActiveMsg.h"

class NetworkInterfaces;

/**
 * Represents a single network interface
 */ 
class NetworkInterface
{
private:
    friend class NetworkInterfaces;

    std::string     ip_;
    std::string     if_;
    struct sockaddr addr_;
    
    NetworkInterface(ifaddrs* addr)
    {
        if_ = std::string( addr->ifa_name );
        std::swap(addr_, *addr->ifa_addr);
        ip_ = std::string( inet_ntoa(((sockaddr_in*)&addr_)->sin_addr) );        
    }
    
public:
    NetworkInterface()
    { }
    
    inline std::string ip() const
    {
        return ip_;
    }
    
    inline std::string name() const
    {
        return if_;
    }        
};


class NetworkInterfaces
: public Runnable
, public ActiveMsgDelegate
{
public:
    struct Delegate 
    {
        virtual void onInterfaceDetected(const NetworkInterfaces&, const NetworkInterface&) = 0;
        virtual ~Delegate() { };
    };
    
private:
    Delegate*           delegate_;
    ThreadWithMessage   t_;
    enum message_type { IDLE, INTERFACE_DETECTED };
    message_type        message_;
    
    ifaddrs*            addrs_;
    NetworkInterface    curInterface_;
    
public:
    NetworkInterfaces(Delegate* del)
    : delegate_(del)
    , t_(*this, *this)
    , message_(IDLE)
    {        
        if(del == NULL)
            throw std::exception();
        
        t_.start();
    }
    
    ~NetworkInterfaces()
    {
        t_.requestTermination();
        t_.waitTermination();
        
        freeifaddrs(addrs_);
    }
    
    virtual void run();    
    virtual void onCall(const ActiveMsg& msg);
};

#endif // __ASYNC_NET_INTERFACE_H__