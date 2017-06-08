#include "CWWTP.h"
 
#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#ifdef CW_DEBUGGING
int gCWSilentInterval = 5;
#else
int gCWSilentInterval = 30;
#endif

/* 
 * WTP enters sulking when no AC is responding to Discovery Request.
 */
CWStateTransition CWWTPEnterSulking() {
	struct timeval timeout, before, after, delta, newTimeout;
	
	CWLog("\n");
	CWLog("######### Sulking State #########");
	/* 
	 * wait for Silent Interval and discard 
	 * all the packets that are coming 
	 */
	timeout.tv_sec = newTimeout.tv_sec = gCWSilentInterval;
	timeout.tv_usec = newTimeout.tv_usec = 0;
	
	gettimeofday(&before, NULL);

	CW_REPEAT_FOREVER {

		/* check if something is available to read until newTimeout */
		if(CWNetworkTimedPollRead(gWTPSocket, &newTimeout)) { 
			/*
			 * success
			 * if there was no error, raise a "success error", so we can easily handle
			 * all the cases in the switch
			 */
			CWErrorRaise(CW_ERROR_SUCCESS, NULL);
		}

		switch(CWErrorGetLastErrorCode()) {
			case CW_ERROR_TIME_EXPIRED:
				goto cw_sulk_time_over;
				break;
			case CW_ERROR_SUCCESS:
				/* there's something to read */
				{
					CWNetworkLev4Address addr;
					char buf[CW_BUFFER_SIZE];
					int readBytes;
		
					/* read and discard */
					if(!CWErr(CWNetworkReceiveUnsafe(gWTPSocket, buf, CW_BUFFER_SIZE, 0, &addr, &readBytes))) {
						return CW_QUIT;
					}
				}
			case CW_ERROR_INTERRUPTED: 
				/*
				 *  something to read OR interrupted by the 
				 *  system
				 *  wait for the remaining time (NetworkPoll
				 *  will be recalled with the remaining time)
				 */
				gettimeofday(&after, NULL);
		
				CWTimevalSubtract(&delta, &after, &before);
				if(CWTimevalSubtract(&newTimeout, &timeout, &delta) == 1) {
					/* negative delta: time is over */
					goto cw_sulk_time_over;
				}
				break;
				
			default:
				CWErrorHandleLast();
				goto cw_error;
				break;
		}
	}
	cw_sulk_time_over:
		CWLog("End of Sulking Period");
	cw_error:
		return CW_ENTER_DISCOVERY;
}
