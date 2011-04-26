// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "Tests.h"
#include "AsyncSocket.h"
#include "AsyncAddrInfo.h"
#include "HTTPRequest.h"


class TestReqAddr : public AsyncAddrInfo::Delegate {
	virtual void on_resolved(AsyncAddrInfo & who, addrinfo * info) {
		printf("on_resolved %s:%s %d\n", who.get_host().c_str(), who.get_service().c_str(), int(info!=0) );
		infos.erase( std::remove(infos.begin(), infos.end(), &who), infos.end() );
//		infos.push_back( SharedPtr<AsyncAddrInfo>( new AsyncAddrInfo(*this, "gregmacboo.local", "80") ) );
	}
	std::vector< SharedPtr<AsyncAddrInfo> > infos;
public:
	TestReqAddr() {
		infos.push_back( SharedPtr<AsyncAddrInfo>( new AsyncAddrInfo(*this, "pop.mail.ru", "110") ) );
		infos.push_back( SharedPtr<AsyncAddrInfo>( new AsyncAddrInfo(*this, "apple.com", "80") ) );
		infos.push_back( SharedPtr<AsyncAddrInfo>( new AsyncAddrInfo(*this, "freebsd.org", "80") ) );
	}
};

int main (int argc, const char * argv[])
{		
	RunLoop rl;
	TestReqAddr req;

	rl.run(); // Locks because 'f' is not deleted (doesn't deregister msgs)
}

