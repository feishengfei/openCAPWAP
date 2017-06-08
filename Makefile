LDFLAGS = -ljsoncpp -lssl -lcrypto -lpthread -ldl -D_REENTRANT
CFLAGS =  -Wall -g -O0 -D_REENTRANT  
#CFLAGS += -DCW_NO_DTLS -DCW_NO_DTLSCWParseConfigurationUpdateRequest

#DTLS Data Channel
#CFLAGS += -DCW_DTLS_DATA_CHANNEL

# Memory leak
#LDFLAGS += ../dmalloc-5.5.0/libdmallocth.a
#CFLAGS += -DDMALLOC

# Capwap Debugging

CFLAGS += -DCW_DEBUGGING 
#CFLAGS += -DOPENSSL_NO_KRB5


RM = /bin/rm -f 

# list of generated object files for AC. 
AC_OBJS = ACDiscoveryState.o ACJoinState.o ACConfigureState.o ACDataCheckState.o ACRunState.o ACRetransmission.o \
		  ACConfigFile.o ACSettingsFile.o \
		  ACProtocol.o ACProtocol_User.o \
		  ACMainLoop.o ACMultiHomedSocket.o \
		  AC.o

#ACInterface.o 

# list of generated object files for WTP.
WTP_OBJS = WTPDiscoveryState.o WTPJoinState.o WTPConfigureState.o WTPDataCheckState.o WTPRunState.o \
		   WTPSulkingState.o WTPRunStateCheck.o WTPRetransmission.o \
		   WTPConfigFile.o WTPSettingsFile.o \
		   WTPProtocol.o WTPProtocol_User.o \
		   WTP.o

#CW objects
CW_OBJS =  CWCommon.o CWConfigFile.o CWErrorHandling.o CWList.o CWSafeList.o \
		   CWSyslog.o CWNetwork.o CWProtocol.o CWRandom.o CWSecurity.o CWOpenSSLBio.o \
		   CWStevens.o CWThread.o CWBinding.o CWAVL.o \
		   timerlib.o

 
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

$(AC_NAME): $(CW_OBJS) $(AC_OBJS) 
	$(CC) $(CW_OBJS) $(AC_OBJS) $(LDFLAGS) -o $@

$(WTP_NAME): $(CW_OBJS) $(WTP_OBJS) 
	$(CC) -DWRITE_STD_OUTPUT $(CW_OBJS) $(WTP_OBJS) $(LDFLAGS) -o $@

clean: 
	$(RM) $(AC_NAME) $(WTP_NAME) $(WUA_NAME) \
		$(CW_OBJS) \
		$(AC_OBJS) \
		$(WTP_OBJS)

%.o:%.c
	$(CC) $(CFLAGS) -c \
		-o $@ $<
