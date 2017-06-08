#ifndef __CAPWAP_ACInterface_HEADER__
#define __CAPWAP_ACInterface_HEADER__

//No Interface Command
#define	NO_CMD			0
//Manual setting for QoS values
#define QOS_CMD			1
#define CLEAR_CONFIG_MSG_CMD	2
/* 2009 Update: Manual setting for OFDM values*/
#define OFDM_CONTROL_CMD        3
/*Update 2009: 
		Manage UCI configuration command*/
#define UCI_CONTROL_CMD 4
/* Manage WTP Update Command */
#define WTP_UPDATE_CMD	5
//Elena Agostini - 09/2014: IEEE WLAN configuration command
#define IEEE_WLAN_CONFIGURATION_CMD 6

#endif
