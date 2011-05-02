// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSSL.h"

namespace ssl {

	Init::Init()
	{
		SSL_load_error_strings();
		SSL_library_init();
		OpenSSL_add_all_algorithms(); // Required for PKCS12 support
	}
	Init::~Init()
	{}

	Engine::Engine(ContextWrapper & ctx, engine_enum kind)
	: ssl( SSL_new(ctx.getCtx()) )
	, bioIn( BIO_new(BIO_s_mem()) )
	, bioOut( BIO_new(BIO_s_mem()) )
	, checked_peer_certificate(false)
	{
		SSL_set_bio(ssl, bioIn, bioOut);
		if( kind == CLIENT )
			SSL_set_connect_state(ssl);
		else
			SSL_set_accept_state(ssl);
	}
	Engine::~Engine()
	{
		SSL_free(ssl); // TODO: Docs says do not free BIO, they will be freed automatically. Need to check
		//BIO_free(bioOut);
		//BIO_free(bioIn);
	}
		bool Engine::set_p12_certificate_privatekey(const Data & data, const std::string & password)
		{
			if( data.empty() ) // BIO undefined behaviour when writing 0
				return false;
			BIO *mem = BIO_new(BIO_s_mem());
			BIO_write(mem, data.getData(), data.getSize());
			PKCS12 * pkcs12 = d2i_PKCS12_bio(mem, NULL);
			BIO_free(mem);
			mem = 0;
			X509 * cax = 0;
			EVP_PKEY * pkey = 0;
//			int succ = 
			PKCS12_parse(pkcs12, password.c_str(), &pkey, &cax, NULL);
//			int err = ERR_get_error();
//			const char * err_str = ERR_error_string(err, 0);
			int cert_res = SSL_use_certificate(ssl, cax);
			if (cax)
				X509_free(cax);
			cax = 0;
			int key_res = SSL_use_PrivateKey(ssl, pkey);
			if( pkey )
				EVP_PKEY_free(pkey);
			pkey = 0;
			int check_res = SSL_check_private_key(ssl);
			return cert_res == 1 && key_res == 1 && check_res == 1;
		}
		bool Engine::set_pem_certificate_privatekey(const Data & data, const std::string & password)
		{
			if( data.empty() ) // BIO undefined behaviour when writing 0
				return false;
			BIO *mem = BIO_new(BIO_s_mem());
			BIO_write(mem, data.getData(), data.getSize());
			X509 * cax = PEM_read_bio_X509(mem, NULL, 0, const_cast<char *>(password.c_str())); // Stupid C guys
			BIO_free(mem);
			mem = 0;
			
			mem = BIO_new(BIO_s_mem());
			BIO_write(mem, data.getData(), data.getSize());
			EVP_PKEY * pkey = PEM_read_bio_PrivateKey(mem, NULL, 0, const_cast<char *>(password.c_str())); // Stupid C guys
			BIO_free(mem);
			mem = 0;
			
			int cert_res = SSL_use_certificate(ssl, cax);
			if (cax)
				X509_free(cax);
			cax = 0;
			int key_res = SSL_use_PrivateKey(ssl, pkey);
			if( pkey )
				EVP_PKEY_free(pkey);
			pkey = 0;
			int check_res = SSL_check_private_key(ssl);
			return cert_res == 1 && key_res == 1 && check_res == 1;
		}
		int Engine::read(Data & data) // Read application data
		{
			data = waiting_to_app_data;
			waiting_to_app_data = Data();
			return data.getSize();
		}
		int Engine::write(Data & data) // Write application data
		{
			if( data.empty() ) // SSL_write undefined behaviour when writing 0
				return 0;
			int write = SSL_write(ssl, data.getData(), data.getSize());
			wLog("SSL_write: %d", write);
			if( write <= 0 )
				return 0;
			data = Data(data, write, data.getSize() - write);
			return write;
		}
		void Engine::transfer_data(DataInputStream & network_in, DataOutputStream & network_out)
		{
			read_socket(network_in);
			read_app();
			write_socket(network_out);
		}
		int Engine::get_verify_error()
		{
			if( checked_peer_certificate )
				return 0;
			X509 * cer = SSL_get_peer_certificate(ssl);
			if( !cer )
				return 0;
			X509_free(cer);
			checked_peer_certificate = true;
			int err = int(SSL_get_verify_result(ssl));
			return err;
		}

	void Engine::read_app()
		{
			if( !waiting_to_app_data.empty() )
				return;
			Data tmp(16384); // SSL_pending has a bug that returns 0 until you call SSL_read
			wLog("before SSL_read");
			int len = SSL_read(ssl, tmp.lock(), tmp.getSize());
			int err = SSL_get_error(ssl, len);
			wLog("SSL_read: %d %d", len, err);
			if( len <= 0 )
				return;
			waiting_to_app_data = Data(tmp, 0, len);
		}
		void Engine::read_socket(DataInputStream & network_in)
		{
			if( BIO_pending(bioIn) > 64*1024 ) // Enough for 4 ssl packets
				return;
			Data data;
			if( network_in.read(data) == 0 )
				return;
			wLog("sock.read: %d", data.getSize());
			int len = BIO_write(bioIn, data.getData(), data.getSize()); // Consumes everything by docs
			wLog("BIO_write: %d", len);
		}
		void Engine::write_socket(DataOutputStream & network_out)
		{
			while(true)
			{
				if( waiting_to_socket_data.empty() )
				{
					int cbPending = int(BIO_ctrl_pending(bioOut));
					if( cbPending == 0 )
						break;
					waiting_to_socket_data = Data(cbPending);
					int len = BIO_read(bioOut, waiting_to_socket_data.lock(), waiting_to_socket_data.getSize());
					wLog("BIO_read: %d", len);
					waiting_to_socket_data = Data(waiting_to_socket_data, 0, len);				
				}
				int slen = network_out.write(waiting_to_socket_data);
				wLog("sock.write: %d", slen);
				if( !waiting_to_socket_data.empty() )
					break;
			}		
		}

}