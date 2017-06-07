#ifndef __CAPWAP_WTPProtocol_HEADER__
#define __CAPWAP_WTPProtocol_HEADER__

/*_____________________________________________________*/
/*  *******************___TYPES___*******************  */
typedef struct {
	int ACIPv4ListCount;
	int *ACIPv4List;	
}ACIPv4ListValues;

typedef struct {
	int ACIPv6ListCount;
	struct in6_addr *ACIPv6List;	
}ACIPv6ListValues;

typedef struct {
	int stations;
	int limit;
	int activeWTPs;
	int maxWTPs;
	CWAuthSecurity security;
	int RMACField;
//	int WirelessField;
	int DTLSPolicy;
	CWACVendorInfos vendorInfos;
	char *name;
	CWProtocolIPv4NetworkInterface *IPv4Addresses;
	int IPv4AddressesCount;
	CWProtocolIPv6NetworkInterface *IPv6Addresses;
	int IPv6AddressesCount;
	ACIPv4ListValues ACIPv4ListInfo;
	ACIPv6ListValues ACIPv6ListInfo;
	CWNetworkLev4Address preferredAddress;
	CWNetworkLev4Address incomingAddress;
} CWACInfoValues;

typedef struct {
	CWACInfoValues ACInfoPtr;
	CWProtocolResultCode code;
	ACIPv4ListValues ACIPv4ListInfo;
	ACIPv6ListValues ACIPv6ListInfo;

	/*
	 * ECN Support Msg Elem MUST be included in Join Request/Response Messages
	 */
	int ECNSupport;
} CWProtocolJoinResponseValues;

typedef struct {
	ACIPv4ListValues ACIPv4ListInfo;
	ACIPv6ListValues ACIPv6ListInfo;
	int discoveryTimer;
	int echoRequestTimer;
	int radioOperationalInfoCount;
	CWRadioOperationalInfoValues *radioOperationalInfo;
	WTPDecryptErrorReport radiosDecryptErrorPeriod;
	int idleTimeout;
	int fallback;
	//void * bindingValues;
} CWProtocolConfigureResponseValues;

typedef struct {
	//void * bindingValues;
	/*Update 2009:
		add new non-binding specific values*/
	void * protocolValues;
} CWProtocolConfigurationUpdateRequestValues;

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

/* Encoder */

CWBool CWAssembleMsgElemACName(CWProtocolMessage *msgPtr);				// 4
CWBool CWAssembleMsgElemACNameWithIndex(CWProtocolMessage *msgPtr);			// 5
CWBool CWAssembleMsgElemDataTransferData(CWProtocolMessage *msgPtr, int data_type);	//13
CWBool CWAssembleMsgElemDiscoveryType(CWProtocolMessage *msgPtr);			//20
CWBool CWAssembleMsgElemDuplicateIPv4Address(CWProtocolMessage *msgPtr);		//21
CWBool CWAssembleMsgElemLocationData(CWProtocolMessage *msgPtr);			//27
CWBool CWAssembleMsgElemStatisticsTimer(CWProtocolMessage *msgPtr);			//33
CWBool CWAssembleMsgElemWTPBoardData(CWProtocolMessage *msgPtr);			//35
CWBool CWAssembleMsgElemWTPDescriptor(CWProtocolMessage *msgPtr);			//36
CWBool CWAssembleMsgElemWTPFrameTunnelMode(CWProtocolMessage *msgPtr);			//38
CWBool CWAssembleMsgElemWTPIPv4Address(CWProtocolMessage *msgPtr);			//39
CWBool CWAssembleMsgElemWTPMACType(CWProtocolMessage *msgPtr);				//40
CWBool CWAssembleMsgElemWTPName(CWProtocolMessage *msgPtr);				//41
CWBool CWAssembleMsgElemWTPOperationalStatistics(CWProtocolMessage *msgPtr,int radio);	//42
CWBool CWAssembleMsgElemWTPRadioStatistics(CWProtocolMessage *msgPtr,int radio);	//43
CWBool CWAssembleMsgElemWTPRebootStatistics(CWProtocolMessage *msgPtr);			//44
CWBool CWAssembleMsgElemECNSupport(CWProtocolMessage *msgPtr);					//53

/* Decoder */
CWBool CWParseACDescriptor(CWProtocolMessage *msgPtr, int len, CWACInfoValues *valPtr);					// 1
CWBool CWParseACIPv4List(CWProtocolMessage *msgPtr, int len, ACIPv4ListValues *valPtr);					// 2
CWBool CWParseACIPv6List(CWProtocolMessage *msgPtr, int len, ACIPv6ListValues *valPtr);					// 3
CWBool CWParseCWControlIPv4Addresses(CWProtocolMessage *msgPtr, int len, CWProtocolIPv4NetworkInterface *valPtr);	//10 
CWBool CWParseCWControlIPv6Addresses(CWProtocolMessage *msgPtr, int len, CWProtocolIPv6NetworkInterface *valPtr);	//11 
CWBool CWParseCWTimers (CWProtocolMessage *msgPtr, int len, CWProtocolConfigureResponseValues *valPtr);			//12 
CWBool CWParseDecryptErrorReportPeriod (CWProtocolMessage *msgPtr, int len, WTPDecryptErrorReportValues *valPtr);	//16 
CWBool CWParseIdleTimeout (CWProtocolMessage *msgPtr, int len, CWProtocolConfigureResponseValues *valPtr);		//26 
CWBool CWParseWTPFallback (CWProtocolMessage *msgPtr, int len, CWProtocolConfigureResponseValues *valPtr);		//37 

/* ECN Support Msg Elem MUST be included in Join Request/Response Messages */
CWBool CWParseACECNSupport(CWProtocolMessage *msgPtr, int len, int *valPtr);


void CWWTPResetRebootStatistics(WTPRebootStatisticsInfo *rebootStatistics);

int CWWTPGetDiscoveryType(void);
int CWWTPGetMaxRadios(void);
int CWWTPGetRadiosInUse(void);
int CWWTPGetEncCapabilities(void);
CWBool CWWTPGetBoardData(CWWTPVendorInfos *valPtr);
CWBool CWWTPGetVendorInfos(CWWTPVendorInfos *valPtr);
int CWWTPGetMACType(void);
char *CWWTPGetLocation(void);
int CWWTPGetSessionID(void);
int CWWTPGetIPv4Address(void);
int CWWTPGetIPv4StatusDuplicate(void);
int CWWTPGetIPv6StatusDuplicate(void);
char *CWWTPGetName(void);
CWBool CWWTPGetRadiosInformation(CWRadiosInformation *valPtr);
int CWWTPGetACIndex();
char* CWWTPGetACName();
int CWWTPGetFrameTunnelMode();
CWBool CWGetWTPRadiosOperationalState(int radioID, CWRadiosOperationalInfo *valPtr);
CWBool CWAssembleMsgElemDecryptErrorReport(CWProtocolMessage *msgPtr, int radioID);
CWBool CWAssembleMsgElemDuplicateIPv6Address(CWProtocolMessage *msgPtr);

//---------------------------------------------------------/
void CWWTPDestroyVendorInfos(CWWTPVendorInfos *valPtr);

#endif
