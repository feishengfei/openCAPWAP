#include "CWCommon.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

__inline__ void CWRandomInitLib() {
	// set seed
    srand( (unsigned)time( NULL ) );
}

__inline__ int CWRandomIntInRange(int min, int max) {
	return min + (rand() % (max-min));
}
