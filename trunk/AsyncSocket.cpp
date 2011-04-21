// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSocket.h"

#if defined(_CRAP_SOCKET_KEVENT_)
#include "AsyncSocketKevent.cpp"

#elif defined(_CRAP_SOCKET_SELECT_)
#include "AsyncSocketSelect.cpp"
#endif
