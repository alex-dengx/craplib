// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __SSL_TESTS_H__
#define __SSL_TESTS_H__

#include "AsyncSocket.h"
#include "AsyncSSL.h"
#include "AsyncSecureSocket.h"


SUITE(SSL);

TEST(Basic, SSL) {
    RunLoop rl;
        // TODO: write tests
    rl.run();    
};

#endif // __SSL_TESTS_H__