#include "CWAC.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWBool CWAssembleJoinResponse(CWProtocolMessage **messagesPtr,
			      int *fragmentsNumPtr,
			      int PMTU,
			      int seqNum,
			      CWList msgElemList,
			      CWWTPProtocolManager *WTPProtocolManager);

CWBool CWParseJoinRequestMessage(char *msg,
				 int len,
				 int *seqNumPtr,
				 CWProtocolJoinRequestValues *valuesPtr);

CWBool CWSaveJoinRequestMessage(CWProtocolJoinRequestValues *joinRequest,
				CWWTPProtocolManager *WTPProtocolManager);


CWBool ACEnterJoin(int WTPIndex, CWProtocolMessage *msgPtr)
{	
	int seqNum;
	CWProtocolJoinRequestValues joinRequest;
	CWList msgElemList = NULL;
	
	CWLog("\n");
	CWLog("######### Join State #########");	

	if(msgPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if(!(CWParseJoinRequestMessage(msgPtr->msg, msgPtr->offset, &seqNum, &joinRequest))) {
		/* note: we can kill our thread in case of out-of-memory 
		 * error to free some space.
		 * we can see this just calling CWErrorGetLastErrorCode()
		 */
		return CW_FALSE;
	}

	// cancel waitJoin timer
	if(!CWTimerCancel(&(gWTPs[WTPIndex].currentTimer)))
	{
		return CW_FALSE;
	}

	CWBool ACIpv4List = CW_FALSE;
	CWBool ACIpv6List = CW_FALSE;
	CWBool resultCode = CW_TRUE;
	int resultCodeValue = CW_PROTOCOL_SUCCESS;
	/* CWBool sessionID = CW_FALSE; */

	if(!(CWSaveJoinRequestMessage(&joinRequest, &(gWTPs[WTPIndex].WTPProtocolManager)))) {
		resultCodeValue = CW_PROTOCOL_FAILURE_RES_DEPLETION;
	}
	
	CWMsgElemData *auxData;
	if(ACIpv4List) {
		CW_CREATE_OBJECT_ERR(auxData, CWMsgElemData, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
                auxData->type = CW_MSG_ELEMENT_AC_IPV4_LIST_CW_TYPE;
		auxData->value = 0;
		CWAddElementToList(&msgElemList,auxData);
	}
	if(ACIpv6List){
		CW_CREATE_OBJECT_ERR(auxData, CWMsgElemData, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
                auxData->type = CW_MSG_ELEMENT_AC_IPV6_LIST_CW_TYPE;
                auxData->value = 0;
                CWAddElementToList(&msgElemList,auxData);
	}
	if(resultCode){
		CW_CREATE_OBJECT_ERR(auxData, CWMsgElemData, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
                auxData->type =  CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE;
                auxData->value = resultCodeValue;
                CWAddElementToList(&msgElemList,auxData);
	}
	
	/*
 	if(sessionID){
 		CW_CREATE_OBJECT_ERR(auxData, CWMsgElemData, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
                 auxData->type =  CW_MSG_ELEMENT_SESSION_ID_CW_TYPE;
                 auxData->value = CWRandomIntInRange(0, INT_MAX);
                 CWAddElementToList(&msgElemList,auxData);
 	}
 	*/

	/* random session ID */
	if(!(CWAssembleJoinResponse(&(gWTPs[WTPIndex].messages),
				    &(gWTPs[WTPIndex].messagesCount),
				    gWTPs[WTPIndex].pathMTU,
				    seqNum,
				    msgElemList,
				    &(gWTPs[WTPIndex].WTPProtocolManager)))){

		CWDeleteList(&msgElemList, CWProtocolDestroyMsgElemData);
		return CW_FALSE;
	}
	
	CWDeleteList(&msgElemList, CWProtocolDestroyMsgElemData);
	
	if(!CWACSendFragments(WTPIndex)) {
		return CW_FALSE;
 	}

	gWTPs[WTPIndex].currentState = CW_ENTER_CONFIGURE;
	
	return CW_TRUE;
}

/*
 * Assemble Join Response.
 */
CWBool CWAssembleJoinResponse(CWProtocolMessage **messagesPtr,
			      int *fragmentsNumPtr,
			      int PMTU,
			      int seqNum,
			      CWList msgElemList,
			      CWWTPProtocolManager *WTPProtocolManager) {

	CWProtocolMessage *msgElems= NULL;
	int msgElemCount = 0;
	/* Result code is not included because it's already
	 * in msgElemList. Control IPv6 to be added.
	 */
	const int mandatoryMsgElemCount=6;
	CWProtocolMessage *msgElemsBinding= NULL;
	const int msgElemBindingCount=0;
	int i;
	CWListElement *current;
	int k = -1;
	
	if(messagesPtr == NULL || fragmentsNumPtr == NULL || msgElemList == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	msgElemCount = CWCountElementInList(msgElemList);

	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(msgElems,
					 msgElemCount + mandatoryMsgElemCount,
					 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););

	CWDebugLog("Assembling Join Response...");
	
	if(
	   (!(CWAssembleMsgElemACDescriptor(&(msgElems[++k])))) ||
	   (!(CWAssembleMsgElemACName(&(msgElems[++k])))) ||
		/*
	 	 * ECN Support Msg Elem MUST be included in Join Request/Response Messages
	 	 */
	     (!(CWAssembleMsgElemECNSupport(&(msgElems[++k])))) ||

		/*
		 * Add AC local IPv4 Address Msg. Elem.
		 */
		(!(CWAssembleMsgElemCWLocalIPv4Addresses(&(msgElems[++k])))) ||

	   (!(CWAssembleMsgElemCWControlIPv4Addresses(&(msgElems[++k]))))
	) {
		CWErrorHandleLast();
		int i;
		for(i = 0; i <= k; i++) {CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
		CW_FREE_OBJECT(msgElems);
		/* error will be handled by the caller */
		return CW_FALSE;
	} 
		
	current=msgElemList;
	for (i=0; i<msgElemCount; i++) {

                switch (((CWMsgElemData *) (current->data))->type) {

			case CW_MSG_ELEMENT_AC_IPV4_LIST_CW_TYPE:
				if (!(CWAssembleMsgElemACIPv4List(&(msgElems[++k]))))
					goto cw_assemble_error;	
				break;			
			case CW_MSG_ELEMENT_AC_IPV6_LIST_CW_TYPE:
				if (!(CWAssembleMsgElemACIPv6List(&(msgElems[++k]))))
					goto cw_assemble_error;
				break;
			case CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE:
				if (!(CWAssembleMsgElemResultCode(&(msgElems[++k]), ((CWMsgElemData *) current->data)->value)))
					goto cw_assemble_error;
				break;
			/*
			case CW_MSG_ELEMENT_SESSION_ID_CW_TYPE:
				if (!(CWAssembleMsgElemSessionID(&(msgElems[++k]), ((CWMsgElemData *) current->data)->value)))
					goto cw_assemble_error;
				break;
			*/
                        default: {
                                int j;
                                for(j = 0; j <= k; j++) { CW_FREE_PROTOCOL_MESSAGE(msgElems[j]);}
                                CW_FREE_OBJECT(msgElems);
                                return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Unrecognized Message Element for Join Response Message");
				break;
		        }
                }
		current = current->next;
	}

	if (!(CWAssembleMessage(messagesPtr,
				fragmentsNumPtr,
				PMTU,
				seqNum,
				CW_MSG_TYPE_VALUE_JOIN_RESPONSE,
				msgElems,
				msgElemCount + mandatoryMsgElemCount,
				msgElemsBinding,
				msgElemBindingCount,
#ifdef CW_NO_DTLS
				CW_PACKET_PLAIN)))
#else
 				CW_PACKET_CRYPT)))
#endif
		return CW_FALSE;

	CWDebugLog("Join Response Assembled");
	
	return CW_TRUE;

cw_assemble_error:
	{
		int i;
		for(i = 0; i <= k; i++) { CW_FREE_PROTOCOL_MESSAGE(msgElems[i]);}
		CW_FREE_OBJECT(msgElems);
		/* error will be handled by the caller */
		return CW_FALSE;
	}
	return CW_TRUE;
}

/* 
 * Parses Join Request.
 */
CWBool CWParseJoinRequestMessage(char *msg,
				 int len,
				 int *seqNumPtr,
				 CWProtocolJoinRequestValues *valuesPtr) {

	CWControlHeaderValues controlVal;
	int offsetTillMessages;
	CWProtocolMessage completeMsg;
	
	if(msg == NULL || seqNumPtr == NULL || valuesPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWDebugLog("Parse Join Request");
	
	completeMsg.msg = msg;
	completeMsg.offset = 0;

	if(!(CWParseControlHeader(&completeMsg, &controlVal)))
		/* will be handled by the caller */
		return CW_FALSE;

	/* different type */
	if(controlVal.messageTypeValue != CW_MSG_TYPE_VALUE_JOIN_REQUEST)
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message is not Join Request as Expected");
	
	*seqNumPtr = controlVal.seqNum;
	/* skip timestamp */
	controlVal.msgElemsLen -= CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS;
	offsetTillMessages = completeMsg.offset;
	
	/* parse message elements */
	while((completeMsg.offset-offsetTillMessages) < controlVal.msgElemsLen) {

		unsigned short int elemType = 0;/* = CWProtocolRetrieve32(&completeMsg); */
		unsigned short int elemLen =0 ;	/* = CWProtocolRetrieve16(&completeMsg); */
		
		CWParseFormatMsgElem(&completeMsg,&elemType,&elemLen);
		
//		CWLog("Parsing Message Element: %u, elemLen: %u", elemType, elemLen);
									
		switch(elemType) {
			case CW_MSG_ELEMENT_LOCATION_DATA_CW_TYPE:
				if(!(CWParseLocationData(&completeMsg, elemLen, &(valuesPtr->location)))) 
					/* will be handled by the caller */
					return CW_FALSE;
				break;
				
			case CW_MSG_ELEMENT_WTP_BOARD_DATA_CW_TYPE:
				if(!(CWParseWTPBoardData(&completeMsg, elemLen, &(valuesPtr->WTPBoardData)))) 
					/* will be handled by the caller */
					return CW_FALSE;
				break; 
				
			case CW_MSG_ELEMENT_SESSION_ID_CW_TYPE:
				valuesPtr->sessionID  = CWParseSessionID(&completeMsg, elemLen);
				break;
				
			case CW_MSG_ELEMENT_WTP_DESCRIPTOR_CW_TYPE:
				if(!(CWParseWTPDescriptor(&completeMsg, elemLen, &(valuesPtr->WTPDescriptor))))
					/* will be handled by the caller */
					return CW_FALSE;
				break;
				
			case CW_MSG_ELEMENT_LOCAL_IPV4_ADDRESS_CW_TYPE:
				if(!(CWParseWTPIPv4Address(&completeMsg, elemLen, valuesPtr)))
					/* will be handled by the caller */
					return CW_FALSE;
				break;
				
			case CW_MSG_ELEMENT_WTP_NAME_CW_TYPE:
				if(!(CWParseWTPName(&completeMsg, elemLen, &(valuesPtr->name))))
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
			
			/*
			 * Elena Agostini - 02/2014: ECN Support Msg Elem MUST be included in Join Request/Response Messages
		 	 */
			case CW_MSG_ELEMENT_ECN_SUPPORT_CW_TYPE:
				if(!(CWParseWTPECNSupport(&completeMsg, elemLen, &(valuesPtr->ECNSupport))))
					/* will be handled by the caller */
					return CW_FALSE;
				break;
	
			default:
				completeMsg.offset += elemLen;
				CWLog("Unrecognized Message Element(%d) in Discovery response",elemType);
				break;
		}
		/*CWDebugLog("bytes: %d/%d", (completeMsg.offset-offsetTillMessages), controlVal.msgElemsLen);*/
	}

	if (completeMsg.offset != len) 
		return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Garbage at the End of the Message");
		
	return CW_TRUE;
}

CWBool CWSaveJoinRequestMessage(CWProtocolJoinRequestValues *joinRequest,
				CWWTPProtocolManager *WTPProtocolManager) {

	CWDebugLog("Saving Join Request...");
	
	if(joinRequest == NULL || WTPProtocolManager == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if ((joinRequest->location)!= NULL) {

		CW_FREE_OBJECT(WTPProtocolManager->locationData);
		WTPProtocolManager->locationData= joinRequest->location;
	}
	else 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if ((joinRequest->name)!= NULL) {

		CW_FREE_OBJECT(WTPProtocolManager->name);
		WTPProtocolManager->name= joinRequest->name;
	}
	else 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
		
	CW_FREE_OBJECT((WTPProtocolManager->WTPBoardData).vendorInfos);
	WTPProtocolManager->WTPBoardData = joinRequest->WTPBoardData;

	/*
	 * SessionID string wasn't saved in right way
	 */
	 CW_CREATE_ARRAY_ERR(WTPProtocolManager->sessionID, WTP_SESSIONID_LENGTH, char, return CW_FALSE;);
	 memcpy(WTPProtocolManager->sessionID, joinRequest->sessionID, WTP_SESSIONID_LENGTH);
	//CW_CREATE_STRING_FROM_STRING_ERR(WTPProtocolManager->sessionID, joinRequest->sessionID, {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);});

	WTPProtocolManager->ipv4Address= joinRequest->addr;
	
	WTPProtocolManager->descriptor= joinRequest->WTPDescriptor;
	WTPProtocolManager->radiosInfo.radioCount = (joinRequest->WTPDescriptor).radiosInUse;
	CW_FREE_OBJECT(WTPProtocolManager->radiosInfo.radiosInfo);

	CWDebugLog("Join Request Saved");
	return CW_TRUE;
}
