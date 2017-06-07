#include "CWAC.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWBool CWACParseGenericRunMessage(int WTPIndex,
				  CWProtocolMessage *msg,
				  CWControlHeaderValues* controlVal);

CWBool CWParseConfigurationUpdateResponseMessage(CWProtocolMessage* msgPtr,
						 int len,
						 CWProtocolResultCode* resultCode,
						 CWProtocolVendorSpecificValues** protocolValues);

CWBool CWSaveConfigurationUpdateResponseMessage(CWProtocolResultCode resultCode,
						int WTPIndex,
						CWProtocolVendorSpecificValues* protocolValues);

CWBool CWParseClearConfigurationResponseMessage(CWProtocolMessage* msgPtr,
						int len,
						CWProtocolResultCode* resultCode);
						
CWBool CWParseStationConfigurationResponseMessage(CWProtocolMessage* msgPtr,
						  int len,
						  CWProtocolResultCode* resultCode);

CWBool CWParseWTPDataTransferRequestMessage(CWProtocolMessage *msgPtr,
					    int len,
					    CWProtocolWTPDataTransferRequestValues *valuesPtr);

CWBool CWAssembleWTPDataTransferResponse(CWProtocolMessage **messagesPtr,
					 int *fragmentsNumPtr,
					 int PMTU, int seqNum);

CWBool CWParseWTPEventRequestMessage(CWProtocolMessage *msgPtr,
				     int len,
				     CWProtocolWTPEventRequestValues *valuesPtr);

CWBool CWSaveWTPEventRequestMessage(CWProtocolWTPEventRequestValues *WTPEventRequest,
				    CWWTPProtocolManager *WTPProtocolManager);

CWBool CWAssembleWTPEventResponse(CWProtocolMessage **messagesPtr,
				  int *fragmentsNumPtr,
				  int PMTU,
				  int seqNum);

CWBool CWParseChangeStateEventRequestMessage2(CWProtocolMessage *msgPtr,
					      int len,
					      CWProtocolChangeStateEventRequestValues **valuesPtr);

CWBool CWParseEchoRequestMessage(CWProtocolMessage *msgPtr,
				 int len);

CWBool CWAssembleEchoResponse(CWProtocolMessage **messagesPtr,
			      int *fragmentsNumPtr,
			      int PMTU,
			      int seqNum);

CWBool CWStartNeighborDeadTimer(int WTPIndex);
CWBool CWStopNeighborDeadTimer(int WTPIndex);
CWBool CWRestartNeighborDeadTimer(int WTPIndex);
CWBool CWRestartNeighborDeadTimerForEcho(int WTPIndex);

/* argument passed to the thread func */
typedef struct {
	int index;
	CWSocket sock;
	int interfaceIndex;
} CWACThreadArg;

