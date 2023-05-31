#define _GNU_SOURCE
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define MAX_LINES 10

#ifdef  TEMP_PANEL
#define EXEC_DIR "scripts/temppanel"
#define CFG_NAME "wfx_scripts_temppanel.ini"
#define ENVVAR_NAME "DC_WFX_TP_SCRIPT_DATA"
#else
#define EXEC_DIR "scripts"
#define CFG_NAME "wfx_scripts.ini"
#define ENVVAR_NAME "DC_WFX_SCRIPT_DATA"
#endif

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gDialogApi->SendDlgMsg

#define LIST_REGEXP "([0-9cbdflrstwxST\\-]+)\\s+(\\d{4}\\-?\\d{2}\\-?\\d{2}[\\stT]\\d{2}:?\\d{2}:?\\d?\\d?Z?)\\s+([0-9\\-]+)\\s+([^\\n]+)"

#define CHECKFIELDS_OPT "Fs_GetSupportedField_Needed"
#define STATUSINFO_OPT "Fs_StatusInfo_Needed"
#define REQUEST_OPT "Fs_Request_Options"
#define ENVVAR_OPT "Fs_Set_" ENVVAR_NAME
#define YESNOMSG_OPT "Fs_YesNo_Message"
#define INFORM_OPT "Fs_Info_Message"
#define CHOICE_OPT "Fs_MultiChoice"
#define SELFILE_OPT "Fs_SelectFile"
#define SELDIR_OPT "Fs_SelectDir"
#define NOISE_OPT "Fs_DebugMode"
#define PUSH_OPT "Fs_PushValue"
#define ACT_OPT "Fs_PropsActs"

#define IN_USE_MARK "Fs_InUse"


#define VERB_INIT     "init"
#define VERB_DEINIT   "deinit"
#define VERB_SETOPT   "setopt"
#define VERB_LIST     "list"
#define VERB_EXISTS   "exists"
#define VERB_GET_FILE "copyout"
#define VERB_PUT_FILE "copyin"
#define VERB_COPY     "cp"
#define VERB_MOVE     "mv"
#define VERB_REMOVE   "rm"
#define VERB_RMDIR    "rmdir"
#define VERB_MKDIR    "mkdir"
#define VERB_CHMOD    "chmod"
#define VERB_QUOTE    "quote"
#define VERB_EXEC     "run"
#define VERB_PROPS    "properties"
#define VERB_MODTIME  "modtime"
#define VERB_STATUS   "statusinfo"
#define VERB_REALNAME "localname"
#define VERB_FIELDS   "getfields"
#define VERB_GETVALUE "getvalue"


typedef struct sVFSDirData
{
	DIR *dir;
	gchar *output;
	GRegex *regex;
	char script[PATH_MAX];
	GMatchInfo *match_info;
} tVFSDirData;

int gPluginNr;
int gCryptoNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;
tCryptProc gCryptProc = NULL;

static gchar *g_props = NULL;
static gchar *g_caller = NULL;
static gchar *g_choice = NULL;
static GKeyFile *g_cfg = NULL;
static gchar **g_fields = NULL;
static gboolean g_noise = FALSE;
static char g_script[PATH_MAX];
static char g_exec_start[PATH_MAX];
static char g_scripts_dir[PATH_MAX];
static char g_lfm_path[PATH_MAX];
static char g_history_file[PATH_MAX];
static const char *g_multichoice_lfm = R"(
object MultichoiceDialogBox: TMultichoiceDialogBox
  Left = 295
  Height = 105
  Top = 84
  Width = 374
  AutoSize = True
  BorderStyle = bsDialog
  Caption = 'Confirmation of parameter'
  ChildSizing.LeftRightSpacing = 10
  ChildSizing.TopBottomSpacing = 10
  ClientHeight = 105
  ClientWidth = 374
  OnCreate = DialogBoxShow
  Position = poScreenCenter
  LCLVersion = '2.0.13.0'
  object lblText: TLabel
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = Owner
    Left = 10
    Height = 1
    Top = 10
    Width = 1
    BorderSpacing.Left = 10
    BorderSpacing.Top = 10
    ParentColor = False
    WordWrap = True
  end
  object cbChoice: TComboBox
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = lblText
    AnchorSideTop.Side = asrBottom
    Left = 10
    Height = 28
    Top = 21
    Width = 358
    BorderSpacing.Top = 10
    Constraints.MinWidth = 300
    ItemHeight = 0
    Style = csOwnerDrawVariable
    TabOrder = 1
  end
  object btnCancel: TBitBtn
    AnchorSideTop.Control = cbChoice
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Control = btnOK
    Left = 162
    Height = 30
    Top = 59
    Width = 97
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Top = 10
    BorderSpacing.Right = 10
    Cancel = True
    Constraints.MinHeight = 30
    Constraints.MinWidth = 97
    DefaultCaption = True
    Kind = bkCancel
    ModalResult = 2
    OnClick = ButtonClick
    ParentFont = False
    TabOrder = 2
  end
  object btnOK: TBitBtn
    AnchorSideTop.Control = cbChoice
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Side = asrBottom
    AnchorSideRight.Control = cbChoice
    Left = 269
    Height = 30
    Top = 59
    Width = 97
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Top = 10
    Constraints.MinHeight = 30
    Constraints.MinWidth = 97
    Default = True
    DefaultCaption = True
    Kind = bkOK
    ModalResult = 1
    OnClick = ButtonClick
    ParentFont = False
    TabOrder = 0
  end
end
)";

unsigned long FileTimeToUnixTime(LPFILETIME ft)
{
	int64_t ll = ft->dwHighDateTime;
	ll = (ll << 32) | ft->dwLowDateTime;
	ll = (ll - 116444736000000000) / 10000000;
	return (unsigned long)ll;
}

static gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

static void SetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

static void LogMessage(int PluginNr, int MsgType, char* LogString)
{
#ifndef  TEMP_PANEL

	if (gLogProc && LogString)
		gLogProc(PluginNr, MsgType, LogString);
	else
	{
#endif
		if (LogString)
		{
			gchar **split = g_strsplit(LogString, "%", -1);
			gchar *escaped = g_strjoinv("%%", split);
			g_strfreev(split);
			gchar *message = g_strdup_printf("%s\n", escaped);
			g_print(message);
			g_free(message);
			g_free(escaped);
		}
#ifndef  TEMP_PANEL
	}
#endif
}

static gboolean ProgressNeeded(gchar *verb)
{
	gboolean result = FALSE;

	if (g_strcmp0(verb, VERB_GET_FILE) == 0 ||
	                g_strcmp0(verb, VERB_PUT_FILE) == 0 ||
	                g_strcmp0(verb, VERB_COPY) == 0 ||
	                g_strcmp0(verb, VERB_MOVE) == 0 ||
	                g_strcmp0(verb, VERB_REMOVE) == 0)
		result = TRUE;

	return result;
}

