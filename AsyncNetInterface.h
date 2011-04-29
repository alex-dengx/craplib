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
        
public:
    NetworkInterface()
    { }
    
    NetworkInterface(const NetworkInterface& other)
    : ip(other.ip)
    , name(other.name)
    , family(other.family)
    {
        if(family == IPv4)
            std::swap(addr, *(struct sockaddr_in*)(&other.addr));        
        else if(family == IPv6)
            std::swap(addr6, *(struct sockaddr_in6*)(&other.addr6));                    
    }
       
    ~NetworkInterface()
    {
    }
    
    std::string     ip;
    std::string     name;
    union {
        struct sockaddr_in  addr;
        struct sockaddr_in6 addr6;
    };
    Family          family;
    
private:
    friend class NetworkInterfaces;
        
    NetworkInterface(ifaddrs* inaddr)
    {
        name = std::string( inaddr->ifa_name );
        
        if(inaddr->ifa_addr->sa_family == AF_INET6) {
            char straddr[INET6_ADDRSTRLEN] = {};
            struct sockaddr_in6* addr = (struct sockaddr_in6*)inaddr->ifa_addr;
            ip = std::string( inet_ntop(AF_INET6, &addr->sin6_addr,
                                         straddr, sizeof(straddr)) );
            family = IPv6;
            std::swap(addr6, *addr);
            
        } else if (inaddr->ifa_addr->sa_family == AF_INET) {
            
            struct sockaddr_in* naddr = (struct sockaddr_in*)inaddr->ifa_addr;
            ip = std::string( inet_ntoa(naddr->sin_addr) );        
            family = IPv4;
            std::swap(addr, *naddr);
            
        } else {
            
            family = UNKNOWN;
            // Not interested
        }
    }
};


class NetworkInterfaces
: public Runnable
, public ActiveMsg::Delegate
{
public:
    struct Delegate 
    {
        virtual void onInterfaceDetected(const NetworkInterfaces&, const NetworkInterface&) = 0;
        virtual ~Delegate() { };
    };
    
    NetworkInterfaces(Delegate* del, NetworkInterface::Family filter)
    : delegate(del)
    , t(*this, this)
    , message(IDLE)
    , filter(filter)
    {        
        if(del == NULL)
            throw std::exception();
        
        t.start();
    }
    
    ~NetworkInterfaces()
    {
        t.requestTermination();
        t.waitTermination();
        
        freeifaddrs(addrs);
    }
    
    virtual void run();    
    virtual void onCall(const ActiveMsg& msg);
    
private:
    Delegate*           delegate;
    ThreadWithMessage   t;
    enum message_type { IDLE, INTERFACE_DETECTED };
    message_type        message;
    
    ifaddrs*                    addrs;
    NetworkInterface            curInterface;
    NetworkInterface::Family    filter;
};

#endif // __ASYNC_NET_INTERFACE_H__