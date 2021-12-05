#include <glib.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

typedef struct sVFSDirData
{
	gchar **env;
	gsize i;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc;
tLogProc gLogProc;
tRequestProc gRequestProc;
tExtensionStartupInfo* gDialogApi = NULL;
char gEnv[MAX_PATH];
static const char *gForm = R"(
object DialogBox: TDialogBox
  Left = 248
  Height = 171
  Top = 170
  Width = 417
  AutoSize = True
  BorderStyle = bsDialog
  Caption = 'View'
  ChildSizing.LeftRightSpacing = 10
  ChildSizing.TopBottomSpacing = 10
  ChildSizing.HorizontalSpacing = 10
  ChildSizing.VerticalSpacing = 10
  ClientHeight = 171
  ClientWidth = 417
  OnShow = DialogBoxShow
  Position = poMainFormCenter
  LCLVersion = '2.2.0.3'
  object lblEnv: TLabel
    AnchorSideLeft.Control = mValue
    AnchorSideRight.Control = mValue
    AnchorSideRight.Side = asrBottom
    Left = 8
    Height = 51
    Top = 5
    Width = 400
    Anchors = [akTop, akLeft, akRight]
    Caption = 'NONE'
    Font.Style = [fsBold]
    ParentFont = False
    ShowAccelChar = False
    WordWrap = True
  end
  object mValue: TMemo
    AnchorSideTop.Control = lblEnv
    AnchorSideTop.Side = asrBottom
    Left = 8
    Height = 100
    Top = 66
    Width = 400
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssAutoVertical
    TabOrder = 1
    TabStop = False
  end
  object btnClose: TBitBtn
    AnchorSideTop.Control = mValue
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Control = mValue
    AnchorSideRight.Side = asrBottom
    Left = 314
    Height = 30
    Top = 176
    Width = 94
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Top = 10
    Cancel = True
    Default = True
    DefaultCaption = True
    Kind = bkClose
    ModalResult = 11
    TabOrder = 0
  end
end
)";


static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (dirdata->env[dirdata->i] != NULL)
	{
		g_strlcpy(FindData->cFileName, dirdata->env[dirdata->i], MAX_PATH);
		FindData->nFileSizeLow = 0;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
		dirdata->i++;
		return TRUE;
	}

	return FALSE;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		gDialogApi->SendDlgMsg(pDlg, "lblEnv", DM_SETTEXT, (intptr_t)gEnv, 0);
		const gchar* value = g_getenv(gEnv);

		if (!value)
			return -1;

		gDialogApi->SendDlgMsg(pDlg, "mValue", DM_SETTEXT, (intptr_t)value, 0);
	}

	return 0;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->i = 0;
	dirdata->env = g_listenv();

	if (SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	g_strfreev(dirdata->env);

	return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	g_strfreev(dirdata->env);

	return 0;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (RemoteName[1] != 0 && (strcmp(Verb, "properties") == 0 || strcmp(Verb, "open") == 0))
	{
		g_strlcpy(gEnv, RemoteName + 1, MAX_PATH);
		gDialogApi->DialogBoxLFM((intptr_t)gForm, (unsigned long)strlen(gForm), DlgProc);
		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex > 0)
		return ft_nomorefields;

	g_strlcpy(FieldName, "Value", maxlen - 1);
	Units[0] = '\0';

	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	const gchar* value = g_getenv(FileName+1);
	if (!value)
		return ft_fieldempty;

	g_strlcpy((char*)FieldValue, value, maxlen - 1);

	return ft_string;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[Plugin(FS).Value{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "Value", maxlen - 1);
	g_strlcpy(ViewWidths, "100,0,100", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Environment variables", maxlen-1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gDialogApi != NULL)
		g_free(gDialogApi);

	gDialogApi = NULL;
}

