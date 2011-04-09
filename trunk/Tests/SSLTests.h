// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SSL_TESTS_H__
#define __SSL_TESTS_H__

#include "AsyncSocket.h"
#include "AsyncSSL.h"
#include "AsyncSecureSocket.h"

class SecureConnection 
: public SecureRWSocket::Delegate
{
    SSLContext      ctx_;
    SecureRWSocket  sock_;
    Data            data_;
    
public:
    SecureConnection()
    : ctx_("server.pem")
    , sock_(this, "localhost", "4433",  // Connect to localhost:4433
            ctx_)                       // Using the given SSL context
    , data_(1024)
    { }

    // Delegate methods
    // TODO: add some delegate method like 'onConnected' to get notified when
    // SSL handshake is done and connection is established
    virtual void onDisconnect(const RWSocket& sock) 
    {
        if(&sock == &sock_) {
            wLog("[%x] client socket disconnected", &sock);
        }
    }
    
    virtual void onRead(const RWSocket& sock, const Data& d)
    {
        if(&sock == &sock_) {
            wLog("[%x] Got data len %d", &sock, d.get_size());
            
            // Echo
            data_ = sock_.write(d);
        }
    }
    
    virtual void onCanWrite(const RWSocket& sock)
    {
        if(&sock == &sock_ && !data_.empty()) {
            data_ = sock_.write(data_);
        }
    }
    
    virtual void onError(const RWSocket& sock) 
    {
        wLog("[%x] error on sock", &sock);
    }    
};


SUITE(SSL);

TEST(Basic, SSL) {
    RunLoop rl;
    
    SecureConnection s;
    
    rl.run();    
};

#endif // __SSL_TESTS_H__