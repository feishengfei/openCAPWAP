#include "CWCommon.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWThreadSpecific gLastError;

void CWErrorHandlingInitLib()
{	
	if(!CWThreadCreateSpecific(&gLastError, NULL)) 
	{
		fprintf(stderr, "Critical Error, closing the process...");
		exit(1);
	}
}

CWBool _CWErrorRaise(CWErrorCode code, const char *msg, const char *fileName, int line)
{
	CWErrorHandlingInfo *infoPtr;
	
		infoPtr = CWThreadGetSpecific(&gLastError);
		if(infoPtr==NULL){
			CW_CREATE_OBJECT_ERR(infoPtr, CWErrorHandlingInfo, exit(1););
			infoPtr->code = CW_ERROR_NONE;
			if(!CWThreadSetSpecific(&gLastError, infoPtr))
			{
				CWLog("Critical Error, closing the process..."); 
				exit(1);
			}
		}
	
	if(infoPtr == NULL) 
	{
		CWLog("Critical Error: something strange has happened, closing the process..."); 
		exit(1);
	}
	
	infoPtr->code = code;
	if(msg != NULL) strcpy(infoPtr->message, msg);
	else infoPtr->message[0]='\0';
	if(fileName != NULL) strcpy(infoPtr->fileName, fileName);
	infoPtr->line = line;
	
	return CW_FALSE;
}

void CWErrorPrint(CWErrorHandlingInfo *infoPtr, const char *desc, const char *fileName, int line)
{
	if(infoPtr == NULL) return;
	
	if(infoPtr->message != NULL && infoPtr->message[0]!='\0') {
		CWLog("1 Error: %s. %s .", desc, infoPtr->message);
	} else {
		CWLog("2 Error: %s", desc);
	}
	CWLog("(occurred at line %d in file %s, catched at line %d in file %s).",
		infoPtr->line, infoPtr->fileName, line, fileName);
}

CWErrorCode CWErrorGetLastErrorCode()
{
	CWErrorHandlingInfo *infoPtr;
	
	infoPtr = CWThreadGetSpecific(&gLastError);
	
	if(infoPtr == NULL) return CW_ERROR_GENERAL;
	
	return infoPtr->code;
}

CWBool _CWErrorHandleLast(const char *fileName, int line) {
	CWErrorHandlingInfo *infoPtr;
	
		infoPtr = CWThreadGetSpecific(&gLastError);
	
	if(infoPtr == NULL) {
		CWLog("No Error Pending");
		exit((3));
		return CW_FALSE;
	}
	
	#define __CW_ERROR_PRINT(str)	CWErrorPrint(infoPtr, (str), fileName, line)
	
	switch(infoPtr->code) {
		case CW_ERROR_SUCCESS:
		case CW_ERROR_NONE:
			return CW_TRUE;
			break;
			
		case CW_ERROR_OUT_OF_MEMORY:
			__CW_ERROR_PRINT("Out of Memory");
				CWExitThread(); // note: we can manage this on per-thread basis: ex. we can
								// kill some other thread if we are a manager thread.
			break;
			
		case CW_ERROR_WRONG_ARG:
			__CW_ERROR_PRINT("Wrong Arguments in Function");
			break;
			
		case CW_ERROR_NEED_RESOURCE:
			__CW_ERROR_PRINT("Missing Resource");
			break;
			
		case CW_ERROR_GENERAL:
			__CW_ERROR_PRINT("Error Occurred");
			break;
		
		case CW_ERROR_CREATING:
			__CW_ERROR_PRINT("Error Creating Resource");
			break;
			
		case CW_ERROR_SENDING:
			__CW_ERROR_PRINT("Error Sending");
			break;
		
		case CW_ERROR_RECEIVING:
			__CW_ERROR_PRINT("Error Receiving");
			break;
			
		case CW_ERROR_INVALID_FORMAT:
			__CW_ERROR_PRINT("Invalid Format");
			break;
				
		case CW_ERROR_INTERRUPTED:
		default:
			break;
	}
	
	return CW_FALSE;
}