static gboolean ExecuteScript(gchar *script_name, gchar *verb, char *arg1, char *arg2, gchar **output)
{
	gchar *argv[4];
	gsize len, term;
	gchar *line = NULL;
	GPid pid;
	gint status = 0;
	gint stdout_fp, stderr_fp;
	gchar *message = NULL;
	gboolean result = TRUE;
	gchar **envp = NULL;
	GError *err = NULL;

	if (!script_name || script_name[0] == '\0')
		return FALSE;

	gchar *script = g_strdup_printf("./%s", script_name);
	gboolean is_progress_needed = ProgressNeeded(verb);

	argv[0] = script;
	argv[1] = verb;
	argv[2] = arg1;
	argv[3] = arg2;
	argv[4] = NULL;

	gchar *env_data = g_key_file_get_string(g_cfg, script_name, ENVVAR_OPT, NULL);

	if (env_data)
	{
		envp = g_environ_setenv(g_get_environ(), ENVVAR_NAME, env_data, TRUE);
		g_free(env_data);
	}


#ifdef  TEMP_PANEL

	if (g_noise)
#else
	if (!g_noise && g_strcmp0(verb, VERB_FIELDS) == 0)
	{

	}
	else
#endif
	{
		message = g_strdup_printf("%s %s %s %s", script, verb, arg1 ? arg1 : "", arg2 ? arg2 : "");
		LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
		g_free(message);

	}

	result = g_spawn_async_with_pipes(g_scripts_dir, argv, envp, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL, &stdout_fp, &stderr_fp, &err);
	g_free(script);
	g_strfreev(envp);

	if (err)
	{
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);
		g_clear_error(&err);
	}

	if (result)
	{
		GIOChannel *stdout_chan = g_io_channel_unix_new(stdout_fp);
		GString *lines = g_string_new(NULL);

		while (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout_chan, &line, &len, &term, &err))
		{
			if (line)
			{
				lines = g_string_append(lines, line);
				line[term] = '\0';

				if (g_noise)
					LogMessage(gPluginNr, MSGTYPE_DETAILS, line);

				if (is_progress_needed)
				{
					gint64 prcnt = g_ascii_strtoll(line, NULL, 0);

					if (prcnt < 0 && prcnt > 100)
						prcnt = 0;

					if (gProgressProc(gPluginNr, arg1, arg2, (int)prcnt))
					{
						kill(pid, SIGTERM);
						break;
					}
				}

				g_free(line);
			}
		}

		if (err)
		{
			LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);
			g_clear_error(&err);
		}


		GIOChannel *stderr_chan = g_io_channel_unix_new(stderr_fp);

		while (G_IO_STATUS_NORMAL == g_io_channel_read_line(stderr_chan, &line, &len, &term, &err))
		{
			if (line)
			{
				line[term] = '\0';
				LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, line);
				g_free(line);
			}
		}

		if (err)
		{
			LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);
			g_clear_error(&err);
		}


		waitpid(pid, &status, 0);
		g_spawn_close_pid(pid);
		g_io_channel_shutdown(stdout_chan, TRUE, &err);

		if (err)
		{
			LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);
			g_clear_error(&err);
		}

		g_io_channel_unref(stdout_chan);

		g_io_channel_shutdown(stderr_chan, TRUE, &err);

		if (err)
		{
			LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);
			g_clear_error(&err);
		}

		g_io_channel_unref(stderr_chan);
		g_close(stdout_fp, NULL);
		g_close(stderr_fp, NULL);

		if (output)
			*output = g_string_free(lines, FALSE);
		else
			g_string_free(lines, TRUE);
	}

#ifdef  TEMP_PANEL

	if (g_noise)
#else
	if (!g_noise && g_strcmp0(verb, VERB_FIELDS) == 0)
	{

	}
	else
#endif
	{
		message = g_strdup_printf("exit status %d", WEXITSTATUS(status));
		LogMessage(gPluginNr, MSGTYPE_OPERATIONCOMPLETE, message);
		g_free(message);
	}

	if (err)
		g_error_free(err);

	if (status != 0)
		result = FALSE;

	return result;
}

static void LoadPreview(uintptr_t pDlg, gchar *file)
{
	FILE *fp;
	int count = 0;
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;
	gboolean readme = TRUE;

	SendDlgMsg(pDlg, "mPreview", DM_SETTEXT, 0, 0);

	gchar *src_file = g_strdup_printf("%s/%s_readme.txt", g_scripts_dir, file);

	if (!g_file_test(src_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		readme = FALSE;
		g_free(src_file);
		src_file = g_strdup_printf("%s/%s", g_scripts_dir, file);
	}

	if ((fp = fopen(src_file, "r")) != NULL)
	{
		SendDlgMsg(pDlg, "mPreview", DM_ENABLE, 1, 0);

		while ((read = getline(&line, &len, fp)) != -1)
		{
			if (!readme && count > MAX_LINES)
			{
				SendDlgMsg(pDlg, "mPreview", DM_LISTADD, (intptr_t)"...", 0);
				break;
			}

			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			SendDlgMsg(pDlg, "mPreview", DM_LISTADD, (intptr_t)line, 0);

			count++;
		}

		g_free(line);
		fclose(fp);
	}

	if (readme)
	{
		g_free(src_file);
		src_file = g_strdup_printf("%s/%s", g_scripts_dir, file);
	}


	GFile *gfile = g_file_new_for_path(src_file);

	if (gfile)
	{
		GFileInfo *fileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, 0, NULL, NULL);

		if (fileinfo)
		{
			const gchar *content_type = g_file_info_get_content_type(fileinfo);
			gchar *description = g_content_type_get_description(content_type);
			SendDlgMsg(pDlg, "lScriptType", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lScriptType", DM_SETTEXT, (intptr_t)description, 0);
			g_free(description);
			g_object_unref(fileinfo);
		}

		g_object_unref(gfile);
	}

	g_free(src_file);
}

static gboolean ValidOpt(gchar *str, gchar *opt)
{
	return (strncmp(str, opt, strlen(opt)) == 0 && strlen(str) > (strlen(opt) + 2));
}

static void FillProps(uintptr_t pDlg)
{
	int i = 1;

	if (g_props && g_props[0] != '\0')
	{
		gchar **split = g_strsplit(g_props, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			if (i > 10)
				break;

			if (ValidOpt(*p, ACT_OPT))
			{
				gchar **acts = g_strsplit(*p + strlen(ACT_OPT) + 1, "\t", -1);

				if (g_strv_length(acts) > 0)
				{
					for (gchar **a = acts; *a != NULL; a++)
					{
						if (*a[0] != '\0')
							SendDlgMsg(pDlg, "cbAct", DM_LISTADD, (intptr_t)*a, 0);
					}

					SendDlgMsg(pDlg, "cbAct", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "btnAct", DM_SHOWITEM, 1, 0);
				}
			}
			else
			{
				gchar **res = g_strsplit(*p, "\t", 2);

				if (!res[0] || !res[1])
					continue;

				if (g_ascii_strcasecmp(res[0], "filetype") == 0)
					SendDlgMsg(pDlg, "lType", DM_SETTEXT, (intptr_t)g_strstrip(res[1]), 0);
				else if (g_ascii_strcasecmp(res[0], "content_type") == 0)
				{
					gchar *description = g_content_type_get_description(g_strstrip(res[1]));

					if (description)
						SendDlgMsg(pDlg, "lType", DM_SETTEXT, (intptr_t)description, 0);

					g_free(description);
				}
				else if (g_ascii_strcasecmp(res[0], "path") == 0)
				{
					SendDlgMsg(pDlg, "ePath", DM_SETTEXT, (intptr_t)g_strstrip(res[1]), 0);
					SendDlgMsg(pDlg, "lPath", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "ePath", DM_SHOWITEM, 1, 0);
				}
				else if (g_ascii_strcasecmp(res[0], "url") == 0)
				{
					SendDlgMsg(pDlg, "lURL", DM_SETTEXT, (intptr_t)g_strstrip(res[1]), 0);
					SendDlgMsg(pDlg, "lURLLabel", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "lURL", DM_SHOWITEM, 1, 0);
				}
				else
				{
					gchar *item = g_strdup_printf("lProp%d", i);
					SendDlgMsg(pDlg, item, DM_SETTEXT, (intptr_t)g_strstrip(res[0]), 0);
					SendDlgMsg(pDlg, item, DM_SHOWITEM, 1, 0);
					g_free(item);
					item = g_strdup_printf("lValue%d", i);
					SendDlgMsg(pDlg, item, DM_SETTEXT, (intptr_t)g_strstrip(res[1]), 0);
					SendDlgMsg(pDlg, item, DM_SHOWITEM, 1, 0);
					g_free(item);
					i++;
				}

				g_strfreev(res);
			}
		}

		g_strfreev(split);
	}
}

static void LogCryptProc(int ret)
{
	switch (ret)
	{
	case FS_FILE_OK:
		if (g_noise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "CryptProc: Success");

		break;

	case FS_FILE_NOTSUPPORTED:
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc: Encrypt/Decrypt failed");
		break;

	case FS_FILE_WRITEERROR:
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc: Could not write password to password store");
		break;

	case FS_FILE_READERROR:
		if (g_noise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "CryptProc: Password not found in password store");

		break;

	case FS_FILE_NOTFOUND:
		if (g_noise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "CryptProc: No master password entered yet");

		break;
	}
}

static void ParseOpts(gchar *script, gchar *text);

static void SetOpt(gchar *script, gchar *opt, gchar *value)
{
	gchar *output = NULL;
	ExecuteScript(script, VERB_SETOPT, opt, value, &output);

	if (output && output[0] != '\0')
		ParseOpts(script, output);

	g_free(output);
	output = NULL;
}

intptr_t DCPCALL SelectFileDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			gchar *output = NULL;
			char *text = (char*)SendDlgMsg(pDlg, "lblText", DM_GETTEXT, 0, 0);
			char *res = (char*)SendDlgMsg(pDlg, "FileName", DM_GETTEXT, 0, 0);

			if (res && res[0] != '\0')
			{
				g_key_file_set_string(g_cfg, g_script, text, res);
				ExecuteScript(g_script, VERB_SETOPT, text, res, &output);

				if (output && output[0] != '\0')
					ParseOpts(g_script, output);

				g_free(output);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "empty argument selected");
		}

		break;
	}

	return 0;
}

