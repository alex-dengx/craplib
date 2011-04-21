// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "LogUtil.h"
#include <stdio.h>
using namespace std;

void LogUtil::log(const string& format, ...) 
{
	va_list			lst;
	va_start(lst, format);
	LogUtil::lstLog(format, lst);
	va_end(lst);
}

void LogUtil::lstLog(const string& format, va_list lst) 
{
#ifdef ANDROID
	char *buf = NULL;
	vasprintf(&buf, format.c_str(), lst);
	__android_log_print(ANDROID_LOG_INFO,LOG_TAG,"%s", buf);
	free(buf);
#else	
	vprintf(string(format).append("\n").c_str(), lst);
#endif
}

// Plain C version
extern "C" void CrapLog(const char* format, ...) 
{
	va_list			lst;
	va_start(lst, format);
	
	LogUtil::lstLog(format, lst);

	va_end(lst);
}
