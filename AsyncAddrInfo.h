// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_FILE_H__
#define __ASYNC_FILE_H__

#include "Thread.h"
#include "ActiveMsg.h"
#include "LogUtil.h"

#include <netdb.h>
#include <fstream>

class AsyncAddrInfo 
{
	class Impl;
	friend class Impl;
public:
	struct HostService
	{
		HostService() {}
		HostService(const std::string & host, const std::string & service): host(host), service(service) {}

		std::string host;
		std::string service;
	};
	class Delegate
	{
	public:	
		virtual ~Delegate() {}
		virtual void on_resolved(AsyncAddrInfo & who, addrinfo * info)=0;
	};
    AsyncAddrInfo(Delegate & delegate, const std::string & host, const std::string & service);
	~AsyncAddrInfo();

	const std::string & get_host()const { return hs.host; }
	const std::string & get_service()const { return hs.service; }
private:
	HostService hs;
	Delegate & delegate;
};

#endif // __ASYNC_FILE_H__