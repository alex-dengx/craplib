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
public:
    enum Family {
        UNKNOWN = 0,
        ANY = 1,
        IP = 2,
        IPv4 = 4,
        IPv6 = 6
    };
    
private:
    friend class NetworkInterfaces;

    std::string     ip_;
    std::string     if_;
    union {
        struct sockaddr_in  addr_;
        struct sockaddr_in6 addr6_;
    };
    Family          family_;
    
    NetworkInterface(ifaddrs* inaddr)
    {
        if_ = std::string( inaddr->ifa_name );
                
        if(inaddr->ifa_addr->sa_family == AF_INET6) {
            char straddr[INET6_ADDRSTRLEN] = {};
            struct sockaddr_in6* addr = (struct sockaddr_in6*)inaddr->ifa_addr;
            ip_ = std::string( inet_ntop(AF_INET6, &addr->sin6_addr,
                                         straddr, sizeof(straddr)) );
            family_ = IPv6;
            std::swap(addr6_, *addr);
            
        } else if (inaddr->ifa_addr->sa_family == AF_INET) {
            
            struct sockaddr_in* addr = (struct sockaddr_in*)inaddr->ifa_addr;
            ip_ = std::string( inet_ntoa(addr->sin_addr) );        
            family_ = IPv4;
            std::swap(addr_, *addr);
            
        } else {
            
            family_ = UNKNOWN;
            // Not interested
        }
    }
    
public:
    NetworkInterface()
    { }
    
    NetworkInterface(const NetworkInterface& other)
    : ip_(other.ip_)
    , if_(other.if_)
    , family_(other.family_)
    {
        if(family_==IPv4)
            std::swap(addr_, *(struct sockaddr_in*)(&other.addr_));        
        else if(family_==IPv6)
            std::swap(addr6_, *(struct sockaddr_in6*)(&other.addr6_));                    
    }
       
    ~NetworkInterface()
    {
    }
    
    inline std::string ip() const
    {
        return ip_;
    }
    
    inline std::string name() const
    {
        return if_;
    }        
    
    inline Family family() const 
    {
        return family_;
    }
    
    inline const struct sockaddr_in* const addr() const 
    {
        return &addr_;
    }
    
    inline const struct sockaddr_in6* const addr6() const 
    {
        return &addr6_;
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
    
    ifaddrs*                    addrs_;
    NetworkInterface            curInterface_;
    NetworkInterface::Family    filter_;
    
public:
    NetworkInterfaces(Delegate* del, NetworkInterface::Family filter)
    : delegate_(del)
    , t_(*this, *this)
    , message_(IDLE)
    , filter_(filter)
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