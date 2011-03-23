// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#include <string>
#include "Data.h"

class RWSocket
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
    
public:
    RWSocket(Delegate& del, const std::string& host, const std::string& port)
    : delegate_(del)
    {
//        struct addrinfo hints = {};
//        struct addrinfo * res = 0;
//        
//        hints.ai_family = AF_INET6;
//        hints.ai_protocol = IPPROTO_TCP;
//        hints.ai_socktype = SOCK_STREAM;
//        //        hints.ai_flags |= AI_CANONNAME;
//        
//        int errcode = getaddrinfo ("gregmacboo.local", "3333", &hints, &res);
//        if (errcode != 0)
//        {
//            printf ("getaddrinfo failed");
//            return;
//        }
//        
//        impl = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
//        int set = 1;
//        errcode = setsockopt(impl, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
//        errcode = connect(impl, res->ai_addr, res->ai_addrlen);
//        if ( errcode < 0)
//        {
//            printf ("connect failed, will get error on write\n");
//            //            return -1;
//        }
//        freeaddrinfo(res); res = 0; // memory BUG 
    }
    
    const Data& write(const Data& bytes);
};


class LASocket
{
    
};

#endif // __ASYNC_SOCKET_H__