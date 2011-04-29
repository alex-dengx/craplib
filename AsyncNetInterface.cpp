// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncNetInterface.h"

void NetworkInterfaces::run() 
{
    getifaddrs(&addrs); // Get the list
    
    ifaddrs* p = &addrs[0];
    while(p != NULL) {
        
        ActiveCall c(t.msg);
        curInterface = NetworkInterface(p);
        c.call();
            
        p = p->ifa_next;
    }
}

void NetworkInterfaces::onCall(const ActiveMsg& msg) 
{
    if(filter == NetworkInterface::ANY || curInterface.family == filter ||
       (filter == NetworkInterface::IP 
        && (curInterface.family == NetworkInterface::IPv4 ||
            curInterface.family == NetworkInterface::IPv6)
        ) 
       )
        delegate->onInterfaceDetected(*this, curInterface);
}
