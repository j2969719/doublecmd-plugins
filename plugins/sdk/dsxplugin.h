#include <stdbool.h>
#include "common.h"

typedef struct {
	char StartPath[1024];
	char FileMask[1024];
	DWORD Attributes;
	char AttribStr[128];
	bool CaseSensitive;
	/* Date/time search */
	bool IsDateFrom;
	bool IsDateTo;
	bool IsTimeFrom;
	bool IsTimeTo;
	double DateTimeFrom;
	double DateTimeTo;  // TDateTime
	/* File size search */
	bool IsFileSizeFrom;
	bool IsFileSizeTo;
	int64_t FileSizeFrom;
	int64_t FileSizeTo;
	/* Find/replace text */
	bool IsFindText;
	char FindText[1024];
	bool IsReplaceText;
	char ReplaceText[1024];
	bool NotContainingText;
} tDsxSearchRecord;

typedef struct {
	LONG size;
	LONG PluginInterfaceVersionLow;
	LONG PluginInterfaceVersionHi;
	char DefaultIniName[MAX_PATH];
} tDsxDefaultParamStruct;

/* Prototypes */
/* Callbacks procs */

typedef void (DCPCALL *tSAddFileProc)(int PluginNr, char *FoundFile);
//if FoundFile='' then searching is finished

typedef void (DCPCALL *tSUpdateStatusProc)(int PluginNr, char *CurrentFile, int FilesScaned);

/* Mandatory (must be implemented) */
typedef int  (DCPCALL *tSInit)(tDsxDefaultParamStruct* dsp, tSAddFileProc pAddFileProc, 
                               tSUpdateStatusProc pUpdateStatus);
typedef void (DCPCALL *tSStartSearch)(int PluginNr, tDsxSearchRecord* pSearchRec);
typedef void (DCPCALL *tSStopSearch)(int PluginNr);
typedef void (DCPCALL *tSFinalize)(int PluginNr);
