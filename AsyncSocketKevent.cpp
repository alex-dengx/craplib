// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_KEVENT_

#include "AsyncSocketKevent.h"

void SocketWorker::run() 
{
//    timespec ts;
//    ts.tv_sec = 0;
//    ts.tv_nsec = 1000;
    
    while( true ) {
 
        {
            // Wait till we have some clients
            CondLock lock(c_, true);
        }

        struct kevent kqEvents_[1024];
        int n = kevent(kq_, 0, 0, kqEvents_, 1024, NULL); // &ts        
        if(n == 0) {
            continue;
        }
        else if(n < 0) { 
            wLog("Err on kevent");
            return;
        }
        
        ActiveCall c(t_.msg_);        
//        message_ = ONCHANGES;

        for(int i=0; i<n; ++i) {

            if(kqEvents_[i].flags & EV_ERROR) { 
                err_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                continue;
            }
                   
            switch(kqEvents_[i].filter) {

                case EVFILT_READ: {
                    read_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                    break;
                }
                case EVFILT_WRITE: {
                    write_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                    break;
                }
            }      
            
            if (kqEvents_[i].flags & EV_EOF)  {
                err_.push_back( static_cast<SocketImpl*>(kqEvents_[i].udata) );
                continue;
            }
        }   
        
        c.call();
    }
}

void SocketWorker::onCall(const ActiveMsg& msg)
{
    /*
     * Process all read, write and error changes
     */
    while( !read_.empty() ) {
        SocketImpl* cur = read_.front();
        read_.pop_front();
		cur->onCanRead();
    }
    
    while( !write_.empty() ) {
        SocketImpl* cur = write_.front();
        write_.pop_front();
		cur->onCanWrite();
    }
    
    while( !err_.empty() ) {
        SocketImpl* cur = err_.front();
        err_.pop_front();        
        deregisterSocket(cur);
		cur->onDisconnect();
    }    
}

#endif // _CRAP_SOCKET_KEVENT_