#ifndef __CAPWAP_CWList_HEADER__
#define __CAPWAP_CWList_HEADER__

#define CW_LIST_INIT		NULL

typedef struct _s {
	void *data;
	struct _s *next;
} CWListElement;

typedef enum {
	CW_LIST_ITERATE_RESET,
	CW_LIST_ITERATE
} CWListIterateMode;

typedef CWListElement *CWList;

CWBool CWAddElementToList(CWList *list, void *element);
CWBool CWAddElementToListTail(CWList *list, void *element);
CWList CWListGetFirstElem(CWList *list);
void *CWListGetNext(CWList list, CWListIterateMode mode);
void *CWSearchInList(CWList list, void *baseElement, CWBool (*compareFunc)(void *, void *));
void *CWDeleteInList(CWList *list, void *baseElement, CWBool (*compareFunc)(void *, void *));
void CWDeleteList(CWList *list, void (*deleteFunc)(void *));
int CWCountElementInList(CWList list);
//CWList * FindLastElementInList (CWList list);

#endif
