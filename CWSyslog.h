#ifndef __CAPWAP_CWSyslog_HEADER__
#define __CAPWAP_CWSyslog_HEADER__

__inline__ void CWLog(const char *format, ...);
__inline__ void CWDebugLog(const char *format, ...);
void CWLogInitFile(char *fileName);
#endif
