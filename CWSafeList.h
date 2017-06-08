#ifndef __CAPWAP_CWSafeList_HEADER__
#define __CAPWAP_CWSafeList_HEADER__

#include "CWThread.h"

typedef void* CWSafeList;

typedef struct _CWPrivateSafeElement
{
	void* pData;
	int nSize;
	CWBool dataFlag;
	struct _CWPrivateSafeElement* pPrev;
	struct _CWPrivateSafeElement* pNext;
} CWPrivateSafeElement;

typedef struct _CWPrivateSafeList
{
	CWThreadMutex* pThreadMutex;
	CWThreadCondition* pThreadCond;

	unsigned long nCount;
	CWPrivateSafeElement* pFirstElement;
	CWPrivateSafeElement* pLastElement;
} CWPrivateSafeList;

CWBool CWCreateSafeList(CWSafeList* pSafeList);
void CWDestroySafeList(CWSafeList safeList);

void CWSetMutexSafeList(CWSafeList safeList, CWThreadMutex* pThreadMutex);
void CWSetConditionSafeList(CWSafeList safeList, CWThreadCondition* pThreadCond);

CWBool CWLockSafeList(CWSafeList safeList);
void CWUnlockSafeList(CWSafeList safeList);
CWBool CWWaitElementFromSafeList(CWSafeList safeList);
CWBool CWSignalElementSafeList(CWSafeList safeList);

unsigned long CWGetCountElementFromSafeList(CWSafeList safeList);
CWBool CWAddElementToSafeListHead(CWSafeList safeList, void* pData, int nSize);
void* CWGetHeadElementFromSafeList(CWSafeList safeList, int* pSize);
void* CWRemoveHeadElementFromSafeList(CWSafeList safeList, int* pSize);
void* CWRemoveHeadElementFromSafeListwithDataFlag(CWSafeList safeList, int* pSize,CWBool * dataFlag);
CWBool CWAddElementToSafeListTail(CWSafeList safeList, void* pData, int nSize);
CWBool CWAddElementToSafeListTailwitDataFlag(CWSafeList safeList, void* pData, int nSize,CWBool dataFlag);
void* CWRemoveTailElementFromSafeList(CWSafeList safeList, int* pSize);
void CWCleanSafeList(CWSafeList safeList, void (*deleteFunc)(void *));
 
#endif
 
