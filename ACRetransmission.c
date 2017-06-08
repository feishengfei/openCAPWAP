#include "CWAC.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

/* 
 * CW_FREE_WTP_MSG_ARRAY - free the array of the messages
 * to be sent relative to the WTP with the specified index.
 *	
 * ref -> BUG ML12
 * 20/10/2009 - Donato Capitella
 */
static void inline CW_FREE_WTP_MSG_ARRAY(int WTPIndex) {
        int i;
        for(i = 0; i < gWTPs[WTPIndex].messagesCount; i++) {
                CW_FREE_OBJECT(gWTPs[WTPIndex].messages[i].msg);
        }
        CW_FREE_OBJECT(gWTPs[WTPIndex].messages);
        gWTPs[WTPIndex].messagesCount = 0;
}

CWBool CWACSendFragments(int WTPIndex) {
	
	int i;
	
	if(gWTPs[WTPIndex].messages == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	for(i = 0; i < gWTPs[WTPIndex].messagesCount; i++) {
#ifdef CW_NO_DTLS
		if(!CWNetworkSendUnsafeUnconnected(	gWTPs[WTPIndex].socket, 
							&gWTPs[WTPIndex].address, 
							gWTPs[WTPIndex].messages[i].msg, 
							gWTPs[WTPIndex].messages[i].offset)	) {
#else
		if(!(CWSecuritySend(gWTPs[WTPIndex].session, gWTPs[WTPIndex].messages[i].msg, gWTPs[WTPIndex].messages[i].offset))) {
#endif
			return CW_FALSE;
		}
	}
	
	/*
         * BUG - ML12
         *
         * 20/10/2009 - Donato Capitella
         */
        CW_FREE_WTP_MSG_ARRAY(WTPIndex);

	CWLog("Message Sent\n");
	
	return CW_TRUE;
}


CWBool CWACResendAcknowledgedPacket(int WTPIndex) {
	if(!CWACSendFragments(WTPIndex))
	   return CW_FALSE;
	
	CWThreadSetSignals(SIG_BLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);	
	if(!(CWTimerRequest(gCWRetransmitTimer, &(gWTPs[WTPIndex].thread), &(gWTPs[WTPIndex].currentPacketTimer), CW_SOFT_TIMER_EXPIRED_SIGNAL))) {
		return CW_FALSE;
	}
	CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
	
	return CW_TRUE;
}


__inline__ CWBool CWACSendAcknowledgedPacket(int WTPIndex, int msgType, int seqNum) {
	gWTPs[WTPIndex].retransmissionCount = 0;
	gWTPs[WTPIndex].isRetransmitting = CW_TRUE;
	gWTPs[WTPIndex].responseType=msgType;
	gWTPs[WTPIndex].responseSeqNum=seqNum;
//	CWDebugLog("~~~~~~seq num in Send: %d~~~~~~", gWTPs[WTPIndex].responseSeqNum);
	return CWACResendAcknowledgedPacket(WTPIndex);
}


void CWACStopRetransmission(int WTPIndex) {
	if(gWTPs[WTPIndex].isRetransmitting) {
		int i;
		CWDebugLog("Stop Retransmission");
		gWTPs[WTPIndex].isRetransmitting = CW_FALSE;
		CWThreadSetSignals(SIG_BLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);		
		if(!CWTimerCancel(&(gWTPs[WTPIndex].currentPacketTimer)))
			{CWDebugLog("Error Cancelling a Timer... possible error!");}
		CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);	
		gWTPs[WTPIndex].responseType=UNUSED_MSG_TYPE;
		gWTPs[WTPIndex].responseSeqNum=0;
		
		for(i = 0; i < gWTPs[WTPIndex].messagesCount; i++) {
			CW_FREE_PROTOCOL_MESSAGE(gWTPs[WTPIndex].messages[i]);
		}

		CW_FREE_OBJECT(gWTPs[WTPIndex].messages);
//		CWDebugLog("~~~~~~ End of Stop Retransmission");
	}
}

