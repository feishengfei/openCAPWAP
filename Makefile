LDFLAGS = -ljsoncpp -lssl -lcrypto -lpthread -ldl -D_REENTRANT
CFLAGS =  -Wall -g -O0 -D_REENTRANT  
#CFLAGS += -DCW_NO_DTLS -DCW_NO_DTLSCWParseConfigurationUpdateRequest

#DTLS Data Channel
#CFLAGS += -DCW_DTLS_DATA_CHANNEL

# Memory leak
#LDFLAGS += ../dmalloc-5.5.0/libdmallocth.a
#CFLAGS += -DDMALLOC

# Capwap Debugging

#CFLAGS += -DCW_DEBUGGING 
#CFLAGS += -DOPENSSL_NO_KRB5


RM = /bin/rm -f 

# list of generated object files for AC. 
AC_OBJS = AC.o ACConfigFile.o ACMainLoop.o ACDiscoveryState.o ACJoinState.o \
	ACConfigureState.o ACDataCheckState.o ACRunState.o ACProtocol_User.o \
	ACRetransmission.o CWCommon.o CWConfigFile.o CWErrorHandling.o CWList.o \
	CWSyslog.o ACMultiHomedSocket.o ACProtocol.o CWSafeList.o CWNetwork.o CWProtocol.o \
	CWRandom.o CWSecurity.o CWOpenSSLBio.o CWStevens.o CWThread.o CWBinding.o \
	ACInterface.o ACSettingsFile.o timerlib.o \
	CWAVL.o

# list of generated object files for WTP.
WTP_OBJS = WTP.o WTPConfigFile.o WTPProtocol.o WTPProtocol_User.o \
	WTPDiscoveryState.o WTPJoinState.o WTPConfigureState.o WTPDataCheckState.o WTPRunState.o WTPRunStateCheck.o \
	WTPRetransmission.o WTPSulkingState.o CWCommon.o CWConfigFile.o CWErrorHandling.o CWSafeList.o CWList.o CWSyslog.o CWNetwork.o \
	CWProtocol.o CWRandom.o CWSecurity.o CWOpenSSLBio.o CWStevens.o CWThread.o CWBinding.o \
	WTPSettingsFile.o timerlib.o \
	CWAVL.o 

WUA_OBJS = WUA.o
 
AC_SRCS = $(AC_OBJS:.o=.c) 
WTP_SRCS = $(WTP_OBJS:.o=.c)
WUA_SRCS = $(WUA:OBJS:.o=.c)

# program executables. 
AC_NAME = AC 
WTP_NAME = WTP 
WUA_NAME = WUA

.PHONY: clean 

# top-level rule, to compile everything. 
all: $(AC_NAME) $(WTP_NAME) 

$(AC_NAME): $(AC_OBJS) 
	$(CC) $(AC_OBJS) $(LDFLAGS) -o $@

$(WTP_NAME): $(WTP_OBJS) 
	$(CC) -DWRITE_STD_OUTPUT $(WTP_OBJS) $(LDFLAGS) -o $@

$(WUA_NAME): $(WUA_OBJS) 
	$(CC) $(WUA_OBJS)  $(LDFLAGS) -o $@

clean: 
	$(RM) $(AC_NAME) $(WTP_NAME) $(WUA_NAME) $(AC_OBJS) $(WTP_OBJS) $(WUA_OBJS) 

%.o:%.c
	$(CC) $(CFLAGS) -c \
		-o $@ $<