static void SelectFile(char *text)
{
	char lfmdata_templ[] = ""
	                       "object SelectFileDialogBox: TSelectFileDialogBox\n"
	                       "  Left = 210\n"
	                       "  Height = 138\n"
	                       "  Top = 218\n"
	                       "  Width = 622\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Confirmation of parameter'\n"
	                       "  ChildSizing.LeftRightSpacing = 10\n"
	                       "  ChildSizing.TopBottomSpacing = 10\n"
	                       "  ClientHeight = 138\n"
	                       "  ClientWidth = 622\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnCreate = DialogBoxShow\n"
	                       "  Position = poScreenCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblText: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 10\n"
	                       "    Width = 21\n"
	                       "    Caption = '%s'\n"
	                       "    ParentColor = False\n"
	                       "    WordWrap = True\n"
	                       "  end\n"
	                       "  object FileName: TFileNameEdit\n"
	                       "    AnchorSideLeft.Control = lblText\n"
	                       "    AnchorSideTop.Control = lblText\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 36\n"
	                       "    Top = 37\n"
	                       "    Width = 598\n"
	                       "    Filter = '%s'\n"
	                       "    FilterIndex = 0\n"
	                       "    HideDirectories = False\n"
	                       "    ButtonWidth = 24\n"
	                       "    NumGlyphs = 1\n"
	                       "    BorderSpacing.Top = 20\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 0\n"
	                       "    Text = '%s'\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = FileName\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = FileName\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 511\n"
	                       "    Height = 31\n"
	                       "    Top = 83\n"
	                       "    Width = 97\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    BorderSpacing.Top = 10\n"
	                       "    Constraints.MinHeight = 30\n"
	                       "    Constraints.MinWidth = 97\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkOK\n"
	                       "    ModalResult = 1\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 1\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 404\n"
	                       "    Height = 31\n"
	                       "    Top = 83\n"
	                       "    Width = 97\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    BorderSpacing.Right = 10\n"
	                       "    Cancel = True\n"
	                       "    Constraints.MinHeight = 30\n"
	                       "    Constraints.MinWidth = 97\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkCancel\n"
	                       "    ModalResult = 2\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "end\n";

	gchar **split = g_strsplit(text, "\t", -1);
	guint len = g_strv_length(split);

	if (len > 0)
	{
		gchar *prev = g_key_file_get_string(g_cfg, g_script, split[0], NULL);
		gchar *lfmdata = g_strdup_printf(lfmdata_templ, split[0], split[1] ? split[1] : "*.*|*.*", prev ? prev : "");
		g_free(prev);

		gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), SelectFileDlgProc);
		g_free(lfmdata);
		g_strfreev(split);
	}
}

intptr_t DCPCALL SelectDirDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			gchar *output = NULL;
			char *text = (char*)SendDlgMsg(pDlg, "lblText", DM_GETTEXT, 0, 0);
			char *res = (char*)SendDlgMsg(pDlg, "Directory", DM_GETTEXT, 0, 0);

			if (res && res[0] != '\0')
			{
				g_key_file_set_string(g_cfg, g_script, text, res);
				ExecuteScript(g_script, VERB_SETOPT, text, res, &output);

				if (output && output[0] != '\0')
					ParseOpts(g_script, output);

				g_free(output);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "empty argument selected");
		}

		break;
	}

	return 0;
}
static void SelectDir(char *text)
{
	const char lfmdata_templ[] = ""
	                             "object SelectFileDialogBox: TSelectFileDialogBox\n"
	                             "  Left = 210\n"
	                             "  Height = 138\n"
	                             "  Top = 218\n"
	                             "  Width = 622\n"
	                             "  AutoSize = True\n"
	                             "  BorderStyle = bsDialog\n"
	                             "  Caption = 'Confirmation of parameter'\n"
	                             "  ChildSizing.LeftRightSpacing = 10\n"
	                             "  ChildSizing.TopBottomSpacing = 10\n"
	                             "  ClientHeight = 138\n"
	                             "  ClientWidth = 622\n"
	                             "  DesignTimePPI = 100\n"
	                             "  OnCreate = DialogBoxShow\n"
	                             "  Position = poScreenCenter\n"
	                             "  LCLVersion = '2.2.4.0'\n"
	                             "  object lblText: TLabel\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideTop.Control = Owner\n"
	                             "    Left = 10\n"
	                             "    Height = 1\n"
	                             "    Top = 10\n"
	                             "    Width = 1\n"
	                             "    Caption = '%s'\n"
	                             "    ParentColor = False\n"
	                             "    WordWrap = True\n"
	                             "  end\n"
	                             "  object Directory: TDirectoryEdit\n"
	                             "    AnchorSideLeft.Control = lblText\n"
	                             "    AnchorSideTop.Control = lblText\n"
	                             "    Left = 10\n"
	                             "    Height = 36\n"
	                             "    Top = 20\n"
	                             "    Width = 598\n"
	                             "    ShowHidden = False\n"
	                             "    ButtonWidth = 24\n"
	                             "    NumGlyphs = 1\n"
	                             "    BorderSpacing.Top = 20\n"
	                             "    MaxLength = 0\n"
	                             "    TabOrder = 0\n"
	                             "    Text = '%s'\n"
	                             "  end\n"
	                             "  object btnOK: TBitBtn\n"
	                             "    AnchorSideTop.Control = Directory\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    AnchorSideRight.Control = Directory\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 511\n"
	                             "    Height = 31\n"
	                             "    Top = 66\n"
	                             "    Width = 97\n"
	                             "    Anchors = [akTop, akRight]\n"
	                             "    BorderSpacing.Top = 10\n"
	                             "    Constraints.MinHeight = 30\n"
	                             "    Constraints.MinWidth = 97\n"
	                             "    Default = True\n"
	                             "    DefaultCaption = True\n"
	                             "    Kind = bkOK\n"
	                             "    ModalResult = 1\n"
	                             "    OnClick = ButtonClick\n"
	                             "    TabOrder = 1\n"
	                             "  end\n"
	                             "  object btnCancel: TBitBtn\n"
	                             "    AnchorSideTop.Control = btnOK\n"
	                             "    AnchorSideRight.Control = btnOK\n"
	                             "    Left = 404\n"
	                             "    Height = 31\n"
	                             "    Top = 66\n"
	                             "    Width = 97\n"
	                             "    Anchors = [akTop, akRight]\n"
	                             "    BorderSpacing.Right = 10\n"
	                             "    Cancel = True\n"
	                             "    Constraints.MinHeight = 30\n"
	                             "    Constraints.MinWidth = 97\n"
	                             "    DefaultCaption = True\n"
	                             "    Kind = bkCancel\n"
	                             "    ModalResult = 2\n"
	                             "    OnClick = ButtonClick\n"
	                             "    TabOrder = 2\n"
	                             "  end\n"
	                             "end\n";

	gchar **split = g_strsplit(text, "\t", -1);
	guint len = g_strv_length(split);

	if (len > 0)
	{
		gchar *prev = g_key_file_get_string(g_cfg, g_script, split[0], NULL);
		gchar *lfmdata = g_strdup_printf(lfmdata_templ, split[0], prev ? prev : "");
		g_free(prev);

		gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), SelectDirDlgProc);
		g_free(lfmdata);
		g_strfreev(split);
	}
}

