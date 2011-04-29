// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#ifdef _CRAP_SOCKET_KEVENT_

#include "AsyncSocketKevent.h"

void SocketWorker::run() 
{   
    while( true ) {
        struct kevent kqEvents[1024];
        int n = kevent(kq, 0, 0, kqEvents, 1024, NULL); // &ts        
        if(n == 0) {
            continue;
        }
        else if(n < 0) { 
            wLog("Err on kevent");
            return;
        }
        
        ActiveCall c(t.msg);        

        for(int i=0; i<n; ++i) {

            if(kqEvents[i].flags & EV_ERROR) { 
                err.push_back( static_cast<SocketImpl*>(kqEvents[i].udata) );
                continue;
            }

            if (kqEvents[i].flags & EV_EOF)  {
                err.push_back( static_cast<SocketImpl*>(kqEvents[i].udata) );
                continue;
            }
                   
            switch(kqEvents[i].filter) {

                case EVFILT_READ: {
                    read.push_back( static_cast<SocketImpl*>(kqEvents[i].udata) );
                    break;
                }
                case EVFILT_WRITE: {
                    write.push_back( static_cast<SocketImpl*>(kqEvents[i].udata) );
                    break;
                }
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
    while( !err.empty() ) {
        SocketImpl* cur = err.front();
        err.pop_front();     
        if( clients.find(cur) != clients.end() )
            cur->onDisconnect();
        deregisterSocket(cur);        
    }    
    
    while( !read.empty() ) {
        SocketImpl* cur = read.front();
        read.pop_front();
        if( clients.find(cur) != clients.end() )
            cur->onCanRead();
    }
    
    while( !write.empty() ) {
        SocketImpl* cur = write.front();
        write.pop_front();
        if( clients.find(cur) != clients.end() )
            cur->onCanWrite();
    }
}

#endif // _CRAP_SOCKET_KEVENT_