CWBool ACEnterRun(int WTPIndex, CWProtocolMessage *msgPtr, CWBool dataFlag) {
	
	CWBool toSend= CW_FALSE, timerSet = CW_TRUE;
	CWControlHeaderValues controlVal;
	CWProtocolMessage* messages =NULL;
	int messagesCount=0;
	unsigned char StationMacAddr[MAC_ADDR_LEN];
	char string[10];
	char socketctl_path_name[50];
	char socketserv_path_name[50];
	int msglen = msgPtr->offset;
	
	msgPtr->offset = 0;
	if(dataFlag){
		/* We have received a Data Message... now just log this event and do actions by the dataType */
		
		CWDebugLog("--> Received a DATA Message: Type: %d", msgPtr->data_msgType);

		if(msgPtr->data_msgType == CW_DATA_MSG_FRAME_TYPE)	{

			/*Retrive mac address station from msg*/
			memset(StationMacAddr, 0, MAC_ADDR_LEN);
			memcpy(StationMacAddr, msgPtr->msg+SOURCE_ADDR_START, MAC_ADDR_LEN);
		
			CWLog("CW_DATA_MSG_FRAME_TYPE. I don't do anything?");
		}
		else if(msgPtr->data_msgType == CW_DATA_MSG_KEEP_ALIVE_TYPE){
			
			char * valPtr=NULL;
			CWProtocolMessage *messages = NULL;
			CWProtocolMessage sessionIDmsgElem;
			int fragmentsNum = 0;
			int i;
			unsigned short int elemType = 0;
			unsigned short int elemLen = 0;
#ifndef CW_DTLS_DATA_CHANNEL
			CWNetworkLev4Address address;
			int dataSocket=0;
#endif

			CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
			valPtr = CWParseSessionID(msgPtr, elemLen);
			CWAssembleMsgElemSessionID(&sessionIDmsgElem, valPtr);
			/*
			 * BUG Valgrind: sessionIDmsgElem.data_msgType not initialized
			 */
			sessionIDmsgElem.data_msgType = CW_DATA_MSG_KEEP_ALIVE_TYPE;
			if (!CWAssembleDataMessage(&messages, 
				  &fragmentsNum, 
				  gWTPs[WTPIndex].pathMTU, 
				  &sessionIDmsgElem, 
				  CW_PACKET_PLAIN,
				  1
				  ))
			{
				CWLog("Failure Assembling KeepAlive Request");
				if(messages)
					for(i = 0; i < fragmentsNum; i++) {
						CW_FREE_PROTOCOL_MESSAGE(messages[i]);
					}	
				CW_FREE_OBJECT(messages);
				return CW_FALSE;
			}

#ifndef CW_DTLS_DATA_CHANNEL
			for(i = 0; i < gACSocket.count; i++) {
			    if (gACSocket.interfaces[i].sock == gWTPs[WTPIndex].socket){
				dataSocket = gACSocket.interfaces[i].dataSock;
				CW_COPY_NET_ADDR_PTR(&address,&(gWTPs[WTPIndex].dataaddress));
				break;
			    }
			}

			if (dataSocket == 0){
			      CWLog("data socket of WTP %d isn't ready.");
			      return CW_FALSE;
			}
#endif

			for(i = 0; i < fragmentsNum; i++) {

			/*
			 * DTLS Data Session AC
			 */
			 
#ifdef CW_DTLS_DATA_CHANNEL
				if(!(CWSecuritySend(gWTPs[WTPIndex].sessionData, messages[i].msg, messages[i].offset)))
#else
				if(!CWNetworkSendUnsafeUnconnected(	dataSocket, &(address), messages[i].msg, messages[i].offset))
#endif
				{
					CWLog("Failure sending  KeepAlive Request");
					int k;
					for(k = 0; k < fragmentsNum; k++) {
						CW_FREE_PROTOCOL_MESSAGE(messages[k]);
					}	
					CW_FREE_OBJECT(messages);
					break;
				}

			}

			CWLog("Sent KeepAlive");

			int k;
			for(k = 0; messages && k < fragmentsNum; k++) {
				CW_FREE_PROTOCOL_MESSAGE(messages[k]);
			}	
			CW_FREE_OBJECT(messages);

			
		}else if(msgPtr->data_msgType == CW_IEEE_802_3_FRAME_TYPE){
			
			CWDebugLog("Write 802.3 data to TAP[%d], len:%d",gWTPs[WTPIndex].tap_fd,msglen);
			//write(gWTPs[WTPIndex].tap_fd, msgPtr->msg, msglen);
			write(ACTap_FD, msgPtr->msg, msglen);
		}
		/* Elena Agostini: 80211 Frame Management or Data */
		else if(msgPtr->data_msgType == CW_IEEE_802_11_FRAME_TYPE)	{
			CWDebugLog("802.11 data received");
		}
		else{
			CWDebugLog("Manage special data packets with frequency");

			/************************************************************
			 * Update 2009:												*
			 *				Manage special data packets with frequency	*
			 *				statistics informations.					*
			 ************************************************************/
			
			

		  if(msgPtr->data_msgType == CW_DATA_MSG_STATS_TYPE)
			{			  
			  if(!UnixSocksArray[WTPIndex].data_stats_sock)
				{	//Init Socket only the first time when the function is called
				  if ((UnixSocksArray[WTPIndex].data_stats_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) 
					{
					  CWDebugLog("Error creating socket for data send");
					  return CW_FALSE;
    				}
				  
					  memset(&(UnixSocksArray[WTPIndex].clntaddr), 0, sizeof(UnixSocksArray[WTPIndex].clntaddr));
					  UnixSocksArray[WTPIndex].clntaddr.sun_family = AF_UNIX;
					  
					  //make unix socket client path name by index i 
					snprintf(string,sizeof(string),"%d",WTPIndex);
					string[sizeof(string)-1]=0;
					strcpy(socketctl_path_name,SOCKET_PATH_AC);
					strcat(socketctl_path_name,string);
					strcpy(UnixSocksArray[WTPIndex].clntaddr.sun_path,socketctl_path_name);
					
					unlink(socketctl_path_name);
					
					memset(&(UnixSocksArray[WTPIndex].servaddr), 0, sizeof(UnixSocksArray[WTPIndex].servaddr));
					UnixSocksArray[WTPIndex].servaddr.sun_family = AF_UNIX;

					//make unix socket server path name by index i 
					strcpy(socketserv_path_name, SOCKET_PATH_RECV_AGENT);
					strcat(socketserv_path_name, string);
					strcpy(UnixSocksArray[WTPIndex].servaddr.sun_path, socketserv_path_name);
					printf("\n%s\t%s",socketserv_path_name,socketctl_path_name);fflush(stdout);
				}
			  

			  int nbytes;
			  int pDataLen=656; //len of Monitoring Data
			  
			  //Send data stats from AC thread to monitor client over unix socket
			  nbytes = sendto(UnixSocksArray[WTPIndex].data_stats_sock, msgPtr->msg,pDataLen, 0,
							  (struct sockaddr *) &(UnixSocksArray[WTPIndex].servaddr),sizeof(UnixSocksArray[WTPIndex].servaddr));
			  if (nbytes < 0) 
				{
				  CWDebugLog("Error sending data over socket");
				  return CW_FALSE;
				}
			
			}
		}

		return CW_TRUE;
	}

	if(!(CWACParseGenericRunMessage(WTPIndex, msgPtr, &controlVal))) {
		/* Two possible errors: WRONG_ARG and INVALID_FORMAT
		 * In the second case we have an unexpected response: ignore the
		 * message and log the event.
		 */
		return CW_FALSE;
	}

	switch(controlVal.messageTypeValue) {
		case CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			/*Update 2009:
				Store Protocol specific response data*/
			CWProtocolVendorSpecificValues* protocolValues = NULL;
			
			if(!(CWParseConfigurationUpdateResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode, &protocolValues)))
				return CW_FALSE;
			
			CWACStopRetransmission(WTPIndex);
			
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}

			
			CWSaveConfigurationUpdateResponseMessage(resultCode, WTPIndex, protocolValues);
			
			if (gWTPs[WTPIndex].interfaceCommandProgress == CW_TRUE) {

				CWThreadMutexLock(&gWTPs[WTPIndex].interfaceMutex);
				
				gWTPs[WTPIndex].interfaceResult = 1;
				gWTPs[WTPIndex].interfaceCommandProgress = CW_FALSE;
				CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceComplete);

				CWThreadMutexUnlock(&gWTPs[WTPIndex].interfaceMutex);
			}

			break;
		}
		case CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_REQUEST:
		{
			CWProtocolChangeStateEventRequestValues *valuesPtr;
		
			if(!(CWParseChangeStateEventRequestMessage2(msgPtr, controlVal.msgElemsLen, &valuesPtr)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			if(!(CWSaveChangeStateEventRequestMessage(valuesPtr, &(gWTPs[WTPIndex].WTPProtocolManager))))
				return CW_FALSE;
			if(!(CWAssembleChangeStateEventResponse(&messages,
								&messagesCount,
								gWTPs[WTPIndex].pathMTU,
								controlVal.seqNum)))
				return CW_FALSE;

			toSend = CW_TRUE;
			break;
		}
		case CW_MSG_TYPE_VALUE_ECHO_REQUEST:
		{
			if(!(CWParseEchoRequestMessage(msgPtr, controlVal.msgElemsLen)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			
			if(!(CWAssembleEchoResponse(&messages,
						    &messagesCount,
						    gWTPs[WTPIndex].pathMTU,
						    controlVal.seqNum)))
				return CW_FALSE;

			toSend = CW_TRUE;	
			break;
		}
		case CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			if(!(CWParseStationConfigurationResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode)))
				return CW_FALSE;
			CWACStopRetransmission(WTPIndex);
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}

			break;
		}	
		case CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_RESPONSE:
		{
			CWProtocolResultCode resultCode;
			if(!(CWParseClearConfigurationResponseMessage(msgPtr, controlVal.msgElemsLen, &resultCode)))
				return CW_FALSE;
			CWACStopRetransmission(WTPIndex);
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			
			
			if (gWTPs[WTPIndex].interfaceCommandProgress == CW_TRUE)
			{
				CWThreadMutexLock(&gWTPs[WTPIndex].interfaceMutex);
				
				gWTPs[WTPIndex].interfaceResult = 1;
				gWTPs[WTPIndex].interfaceCommandProgress = CW_FALSE;
				CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceComplete);

				CWThreadMutexUnlock(&gWTPs[WTPIndex].interfaceMutex);
			}

			break;
		}
		case CW_MSG_TYPE_VALUE_DATA_TRANSFER_REQUEST:
		{
			CWProtocolWTPDataTransferRequestValues valuesPtr;
			
			if(!(CWParseWTPDataTransferRequestMessage(msgPtr, controlVal.msgElemsLen, &valuesPtr)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			if(!(CWAssembleWTPDataTransferResponse(&messages, &messagesCount, gWTPs[WTPIndex].pathMTU, controlVal.seqNum))) 
				return CW_FALSE;
			toSend = CW_TRUE;
			break;
		}
		case CW_MSG_TYPE_VALUE_WTP_EVENT_REQUEST:
		{
			CWProtocolWTPEventRequestValues valuesPtr;

			if(!(CWParseWTPEventRequestMessage(msgPtr, controlVal.msgElemsLen, &valuesPtr)))
				return CW_FALSE;
			if (timerSet) {
				if(!CWRestartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			} else {
				if(!CWStartNeighborDeadTimer(WTPIndex)) {
					CWCloseThread();
				}
			}
			if(!(CWSaveWTPEventRequestMessage(&valuesPtr, &(gWTPs[WTPIndex].WTPProtocolManager))))
				return CW_FALSE;
			
			if(!(CWAssembleWTPEventResponse(&messages,
							&messagesCount,
							gWTPs[WTPIndex].pathMTU,
							controlVal.seqNum)))
 				return CW_FALSE;

			toSend = CW_TRUE;	
			break;
		}
		default: 
			/*
			 * We have an unexpected request and we have to send
			 * a corresponding response containing a failure result code
			 */
			CWDebugLog("--> Not valid Request in Run State... we send a failure Response");
			
			if(!(CWAssembleUnrecognizedMessageResponse(&messages,
								   &messagesCount,
								   gWTPs[WTPIndex].pathMTU,
								   controlVal.seqNum,
								   controlVal.messageTypeValue + 1))) 
 				return CW_FALSE;

			toSend = CW_TRUE;
			/*return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message not valid in Run State");*/
	}	
	if(toSend){
		int i;
	
		if(messages == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
		
		for(i = 0; i < messagesCount; i++) {
#ifdef CW_NO_DTLS
			if(!CWNetworkSendUnsafeUnconnected(gWTPs[WTPIndex].socket, 
							   &gWTPs[WTPIndex].address, 
							   messages[i].msg, 
							   messages[i].offset)	) 
#else
			if(!(CWSecuritySend(gWTPs[WTPIndex].session,
					    messages[i].msg,
					    messages[i].offset)))
#endif
			{
				CWFreeMessageFragments(messages, messagesCount);
				CW_FREE_OBJECT(messages);
				return CW_FALSE;
			}
		}
		CWFreeMessageFragments(messages, messagesCount);
		CW_FREE_OBJECT(messages);
	}
	gWTPs[WTPIndex].currentState = CW_ENTER_RUN;
	gWTPs[WTPIndex].subState = CW_WAITING_REQUEST;

	return CW_TRUE;
}

CWBool CWACParseGenericRunMessage(int WTPIndex,
				  CWProtocolMessage *msg,
				  CWControlHeaderValues* controlVal) {

	if(msg == NULL || controlVal == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!(CWParseControlHeader(msg, controlVal)))
		/* will be handled by the caller */
		return CW_FALSE;

	/* skip timestamp */
	controlVal->msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;

	/* Check if it is a request */
	if(controlVal->messageTypeValue % 2 == 1){

		return CW_TRUE;	
	}
		
	if((gWTPs[WTPIndex].responseSeqNum != controlVal->seqNum)) {
		/* Elena Agostini: gWTPs[WTPIndex].responseType is never set if it is a response
		 * || (gWTPs[WTPIndex].responseType != controlVal->messageTypeValue)*/
/*
		CWLog("gWTPs seqNum: %d\n", gWTPs[WTPIndex].responseSeqNum);
		CWLog("controlVal seqNum: %d\n", controlVal->seqNum);
		
		CWLog("gWTPs responseType: %d\n", gWTPs[WTPIndex].responseType);
		CWLog("controlVal responseType: %d\n", controlVal->messageTypeValue);
*/
		CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Seq Num or Msg Type not valid!");
		return CW_FALSE;
	}

	return CW_TRUE;	
}

/*Update 2009:
	Added vendValues to include a response payload (to pass response data)*/
CWBool CWParseConfigurationUpdateResponseMessage(CWProtocolMessage* msgPtr,
						 int len,
						 CWProtocolResultCode* resultCode,
						 CWProtocolVendorSpecificValues** vendValues) {

	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing Configuration Update Response...");

	/* parse message elements */
	while((msgPtr->offset - offsetTillMessages) < len) {

		unsigned short int elemType = 0;
		unsigned short int elemLen = 0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode=CWProtocolRetrieve32(msgPtr);
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("Configuration Update Response Parsed");

	return CW_TRUE;	
}


CWBool CWParseClearConfigurationResponseMessage(CWProtocolMessage* msgPtr, int len, CWProtocolResultCode* resultCode)
{
	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing Clear Configuration Response...");

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode=CWProtocolRetrieve32(msgPtr);
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Configuration Update Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("Clear Configuration Response Parsed");

	return CW_TRUE;	
}	
		
CWBool CWParseStationConfigurationResponseMessage(CWProtocolMessage* msgPtr, int len, CWProtocolResultCode* resultCode)
{
	int offsetTillMessages;

	if(msgPtr == NULL || resultCode==NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("Parsing Station Configuration Response...");

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				*resultCode=CWProtocolRetrieve32(msgPtr);
				CWLog("Result Code: %d", (*resultCode));
				break;	
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Station Configuration Response");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");

	CWLog("Station Configuration Response Parsed");

	return CW_TRUE;	
}

CWBool CWSaveConfigurationUpdateResponseMessage(CWProtocolResultCode resultCode,
						int WTPIndex,
						CWProtocolVendorSpecificValues* vendValues) {
	int closeWTPManager = CW_FALSE;

	if (vendValues != NULL) {
		char * responseBuffer; 
		int socketIndex, payloadSize, headerSize, netWTPIndex, netresultCode, netpayloadSize;
		payloadSize = 0;


		/********************************
		 *Payload Management		*
		 ********************************/

		headerSize = 3*sizeof(int);
		
		if ( ( responseBuffer = malloc( headerSize+payloadSize ) ) != NULL ) {

			netWTPIndex = htonl(WTPIndex);
			memcpy(responseBuffer, &netWTPIndex, sizeof(int));

			netresultCode = htonl(resultCode);
			memcpy(responseBuffer+sizeof(int), &netresultCode, sizeof(int));

			netpayloadSize = htonl(payloadSize);
			memcpy(responseBuffer+(2*sizeof(int)), &netpayloadSize, sizeof(int));
			

			socketIndex = gWTPs[WTPIndex].applicationIndex;	

			/****************************************************
		         * Forward payload to correct application  	    *
			 ****************************************************/

			if(!CWErr(CWThreadMutexLock(&appsManager.socketMutex[socketIndex]))) {
				CWLog("Error locking numSocketFree Mutex");
				return CW_FALSE;
			}
			
			if ( Writen(appsManager.appSocket[socketIndex], responseBuffer, headerSize+payloadSize)  < 0 ) {
				CWThreadMutexUnlock(&appsManager.socketMutex[socketIndex]);
				CWLog("Error locking numSocketFree Mutex");
				return CW_FALSE;
			}

			CWThreadMutexUnlock(&appsManager.socketMutex[socketIndex]);

		}
		CW_FREE_OBJECT(responseBuffer);
		CW_FREE_OBJECT(vendValues->payload);
		CW_FREE_OBJECT(vendValues);

	}	
	/*
	 * On a positive WTP_COMMIT_ACK, we need to close the WTP Manager.
	 */
	if (closeWTPManager) {
		gWTPs[WTPIndex].isRequestClose = CW_TRUE;
		CWSignalThreadCondition(&gWTPs[WTPIndex].interfaceWait);
	}

	CWDebugLog("Configuration Update Response Saved");
	return CW_TRUE;
}

CWBool CWParseWTPDataTransferRequestMessage(CWProtocolMessage *msgPtr, int len, CWProtocolWTPDataTransferRequestValues *valuesPtr)
{
	int offsetTillMessages;
	

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	

	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ WTP Data Transfer (Run) ________#");
	CWLog("Parsing WTP Data Transfer Request...");
	
	

	// parse message elements
	while((msgPtr->offset - offsetTillMessages) < len) {
		unsigned short int elemType=0;
		unsigned short int elemLen=0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_DATA_TRANSFER_DATA_CW_TYPE:{	
				if (!(CWParseMsgElemDataTransferData(msgPtr, elemLen, valuesPtr)))
					return CW_FALSE;
				CWDebugLog("----- %s --------\n",valuesPtr->debug_info);
				break;	
			}
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in WTP Data Transfer Request");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");


	return CW_TRUE;	
}

CWBool CWParseWTPEventRequestMessage(CWProtocolMessage *msgPtr,
				     int len,
				     CWProtocolWTPEventRequestValues *valuesPtr) {

	int offsetTillMessages;
	int i=0, k=0;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	/*
	CW_CREATE_OBJECT_ERR(valuesPtr, CWProtocolWTPEventRequestValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	*/
	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ WTP Event (Run) ________#");
	CWLog("Parsing WTP Event Request...");
	
	valuesPtr->errorReportCount = 0;
	valuesPtr->errorReport = NULL;
	valuesPtr->duplicateIPv4 = NULL;
	valuesPtr->duplicateIPv6 = NULL;
	valuesPtr->WTPOperationalStatisticsCount = 0;
	valuesPtr->WTPOperationalStatistics = NULL;
	valuesPtr->WTPRadioStatisticsCount = 0;
	valuesPtr->WTPRadioStatistics = NULL;
	valuesPtr->WTPRebootStatistics = NULL;
	valuesPtr->WTPStaDeleteInfo = NULL;

	/* parse message elements */
	while((msgPtr->offset - offsetTillMessages) < len) {

		unsigned short int elemType = 0;
		unsigned short int elemLen = 0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->errorReport, 
						     CWDecryptErrorReportValues,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseMsgElemDecryptErrorReport(msgPtr, elemLen, valuesPtr->errorReport)))
					return CW_FALSE;
				break;	
			case CW_MSG_ELEMENT_DUPLICATE_IPV4_ADDRESS_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->duplicateIPv4,
						     WTPDuplicateIPv4,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	
				
				CW_CREATE_ARRAY_ERR((valuesPtr->duplicateIPv4)->MACoffendingDevice_forIpv4,
						    6,
						    unsigned char,
						    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseMsgElemDuplicateIPv4Address(msgPtr, elemLen, valuesPtr->duplicateIPv4)))
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_DUPLICATE_IPV6_ADDRESS_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->duplicateIPv6,
						     WTPDuplicateIPv6,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				CW_CREATE_ARRAY_ERR((valuesPtr->duplicateIPv6)->MACoffendingDevice_forIpv6,
						    6,
						    unsigned char,
						    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseMsgElemDuplicateIPv6Address(msgPtr, elemLen, valuesPtr->duplicateIPv6)))
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE:
				valuesPtr->WTPOperationalStatisticsCount++;
				msgPtr->offset += elemLen;
				break;
			case CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE:
				valuesPtr->WTPRadioStatisticsCount++;
				msgPtr->offset += elemLen;
				break;
			case CW_MSG_ELEMENT_WTP_REBOOT_STATISTICS_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->WTPRebootStatistics,
						     WTPRebootStatisticsInfo,
						     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

				if (!(CWParseWTPRebootStatistics(msgPtr, elemLen, valuesPtr->WTPRebootStatistics)))
					return CW_FALSE;	
				break;
			//Elena Agostini - 11/2014: Delete Station MsgElem
			case CW_MSG_ELEMENT_DELETE_STATION_CW_TYPE:
				CW_CREATE_OBJECT_ERR(valuesPtr->WTPStaDeleteInfo, CWMsgElemDataDeleteStation, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				if (!(CWParseWTPDeleteStation(msgPtr, elemLen, valuesPtr->WTPStaDeleteInfo)))
					return CW_FALSE;	
				break;
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in WTP Event Request");
				break;	
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	CW_CREATE_ARRAY_ERR(valuesPtr->WTPOperationalStatistics,
			    valuesPtr->WTPOperationalStatisticsCount,
			    WTPOperationalStatisticsValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	CW_CREATE_ARRAY_ERR(valuesPtr->WTPRadioStatistics,
			    valuesPtr->WTPRadioStatisticsCount,
			    WTPRadioStatisticsValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););	

	msgPtr->offset = offsetTillMessages;

	while((msgPtr->offset - offsetTillMessages) < len) {
	
		unsigned short int elemType = 0;
		unsigned short int elemLen = 0;
		
		CWParseFormatMsgElem(msgPtr, &elemType, &elemLen);
		
		switch(elemType) {
			case CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE:
				if (!(CWParseWTPOperationalStatistics(msgPtr,
								      elemLen,
								      &(valuesPtr->WTPOperationalStatistics[k++]))))
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE:
				if (!(CWParseWTPRadioStatistics(msgPtr,
								elemLen,
								&(valuesPtr->WTPRadioStatistics[i++]))))
					return CW_FALSE;
				break;
			default:
				msgPtr->offset += elemLen;
				break;
		}
	}
	CWLog("WTP Event Request Parsed");
	return CW_TRUE;	
}

CWBool CWSaveWTPEventRequestMessage(CWProtocolWTPEventRequestValues *WTPEventRequest,
				    CWWTPProtocolManager *WTPProtocolManager) {

	if(WTPEventRequest == NULL || WTPProtocolManager == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if(WTPEventRequest->WTPRebootStatistics) {	

		CW_FREE_OBJECT(WTPProtocolManager->WTPRebootStatistics);
		WTPProtocolManager->WTPRebootStatistics = WTPEventRequest->WTPRebootStatistics;
	}

	if((WTPEventRequest->WTPOperationalStatisticsCount) > 0) {

		int i,k;
		CWBool found = CW_FALSE;

		for(i = 0; i < (WTPEventRequest->WTPOperationalStatisticsCount); i++) {
			
			found=CW_FALSE;
			for(k=0; k<(WTPProtocolManager->radiosInfo).radioCount; k++) {

				if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == (WTPEventRequest->WTPOperationalStatistics[i]).radioID) {

					found=CW_TRUE;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].TxQueueLevel = (WTPEventRequest->WTPOperationalStatistics[i]).TxQueueLevel;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].wirelessLinkFramesPerSec = (WTPEventRequest->WTPOperationalStatistics[i]).wirelessLinkFramesPerSec;
				}
			}

			found = found;
		}
	}

	if((WTPEventRequest->WTPRadioStatisticsCount) > 0) {
		
		int i,k;
		CWBool found;

		for(i=0; i < (WTPEventRequest->WTPRadioStatisticsCount); i++) {
			found=CW_FALSE;
			for(k = 0; k < (WTPProtocolManager->radiosInfo).radioCount; k++)  {

				if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == (WTPEventRequest->WTPOperationalStatistics[i]).radioID) {

					found=CW_TRUE;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].statistics = (WTPEventRequest->WTPRadioStatistics[i]).WTPRadioStatistics;
				}
			}
			found = found;
		}
	}
	/*
	CW_FREE_OBJECT((WTPEventRequest->WTPOperationalStatistics), (WTPEventRequest->WTPOperationalStatisticsCount));
	CW_FREE_OBJECTS_ARRAY((WTPEventRequest->WTPRadioStatistics), (WTPEventRequest->WTPRadioStatisticsCount));
	Da controllare!!!!!!!
	*/
	CW_FREE_OBJECT(WTPEventRequest->WTPOperationalStatistics);
	CW_FREE_OBJECT(WTPEventRequest->WTPRadioStatistics);
	/*CW_FREE_OBJECT(WTPEventRequest);*/
	
	//Elena Agostini - 11/2014: Delete Station MsgElem
	if(WTPEventRequest->WTPStaDeleteInfo != NULL)
	{
				
		if(WTPEventRequest->WTPStaDeleteInfo->staAddr == NULL)
			return CW_FALSE;

		//---- Delete AVL node (per ora solo per STA addr)
		CWThreadMutexLock(&mutexAvlTree);
		avlTree = AVLdeleteNode(avlTree, WTPEventRequest->WTPStaDeleteInfo->staAddr, WTPEventRequest->WTPStaDeleteInfo->radioID);
		CWThreadMutexUnlock(&mutexAvlTree);
		//----
	}

	return CW_TRUE;
}