intptr_t DCPCALL MultiChoiceDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam);

static void ParseOpts(gchar *script, gchar *text)
{
	gchar *output = NULL;
	gboolean request_values = FALSE;

	if (text)
	{
		gchar **split = g_strsplit(text, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			if (*p[0] != 0)
			{
				if (g_strcmp0(*p, REQUEST_OPT) == 0)
					request_values = TRUE;
				else if (g_strcmp0(*p, STATUSINFO_OPT) == 0)
					g_key_file_set_boolean(g_cfg, script, STATUSINFO_OPT, TRUE);
				else if (g_strcmp0(*p, CHECKFIELDS_OPT) == 0)
					g_key_file_set_boolean(g_cfg, script, CHECKFIELDS_OPT, TRUE);
				else if (ValidOpt(*p, ENVVAR_OPT))
				{
					g_key_file_set_string(g_cfg, script, ENVVAR_OPT, *p + strlen(ENVVAR_OPT) + 1);

					if (g_noise)
					{
						gchar *message = g_strdup_printf("%s value will be set to \"%s\"", ENVVAR_NAME, *p + strlen(ENVVAR_OPT) + 1);
						LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
						g_free(message);
					}
				}
				else if (ValidOpt(*p, INFORM_OPT) && gRequestProc)
					gRequestProc(gPluginNr, RT_MsgOK, script, *p + strlen(INFORM_OPT) + 1, NULL, 0);
				else if (ValidOpt(*p, YESNOMSG_OPT) && gRequestProc)
				{
					gboolean is_yes = gRequestProc(gPluginNr, RT_MsgYesNo, script, *p + strlen(YESNOMSG_OPT) + 1, NULL, 0);

					SetOpt(script, *p + strlen(YESNOMSG_OPT) + 1, is_yes ? "Yes" : "No");
				}
				else if (ValidOpt(*p, PUSH_OPT))
				{
					gchar **res = g_strsplit(*p + strlen(PUSH_OPT) + 1, "\t", 2);
					SetOpt(script, res[0], res[1]);
					g_strfreev(res);
				}
				else if (ValidOpt(*p, CHOICE_OPT))
				{
					if (gDialogApi)
					{
						g_strlcpy(g_script, script, PATH_MAX);
						g_choice = g_strdup(*p + strlen(CHOICE_OPT) + 1);
						gDialogApi->DialogBoxLFM((intptr_t)g_multichoice_lfm, (unsigned long)strlen(g_multichoice_lfm), MultiChoiceDlgProc);
						g_free(g_choice);
						g_choice = NULL;
					}
					else
						LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "DialogApi not initialized");
				}
				else if (ValidOpt(*p, SELFILE_OPT))
				{
					g_strlcpy(g_script, script, PATH_MAX);
					SelectFile(*p + strlen(SELFILE_OPT) + 1);
				}
				else if (ValidOpt(*p, SELDIR_OPT))
				{
					g_strlcpy(g_script, script, PATH_MAX);
					SelectDir(*p + strlen(SELDIR_OPT) + 1);
				}
				else if (strncmp(*p, NOISE_OPT, 3) == 0)
					LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Options starting with \"Fs_\" are reserved, ignored");
				else if (request_values == TRUE)
				{
					char value[MAX_PATH] = "";

					int type = (strncasecmp("password", *p, 8) == 0) ? RT_Password : RT_Other;

					if (type == RT_Password)
					{
						if (!gCryptProc)
							LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc not initialized");
						else
						{
							int ret = gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_LOAD_PASSWORD_NO_UI, script, value, MAX_PATH);

							if (ret == FS_FILE_NOTFOUND)
								ret = gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_LOAD_PASSWORD, script, value, MAX_PATH);

							LogCryptProc(ret);
						}
					}
					else
					{

						gchar *prev = g_key_file_get_string(g_cfg, script, *p, NULL);

						if (prev)
						{
							g_strlcpy(value, prev, MAX_PATH);
							g_free(prev);
						}
					}

					if (gRequestProc && gRequestProc(gPluginNr, type, script, (type == RT_Password) ? NULL : *p, value, MAX_PATH))
					{
						ExecuteScript(script, VERB_SETOPT, *p, value, &output);

						if (type == RT_Password)
						{
							if (!gCryptProc)
								LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc not initialized");
							else
								LogCryptProc(gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_SAVE_PASSWORD, script, value, MAX_PATH));
						}
						else
							g_key_file_set_string(g_cfg, script, *p, value);

						if (output && output[0] != '\0')
							ParseOpts(script, output);

						g_free(output);
						output = NULL;
					}
				}
				else
				{
					gchar *message = g_strdup_printf("%s directive not received, Line \"%s\" will be omitted.", REQUEST_OPT, *p);
					LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, message);
					g_free(message);
				}
			}
		}

		g_strfreev(split);
	}
}

static void DeInitializeScript(gchar *script)
{
	if (g_key_file_get_boolean(g_cfg, script, IN_USE_MARK, NULL))
	{
		if (!ExecuteScript(script, VERB_DEINIT, NULL, NULL, NULL) && g_noise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "Deinitialization not implemented or not completed successfully");

		g_key_file_remove_key(g_cfg, script, IN_USE_MARK, NULL);
		g_key_file_remove_key(g_cfg, script, ENVVAR_OPT, NULL);
#ifndef  TEMP_PANEL
		gchar *message = g_strdup_printf("DISCONNECT /%s", script);

		if (gLogProc)
			gLogProc(gPluginNr, MSGTYPE_DISCONNECT, message);

		g_free(message);
#endif
	}
}

static void InitializeScript(gchar *script)
{
	gchar *output = NULL;

	DeInitializeScript(script);
	g_noise = g_key_file_get_boolean(g_cfg, script, NOISE_OPT, NULL);
#ifndef TEMP_PANEL
	gchar *message = g_strdup_printf("CONNECT /%s", script);
	LogMessage(gPluginNr, MSGTYPE_CONNECT, message);
	g_free(message);
#endif

	ExecuteScript(script, VERB_INIT, NULL, NULL, &output);
	g_key_file_set_boolean(g_cfg, script, IN_USE_MARK, TRUE);
	ParseOpts(script, output);
	g_free(output);
}

