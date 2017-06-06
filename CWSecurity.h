/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                *
 *                                                                                         *
 * You should have received a copy of the GNU General Public License along with this       *
 * program; if not, write to the:                                                          *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                    *
 * MA  02111-1307, USA.                                                                    *
 *                                                                                         *
 * In addition, as a special exception, the copyright holders give permission to link the  *
 * code of portions of this program with the OpenSSL library under certain conditions as   *
 * described in each individual source file, and distribute linked combinations including  * 
 * the two. You must obey the GNU General Public License in all respects for all of the    *
 * code used other than OpenSSL.  If you modify file(s) with this exception, you may       *
 * extend this exception to your version of the file(s), but you are not obligated to do   *
 * so.  If you do not wish to do so, delete this exception statement from your version.    *
 * If you delete this exception statement from all source files in the program, then also  *
 * delete it here.                                                                         *
 * 
 * --------------------------------------------------------------------------------------- *
 * Project:  Capwap                                                                        *
 *                                                                                         *
 * Author :  Ludovico Rossi (ludo@bluepixysw.com)                                          *
 *           Del Moro Andrea (andrea_delmoro@libero.it)                                    *
 *           Giovannini Federica (giovannini.federica@gmail.com)                           *
 *           Massimo Vellucci (m.vellucci@unicampus.it)                                    *
 *           Mauro Bisson (mauro.bis@gmail.com)                                            *
 *******************************************************************************************/


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
