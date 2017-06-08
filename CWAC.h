#ifndef __CAPWAP_CWAC_HEADER__
#define __CAPWAP_CWAC_HEADER__

/*_______________________________________________________*/
/*  *******************___INCLUDE___*******************  */
#include "CWCommon.h"
#include "ACMultiHomedSocket.h"
#include "ACProtocol.h"

#include <ctype.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
/*______________________________________________________*/
/*  *******************___DEFINE___*******************  */
#define CW_MAX_WTP				100
#define CW_CRITICAL_TIMER_EXPIRED_SIGNAL	SIGUSR2
#define CW_SOFT_TIMER_EXPIRED_SIGNAL		SIGUSR1
#define AC_LOG_FILE_NAME				"AC"
#define MAC_ADDR_LEN		6
#define DEST_ADDR_START		4
#define SOURCE_ADDR_START	10

/*Definition of socket's path to send data stats */
#define SOCKET_PATH_AC 				"/tmp/af_unix_ac_client"
#define SOCKET_PATH_RECV_AGENT     		"/tmp/monitorclt"

/********************************************************
 * 2009 Updates:                                        *
 * For manage the applications connected to AC we use   *
 * an array of socket. Through this array we can set    *
 * easily the correct answer socket.                    * 
 * isFree and numSocketFree are used for management.    *
 * The mutex array is used for serialize the correct	*
 * write operation by the different wtp thread on the	*
 * relative application socket.							*
 ********************************************************/

#define MAX_APPS_CONNECTED_TO_AC 4

typedef struct {
	CWSocket appSocket[MAX_APPS_CONNECTED_TO_AC];
	CWBool isFree[MAX_APPS_CONNECTED_TO_AC];
	CWThreadMutex socketMutex[MAX_APPS_CONNECTED_TO_AC];
	int numSocketFree;
	CWThreadMutex numSocketFreeMutex;
} applicationsManager;

applicationsManager appsManager;

/*_____________________________________________________*/
/*  *******************___TYPES___*******************  */


/* 
 * Struct that describes a WTP from the AC's point of view 
 */
typedef struct {
	CWNetworkLev4Address address;
	CWNetworkLev4Address dataaddress;
	CWThread thread;
	CWSecuritySession session;
	/*
	 * Elena Agostini - 03/2014
	 * DTLS Data Session AC
	 */
	CWSecuritySession sessionData;
	CWBool isNotFree;
	CWBool isRequestClose;
	CWStateTransition currentState;
	int interfaceIndex;
	CWSocket socket;
	char buf[CW_BUFFER_SIZE];
	enum  {
		CW_DTLS_HANDSHAKE_IN_PROGRESS,
		CW_WAITING_REQUEST,
		CW_COMPLETED,
	} subState;		
	CWSafeList packetReceiveList;
	
	/*
	 * Elena Agostini - 03/2014: DTLS Data Channel
	 */
	CWSafeList packetReceiveDataList;
	CWBool sessionDataActive;
	
	/* depends on the current state: WaitJoin, NeighborDead */
	CWTimerID currentTimer;
	CWTimerID heartbeatTimer; 
	CWList fragmentsList;
	int pathMTU;
	
	/**** ACInterface ****/
	int interfaceResult;
	CWBool interfaceCommandProgress;
	CWThreadMutex interfaceSingleton;
	CWThreadMutex interfaceMutex;
	CWThreadCondition interfaceWait;
	CWThreadCondition interfaceComplete;
	/********************************************************
	 * 2009 Updates:                                        *
	 * - ofdmValues is a struct for setting the values of   *
	 *   a Configuration Update Request with message        *
	 *   elemente OFDM.                                     *
	 * - applicationIndex is the index of application which *
	 *   request the command to WTP                         *
	 *   (it will used for selecting the correct socket)    *
	 ********************************************************/
	
	CWProtocolVendorSpecificValues* vendorValues;
	int applicationIndex;
	
	
	/**** ACInterface ****/
	CWWTPProtocolManager WTPProtocolManager;
	//Elena Agostini - 09/2014: WUM channel for WLAN Iface operations
	
	/* Retransmission */
	CWProtocolMessage *messages;
 	int messagesCount;
 	int retransmissionCount;
 	CWTimerID currentPacketTimer;
 	CWBool isRetransmitting;
	
	/* expected response */
	int responseType;
	int responseSeqNum;
	
	/* //Unrecognized message type value
	 * int unrecognizedMsgType;
	 */
	// TAP interface name
	char tap_name[IFNAMSIZ];
	int tap_fd;
	
} CWWTPManager;	

//Elena Agostini - 07/2014: max num contemporary threads of generic DTLS data session
#define WTP_MAX_TMP_THREAD_DTLS_DATA 30


/* DTLS Data Channel Generic Thread */
typedef struct genericHandshakeThread {
	CWThread thread_GenericDataChannelHandshake;
	CWSocket dataSock;
	CWNetworkLev4Address addressWTPPtr;
	CWSafeList packetDataList;
	CWThreadMutex interfaceMutex;
	CWThreadCondition interfaceWait;
	struct genericHandshakeThread * next;
} genericHandshakeThread;
typedef genericHandshakeThread * genericHandshakeThreadPtr;

/* Start list generic threads DTLS Data Channel Handshake*/
extern genericHandshakeThreadPtr listGenericThreadDTLSData[WTP_MAX_TMP_THREAD_DTLS_DATA];

/*________________________________________________________________*/
/*  *******************___EXTERN VARIABLES___*******************  */
extern CWWTPManager gWTPs[CW_MAX_WTP];
//Elena Agostini: Unique TAP interface
extern int ACTap_FD;
extern char * ACTap_name;