intptr_t DCPCALL MultiChoiceDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		if (g_choice)
		{
			gsize count = 0;
			gchar **split = g_strsplit(g_choice, "\t", -1);
			SendDlgMsg(pDlg, "lblText", DM_SETTEXT, (intptr_t)split[0], 0);
			gchar **items = split + 1;

			if (items)
			{
				for (gchar **p = items; *p != NULL; p++)
				{
					if (*p[0] != '\0')
					{
						SendDlgMsg(pDlg, "cbChoice", DM_LISTADD, (intptr_t)*p, 0);
						count++;
					}
				}

				if (g_noise && count == 1)
					LogMessage(gPluginNr, MSGTYPE_DETAILS, "Fs_MultiChoice: there is no choice");
				else if (count < 1)
					LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Fs_MultiChoice: there is no choice");

				gint index = g_key_file_get_integer(g_cfg, g_script, split[0], NULL);
				SendDlgMsg(pDlg, "cbChoice", DM_LISTSETITEMINDEX, (index < 0 || index >= count) ? 0 : (intptr_t)index, 0);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Fs_MultiChoice: there is no choice");

			g_strfreev(split);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			gchar *output = NULL;
			int index = (int)SendDlgMsg(pDlg, "cbChoice", DM_LISTGETITEMINDEX, 0, 0);
			char *text = (char*)SendDlgMsg(pDlg, "lblText", DM_GETTEXT, 0, 0);
			g_key_file_set_integer(g_cfg, g_script, text, index);
			char *res = (char*)SendDlgMsg(pDlg, "cbChoice", DM_GETTEXT, 0, 0);

			if (res && res[0] != '\0')
			{
				ExecuteScript(g_script, VERB_SETOPT, text, res, &output);

				if (output && output[0] != '\0')
					ParseOpts(g_script, output);

				g_free(output);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "empty argument selected");
		}

		break;
	}

	return 0;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		g_noise = g_key_file_get_boolean(g_cfg, g_script, NOISE_OPT, NULL);
		SendDlgMsg(pDlg, "ckNoise", DM_SETCHECK, (intptr_t)g_noise, 0);
		SendDlgMsg(pDlg, "lScriptName", DM_SETTEXT, (intptr_t)g_script, 0);
		LoadPreview(pDlg, g_script);

		if (!g_key_file_get_boolean(g_cfg, g_script, IN_USE_MARK, NULL))
			SendDlgMsg(pDlg, "btnUnmount", DM_SHOWITEM, 0, 0);

		if (g_caller && g_props)
		{
			gchar *content_type = g_content_type_guess(g_caller, NULL, 0, NULL);
			gchar *descr = g_content_type_get_description(content_type);
			gchar *filename = g_path_get_basename(g_caller);
			SendDlgMsg(pDlg, "lFileName", DM_SETTEXT, (intptr_t)filename, 0);
			SendDlgMsg(pDlg, "lType", DM_SETTEXT, (intptr_t)descr, 0);
			g_free(filename);
			g_free(content_type);
			g_free(descr);
			SendDlgMsg(pDlg, "pProperties", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lFileName", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lTypeLabel", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lType", DM_SHOWITEM, 1, 0);
			FillProps(pDlg);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "lURL") == 0)
		{
			char *url = (char*)SendDlgMsg(pDlg, DlgItemName, DM_GETTEXT, 0, 0);
			gchar *command = g_strdup_printf("xdg-open %s", url);
			system(command);
			g_free(command);
		}
		else if (strcmp(DlgItemName, "btnUnmount") == 0)
		{
			DeInitializeScript(g_script);
		}
		else if (strcmp(DlgItemName, "btnAct") == 0)
		{
			char *cmd = (char*)SendDlgMsg(pDlg, "cbAct", DM_GETTEXT, 0, 0);
			SetOpt(g_script, cmd, g_caller);
			SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "ckNoise") == 0)
		{
			g_noise = (gboolean)SendDlgMsg(pDlg, DlgItemName, DM_GETCHECK, 0, 0);
			g_key_file_set_boolean(g_cfg, g_script, NOISE_OPT, g_noise);
		}
		else if (strcmp(DlgItemName, "cbAct") == 0)
		{
			if (SendDlgMsg(pDlg, "cbAct", DM_LISTGETITEMINDEX, 0, 0) == -1)
				SendDlgMsg(pDlg, "btnAct", DM_ENABLE, 0, 0);
			else
				SendDlgMsg(pDlg, "btnAct", DM_ENABLE, 1, 0);
		}

		break;
	}

	return 0;
}


static void ShowPropertiesDlg(void)
{
	if (gDialogApi && g_file_test(g_lfm_path, G_FILE_TEST_EXISTS))
		gDialogApi->DialogBoxLFMFile(g_lfm_path, DlgProc);
	else  if (gRequestProc && g_props)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, g_props, NULL, 0);
}


static gchar *ExtractScriptFromPath(char *path)
{
	char *result = NULL;

	gchar **split = g_strsplit(path, "/", -1);

	if (split)
	{
		result = g_strdup(split[1]);
		g_strfreev(split);
	}

	return result;
}

static gboolean InvalidPath(char *path)
{
	return (g_strcmp0(path, "/.") == 0 || g_strcmp0(path, "/..") == 0 || strncmp(path, "/../", 4) == 0);
}

static gboolean RootDir(char *path)
{
	*path++;

	while (*path)
	{
		if (*path == '/')
			return FALSE;

		*path++;
	}

	return TRUE;
}

static gchar *StripScriptFromPath(char *path)
{
	gchar *result = NULL;

	gchar **split = g_strsplit(path, "/", -1);

	if (split)
	{
		gchar **target = split;
		target++;
		target++;
		gchar *temp = g_strjoinv("/", target);
		g_strfreev(split);
		result = g_strdup_printf("/%s", temp);
		g_free(temp);
	}

	return result;
}

static gboolean SetScriptsFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	struct stat buf;
	struct dirent *ent;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while ((ent = readdir(dirdata->dir)) != NULL)
	{
		if (ent->d_type == DT_REG)
		{
			gchar *src_file = g_strdup_printf("%s/%s", g_scripts_dir, ent->d_name);

			if (g_file_test(src_file, G_FILE_TEST_IS_EXECUTABLE))
			{
				g_strlcpy(FindData->cFileName, ent->d_name, sizeof(FindData->cFileName));

				FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
				FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;

				if (stat(src_file, &buf) == 0)
				{
					UnixTimeToFileTime((time_t)buf.st_mtime, &FindData->ftLastWriteTime);
					UnixTimeToFileTime((time_t)buf.st_atime, &FindData->ftLastAccessTime);
					FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
					FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
				}
				else
				{
					SetCurrentFileTime(&FindData->ftLastWriteTime);
					FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
					FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
				}

				FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;
				FindData->dwReserved0 = S_IFLNK | S_IREAD | S_IEXEC;

				return TRUE;
			}
		}
	}

	return FALSE;
}

