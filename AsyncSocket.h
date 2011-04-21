// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __ASYNC_SOCKET_H__
#define __ASYNC_SOCKET_H__

#if defined(__MACH__)

#define _CRAP_SOCKET_KEVENT_
#include "AsyncSocketKevent.h"

#elif defined(__linux__)

#define _CRAP_SOCKET_SELECT_
#include "AsyncSocketSelect.h"

#endif

#endif // __ASYNC_SOCKET_H__