extern CWThreadMutex gWTPsMutex;
extern CWSecurityContext gACSecurityContext;
extern int gACHWVersion;
extern int gACSWVersion;
extern int gActiveStations;
extern int gActiveWTPs;
extern CWThreadMutex gActiveWTPsMutex;
extern int gLimit;
extern int gMaxWTPs;
extern CWAuthSecurity gACDescriptorSecurity;
extern int gRMACField;
extern int gWirelessField;
extern int gDTLSPolicy;
extern CWThreadSpecific gIndexSpecific;
extern char *gACName;
extern int gDiscoveryTimer;
extern int gEchoRequestTimer;
extern int gIdleTimeout;
extern CWProtocolNetworkInterface *gInterfaces;
extern int gInterfacesCount;
extern char **gMulticastGroups;
extern int gMulticastGroupsCount;
extern CWMultiHomedSocket gACSocket;
extern int gHostapd_port;
extern char* gHostapd_unix_path;
extern unsigned char WTPRadioInformationType;
/*
 * Elena Agostini - 02/2014
 *
 * ECN Support Msg Elem MUST be included in Join Request/Response Messages
 */
extern int gACECNSupport;

/*
 * Elena Agostini - 02/2014
 * OpenSSL params variables
 */
extern char *gACCertificate;
extern char *gACKeyfile;
extern char *gACPassword;

/*
 * Elena Agostini - 03/2014: DTLS Data Channel
 */
extern CWBool ACSessionDataActive;

/*________________________________________________________________*/

/*  *******************___GLOBAL VARIABLES FOR ATH_MONITOR TEST___*******************  */
typedef struct {
		struct  sockaddr_un servaddr;/* address of Receive Agent */
		struct  sockaddr_un clntaddr;/* address on AC-side */
		int data_stats_sock;
		} UNIX_SOCKS_INFO;

UNIX_SOCKS_INFO UnixSocksArray[CW_MAX_WTP];

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

/* in AC.c */
void CWACInit(void);
void CWACEnterMainLoop(void);
CWBool CWACSendAcknowledgedPacket(int WTPIndex, int msgType, int seqNum);
CWBool CWACResendAcknowledgedPacket(int WTPIndex);
void CWACStopRetransmission(int WTPIndex);
void CWACDestroy(void);

/* in ACTest.h */
CWBool ACQosTest(int WTPIndex);

/* in ACRunState.c */
CWBool CWSaveChangeStateEventRequestMessage(CWProtocolChangeStateEventRequestValues *valuesPtr,
					    CWWTPProtocolManager *WTPProtocolManager);


CW_THREAD_RETURN_TYPE CWACipc_with_ac_hostapd(void *arg);

/*
 * DTLS Data Channel Receive Packet
 */
CW_THREAD_RETURN_TYPE CWACReceiveDataChannel(void *arg);

/****************************************************************
 * 2009 Updates:												*
 *				msgElement is used for differentiation between	*
 *				all message elements							*
 ****************************************************************/

CWBool CWAssembleConfigurationUpdateRequest(CWProtocolMessage **messagesPtr,
					    int *fragmentsNumPtr,
					    int PMTU,
						int seqNum,
						int msgElement);


CWBool CWAssembleClearConfigurationRequest(CWProtocolMessage **messagesPtr,
					   int *fragmentsNumPtr, int PMTU,
					   int seqNum);

/* in ACDiscoveryState.c */
CWBool CWAssembleDiscoveryResponse(CWProtocolMessage **messagesPtr, int seqNum);
CWBool CWParseDiscoveryRequestMessage(char *msg,
				      int len,
				      int *seqNumPtr,
				      CWDiscoveryRequestValues *valuesPtr);

/* in ACRetransmission.c */
CWBool CWACSendFragments(int WTPIndex);

/* in ACRunStateCheck.c */
CWBool CWACCheckForConfigurationUpdateRequest(int WTPIndex);

/* in ACProtocol_User.c */
CWBool CWACGetVendorInfos(CWACVendorInfos *valPtr);
int CWACGetRMACField();
int CWACGetWirelessField();
int CWACGetDTLSPolicy();
void CWACDestroyVendorInfos(CWACVendorInfos *valPtr);

/* in ACMainLoop.c */
void CWACManageIncomingPacket(CWSocket sock,
			      char *buf,
			      int len,
			      int incomingInterfaceIndex,
			      CWNetworkLev4Address *addrPtr,
			      CWBool dataFlag );

void *CWManageWTP(void *arg);
void CWCloseThread();
CW_THREAD_RETURN_TYPE CWGenericWTPDataHandshake(void *arg);

/* in CWSecurity.c */
CWBool CWSecurityInitSessionServer(CWWTPManager* pWtp,
				   CWSocket sock,
				   CWSecurityContext ctx,
				   CWSecuritySession *sessionPtr,
				   int *PMTUPtr);

CWBool ACEnterJoin(int WTPIndex, CWProtocolMessage *msgPtr);
CWBool ACEnterConfigure(int WTPIndex, CWProtocolMessage *msgPtr);
CWBool ACEnterDataCheck(int WTPIndex, CWProtocolMessage *msgPtr);
CWBool ACEnterRun(int WTPIndex, CWProtocolMessage *msgPtr, CWBool dataFlag);

CWBool CWSecurityInitSessionServerDataChannel(CWWTPManager* pWtp, 
					CWNetworkLev4Address * address,
				   CWSocket sock,
				   CWSecurityContext ctx,
				   CWSecuritySession *sessionPtr,
				   int *PMTUPtr);
				   
CW_THREAD_RETURN_TYPE CWInterface(void* arg);

#endif