static gboolean SetFindData(tVFSDirData * dirdata, WIN32_FIND_DATAA * FindData)
{
	gchar *message = NULL, *string = NULL;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while (g_match_info_matches(dirdata->match_info))
	{
		string = g_match_info_fetch(dirdata->match_info, 0);

		if (g_noise)
		{
			message = g_strdup_printf("Found: %s", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 1);

		if (g_noise)
		{
			message = g_strdup_printf("mode: %s", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		int i = 1;
		gboolean octalmode = FALSE;
		mode_t mode = S_IFREG;
		FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;

		if (string[0] != '-')
		{
			if (string[0] == 'd')
				mode = S_IFDIR;
			else if (string[0] == 'b')
				mode = S_IFBLK;
			else if (string[0] == 'c')
				mode = S_IFCHR;
			else if (string[0] == 'f')
				mode = S_IFIFO;
			else if (string[0] == 'l')
				mode = S_IFLNK;
			else if (string[0] == 's')
				mode = S_IFSOCK;
			else
			{
				mode = strtol(string + 1, NULL, 8);
				octalmode = TRUE;
			}
		}

		if (!octalmode)
		{
			if (string[i++] == 'r')
				mode |= S_IRUSR;

			if (string[i++] == 'w')
				mode |= S_IWUSR;

			if (string[i] == 'x')
				mode |= S_IXUSR;
			else if (string[i] == 'S')
				mode |= S_ISUID;
			else if (string[i] == 's')
				mode |= S_ISUID | S_IXUSR;

			i++;

			if (string[i++] == 'r')
				mode |= S_IRGRP;

			if (string[i++] == 'w')
				mode |= S_IWGRP;

			if (string[i] == 'x')
				mode |= S_IXGRP;
			else if (string[i] == 'S')
				mode |= S_ISGID;
			else if (string[i] == 's')
				mode |= S_ISGID | S_IXGRP;

			i++;

			if (string[i++] == 'r')
				mode |= S_IROTH;

			if (string[i++] == 'w')
				mode |= S_IWOTH;

			if (string[i] == 'x')
				mode |= S_IXOTH;
			else if (string[i] == 'T')
				mode |= S_ISVTX;
			else if (string[i] == 't')
				mode |= S_ISVTX | S_IXOTH;
		}

		FindData->dwReserved0 = mode;
		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 2);

		if (g_noise)
		{
			message = g_strdup_printf("date: %s", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		gint64 filetime = 0;
		GDateTime *dt = g_date_time_new_from_iso8601(string, NULL);

		if (dt)
		{
			filetime = g_date_time_to_unix(dt);
			g_date_time_unref(dt);
		}
		else if (g_noise)
		{
			message = g_strdup_printf("\"%s\" is not a valid ISO 8601 UTC formatted string", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		if (filetime == 0)
		{
			struct tm tm = {0};

			gchar *formats[] =
			{
				"%Y-%m-%d %R",
				"%Y-%m-%dT%R",
				"%Y-%m-%dt%R",
				"%Y%m%d %R",
				NULL
			};

			for (char **p = formats; *p != NULL; p++)
			{
				if (strptime(string, *p, &tm))
				{
					filetime = (gint64)mktime(&tm);

					if (g_noise)
					{
						message = g_strdup_printf("Used %s to parse the date", *p);
						LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
						g_free(message);
					}

					break;
				}
			}
		}

		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;

		if (filetime > 0)
			UnixTimeToFileTime((time_t)filetime, &FindData->ftLastWriteTime);
		else
		{

			SetCurrentFileTime(&FindData->ftLastWriteTime);

			if (g_noise)
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "The current datetime has been set");

			//			FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
			//			FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 3);

		if (g_noise)
		{
			message = g_strdup_printf("size: %s", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		gdouble filesize = g_ascii_strtod(string, NULL);

		if (filesize < 0)
		{
			FindData->nFileSizeHigh = 0xFFFFFFFF;
			FindData->nFileSizeLow = 0xFFFFFFFE;
		}
		else
		{
			FindData->nFileSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = (int64_t)filesize & 0x00000000FFFFFFFF;
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 4);

		if (g_noise)
		{
			message = g_strdup_printf("name: %s", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		g_strlcpy(FindData->cFileName, string, sizeof(FindData->cFileName));
		g_free(string);

		g_match_info_next(dirdata->match_info, NULL);
		return TRUE;
	}

	return FALSE;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA * FindData)
{
	tVFSDirData *dirdata;

	if (Path[1] == '\0')
	{
		DIR *dir = opendir(g_scripts_dir);

		if (dir == NULL)
			return (HANDLE)(-1);

		dirdata = g_new0(tVFSDirData, 1);

		if (!dirdata)
		{
			closedir(dir);
			return (HANDLE)(-1);
		}

		dirdata->dir = dir;

		if (!SetScriptsFindData(dirdata, FindData))
		{
			closedir(dir);
			return (HANDLE)(-1);
		}
	}
	else
	{
		if (!g_key_file_get_boolean(g_cfg, g_script, IN_USE_MARK, NULL))
			return (HANDLE)(-1);

		gchar *output = NULL;
		gchar *script = ExtractScriptFromPath(Path);
		gchar *list_path = StripScriptFromPath(Path);
		size_t len = strlen(list_path);

		if (len > 1 && list_path[len - 1] == '/')
			list_path[len - 1] = '\0';

		if (!ExecuteScript(script, VERB_LIST, list_path, NULL, &output))
		{
			g_free(list_path);
			g_free(script);
			g_free(output);
			return (HANDLE)(-1);
		}

		g_free(list_path);
		g_free(script);

		dirdata = g_new0(tVFSDirData, 1);

		if (!dirdata)
		{
			g_free(output);
			return (HANDLE)(-1);
		}

		dirdata->regex = g_regex_new(LIST_REGEXP, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, NULL);

		if (!g_regex_match(dirdata->regex, output, 0, &(dirdata->match_info)))
		{
			if (g_noise)
			{
				gchar *message = g_strdup_printf("%s: no file list received", Path);
				LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
				g_free(message);
			}

			g_regex_unref(dirdata->regex);
			g_free(output);
			return (HANDLE)(-1);
		}

		dirdata->output = output;

		if (!SetFindData(dirdata, FindData))
		{
			g_match_info_free(dirdata->match_info);
			g_regex_unref(dirdata->regex);
			g_free(output);
			g_free(dirdata);
			return (HANDLE)(-1);
		}
	}

	return dirdata;
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA * FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->dir)
		return SetScriptsFindData(dirdata, FindData);
	else
		return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->dir)
		closedir(dirdata->dir);

	if (dirdata->match_info)
		g_match_info_free(dirdata->match_info);

	if (dirdata->regex)
		g_regex_unref(dirdata->regex);

	g_free(dirdata->output);
	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct * ri)
{
	struct stat buf;
	int result = FS_FILE_OK;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	if (RootDir(RemoteName))
	{
		char path[PATH_MAX];
		g_strlcpy(path, g_scripts_dir, PATH_MAX);
		strcat(path, RemoteName);

		if (symlink(path, LocalName) != 0)
			return FS_FILE_WRITEERROR;
	}
	else
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *path = StripScriptFromPath(RemoteName);

		if (InvalidPath(path) || !ExecuteScript(script, VERB_GET_FILE, path, LocalName, NULL))
			result = FS_FILE_NOTSUPPORTED;

#ifndef TEMP_PANEL

		if (result == FS_FILE_OK)
		{
			gchar *message = g_strdup_printf("%s -> %s", path, LocalName);
			LogMessage(gPluginNr, MSGTYPE_TRANSFERCOMPLETE, message);
			g_free(message);
		}

#endif
		g_free(script);
		g_free(path);
	}

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	if (result == FS_FILE_OK && lstat(LocalName, &buf) != 0)
		return FS_FILE_WRITEERROR;

	return result;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	if (gProgressProc(gPluginNr, LocalName, RemoteName, 0))
		return FS_FILE_USERABORT;

	if (RootDir(RemoteName))
		return FS_FILE_NOTSUPPORTED;

	int result = FS_FILE_OK;
	gchar *path = StripScriptFromPath(RemoteName);

	if (InvalidPath(path))
	{
		g_free(path);
		return FS_FILE_NOTSUPPORTED;
	}

	gchar *script = ExtractScriptFromPath(RemoteName);

	if (CopyFlags == 0 && ExecuteScript(script, VERB_EXISTS, path, NULL, NULL))
		result = FS_FILE_EXISTS;
	else
	{
		if (!ExecuteScript(script, VERB_PUT_FILE, LocalName, path, NULL))
			result = FS_FILE_NOTSUPPORTED;

#ifndef TEMP_PANEL

		if (result == FS_FILE_OK)
		{
			gchar *message = g_strdup_printf("%s -> %s", path, LocalName);
			LogMessage(gPluginNr, MSGTYPE_TRANSFERCOMPLETE, message);
			g_free(message);
		}

#endif
		gProgressProc(gPluginNr, RemoteName, path, 100);
	}

	g_free(script);
	g_free(path);

	return result;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct * ri)
{
	// iwanttobelive
	gboolean wtf_overwrite = (gboolean)abs((int)OverWrite % 2);

	if (gProgressProc(gPluginNr, OldName, NewName, 0))
		return FS_FILE_USERABORT;

	if (RootDir(NewName))
		return FS_FILE_NOTSUPPORTED;

	gchar *newpath = StripScriptFromPath(NewName);

	if (InvalidPath(newpath))
	{
		g_free(newpath);
		return FS_FILE_NOTSUPPORTED;
	}

	int result = FS_FILE_OK;
	gchar *script = ExtractScriptFromPath(OldName);
	gchar *oldpath = StripScriptFromPath(OldName);

	if (!wtf_overwrite && ExecuteScript(script, VERB_EXISTS, newpath, NULL, NULL))
		return FS_FILE_EXISTS;

	if (!ExecuteScript(script, Move ? VERB_MOVE : VERB_COPY, oldpath, newpath, NULL))
		return FS_FILE_NOTSUPPORTED;

	gProgressProc(gPluginNr, oldpath, newpath, 100);

	g_free(script);
	g_free(oldpath);
	g_free(newpath);

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	gboolean result = FALSE;

	if (!RootDir(Path))
	{
		gchar *newpath = StripScriptFromPath(Path);

		if (InvalidPath(newpath))
		{
			g_free(newpath);
			return FS_FILE_NOTSUPPORTED;
		}

		gchar *output = NULL;
		gchar *script = ExtractScriptFromPath(Path);
		result = ExecuteScript(script, VERB_MKDIR, newpath, NULL, &output);

		if (output && output[0] != '\0')
			ParseOpts(script, output);

		g_free(output);
		g_free(script);
		g_free(newpath);
	}

	return result;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gboolean result = FALSE;

	if (!RootDir(RemoteName))
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *newpath = StripScriptFromPath(RemoteName);
		result = ExecuteScript(script, VERB_REMOVE, newpath, NULL, NULL);
		g_free(script);
		g_free(newpath);
	}

	return result;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	gboolean result = FALSE;

	if (!RootDir(RemoteName))
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *newpath = StripScriptFromPath(RemoteName);
		result = ExecuteScript(script, VERB_RMDIR, newpath, NULL, NULL);
		g_free(script);
		g_free(newpath);
	}

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	gchar *output = NULL;
	gchar *script = ExtractScriptFromPath(RemoteName);
	gchar *path = StripScriptFromPath(RemoteName);
	int result = FS_EXEC_ERROR;

	if (strcmp(Verb, "open") == 0)
	{
		if (RootDir(RemoteName))
		{
			InitializeScript(script);
			g_strlcpy(g_script, script, PATH_MAX);
			result = FS_EXEC_SYMLINK;
		}
		else
		{
			if (!ExecuteScript(script, VERB_EXEC, path, NULL, &output))
				result = FS_EXEC_YOURSELF;
			else if (output && output[0] != '\0')
			{
				if (strncmp(output, NOISE_OPT, 3) == 0)
				{
					ParseOpts(script, output);
					result = FS_EXEC_OK;
				}
				else
				{
					size_t len = strlen(output);

					if (len > 0)
						output[len - 1] = '\0';

					g_strlcpy(RemoteName, output, MAX_PATH - 1);
					result = FS_EXEC_SYMLINK;
				}
			}
			else
				result = FS_EXEC_OK;
		}
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		if (RemoteName[1] != '\0' && g_strcmp0(RemoteName, "/..") != 0)
		{
			g_strlcpy(g_script, script, PATH_MAX);

			if (!RootDir(RemoteName) && !InvalidPath(path))
			{
				gboolean is_ok = ExecuteScript(script, VERB_PROPS, path, NULL, &g_props);

				if (!is_ok || (g_props && g_props[0] != '\0'))
				{

					if (strncmp(g_props, NOISE_OPT, 3) == 0)
						ParseOpts(script, g_props);
					else
					{
						g_caller = path;
						ShowPropertiesDlg();
						g_caller = NULL;
					}
				}

				g_free(g_props);
				g_props = NULL;

			}
			else
				ShowPropertiesDlg();

			result = FS_EXEC_OK;
		}

	}
	else if (!RootDir(RemoteName) && !InvalidPath(path))
	{
		if (strncmp(Verb, "chmod", 5) == 0)
		{
			if (ExecuteScript(script, VERB_CHMOD, path, Verb + 6, NULL))
				result = FS_EXEC_OK;
		}
		else if (strncmp(Verb, "quote", 5) == 0)
		{
			ExecuteScript(script, VERB_QUOTE, Verb + 6, path, NULL);
			result = FS_EXEC_OK;
		}
	}

	g_free(output);
	g_free(script);
	g_free(path);

	return result;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME * CreationTime, FILETIME * LastAccessTime, FILETIME * LastWriteTime)
{
	gint64 time = -1;
	gchar *date = NULL;
	gboolean result = FALSE;

	gchar *newpath = StripScriptFromPath(RemoteName);

	if (LastWriteTime && !RootDir(RemoteName) && !InvalidPath(newpath))
	{
		time = (gint64)FileTimeToUnixTime(LastWriteTime);

		if (time > 0)
		{
			GDateTime *dt = g_date_time_new_from_unix_local(time);
			GDateTime *utc_dt = g_date_time_to_utc(dt);

			if (utc_dt)
				date = g_date_time_format_iso8601(utc_dt);

			g_date_time_unref(utc_dt);
			g_date_time_unref(dt);

			if (date)
			{
				gchar *script = ExtractScriptFromPath(RemoteName);

				result = ExecuteScript(script, VERB_MODTIME, newpath, date, NULL);
				g_free(date);
				g_free(script);
			}
		}
	}

	g_free(newpath);

	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
#ifdef  TEMP_PANEL
	g_strlcpy(DefRootName, "Scripts (Temporary Panels)", maxlen - 1);
}
#else
	g_strlcpy(DefRootName, "Scripts", maxlen - 1);
}

BOOL DCPCALL FsDisconnect(char* DisconnectRoot)
{
	DeInitializeScript(DisconnectRoot + 1);
	return TRUE;
}
#endif
#ifdef  FIELDS_API
int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		DIR *dir;
		struct dirent *ent;
		gchar *fields = NULL;
		gchar *output = NULL;

		g_strfreev(g_fields);
		g_fields = NULL;

		GString *buffer = g_string_new(NULL);

		if ((dir = opendir(g_scripts_dir)) != NULL)
		{
			while ((ent = readdir(dir)) != NULL)
			{
				if (ent->d_type == DT_REG)
				{
					gchar *src_file = g_strdup_printf("%s/%s", g_scripts_dir, ent->d_name);

					if (g_file_test(src_file, G_FILE_TEST_IS_EXECUTABLE) && g_key_file_get_boolean(g_cfg, ent->d_name, CHECKFIELDS_OPT, NULL))
					{
						ExecuteScript(ent->d_name, VERB_FIELDS, NULL, NULL, &output);

						if (output)
						{
							buffer = g_string_append(buffer, output);
							g_free(output);
						}

					}

					g_free(src_file);
				}
			}

			closedir(dir);
		}

		fields = g_string_free(buffer, FALSE);

		if (fields)
		{
			g_fields = g_strsplit(fields, "\n", -1);
			g_free(fields);
		}
	}

	if (!g_fields || !g_fields[FieldIndex] || g_fields[FieldIndex][0] == '\0')
		return ft_nomorefields;

	Units[0] = '\0';
	g_print(g_fields[FieldIndex]);
	g_strlcpy(FieldName, g_fields[FieldIndex], maxlen - 1);
	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = ft_fieldempty;
	gchar *output = NULL;
	gchar *script = ExtractScriptFromPath(FileName);
	gchar *path = StripScriptFromPath(FileName);

	ExecuteScript(script, VERB_GETVALUE, g_fields[FieldIndex], path, &output);

	if (output)
	{
		size_t len = strlen(output);

		if (len > 0)
			output[len - 1] = '\0';

		g_strlcpy((char*)FieldValue, output, maxlen - 1);

		result = ft_string;
	}

	g_free(output);
	g_free(script);
	g_free(path);

	return result;
}
#endif
BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	return FALSE;
}

#ifdef  TEMP_PANEL
BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gboolean result = FALSE;

	if (!RootDir(RemoteName))
	{
		gchar *output = NULL;
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *newpath = StripScriptFromPath(RemoteName);
		ExecuteScript(g_script, VERB_REALNAME, newpath, NULL, &output);
		g_free(script);
		g_free(newpath);

		if (output && output[0] != 0)
		{
			size_t len = strlen(output);

			if (len > 0)
				output[len - 1] = '\0';

			g_strlcpy(RemoteName, output, maxlen - 1);

			result = TRUE;
		}

		g_free(output);
	}
	else
	{
		char path[maxlen];
		g_strlcpy(path, g_scripts_dir, maxlen);
		strcat(path, RemoteName);
		g_strlcpy(RemoteName, path, maxlen - 1);
		result = TRUE;
	}

	return result;
}
#endif

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{

	if (RootDir(RemoteDir))
		return;

	gchar *script = ExtractScriptFromPath(RemoteDir);

	g_noise = g_key_file_get_boolean(g_cfg, script, NOISE_OPT, NULL);

	if (!g_key_file_get_boolean(g_cfg, script, STATUSINFO_OPT, NULL))
	{
		g_free(script);
		return;
	}

	gchar *output = NULL;
	gchar *path = StripScriptFromPath(RemoteDir);

	switch (InfoOperation)
	{
	case FS_STATUS_OP_LIST:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "list start" : "list end", path, &output);
		break;

	case FS_STATUS_OP_GET_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_single start" : "get_single end", path, &output);
		break;

	case FS_STATUS_OP_GET_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_multi start" : "get_multi end", path, &output);
		break;

	case FS_STATUS_OP_PUT_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_single start" : "put_single end", path, &output);
		break;

	case FS_STATUS_OP_PUT_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_multi start" : "put_multi end", path, &output);
		break;

	case FS_STATUS_OP_GET_MULTI_THREAD:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_multi_thread start" : "get_multi_thread end", path, &output);
		break;

	case FS_STATUS_OP_PUT_MULTI_THREAD:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_multi_thread start" : "put_multi_thread end", path, &output);
		break;

	case FS_STATUS_OP_RENMOV_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "renmov_single start" : "renmov_single end", path, &output);
		break;

	case FS_STATUS_OP_RENMOV_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "renmov_multi start" : "renmov_multi end", path, &output);
		break;

	case FS_STATUS_OP_DELETE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "delete start" : "delete end", path, &output);
		break;

	case FS_STATUS_OP_ATTRIB:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "attrib start" : "attrib end", path, &output);
		break;

	case FS_STATUS_OP_MKDIR:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "mkdir start" : "mkdir end", path, &output);
		break;

	case FS_STATUS_OP_EXEC:
		if (InfoStartEnd == FS_STATUS_START)
			g_strlcpy(g_exec_start, script, PATH_MAX);
		else if (g_strcmp0(g_exec_start, g_script) != 0)
			return;

		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "exec start" : "exec end", path, &output);
		break;

	case FS_STATUS_OP_CALCSIZE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "calcsize start" : "calcsize end", path, &output);
		break;

	case FS_STATUS_OP_SEARCH:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "search start" : "search end", path, &output);
		break;

	case FS_STATUS_OP_SEARCH_TEXT:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "search_text start" : "search_text end", path, &output);
		break;

	case FS_STATUS_OP_SYNC_SEARCH:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_search start" : "sync_search end", path, &output);
		break;

	case FS_STATUS_OP_SYNC_GET:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_get start" : "sync_get end", path, &output);
		break;

	case FS_STATUS_OP_SYNC_PUT:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_put start" : "sync_put end", path, &output);
		break;

	case FS_STATUS_OP_SYNC_DELETE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_delete start" : "sync_delete end", path, &output);
		break;
	}

	if (output && output[0] != '\0')
		ParseOpts(script, output);

	g_free(output);
	g_free(script);
	g_free(path);
}