CWBool CWAssembleWTPDataTransferResponse (CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum) 
{
	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=0;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;

	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling WTP Data Transfer Response...");
		
	if(!(CWAssembleMessage(messagesPtr, 
	                       fragmentsNumPtr, 
	                       PMTU, 
	                       seqNum,
			               CW_MSG_TYPE_VALUE_DATA_TRANSFER_RESPONSE,
			               msgElems, 
			               msgElemCount, 
			               msgElemsBinding,
						   msgElemBindingCount, 
#ifdef CW_NO_DTLS
						   CW_PACKET_PLAIN))) 
#else
			               CW_PACKET_CRYPT))) 
#endif 

		return CW_FALSE;
	
	CWLog("WTP Data Transfer Response Assembled");
	
	return CW_TRUE;
}

CWBool CWAssembleWTPEventResponse(CWProtocolMessage **messagesPtr,
				  int *fragmentsNumPtr,
				  int PMTU,
				  int seqNum) {

	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=0;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;

	if(messagesPtr == NULL || fragmentsNumPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling WTP Event Response...");
		
	if(!(CWAssembleMessage(messagesPtr,
			       fragmentsNumPtr,
			       PMTU,
			       seqNum,
			       CW_MSG_TYPE_VALUE_WTP_EVENT_RESPONSE,
			       msgElems,
			       msgElemCount,
			       msgElemsBinding,
			       msgElemBindingCount,
#ifdef CW_NO_DTLS
			      CW_PACKET_PLAIN))) 
#else
			      CW_PACKET_CRYPT))) 
