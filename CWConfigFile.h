#ifndef __CAPWAP_CWConfigFile_HEADER__
#define __CAPWAP_CWConfigFile_HEADER__
 
typedef char **CWStringArray;

typedef struct {
	enum {
		CW_INTEGER,
		CW_STRING,
		CW_STRING_ARRAY
	} type;
	
	union {
		int int_value;
		char *str_value;
		char **str_array_value;
	} value;
	
	char *code;
	char *endCode;
	
	int count;
} CWConfigValue;

extern CWConfigValue *gConfigValues;
extern int gConfigValuesCount;


CWBool CWParseConfigFile();
char * CWGetCommand(FILE *configFile);
CWBool CWConfigFileInitLib(void);
CWBool CWConfigFileDestroyLib(void);

#endif
