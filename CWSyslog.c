#include <syslog.h>
#include "CWCommon.h"

__inline__ void CWLog(const char *format, ...)
{
	va_list args;
	va_start(args, format);
		vsyslog(LOG_INFO, format, args);
	va_end(args);
}

__inline__ void CWDebugLog(const char *format, ...)
{
	va_list args;
	va_start(args, format);
		vsyslog(LOG_INFO, format, args);
	va_end(args);
}

void CWLogInitFile(const char *processName)
{
	openlog(processName, LOG_PID, LOG_DAEMON);
}