#endif
	  
		return CW_FALSE;
	
	CWLog("WTP Event Response Assembled");
	
	return CW_TRUE;
}

CWBool CWParseChangeStateEventRequestMessage2(CWProtocolMessage *msgPtr,
					      int len,
					      CWProtocolChangeStateEventRequestValues **valuesPtr) {

	int offsetTillMessages;
	int i=0;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_OBJECT_ERR(*valuesPtr,
			     CWProtocolChangeStateEventRequestValues,
			     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ WTP Change State Event (Run) ________#");
	
	(*valuesPtr)->radioOperationalInfo.radiosCount = 0;
	(*valuesPtr)->radioOperationalInfo.radios = NULL;
	
	/* parse message elements */
	while((msgPtr->offset-offsetTillMessages) < len) {
		unsigned short int elemType = 0;/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int elemLen = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(msgPtr,&elemType,&elemLen);		

		/*CWDebugLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen);*/

		switch(elemType) {
			case CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE:
				/* just count how many radios we have, so we 
				 * can allocate the array
				 */
				((*valuesPtr)->radioOperationalInfo.radiosCount)++;
				msgPtr->offset += elemLen;
				break;
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE: 
				if(!(CWParseResultCode(msgPtr, elemLen, &((*valuesPtr)->resultCode)))) 
					return CW_FALSE;
				break;
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element in Change State Event Request");
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	CW_CREATE_ARRAY_ERR((*valuesPtr)->radioOperationalInfo.radios,
			    (*valuesPtr)->radioOperationalInfo.radiosCount,
			    CWRadioOperationalInfoValues,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		
	msgPtr->offset = offsetTillMessages;
	
	i = 0;

	while(msgPtr->offset-offsetTillMessages < len) {
		unsigned short int type = 0;	/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int len = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(msgPtr,&type,&len);		

		switch(type) {
			case CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE:
				/* will be handled by the caller */
				if(!(CWParseWTPRadioOperationalState(msgPtr, len, &((*valuesPtr)->radioOperationalInfo.radios[i])))) 
					return CW_FALSE;
				i++;
				break;
			default:
				msgPtr->offset += len;
				break;
		}
	}
	CWLog("Change State Event Request Parsed");
	return CW_TRUE;
}

CWBool CWSaveChangeStateEventRequestMessage(CWProtocolChangeStateEventRequestValues *valuesPtr,
					    CWWTPProtocolManager *WTPProtocolManager) {

	CWBool found;
	CWBool retValue = CW_TRUE;

	if(valuesPtr == NULL || WTPProtocolManager == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if((valuesPtr->radioOperationalInfo.radiosCount) >0) {
	
		int i,k;
		for(i=0; i<(valuesPtr->radioOperationalInfo.radiosCount); i++) {
			found=CW_FALSE;
			for(k=0; k<(WTPProtocolManager->radiosInfo).radioCount; k++) {
				if((WTPProtocolManager->radiosInfo).radiosInfo[k].radioID == (valuesPtr->radioOperationalInfo.radios[i]).ID) {

					found=CW_TRUE;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].operationalState = (valuesPtr->radioOperationalInfo.radios[i]).state;
					(WTPProtocolManager->radiosInfo).radiosInfo[k].operationalCause = (valuesPtr->radioOperationalInfo.radios[i]).cause;
				}
				if(!found) 
					retValue= CW_FALSE;	
			}
		}
	}
	
	CW_FREE_OBJECT(valuesPtr->radioOperationalInfo.radios)
	CW_FREE_OBJECT(valuesPtr);	

	return retValue;
}


CWBool CWParseEchoRequestMessage(CWProtocolMessage *msgPtr, int len) {

	int offsetTillMessages;

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	if((msgPtr->msg) == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	offsetTillMessages = msgPtr->offset;
	
	CWLog("");
	CWLog("#________ Echo Request (Run) ________#");
	
	/* parse message elements */
	while((msgPtr->offset-offsetTillMessages) < len) {
		unsigned short int elemType = 0;/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int elemLen = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(msgPtr,&elemType,&elemLen);		

		/*CWDebugLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen);*/

		switch(elemType) {
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Echo Request must carry no message elements");
		}
	}
	
	if((msgPtr->offset - offsetTillMessages) != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
//	CWLog("Echo Request Parsed");
	
	return CW_TRUE;
}

CWBool CWAssembleEchoResponse(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum) {

	CWProtocolMessage *msgElems= NULL;
	const int msgElemCount=0;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Echo Response...");
		
	if(!(CWAssembleMessage(messagesPtr,
			       fragmentsNumPtr,
			       PMTU,
			       seqNum,
			       CW_MSG_TYPE_VALUE_ECHO_RESPONSE,
			       msgElems,
			       msgElemCount,
			       msgElemsBinding,
			       msgElemBindingCount,
#ifdef CW_NO_DTLS
			      CW_PACKET_PLAIN)))
#else
			      CW_PACKET_CRYPT))) 
#endif
  
		return CW_FALSE;
	
	CWLog("Echo Response Assembled");
	return CW_TRUE;
}

CWBool CWAssembleConfigurationUpdateRequest(CWProtocolMessage **messagesPtr,
					    int *fragmentsNumPtr,
					    int PMTU,
						int seqNum,
						int msgElement) {

	CWProtocolMessage *msgElemsBinding = NULL;
	int msgElemBindingCount=0;
	CWProtocolMessage *msgElems = NULL;
	int msgElemCount=0;
	
	if (messagesPtr == NULL || fragmentsNumPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Configuration Update Request...");

	if(!(CWAssembleMessage(messagesPtr,
			       fragmentsNumPtr,
			       PMTU,
			       seqNum,
			       CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_REQUEST,
			       msgElems,
			       msgElemCount,
			       msgElemsBinding,
			       msgElemBindingCount,
#ifdef CW_NO_DTLS
			      CW_PACKET_PLAIN)))
#else
			      CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;

	CWLog("Configuration Update Request Assembled");
	
	return CW_TRUE;
}

CWBool CWAssembleClearConfigurationRequest(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum) 
{
	CWProtocolMessage *msgElemsBinding = NULL;
	int msgElemBindingCount=0;
	CWProtocolMessage *msgElems = NULL;
	int msgElemCount=0;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWLog("Assembling Clear Configuration Request...");
	
	
	if(!(CWAssembleMessage(messagesPtr, 
						   fragmentsNumPtr, 
						   PMTU, 
						   seqNum,
						   CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_REQUEST, 
						   msgElems, 
						   msgElemCount, 
						   msgElemsBinding, 
						   msgElemBindingCount, 
#ifdef CW_NO_DTLS
						   CW_PACKET_PLAIN)))
#else
						   CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;

	CWLog("Clear Configuration Request Assembled");
	
	return CW_TRUE;
}


CWBool CWStartNeighborDeadTimer(int WTPIndex) {

	/* start NeighborDeadInterval timer */
	if(!CWErr(CWTimerRequest(gCWNeighborDeadInterval,
				 &(gWTPs[WTPIndex].thread),
				 &(gWTPs[WTPIndex].currentTimer),
				 CW_CRITICAL_TIMER_EXPIRED_SIGNAL))) {
		return CW_FALSE;
	}
	return CW_TRUE;
}

CWBool CWStartNeighborDeadTimerForEcho(int WTPIndex){
	
	int echoInterval;

	/* start NeighborDeadInterval timer */
	CWACGetEchoRequestTimer(&echoInterval);
	if(!CWErr(CWTimerRequest(echoInterval,
				 &(gWTPs[WTPIndex].thread),
				 &(gWTPs[WTPIndex].currentTimer),
				 CW_CRITICAL_TIMER_EXPIRED_SIGNAL))) {
		return CW_FALSE;
	}
	return CW_TRUE;
}

CWBool CWStopNeighborDeadTimer(int WTPIndex) {

	if(!CWTimerCancel(&(gWTPs[WTPIndex].currentTimer))) {
	
		return CW_FALSE;
	}
	return CW_TRUE;
}

CWBool CWRestartNeighborDeadTimer(int WTPIndex) {
	
	CWThreadSetSignals(SIG_BLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);	
	
	if(!CWStopNeighborDeadTimer(WTPIndex)) return CW_FALSE;
	if(!CWStartNeighborDeadTimer(WTPIndex)) return CW_FALSE;
	
	CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
	
//	CWDebugLog("NeighborDeadTimer restarted");
	return CW_TRUE;
}

CWBool CWRestartNeighborDeadTimerForEcho(int WTPIndex) {
	
	CWThreadSetSignals(SIG_BLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);

	if(!CWStopNeighborDeadTimer(WTPIndex)) return CW_FALSE;
	if(!CWStartNeighborDeadTimerForEcho(WTPIndex)) return CW_FALSE;

	CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
	
//	CWDebugLog("NeighborDeadTimer restarted for Echo interval");
	return CW_TRUE;
}

/*
 * Elena Agostini - 03/2014
 * 
 * AC Thread for Data Channel packet (DTLS && Plain)
 */
CW_THREAD_RETURN_TYPE CWACReceiveDataChannel(void *arg) {

	int 		i = ((CWACThreadArg*)arg)->index;
	//CWSocket 	sock = ((CWACThreadArg*)arg)->sock;
	
	int dataSocket=0, countPacketDataList=0, readBytes, indexLocal=0;
	CWNetworkLev4Address address;
	CWBool sessionDataActiveLocal = CW_FALSE;
	char* pData;
	
	
	for(indexLocal = 0; indexLocal < gACSocket.count; indexLocal++) {	
		if (gACSocket.interfaces[indexLocal].sock == gWTPs[i].socket){
			dataSocket = gACSocket.interfaces[indexLocal].dataSock;
			CW_COPY_NET_ADDR_PTR(&address,&(gWTPs[i].dataaddress));
			break;
		}
	}

	if (dataSocket == 0){
		CWLog("data socket of WTP isn't ready.");
		/* critical error, close session */
		CWErrorHandleLast();
		CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
		CWCloseThread();
	}

	/* Info Socket Dati */
	struct sockaddr_in *tmpAdd = (struct sockaddr_in *) &(address);
	CWLog("New DTLS Session Data. %s:%d, socket: %d", inet_ntoa(tmpAdd->sin_addr), ntohs(tmpAdd->sin_port), dataSocket);

	/* Session DTLS Data */
	if(!CWErr(CWSecurityInitSessionServerDataChannel(&(gWTPs[i]),	
									&(address),
									dataSocket,
									gACSecurityContext,
									&(gWTPs[i].sessionData),
									&(gWTPs[i].pathMTU))))
	{
		CWErrorHandleLast();
		CWTimerCancel(&(gWTPs[i].currentTimer));
		CWCloseThread();
	}
	
	/* I read the data from the packetlist and rewrite them deciphered */	
	CW_REPEAT_FOREVER {
		countPacketDataList=0;
		
		CWThreadMutexLock(&gWTPs[i].interfaceMutex);
		sessionDataActiveLocal = gWTPs[i].sessionDataActive;
		CWThreadMutexUnlock(&gWTPs[i].interfaceMutex);
		if(sessionDataActiveLocal == CW_TRUE)
		{
			//Se ci sono pacchetti sulla lista dati ... 
			CWLockSafeList(gWTPs[i].packetReceiveDataList);
			countPacketDataList = CWGetCountElementFromSafeList(gWTPs[i].packetReceiveDataList);
			CWUnlockSafeList(gWTPs[i].packetReceiveDataList);
			
			if(countPacketDataList > 0) {
				// ... li legge cifrati ... 
	//			CWLog("+++ Thread DTLS Session Data. %s:%d, socket: %d. Ricevuto pacchetto dati.", inet_ntoa(tmpAdd->sin_addr), ntohs(tmpAdd->sin_port), dataSocket);
				if(!CWErr(CWSecurityReceive(gWTPs[i].sessionData,
											gWTPs[i].buf,
											CW_BUFFER_SIZE - 1,
											&readBytes)))
				{		
					CWDebugLog("Error during security receive");
					CWThreadSetSignals(SIG_UNBLOCK, 1, CW_SOFT_TIMER_EXPIRED_SIGNAL);
					continue;
				}
				
				//... e decifrato il pacchetto dati lo mette sulla packetList ufficiale
				CW_CREATE_OBJECT_SIZE_ERR(pData, readBytes, { CWLog("Out Of Memory"); return NULL; });
				memcpy(pData, gWTPs[i].buf, readBytes);

				CWLockSafeList(gWTPs[i].packetReceiveList);
				CWAddElementToSafeListTailwitDataFlag(gWTPs[i].packetReceiveList, pData, readBytes, CW_TRUE);
				CWUnlockSafeList(gWTPs[i].packetReceiveList);
				
				//Free
			}
		}
		else break;
	}
	
	gWTPs[i].sessionData = NULL;
	
	return NULL;
}
