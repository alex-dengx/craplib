//
//  HTTPRequest.h
//  craplib
//
//  Created by Григорий Бутейко on 22.04.11.
//  Copyright 2011 VITO. All rights reserved.
//

#pragma once
#include "AsyncSocket.h"

class HTTPRequest : private RWSocket::Delegate {
public:
	class Delegate : public RWSocket::Delegate {
	public:
		virtual void onHeader(HTTPRequest * who, const std::string& name, const std::string& value)=0;
		virtual void onFinishHeaders(HTTPRequest * who)=0;
		virtual void onBodyFinished(HTTPRequest * who)=0;
		virtual ~Delegate()
		{}
	};
	HTTPRequest(Delegate * delegate, const std::string& host, const std::string& service):delegate(delegate), socket(this, host, service)
	{}
	HTTPRequest(Delegate * delegate, Socket& sock):delegate(delegate), socket(this, sock)
	{}
	void sendHeader(const std::string& header);
	void startBody(long long content_size);
	void sendChunk(const Data & chunk);
private:
	Delegate * delegate;
	RWSocket socket;
	virtual void onDisconnect(const RWSocket&);
	virtual void onRead(const RWSocket&, const Data&);
	virtual void onCanWrite(const RWSocket&);
	virtual void onError(const RWSocket&);
};
