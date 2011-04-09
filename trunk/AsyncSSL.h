// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SSL_H__
#define __ASYNC_SSL_H__

#include "StaticRefCounted.h"
#include "LogUtil.h"
#include <string>

/**
 * This class gets automatically constructed when SSL is needed first time
 * and is being destructed when there are no more SSL contexts in the application
 */
class SSLLoader
{
public:    
    SSLLoader()
    {
        // TODO: load openssl library
        wLog("Load SSL library..");
    }
    
    ~SSLLoader()
    {
        // TODO: free openssl library
        wLog("UnLoad SSL library..");
    }
};

class SSLContext
: private StaticRefCounted<SSLLoader>
{
public:
    SSLContext(const std::string& key, const std::string& pass=std::string(""))
    { 
        // TODO: create a SSL_CTX
        wLog("SSL context being initialized: %s", key.c_str());
    }

    ~SSLContext()
    {
        // TODO: free the context
    }
};

#endif // __ASYNC_SSL_H__