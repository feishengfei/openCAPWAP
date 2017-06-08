#ifndef __CAPWAP_CWErrorHandling_HEADER__
#define __CAPWAP_CWErrorHandling_HEADER__

typedef enum {
	CW_ERROR_SUCCESS = 1,
	CW_ERROR_OUT_OF_MEMORY,
	CW_ERROR_WRONG_ARG,
	CW_ERROR_INTERRUPTED,
	CW_ERROR_NEED_RESOURCE,
	CW_ERROR_COMUNICATING,
	CW_ERROR_CREATING,
	CW_ERROR_GENERAL,
	CW_ERROR_OPERATION_ABORTED,
	CW_ERROR_SENDING,
	CW_ERROR_RECEIVING,
	CW_ERROR_INVALID_FORMAT,
	CW_ERROR_TIME_EXPIRED,
	CW_ERROR_NONE
} CWErrorCode;


typedef struct {
	CWErrorCode code;
	char message[256];
	int line;
	char fileName[64];
} CWErrorHandlingInfo;

#define CWErrorRaiseSystemError(error)		{					\
							char buf[256];			\
							strerror_r(errno, buf, 256);	\
							CWErrorRaise(error, buf);	\
							return CW_FALSE;		\
						}
											
#define CWErrorRaise(code, msg) 		_CWErrorRaise(code, msg, __FILE__, __LINE__)
#define CWErr(arg)				((arg) || _CWErrorHandleLast(__FILE__, __LINE__))
#define CWErrorHandleLast()			_CWErrorHandleLast(__FILE__, __LINE__)

CWBool _CWErrorRaise(CWErrorCode code, const char *msg, const char *fileName, int line);
void CWErrorPrint(CWErrorHandlingInfo *infoPtr, const char *desc, const char *fileName, int line);
CWErrorCode CWErrorGetLastErrorCode(void);
CWBool _CWErrorHandleLast(const char *fileName, int line);

#endif
