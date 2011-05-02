// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SSL_H__
#define __ASYNC_SSL_H__

#include "StaticRefCounted.h"
#include "Data.h"
#include "LogUtil.h"

#include <string>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>

namespace ssl {
	
	class Init
	{
	public:	
		Init();
		~Init();
	};

	class ContextWrapper
	{
		SSL_CTX * ctx;		
	public:
		SSL_CTX * getCtx()const { return ctx; }
		explicit ContextWrapper(SSL_METHOD *meth):ctx(SSL_CTX_new(meth))
		{}
		~ContextWrapper()
		{
			SSL_CTX_free(ctx);
		}
	};
	
	enum engine_enum { SERVER, CLIENT };

	class Engine
	: public DataInputStream
	, public DataOutputStream
	{
	public:
		explicit Engine(ContextWrapper & ctx, engine_enum kind);
		virtual ~Engine();
		SSL * getSSL()const { return ssl; }
		bool set_p12_certificate_privatekey(const Data & data, const std::string & password);
		bool set_pem_certificate_privatekey(const Data & data, const std::string & password);
		virtual int read(Data & data); // Read application data
		virtual int write(Data & data); // Write application data
		void transfer_data(DataInputStream & network_in, DataOutputStream & network_out);
		int get_verify_error();
	private:
		SSL * ssl;
		BIO * bioIn;
		BIO * bioOut;
		
		bool checked_peer_certificate;
		Data waiting_to_app_data;
		Data waiting_to_socket_data;
		void read_app();
		void read_socket(DataInputStream & network_in);
		void write_socket(DataOutputStream & network_out);
	};

}
#endif // __ASYNC_SSL_H__