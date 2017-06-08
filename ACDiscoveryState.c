#include "CWAC.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

__inline__ int CWACGetHWVersion();
__inline__ int CWACGetSWVersion();
__inline__ int CWACGetStations();
__inline__ int CWACGetLimit();
__inline__ int CWACGetActiveWTPs();
__inline__ int CWACGetMaxWTPs();
__inline__ int CWACGetSecurity();
__inline__ char *CWACGetName();
__inline__ int CWACGetInterfacesCount();

/*_________________________________________________________*/
/*  *******************___FUNCTIONS___*******************  */

/* send Discovery Response to the host at the specified address */
CWBool CWAssembleDiscoveryResponse(CWProtocolMessage **messagesPtr, int seqNum) {

	CWProtocolMessage *msgElems= NULL;
	int msgElemCount = 4;
	CWProtocolMessage *msgElemsBinding= NULL;
	int msgElemBindingCount=0;
	int fragmentsNum;
	
	int k = -1;
	if(messagesPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(CWACSupportIPv6()) {
		msgElemCount++;
	}
	
	CWLog("Send Discovery Response");
	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems, 
					 msgElemCount,
					 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	/* Assemble Message Elements */
	if (
		(!(CWAssembleMsgElemACDescriptor(&(msgElems[++k])))) ||
		(!(CWAssembleMsgElemACName(&(msgElems[++k])))) ||
		(!(CWAssembleMsgElemCWControlIPv4Addresses(&(msgElems[++k]))))
		/*(CWACSupportIPv6() && (!(CWAssembleMsgElemCWControlIPv6Addresses(&(msgElems[++k])))))*/
	) {
		CWErrorHandleLast();
		int i;
		for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
		CW_FREE_OBJECT(msgElems);
		return CW_FALSE; // error will be handled by the caller
	}
	
	return CWAssembleMessage(messagesPtr,
				 &fragmentsNum,
				 0,
				 seqNum,
				 CW_MSG_TYPE_VALUE_DISCOVERY_RESPONSE,
				 msgElems,
				 msgElemCount,
				 msgElemsBinding,
				 msgElemBindingCount,
				 CW_PACKET_PLAIN);
}

CWBool CWParseDiscoveryRequestMessage(char *msg, 
				      int len,
				      int *seqNumPtr,
				      CWDiscoveryRequestValues *valuesPtr) {
						
	CWControlHeaderValues controlVal;
	CWProtocolTransportHeaderValues transportVal;
	int offsetTillMessages;
		
	CWProtocolMessage completeMsg;
	

	if(msg == NULL || seqNumPtr == NULL || valuesPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Parse Discovery Request");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;
	
	CWBool dataFlag = CW_FALSE;
	if(!(CWParseTransportHeader(&completeMsg, &transportVal, &dataFlag))) 
		/* will be handled by the caller */
		return CW_FALSE;
	if(!(CWParseControlHeader(&completeMsg, &controlVal))) 
		/* will be handled by the caller */
		return CW_FALSE;
	
	/* different type */

	if(controlVal.messageTypeValue != CW_MSG_TYPE_VALUE_DISCOVERY_REQUEST)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message is not Discovery Request as Expected");
	
	*seqNumPtr = controlVal.seqNum;
	
	/* skip timestamp */
	controlVal.msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;
	offsetTillMessages = completeMsg.offset;
	
	/* (*valuesPtr).radios.radiosCount = 0; */

	/* parse message elements */
	while((completeMsg.offset-offsetTillMessages) < controlVal.msgElemsLen) {
	
		unsigned short int elemType = 0;/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int elemLen = 0;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(&completeMsg,&elemType,&elemLen);		
	
		/* CWDebugLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen); */
		
		switch(elemType) {
			case CW_MSG_ELEMENT_DISCOVERY_TYPE_CW_TYPE:
				if(!(CWParseDiscoveryType(&completeMsg, elemLen, valuesPtr))) 
					/* will be handled by the caller */
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_BOARD_DATA_CW_TYPE:
				if(!(CWParseWTPBoardData(&completeMsg, elemLen, &(valuesPtr->WTPBoardData)))) 
					/* will be handled by the caller */
					return CW_FALSE;
				break; 
			case CW_MSG_ELEMENT_WTP_DESCRIPTOR_CW_TYPE:
				if(!(CWParseWTPDescriptor(&completeMsg, elemLen, &(valuesPtr->WTPDescriptor))))
					/* will be handled by the caller */
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_FRAME_TUNNEL_MODE_CW_TYPE:
				if(!(CWParseWTPFrameTunnelMode(&completeMsg, elemLen, &(valuesPtr->frameTunnelMode)))) 
					/* will be handled by the caller */
					return CW_FALSE;
				break;
			case CW_MSG_ELEMENT_WTP_MAC_TYPE_CW_TYPE:
				if(!(CWParseWTPMACType(&completeMsg, elemLen, &(valuesPtr->MACType))))
					/* will be handled by the caller */
					return CW_FALSE;
				break;
			/*case CW_MSG_ELEMENT_WTP_RADIO_INFO_CW_TYPE:
				// just count how many radios we have, so we can allocate the array
			  	(*valuesPtr).radios.radiosCount++;
				completeMsg.offset += elemLen;
				break;
			*/
			default:
				return CWErrorRaise(CW_ERROR_INVALID_FORMAT,
					"Unrecognized Message Element");
		}
		
		/*CWDebugLog("bytes: %d/%d", (completeMsg.offset-offsetTillMessages), controlVal.msgElemsLen);*/
	}
	
	if(completeMsg.offset != len) return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
	
	return CW_TRUE;
}

void CWDestroyDiscoveryRequestValues(CWDiscoveryRequestValues *valPtr)
{
	int i;

	if(valPtr == NULL) return;
	for(i = 0; i < (valPtr->WTPDescriptor.vendorInfos).vendorInfosCount; i++) {

		CW_FREE_OBJECT(((valPtr->WTPDescriptor.vendorInfos).vendorInfos)[i].valuePtr);
	}
	CW_FREE_OBJECT((valPtr->WTPDescriptor.vendorInfos).vendorInfos);

	for(i = 0; i < valPtr->WTPBoardData.vendorInfosCount; i++) {
		CW_FREE_OBJECT(valPtr->WTPBoardData.vendorInfos[i].valuePtr);
	}
	CW_FREE_OBJECT(valPtr->WTPBoardData.vendorInfos);

	/*CW_FREE_OBJECT((valPtr->radios).radios);*/
}
