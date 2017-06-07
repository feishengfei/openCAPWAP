#include "CWWTP.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWBool CWAssembleChangeStateEventRequest(CWProtocolMessage **messagesPtr,
					 int *fragmentsNumPtr,
					 int PMTU,
					 int seqNum,
					 CWList msgElemList);

CWBool CWParseChangeStateEventResponseMessage(char *msg,
					      int len,
					      int seqNum,
					      void *values);

CWBool CWSaveChangeStateEventResponseMessage(void *changeStateEventResp);

CWStateTransition CWWTPEnterDataCheck() {

	int seqNum;
	
	CWLog("\n");
	CWLog("######### Data Check State #########");
	
	CWLog("\n");
	CWLog("#________ Change State Event (Data Check) ________#");
	
	/* Send Change State Event Request */
	seqNum = CWGetSeqNum();
	
	if(!CWErr(CWWTPSendAcknowledgedPacket(seqNum, 
					      NULL,
					      CWAssembleChangeStateEventRequest,
					      CWParseChangeStateEventResponseMessage,
					      CWSaveChangeStateEventResponseMessage,
					      NULL)))
		return CW_ENTER_RESET;
	
/*
 * Initilize DTLS Data Session WTP + first KeepAlive
 */

#ifdef CW_DTLS_DATA_CHANNEL
	CWProtocolMessage 	msgPtr;
	
	struct sockaddr_in *tmpAdd = (struct sockaddr_in *) &(gACInfoPtr->preferredAddress);
	tmpAdd->sin_port = htons(5247);
	CWLog("[DTLS] WTP Run Handshake with %s:%d", inet_ntoa(tmpAdd->sin_addr), ntohs(tmpAdd->sin_port));

	CWNetworkLev4Address * gACAddressDataChannel = (CWNetworkLev4Address *)tmpAdd;
	
	if(!CWErr(CWSecurityInitSessionClient(gWTPDataSocket,
					      gACAddressDataChannel,
					      gPacketReceiveDataList,
					      gWTPSecurityContext,
					      &gWTPSessionData,
					      &gWTPPathMTU))) {
		
		/* error setting up DTLS session */
		CWSecurityDestroyContext(gWTPSecurityContext);
		gWTPSecurityContext = NULL;
		return CW_FALSE;
	}
	
	CWLog("[DTLS] OK now assemble first KeepAlive");
	
	/*
	 * If handshake ok, first KeepAlive DTLS to AC 
	 */
	CWProtocolMessage *messages = NULL;
	CWProtocolMessage sessionIDmsgElem;
	int fragmentsNum = 1;

	CWAssembleMsgElemSessionID(&sessionIDmsgElem, &gWTPSessionID[0]);
	sessionIDmsgElem.data_msgType = CW_DATA_MSG_KEEP_ALIVE_TYPE;
	
	//Send WTP Event Request
	if (!CWAssembleDataMessage(&messages, 
			    &fragmentsNum, 
			    gWTPPathMTU, 
			    &sessionIDmsgElem, 
				CW_PACKET_CRYPT,
			    1
			    ))
	{
		int i;

		CWDebugLog("Failure Assembling KeepAlive Request");
		if(messages)
			for(i = 0; i < fragmentsNum; i++) {
				CW_FREE_PROTOCOL_MESSAGE(messages[i]);
			}	
		CW_FREE_OBJECT(messages);
		return CW_FALSE;
	}
	
	int i;
	for(i = 0; i < fragmentsNum; i++) {
		if(!(CWSecuritySend(gWTPSessionData, messages[i].msg, messages[i].offset))) {
			CWLog("Failure sending  KeepAlive Request");
			int k;
			for(k = 0; k < fragmentsNum; k++) {
				CW_FREE_PROTOCOL_MESSAGE(messages[k]);
			}	
			CW_FREE_OBJECT(messages);
			break;
		}
	}

	int k;
	for(k = 0; messages && k < fragmentsNum; k++) {
		CW_FREE_PROTOCOL_MESSAGE(messages[k]);
	}	
	CW_FREE_OBJECT(messages);
	
	CWLog("Send KeepAlive");
	
	if(!CWReceiveDataMessage(&msgPtr))
		{
			CW_FREE_PROTOCOL_MESSAGE(msgPtr);
			CWDebugLog("Failure Receiving DTLS Data Channel");
			return CW_ENTER_RESET;		
		}
				
	if (msgPtr.data_msgType == CW_DATA_MSG_KEEP_ALIVE_TYPE) {
		return CW_ENTER_RUN;
	}
			
#endif
	
	return CW_ENTER_RUN;
}

CWBool CWAssembleChangeStateEventRequest(CWProtocolMessage **messagesPtr,
					 int *fragmentsNumPtr,
					 int PMTU,
					 int seqNum,
					 CWList msgElemList) {

	CWProtocolMessage 	*msgElems= NULL;
	CWProtocolMessage 	*msgElemsBinding= NULL;
	const int		msgElemCount = 2;
	int 			msgElemBindingCount=0;
	int 			resultCode = CW_PROTOCOL_SUCCESS;
	int 			k = -1;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, 
					 msgElemCount,
					 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
		
	CWLog("Assembling Change State Event Request...");	

	/* Assemble Message Elements */
	if (!(CWAssembleMsgElemRadioOperationalState(-1, &(msgElems[++k]))) ||
	    !(CWAssembleMsgElemResultCode(&(msgElems[++k]), resultCode))) {

		int i;
	
		for(i = 0; i <= k; i++) { 
			
			CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);
		}
		CW_FREE_OBJECT(msgElems);
		/* error will be handled by the caller */
		return CW_FALSE;
	}

	if (!(CWAssembleMessage(messagesPtr,
				fragmentsNumPtr,
				PMTU,
				seqNum,
				CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_REQUEST,
				msgElems, msgElemCount,
				msgElemsBinding,
				msgElemBindingCount,
#ifdef CW_NO_DTLS
				CW_PACKET_PLAIN
#else
				CW_PACKET_CRYPT
#endif
				)))
	 	return CW_FALSE;
	
	CWLog("Change State Event Request Assembled");
	return CW_TRUE;
}

CWBool CWParseChangeStateEventResponseMessage(char *msg,
					      int len,
					      int seqNum,
					      void *values) {

	CWControlHeaderValues controlVal;
	CWProtocolMessage completeMsg;
	
	if(msg == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Parsing Change State Event Response...");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;
	
	/* error will be handled by the caller */
	if(!(CWParseControlHeader(&completeMsg, &controlVal))) return CW_FALSE; 
	
	if(controlVal.messageTypeValue != CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_RESPONSE)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Message is not Change State Event Response as Expected");
	
	if(controlVal.seqNum != seqNum) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Different Sequence Number");
	
	/* skip timestamp */
	controlVal.msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;
	
	if(controlVal.msgElemsLen != 0 ) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, 
				    "Change State Event Response must carry no message elements");

	CWLog("Change State Event Response Parsed");
	return CW_TRUE;
}

CWBool CWSaveChangeStateEventResponseMessage (void *changeStateEventResp)
{
	CWDebugLog("Saving Change State Event Response...");
	CWDebugLog("Change State Event Response Saved");
	return CW_TRUE;
}
