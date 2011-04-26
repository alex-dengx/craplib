// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "Thread.h"
#include "AsyncAddrInfo.h"
#include "LogUtil.h"
#include <vector>
#include <deque>

class AsyncAddrInfo::Impl
{
	class Worker;
	
	std::vector< SharedPtr<Worker> > free_workers;
	std::vector< SharedPtr<Worker> > working_workers;
	typedef std::deque<AsyncAddrInfo *> Addrs;
	Addrs waiting_addrs;
	
	void assign_work();
public:
	Impl();
	~Impl();
	void return_worker_with(AsyncAddrInfo * addr);

	void start_resolve(AsyncAddrInfo * addr);
	void cancel_resolve(AsyncAddrInfo * addr);
	
	void on_resolved(AsyncAddrInfo * addr, addrinfo * info);

	static Impl static_impl;
};

AsyncAddrInfo::Impl AsyncAddrInfo::Impl::static_impl;


class AsyncAddrInfo::Impl::Worker : public Runnable, public ActiveMsgDelegate {
public:
	explicit Worker():thm(*this, *this), addr(0), res(0), has_work_or_quit(false)
	{
		thm.start();
	}
	void start_work(AsyncAddrInfo * addr)
	{
		this->addr = addr;
		wLog("Worker::start_work %s:%s", addr->get_host().c_str(), addr->get_service().c_str());
		CondLock lock(has_work_or_quit);
		need_work.push_back(addr->hs);
		lock.set(!need_work.empty());
	}
	bool cancel_work(AsyncAddrInfo * addr)
	{
		if( addr != this->addr )
			return false;
		wLog("Worker::cancel_work %s:%s", addr->get_host().c_str(), addr->get_service().c_str());
		addr = 0;
		return true;
	}
	~Worker()
	{
		thm.requestTermination(); // No way to tell getaddrinfo to unlock and quit immediately - stupid Linux guys!
		CondLock lock(has_work_or_quit);
		lock.set(true);
		thm.waitTermination();
		if( res )
			freeaddrinfo(res);
	}
	struct addrinfo * res; // protected by thm.msg
	AsyncAddrInfo::HostService res_work; // protected by thm.msg
	
	std::deque<AsyncAddrInfo::HostService> need_work; // protected by has_work_or_quit
	AsyncAddrInfo * addr;
private:
	ThreadWithMessage thm;
	CondVar has_work_or_quit;
	
	void onCall(const ActiveMsg& msg) {
		if( !addr )
			return;
		if( addr->hs.host == res_work.host && addr->hs.service == res_work.service )
		{
			Impl::static_impl.on_resolved(addr, res);
//			addr = 0; - will get cancel_work call, which will set addr to 0
			return;
		}
	}
	
	virtual void run() 
	{
		while(true)
		{
			AsyncAddrInfo::HostService local_work;
			{
				CondLock lock(has_work_or_quit, true);
				if( thm.msg_.is_destroyed() )
					return;
				local_work = need_work.front();
				need_work.pop_front();
				lock.set(!need_work.empty());
			}
//			wLog("Worker::run got work");
			struct addrinfo * local_res = 0;
			struct addrinfo hints = {};
			
			hints.ai_family = AF_UNSPEC;// AF_INET6;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_socktype = SOCK_STREAM;
//			hints.ai_flags |= AI_V4MAPPED; //AI_V4MAPPED;//AI_ALL;//AI_CANONNAME;
//			sleep(2); // Imitate delay
			wLog("BEFORE getaddrinfo %s:%s", local_work.host.c_str(), local_work.service.c_str());
			int errcode = getaddrinfo (local_work.host.c_str(), local_work.service.c_str(), &hints, &local_res);
			if (errcode != 0)
			{
				printf ("getaddrinfo failed\n");
				//					return;
			}
			//				freeaddrinfo(res); res = 0; // memory BUG :)
			
			/*				impl = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
			 int set = 1;
			 errcode = setsockopt(impl, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
			 errcode = connect(impl, res->ai_addr, res->ai_addrlen);
			 if ( errcode < 0)
			 {
			 printf ("connect failed, will get error on write\n");
			 //			return -1;
			 }*/
			
			ActiveCall call(thm.msg_);
			if( res )
				freeaddrinfo(res);
			res = local_res;
			res_work = local_work;
			call.call();
		}
	};
};

AsyncAddrInfo::Impl::Impl()
{
}

AsyncAddrInfo::Impl::~Impl()
{}

void AsyncAddrInfo::Impl::start_resolve(AsyncAddrInfo * addr)
{
	waiting_addrs.push_back(addr);
	assign_work();
}

void AsyncAddrInfo::Impl::cancel_resolve(AsyncAddrInfo * addr)
{
	Addrs::iterator it = std::find(waiting_addrs.begin(), waiting_addrs.end(), addr);
	if( it != waiting_addrs.end() )
	{
		waiting_addrs.erase(it);
		return;
	}
	return_worker_with(addr);
}

void AsyncAddrInfo::Impl::return_worker_with(AsyncAddrInfo * addr)
{
	for(int i = 0; i != working_workers.size();)
		if( working_workers[i]->cancel_work(addr) )
		{
			free_workers.push_back( working_workers[i] );
			std::swap( working_workers[i], working_workers.back() );
			working_workers.pop_back();
		}
		else
			++i;
}


void AsyncAddrInfo::Impl::on_resolved(AsyncAddrInfo * addr, addrinfo * info)
{
	return_worker_with(addr);
	addr->delegate.on_resolved(*addr, info);
	assign_work();
}

void AsyncAddrInfo::Impl::assign_work()
{
	if( working_workers.size() + free_workers.size() == 0 )
	{
		for(int i = 0; i != 2; ++i)
			free_workers.push_back( SharedPtr<Worker>( new Worker() ) );
	}

//	wLog("waiting_addrs=%d free_workers=%d working_workers=%d", waiting_addrs.size(), free_workers.size(), working_workers.size());
	if( free_workers.empty() || waiting_addrs.empty() )
	{
		return;
	}
	working_workers.push_back( free_workers.back() );
	free_workers.pop_back();
	AsyncAddrInfo * addr = waiting_addrs.front();
	waiting_addrs.pop_front();
//	wLog("%d %s:%s %d", waiting_addrs.size(), addr->get_host().c_str(), addr->get_service().c_str());
	working_workers.back()->start_work( addr );
//	wLog("waiting_addrs=%d free_workers=%d working_workers=%d", waiting_addrs.size(), free_workers.size(), working_workers.size());
}

AsyncAddrInfo::AsyncAddrInfo(Delegate & delegate, const std::string & host, const std::string & service):
	delegate(delegate), hs(host, service)
{
	Impl::static_impl.start_resolve(this);
}

AsyncAddrInfo::~AsyncAddrInfo()
{
	Impl::static_impl.cancel_resolve(this);
}
