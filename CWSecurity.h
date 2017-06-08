#ifndef __CAPWAP_CWSecurity_HEADER__
#define __CAPWAP_CWSecurity_HEADER__

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

typedef SSL_CTX *CWSecurityContext;
typedef SSL *CWSecuritySession;

#define	CWSecuritySetPeerForSession(session, addrPtr)	BIO_ctrl((session)->rbio, BIO_CTRL_DGRAM_SET_PEER, 1, (addrPtr))

CWBool CWSecurityInitLib(void);
CWBool CWSecurityInitSessionClient(CWSocket sock,
				   CWNetworkLev4Address *addrPtr,
				   CWSafeList packetReceiveList,
				   CWSecurityContext ctx,
				   CWSecuritySession *sessionPtr,
				   int *PMTUPtr);

CWBool CWSecurityInitGenericSessionServerDataChannel(CWSafeList packetDataList,
					CWNetworkLev4Address * address,
				   CWSocket sock,
				   CWSecurityContext ctx,
				   CWSecuritySession *sessionPtr,
				   int *PMTUPtr);
				   
CWBool CWSecuritySend(CWSecuritySession session, const char *buf, int len);
CWBool CWSecurityReceive(CWSecuritySession session, 
			 char *buf,
			 int len,
			 int *readBytesPtr);

CWBool CWSecurityInitContext(CWSecurityContext *ctxPtr,
			     const char *caList,
			     const char *keyfile,
			     const char *passw,
			     CWBool isClient,
			     int (*hackPtr)(void *));

void CWSecurityDestroyContext(CWSecurityContext ctx);
void CWSecurityDestroySession(CWSecuritySession s);

BIO* BIO_new_memory(CWSocket sock,
		    CWNetworkLev4Address* pSendAddress,
		    CWSafeList* pRecvAddress);

void CWSslCleanUp();

#endif