void DCPCALL FsSetCryptCallback(tCryptProc pCryptProc, int CryptoNr, int Flags)
{
	gCryptoNr = CryptoNr;
	gCryptProc = pCryptProc;
}

int DCPCALL FsGetBackgroundFlags(void)
{
	return BG_DOWNLOAD | BG_UPLOAD;
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	g_strlcpy(g_history_file, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(g_history_file, '/');

	if (pos)
		strcpy(pos + 1, CFG_NAME);

	if (g_cfg == NULL)
	{
		g_cfg = g_key_file_new();
		g_key_file_load_from_file(g_cfg, g_history_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

		gchar **groups = g_key_file_get_groups(g_cfg, NULL);

		for (char **script = groups; *script != NULL; script++)
		{
			if (g_key_file_get_boolean(g_cfg, *script, IN_USE_MARK, NULL))
				g_key_file_set_boolean(g_cfg, *script, IN_USE_MARK, FALSE);
		}
	}
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
		g_strlcpy(g_scripts_dir, gDialogApi->PluginDir, PATH_MAX);
		strcat(g_scripts_dir, EXEC_DIR);
		g_strlcpy(g_lfm_path, gDialogApi->PluginDir, PATH_MAX);
		strcat(g_lfm_path, "dialog.lfm");
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	gLogProc = NULL;
	gchar **groups = g_key_file_get_groups(g_cfg, NULL);

	for (char **script = groups; *script != NULL; script++)
	{
		if (g_key_file_get_boolean(g_cfg, *script, IN_USE_MARK, NULL))
			DeInitializeScript(*script);
	}

	g_strfreev(groups);

	g_key_file_save_to_file(g_cfg, g_history_file, NULL);

	g_strfreev(g_fields);

	if (g_caller != NULL)
		g_free(g_caller);

	if (g_props != NULL)
		g_free(g_props);

	if (g_cfg != NULL)
		g_key_file_free(g_cfg);

	if (gDialogApi != NULL)
		free(gDialogApi);

	gDialogApi = NULL;
}
