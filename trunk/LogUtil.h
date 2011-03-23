// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#pragma once
#ifndef __LOG_UTIL_H__
#define __LOG_UTIL_H__

#ifdef ANDROID

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "craplib"
#endif // Log tag
#endif // Android

#ifdef DEBUG
#define wLog(a...) CrapLog(a)
#else
#define wLog(a...)
#endif

#ifdef __cplusplus
#include <string>
#include <stdarg.h>

namespace crap {
    class LogUtil {
    public:
        static void log(const std::string& format, ...);
        static void lstLog(const std::string& format, va_list lst);
    };
} // namespace crap
using namespace crap;

#endif

/// Define a plain C interface
#ifdef __cplusplus
extern "C"
#endif
void CrapLog(const char* format, ...);

#endif // __LOG_UTIL_H__
