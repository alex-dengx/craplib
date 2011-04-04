// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncNetInterface.h"

void NetworkInterfaces::run() 
{
    getifaddrs(&addrs_); // Get the list
    
    ifaddrs* p = &addrs_[0];
    while(p != NULL) {
        
        ActiveCall c(t_.msg_);
        curInterface_ = NetworkInterface(p);
        c.call();
            
        p = p->ifa_next;
    }
}

void NetworkInterfaces::onCall(const ActiveMsg& msg) 
{
    delegate_->onInterfaceDetected(*this, curInterface_);
}
