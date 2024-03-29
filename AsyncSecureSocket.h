// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SECURE_SOCKET_H__
#define __ASYNC_SECURE_SOCKET_H__

#include "AsyncSocket.h"
#include "AsyncSSL.h"

class SecureRWSocket
: public RWSocket::Delegate
{
private:
    RWSocket sock;
    SSLTube  ssl;
    
public:
    SecureRWSocket(Delegate* del, const std::string& host, const std::string& service, SSLContext& ctx) 
    : sock(this, host, service)
    , ssl()
    { 
        // 1) get data from sock_
        // 2) tell ssl_ to perform handshake using that data
    }
    
    // RWSocket delegate methods
    virtual void onDisconnect(const RWSocket&);
    virtual void onCanRead(const RWSocket&);
    virtual void onCanWrite(const RWSocket&);
};

#endif // __ASYNC_SECURE_SOCKET_H__