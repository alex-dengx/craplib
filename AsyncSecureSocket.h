// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SECURE_SOCKET_H__
#define __ASYNC_SECURE_SOCKET_H__

#include "AsyncSocket.h"
#include "AsyncSSL.h"

class SecureRWSocket
: public RWSocket
{
private:
    SSLContext&     ctx_;
    
public:
    SecureRWSocket(Delegate* del, const std::string& host, const std::string& service, SSLContext& ctx) 
    : RWSocket(del, host, service)
    , ctx_(ctx)
    { }
};

#endif // __ASYNC_SECURE_SOCKET_H__