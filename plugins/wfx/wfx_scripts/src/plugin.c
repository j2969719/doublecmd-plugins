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

#define MAX_SCRIPT_LINES 10

#ifdef  TEMP_PANEL
#define EXEC_DIR "scripts/temppanel"
#define CFG_NAME "wfx_scripts_temppanel"
#define ENVVAR_NAME "DC_WFX_TP_SCRIPT_DATA"
#else
#define EXEC_DIR "scripts"
#define CFG_NAME "wfx_scripts"
#define ENVVAR_NAME "DC_WFX_SCRIPT_DATA"
#endif

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define MessageBox gDialogApi->MessageBox
#define SendDlgMsg gDialogApi->SendDlgMsg

#define REGEXP_LIST "([0-9cbdflrstwxST\\-]+)\\s+(\\d{4}\\-?\\d{2}\\-?\\d{2}[\\stT]\\d{2}:?\\d{2}:?\\d?\\d?Z?)\\s+([0-9\\-]+)\\s+([^\\n]+)"
#define REGEXP_STRING "WFX_SCRIPT_STR_[A-Z0-9_]+"

#define STRIP_OPT(S, O) S + strlen(O) + 1


#define OPT_STATUSINFO "Fs_StatusInfo_Needed"
#define OPT_GETVALUE "Fs_GetValue_Needed"

#define OPT_REQUEST "Fs_Request_Options"
#define OPT_ENVVAR "Fs_Set_" ENVVAR_NAME
#define OPT_YESNOMSG "Fs_YesNo_Message"
#define OPT_INFORM "Fs_Info_Message"
#define OPT_CHOICE "Fs_MultiChoice"
#define OPT_SELFILE "Fs_SelectFile"
#define OPT_SELDIR "Fs_SelectDir"
#define OPT_LOGINFO "Fs_LogInfo"
#define OPT_ACTS "Fs_PropsActs"
#define OPT_PUSH "Fs_PushValue"
#define OPT_ASK "Fs_PushYesNo"


#define MARK_NOISE "Fs_DebugMode"
#define MARK_INUSE "Fs_InUse"
#define MARK_BOOL "Fs_Bool"
#define MARK_PREV "Fs_Old"


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
#define VERB_GETVALUE "getvalue"
#define VERB_RESET    "reset"

#define FIELD_EXTRA "customfield"

typedef struct sVFSDirData
{
	DIR *dir;
	gchar *output;
	GRegex *regex;
	char script[PATH_MAX];
	GMatchInfo *match_info;
} tVFSDirData;

typedef struct sProgressData
{
	gchar *arg1;
	gchar *arg2;
	GPid  pid;
	int *progress;
} tProgressData;

int gPluginNr;
int gCryptoNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;
tCryptProc gCryptProc = NULL;

static gchar *gProps = NULL;
static gchar *gCaller = NULL;
static gchar **gChoice = NULL;
static GKeyFile *gCfg = NULL;
static gboolean gNoise = FALSE;
static char gScript[PATH_MAX];
static char gExecStart[PATH_MAX];
static char gScriptDir[PATH_MAX];
static char gLFMPath[PATH_MAX];
static char gHistoryFile[PATH_MAX];
static char gLang[10];


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


static gboolean CheckCancel(gpointer userdata)
{
	tProgressData *data = (tProgressData*)userdata;

	if (gProgressProc(gPluginNr, data->arg1, data->arg2, *data->progress))
	{
		kill(data->pid, SIGTERM);
		return FALSE;
	}

	return TRUE;
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

static GKeyFile *OpenTranslations(char *Script)
{
	GKeyFile *result = g_key_file_new();
	gchar *lang_file = g_strdup_printf("%s/%s.langs", gScriptDir, Script);

	if (!g_key_file_load_from_file(result, lang_file, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		g_key_file_free(result);
		result = NULL;
	}

	g_free(lang_file);

	return result;
}

static void CloseTranslations(GKeyFile *KeyFile)
{
	if (KeyFile)
		g_key_file_free(KeyFile);
}

static gchar *ReplaceString(gchar *Text, gchar *Old, gchar *New)
{
	gchar *result = NULL;

	if (!Text || !Old || !New)
		return result;

	gchar **split = g_strsplit(Text, Old, -1);
	result = g_strjoinv(New, split);
	g_strfreev(split);
	g_free(Text);

	return result;
}
static gchar *TranslateString(GKeyFile *KeyFile, char *String)
{
	gchar *result = g_strdup(String);

	if (!KeyFile)
		return result;

	GMatchInfo *match_info;
	GRegex *regex = g_regex_new(REGEXP_STRING, 0, 0, NULL);

	if (g_regex_match(regex, String, 0, &match_info))
	{
		while (g_match_info_matches(match_info))
		{
			gchar *template = g_match_info_fetch(match_info, 0);

			gchar *string = g_key_file_get_string(KeyFile, gLang, template, NULL);

			if (!string)
				string = g_key_file_get_string(KeyFile, "Default", template, NULL);

			if (!string && gNoise)
			{
				gchar *message = g_strdup_printf("Translation for %s not found!", template);
				LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, message);
				g_free(message);
			}
			else
			{
				if (gNoise && result[0] == '\0')
				{
					gchar *message = g_strdup_printf("Translation for %s is empty!", template);
					LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
					g_free(message);
				}

				result = ReplaceString(result, template, string);
			}

			g_free(template);
			g_match_info_next(match_info, NULL);
		}
	}

	if (match_info)
		g_match_info_free(match_info);

	if (regex)
		g_regex_unref(regex);

	return result;
}

static gboolean ExecuteScript(gchar * script_name, gchar * verb, char *arg1, char *arg2, gchar **output, gboolean is_progress_needed)
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

	argv[0] = script;
	argv[1] = verb;
	argv[2] = arg1;
	argv[3] = arg2;
	argv[4] = NULL;

	gchar *env_data = g_key_file_get_string(gCfg, script_name, OPT_ENVVAR, NULL);

	if (env_data)
	{
		envp = g_environ_setenv(g_get_environ(), ENVVAR_NAME, env_data, TRUE);
		g_free(env_data);
	}

	if (gNoise)
	{
		message = g_strdup_printf("%s %s %s %s", script, verb, arg1 ? arg1 : "", arg2 ? arg2 : "");
		LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
		g_free(message);

	}

	result = g_spawn_async_with_pipes(gScriptDir, argv, envp, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL, &stdout_fp, &stderr_fp, &err);
	g_free(script);
	g_strfreev(envp);

	if (err)
	{
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);
		g_clear_error(&err);
	}

	if (result)
	{
		guint timer_id;
		int progress = 0;

		if (is_progress_needed)
		{
			tProgressData *data = g_new0(tProgressData, 1);
			data->arg1 = arg1;
			data->arg2 = arg2;
			data->pid = pid;
			data->progress = &progress;
			timer_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000, CheckCancel, data, (GDestroyNotify)g_free);
		}

		GIOChannel *stdout_chan = g_io_channel_unix_new(stdout_fp);
		GString *lines = g_string_new(NULL);

		while (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout_chan, &line, &len, &term, &err))
		{
			if (line)
			{
				lines = g_string_append(lines, line);
				line[term] = '\0';

				if (gNoise)
					LogMessage(gPluginNr, MSGTYPE_DETAILS, line);

				if (is_progress_needed)
				{
					gint64 ret = g_ascii_strtoll(line, NULL, 0);

					if (ret >= 0 || ret <= 100)
						progress = ret;
					else


						if (gProgressProc(gPluginNr, arg1, arg2, progress))
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

		if (fcntl(stdout_fp, F_GETFD) != -1)
			g_close(stdout_fp, NULL);

		if (fcntl(stderr_fp, F_GETFD) != -1)
			g_close(stderr_fp, NULL);

		if (output)
			*output = g_string_free(lines, FALSE);
		else
			g_string_free(lines, TRUE);

		if (is_progress_needed)
			g_source_remove(timer_id);
	}

	if (gNoise)
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

static void LoadPreview(uintptr_t pDlg, gchar * file)
{
	FILE *fp;
	int count = 0;
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;
	gboolean readme = TRUE;

	SendDlgMsg(pDlg, "mPreview", DM_SETTEXT, 0, 0);

	gchar *src_file = g_strdup_printf("%s/%s_readme.txt", gScriptDir, file);

	if (!g_file_test(src_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		readme = FALSE;
		g_free(src_file);
		src_file = g_strdup_printf("%s/%s", gScriptDir, file);
	}

	if ((fp = fopen(src_file, "r")) != NULL)
	{
		SendDlgMsg(pDlg, "mPreview", DM_ENABLE, 1, 0);

		while ((read = getline(&line, &len, fp)) != -1)
		{
			if (!readme && count > MAX_SCRIPT_LINES)
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
		src_file = g_strdup_printf("%s/%s", gScriptDir, file);
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

static gboolean IsValidOpt(gchar * str, gchar * opt)
{
	return (strncmp(str, opt, strlen(opt)) == 0 && strlen(str) > (strlen(opt) + 2));
}

static void FillProps(uintptr_t pDlg)
{
	int i = 1;

	if (gProps && gProps[0] != '\0')
	{
		GKeyFile *langs = OpenTranslations(gScript);
		gchar **split = g_strsplit(gProps, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			if (IsValidOpt(*p, OPT_ACTS))
			{
				gchar **acts = g_strsplit(STRIP_OPT(*p, OPT_ACTS), "\t", -1);

				if (g_strv_length(acts) > 0)
				{
					for (gchar **a = acts; *a != NULL; a++)
					{
						if (*a[0] != '\0')
						{
							gchar *string = TranslateString(langs, *a);
							SendDlgMsg(pDlg, "cbAct", DM_LISTADDSTR, (intptr_t)string, 0);
							SendDlgMsg(pDlg, "lbDataStore", DM_LISTADDSTR, (intptr_t)*a, 0);
							g_free(string);
						}
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

				if (i > 10)
				{
					gchar *prop = TranslateString(langs, g_strstrip(res[0]));
					gchar *value = TranslateString(langs, g_strstrip(res[1]));
					gchar *string = g_strdup_printf("%s\t%s", prop, value);
					SendDlgMsg(pDlg, "lbExtraProps", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "lbExtraProps", DM_LISTADDSTR, (intptr_t)string, 0);
					g_free(prop);
					g_free(value);
					g_free(string);
					continue;
				}

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
					gchar *prop = TranslateString(langs, g_strstrip(res[0]));
					gchar *value = TranslateString(langs, g_strstrip(res[1]));
					gchar *item = g_strdup_printf("lProp%d", i);
					SendDlgMsg(pDlg, item, DM_SETTEXT, (intptr_t)prop, 0);
					SendDlgMsg(pDlg, item, DM_SHOWITEM, 1, 0);
					g_free(item);
					item = g_strdup_printf("lValue%d", i);
					SendDlgMsg(pDlg, item, DM_SETTEXT, (intptr_t)value, 0);
					SendDlgMsg(pDlg, item, DM_SHOWITEM, 1, 0);
					g_free(item);
					g_free(prop);
					g_free(value);
					i++;
				}

				g_strfreev(res);
			}
		}

		g_strfreev(split);
		CloseTranslations(langs);
	}
}

static void LogCryptProc(int ret)
{
	switch (ret)
	{
	case FS_FILE_OK:
		if (gNoise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "CryptProc: Success");

		break;

	case FS_FILE_NOTSUPPORTED:
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc: Encrypt/Decrypt failed");
		break;

	case FS_FILE_WRITEERROR:
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc: Could not write password to password store");
		break;

	case FS_FILE_READERROR:
		if (gNoise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "CryptProc: Password not found in password store");

		break;

	case FS_FILE_NOTFOUND:
		if (gNoise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "CryptProc: No master password entered yet");

		break;
	}
}

static void ParseOpts(gchar * script, gchar * text);

static void SetOpt(gchar * script, gchar * opt, gchar * value)
{
	gchar *output = NULL;
	ExecuteScript(script, VERB_SETOPT, opt, value, &output, FALSE);

	if (output && output[0] != '\0')
		ParseOpts(script, output);

	g_free(output);
	output = NULL;
}

static void SaveHistory(uintptr_t pDlg, char *text, char *value)
{
	int count = (int)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETCOUNT, 0, 0);

	gboolean is_cleared = FALSE;

	if (count == 0)
		is_cleared = (gboolean)SendDlgMsg(pDlg, "lbHistory", DM_SHOWITEM, 0, 0);

	if (!is_cleared && value && value[0] != '\0')

		g_key_file_set_string(gCfg, gScript, text, value);
	else
		g_key_file_remove_key(gCfg, gScript, text, NULL);

	int num = 0;
	gchar *key = NULL;

	for (gsize i = 0; i < count; i++)
	{
		if (num >= MAX_PATH)
			break;

		char *oldvalue = (char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0);

		if (!value || strcmp(oldvalue, value) != 0)
		{
			gchar *key = g_strdup_printf("%s_%s_%d", MARK_PREV, text, num++);
			g_key_file_set_string(gCfg, gScript, key, oldvalue);
			g_free(key);
		}
	}

	key = g_strdup_printf("%s_%s_%d", MARK_PREV, text, num);

	if (g_key_file_has_key(gCfg, gScript, key, NULL))
		g_key_file_remove_key(gCfg, gScript, key, NULL);

	g_free(key);
}

intptr_t DCPCALL DlgRequestValueProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
	{
		char *text = (char*)SendDlgMsg(pDlg, "lblOption", DM_GETTEXT, 0, 0);
		gchar *key = g_strdup_printf("%s_%s_0", MARK_PREV, text);
		gboolean history = g_key_file_has_key(gCfg, gScript, key, NULL);
		g_free(key);
		char *value = g_strdup((char*)SendDlgMsg(pDlg, "edValue", DM_GETTEXT, 0, 0));

		if (value && value[0] != '\0')
		{
			SendDlgMsg(pDlg, "lbHistory", DM_SHOWITEM, (int)history, 0);
			SendDlgMsg(pDlg, "lbHistory", DM_LISTADDSTR, (intptr_t)value, 0);


			if (history)
			{
				int num = 0;
				gchar *key = NULL;

				do
				{
					g_free(key);
					key = g_strdup_printf("%s_%s_%d", MARK_PREV, text, num++);
					gchar *oldvalue = g_key_file_get_string(gCfg, gScript, key, NULL);
					g_print("!!! %s %s", value, oldvalue);

					if (oldvalue && strcmp(oldvalue, value) != 0)
						SendDlgMsg(pDlg, "lbHistory", DM_LISTADDSTR, (intptr_t)oldvalue, 0);

					g_free(oldvalue);
				}
				while (g_key_file_has_key(gCfg, gScript, key, NULL));

				g_free(key);
			}
		}

		g_free(value);
	}

	break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			gchar *output = NULL;
			char *text = g_strdup((char*)SendDlgMsg(pDlg, "lblOption", DM_GETTEXT, 0, 0));
			char *res = g_strdup((char*)SendDlgMsg(pDlg, "edValue", DM_GETTEXT, 0, 0));
			SendDlgMsg(pDlg, "edValue", DM_SHOWDIALOG, 0, 0);

			if (res && res[0] != '\0')
			{
				SaveHistory(pDlg, text, res);
				ExecuteScript(gScript, VERB_SETOPT, text, res, &output, FALSE);

				if (output && output[0] != '\0')
					ParseOpts(gScript, output);

				g_free(output);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "empty argument selected");

			g_free(res);
			g_free(text);
		}
		else if (strcmp(DlgItemName, "lbHistory") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *value = g_strdup((char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0));
				SendDlgMsg(pDlg, "edValue", DM_SETTEXT, (intptr_t)value, 0);
				g_free(value);
			}
		}

		break;

	case DN_KEYUP:
		if (strcmp(DlgItemName, "lbHistory") == 0)
		{
			int16_t *key = (int16_t*)wParam;

			if (lParam == 1 && *key == 46)
			{
				gchar *value = g_strdup((char*)SendDlgMsg(pDlg, "edValue", DM_GETTEXT, 0, 0));
				int i = (int)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEMINDEX, 0, 0);

				if (i != -1)
				{
					gchar *oldvalue = g_strdup((char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0));
					SendDlgMsg(pDlg, "lbHistory", DM_LISTDELETE, i, 0);

					if (g_strcmp0(oldvalue, value) == 0 && SendDlgMsg(pDlg, "lbHistory", DM_LISTGETCOUNT, 0, 0) > 0)
					{
						value = g_strdup((char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, 0, 0));
						SendDlgMsg(pDlg, "edValue", DM_SETTEXT, (intptr_t)value, 0);
					}
				}

				gchar *text = g_strdup((char*)SendDlgMsg(pDlg, "lblOption", DM_GETTEXT, 0, 0));
				SaveHistory(pDlg, text, value);
				g_free(value);
				g_free(text);
			}
		}

		break;
	}

	return 0;
}

static gboolean IsCanShowDlg(char *text)
{
	if (strncmp(text, MARK_NOISE, 3) == 0)
	{
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Options starting with \"Fs_\" are reserved, ignored");
		return FALSE;
	}

	if (!gDialogApi)
	{
		LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "DialogApi not initialized");
		return FALSE;
	}

	return TRUE;
}

static gchar *BuildRequestLFMData(const char *lfmdata_templ, char *text)
{
	GKeyFile *langs = OpenTranslations(gScript);
	gchar *string = TranslateString(langs, text);
	CloseTranslations(langs);
	gchar *prev = g_key_file_get_string(gCfg, gScript, text, NULL);
	gchar *lfmdata = g_strdup_printf(lfmdata_templ, text, string, prev ? prev : "");
	g_free(prev);
	g_free(string);

	return lfmdata;
}


static void ShowSelectFileDlg(char *text)
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
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblOption: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideBottom.Control = Owner\n"
	                       "    AnchorSideBottom.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 189\n"
	                       "    Width = 21\n"
	                       "    Anchors = [akLeft, akBottom]\n"
	                       "    Caption = '%s'\n"
	                       "    ParentColor = False\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object lblText: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    AnchorSideRight.Control = edValue\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 10\n"
	                       "    Width = 21\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    Caption = '%s'\n"
	                       "    ParentColor = False\n"
	                       "    WordWrap = True\n"
	                       "  end\n"
	                       "  object lbHistory: TListBox\n"
	                       "    AnchorSideLeft.Control = edValue\n"
	                       "    AnchorSideTop.Control = lblText\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = edValue\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 83\n"
	                       "    Top = 37\n"
	                       "    Width = 598\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Top = 10\n"
	                       "    ItemHeight = 0\n"
	                       "    OnClick = ListBoxClick\n"
	                       "    OnKeyUp = ListBoxKeyUp\n"
	                       "    TabOrder = 0\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object edValue: TFileNameEdit\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = lbHistory\n"
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
	                       "    BorderSpacing.Top = 10\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 0\n"
	                       "    Text = '%s'\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = edValue\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = edValue\n"
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

	if (!IsCanShowDlg(text))
		return;

	gchar **split = g_strsplit(text, "\t", -1);
	guint len = g_strv_length(split);

	if (len > 0)
	{
		GKeyFile *langs = OpenTranslations(gScript);
		gchar *string = TranslateString(langs, split[0]);
		CloseTranslations(langs);
		gchar *prev = g_key_file_get_string(gCfg, gScript, split[0], NULL);
		gchar *lfmdata = g_strdup_printf(lfmdata_templ, split[0], string, split[1] ? split[1] : "*.*|*.*", prev ? prev : "");
		g_free(prev);
		g_free(string);

		gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgRequestValueProc);
		g_free(lfmdata);
		g_strfreev(split);
	}
}

static void ShowSelectDirDlg(char *text)
{
	const char lfmdata_templ[] = ""
	                             "object SelectDirDialogBox: TSelectDirDialogBox\n"
	                             "  Left = 210\n"
	                             "  Height = 216\n"
	                             "  Top = 218\n"
	                             "  Width = 622\n"
	                             "  AutoSize = True\n"
	                             "  BorderStyle = bsDialog\n"
	                             "  Caption = 'Confirmation of parameter'\n"
	                             "  ChildSizing.LeftRightSpacing = 10\n"
	                             "  ChildSizing.TopBottomSpacing = 10\n"
	                             "  ClientHeight = 216\n"
	                             "  ClientWidth = 622\n"
	                             "  DesignTimePPI = 100\n"
	                             "  OnCreate = DialogBoxShow\n"
	                             "  Position = poOwnerFormCenter\n"
	                             "  LCLVersion = '2.2.4.0'\n"
	                             "  object lblOption: TLabel\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideBottom.Control = Owner\n"
	                             "    AnchorSideBottom.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 17\n"
	                             "    Top = 189\n"
	                             "    Width = 21\n"
	                             "    Anchors = [akLeft, akBottom]\n"
	                             "    Caption = '%s'\n"
	                             "    ParentColor = False\n"
	                             "    Visible = False\n"
	                             "  end\n"
	                             "  object lblText: TLabel\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideTop.Control = Owner\n"
	                             "    AnchorSideRight.Control = edValue\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 17\n"
	                             "    Top = 10\n"
	                             "    Width = 21\n"
	                             "    Anchors = [akTop, akLeft, akRight]\n"
	                             "    Caption = '%s'\n"
	                             "    ParentColor = False\n"
	                             "    WordWrap = True\n"
	                             "  end\n"
	                             "  object lbHistory: TListBox\n"
	                             "    AnchorSideLeft.Control = edValue\n"
	                             "    AnchorSideTop.Control = lblText\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    AnchorSideRight.Control = edValue\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 83\n"
	                             "    Top = 37\n"
	                             "    Width = 598\n"
	                             "    Anchors = [akTop, akLeft, akRight]\n"
	                             "    BorderSpacing.Top = 10\n"
	                             "    ItemHeight = 0\n"
	                             "    OnClick = ListBoxClick\n"
	                             "    OnKeyUp = ListBoxKeyUp\n"
	                             "    TabOrder = 0\n"
	                             "    Visible = False\n"
	                             "  end\n"
	                             "  object edValue: TDirectoryEdit\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideTop.Control = lbHistory\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 36\n"
	                             "    Top = 130\n"
	                             "    Width = 598\n"
	                             "    ShowHidden = False\n"
	                             "    ButtonWidth = 24\n"
	                             "    NumGlyphs = 1\n"
	                             "    BorderSpacing.Top = 10\n"
	                             "    MaxLength = 0\n"
	                             "    TabOrder = 1\n"
	                             "    Text = '%s'\n"
	                             "  end\n"
	                             "  object btnOK: TBitBtn\n"
	                             "    AnchorSideTop.Control = edValue\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    AnchorSideRight.Control = edValue\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 511\n"
	                             "    Height = 31\n"
	                             "    Top = 176\n"
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
	                             "    TabOrder = 2\n"
	                             "  end\n"
	                             "  object btnCancel: TBitBtn\n"
	                             "    AnchorSideTop.Control = btnOK\n"
	                             "    AnchorSideRight.Control = btnOK\n"
	                             "    Left = 404\n"
	                             "    Height = 31\n"
	                             "    Top = 176\n"
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
	                             "    TabOrder = 3\n"
	                             "  end\n"
	                             "end\n";

	if (!IsCanShowDlg(text))
		return;

	gchar *lfmdata = BuildRequestLFMData(lfmdata_templ, text);

	gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgRequestValueProc);
	g_free(lfmdata);
}

static void ShowRequestValueDlg(char *text)
{
	const char lfmdata_templ[] = ""
	                             "object SelectDirDialogBox: TSelectDirDialogBox\n"
	                             "  Left = 210\n"
	                             "  Height = 216\n"
	                             "  Top = 218\n"
	                             "  Width = 622\n"
	                             "  AutoSize = True\n"
	                             "  BorderStyle = bsDialog\n"
	                             "  Caption = 'Confirmation of parameter'\n"
	                             "  ChildSizing.LeftRightSpacing = 10\n"
	                             "  ChildSizing.TopBottomSpacing = 10\n"
	                             "  ClientHeight = 216\n"
	                             "  ClientWidth = 622\n"
	                             "  DesignTimePPI = 100\n"
	                             "  OnCreate = DialogBoxShow\n"
	                             "  Position = poOwnerFormCenter\n"
	                             "  LCLVersion = '2.2.4.0'\n"
	                             "  object lblOption: TLabel\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideBottom.Control = Owner\n"
	                             "    AnchorSideBottom.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 17\n"
	                             "    Top = 189\n"
	                             "    Width = 21\n"
	                             "    Anchors = [akLeft, akBottom]\n"
	                             "    Caption = '%s'\n"
	                             "    ParentColor = False\n"
	                             "    Visible = False\n"
	                             "  end\n"
	                             "  object lblText: TLabel\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideTop.Control = Owner\n"
	                             "    AnchorSideRight.Control = edValue\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 17\n"
	                             "    Top = 10\n"
	                             "    Width = 21\n"
	                             "    Anchors = [akTop, akLeft, akRight]\n"
	                             "    Caption = '%s'\n"
	                             "    ParentColor = False\n"
	                             "    WordWrap = True\n"
	                             "  end\n"
	                             "  object lbHistory: TListBox\n"
	                             "    AnchorSideLeft.Control = edValue\n"
	                             "    AnchorSideTop.Control = lblText\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    AnchorSideRight.Control = edValue\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 83\n"
	                             "    Top = 37\n"
	                             "    Width = 598\n"
	                             "    Anchors = [akTop, akLeft, akRight]\n"
	                             "    BorderSpacing.Top = 10\n"
	                             "    ItemHeight = 0\n"
	                             "    OnClick = ListBoxClick\n"
	                             "    OnKeyUp = ListBoxKeyUp\n"
	                             "    TabOrder = 0\n"
	                             "    Visible = False\n"
	                             "  end\n"
	                             "  object edValue: TEdit\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideTop.Control = lbHistory\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 36\n"
	                             "    Top = 130\n"
	                             "    Width = 598\n"
	                             "    BorderSpacing.Top = 10\n"
	                             "    TabOrder = 1\n"
	                             "    Text = '%s'\n"
	                             "  end\n"
	                             "  object btnOK: TBitBtn\n"
	                             "    AnchorSideTop.Control = edValue\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    AnchorSideRight.Control = edValue\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 511\n"
	                             "    Height = 31\n"
	                             "    Top = 176\n"
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
	                             "    TabOrder = 2\n"
	                             "  end\n"
	                             "  object btnCancel: TBitBtn\n"
	                             "    AnchorSideTop.Control = btnOK\n"
	                             "    AnchorSideRight.Control = btnOK\n"
	                             "    Left = 404\n"
	                             "    Height = 31\n"
	                             "    Top = 176\n"
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
	                             "    TabOrder = 3\n"
	                             "  end\n"
	                             "end\n";

	if (!IsCanShowDlg(text))
		return;

	gchar *lfmdata = BuildRequestLFMData(lfmdata_templ, text);

	gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgRequestValueProc);
	g_free(lfmdata);
}

intptr_t DCPCALL DlgMultiChoiceProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		if (gChoice)
		{
			gsize count = 0;
			GKeyFile *langs = OpenTranslations(gScript);
			gchar *string = TranslateString(langs, gChoice[0]);
			SendDlgMsg(pDlg, "MultichoiceDialogBox", DM_SETTEXT, (intptr_t)gScript, 0);
			SendDlgMsg(pDlg, "lblText", DM_SETTEXT, (intptr_t)string, 0);
			SendDlgMsg(pDlg, "lbDataStore", DM_LISTADDSTR, (intptr_t)gChoice[0], 0);
			g_free(string);
			gchar **items = gChoice + 1;
			gchar *prev_choice = g_key_file_get_string(gCfg, gScript, gChoice[0], NULL);

			if (items)
			{
				for (gchar **p = items; *p != NULL; p++)
				{
					if (*p[0] != '\0')
					{
						string = TranslateString(langs, *p);
						SendDlgMsg(pDlg, "cbChoice", DM_LISTADDSTR, (intptr_t)string, 0);
						SendDlgMsg(pDlg, "lbDataStore", DM_LISTADDSTR, (intptr_t)*p, 0);
						g_free(string);

						if (prev_choice && g_strcmp0(*p, prev_choice) == 0)
							SendDlgMsg(pDlg, "cbChoice", DM_LISTSETITEMINDEX, (intptr_t)count, 0);

						count++;
					}
				}

				if (gNoise && count == 1)
					LogMessage(gPluginNr, MSGTYPE_DETAILS, "Fs_MultiChoice: there is no choice");
				else if (count < 1)
					LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Fs_MultiChoice: there is no choice");

				int index = (int)SendDlgMsg(pDlg, "cbChoice", DM_LISTGETITEMINDEX, 0, 0);

				if (index == -1)
					SendDlgMsg(pDlg, "cbChoice", DM_LISTSETITEMINDEX, 0, 0);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Fs_MultiChoice: there is no choice");

			g_free(prev_choice);
			CloseTranslations(langs);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			gchar *output = NULL;
			char *text = g_strdup((char*)SendDlgMsg(pDlg, "lbDataStore", DM_LISTGETITEM, 0, 0));
			int i = SendDlgMsg(pDlg, "cbChoice", DM_LISTGETITEMINDEX, 0, 0);
			char *res = g_strdup((char*)SendDlgMsg(pDlg, "lbDataStore", DM_LISTGETITEM, ++i, 0));
			g_key_file_set_string(gCfg, gScript, text, res);
			SendDlgMsg(pDlg, "cbChoice", DM_SHOWDIALOG, 0, 0);

			if (res && res[0] != '\0')
			{
				ExecuteScript(gScript, VERB_SETOPT, text, res, &output, FALSE);

				if (output && output[0] != '\0')
					ParseOpts(gScript, output);

				g_free(output);
			}
			else
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "empty argument selected");

			g_free(text);
			g_free(res);
		}

		break;
	}

	return 0;
}

static void ShowMultiChoiceDlg(char *text)
{
	const char *multichoice_lfm = R"(
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
  Position = poOwnerFormCenter
  LCLVersion = '2.0.13.0'
  object lblText: TLabel
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = Owner
    AnchorSideRight.Control = cbChoice
    AnchorSideRight.Side = asrBottom
    Left = 10
    Height = 1
    Top = 10
    Width = 1
    Anchors = [akTop, akLeft, akRight]
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
  object lbDataStore: TListBox
    AnchorSideLeft.Control = Owner
    AnchorSideRight.Control = Owner
    AnchorSideBottom.Control = Owner
    AnchorSideBottom.Side = asrBottom
    Left = 10
    Height = 83
    Top = 123
    Width = 164
    Anchors = [akLeft, akBottom]
    ItemHeight = 0
    TabStop = False
    Visible = False
  end
end
)";

	if (!IsCanShowDlg(text))
		return;

	gChoice = g_strsplit(text, "\t", -1);
	gDialogApi->DialogBoxLFM((intptr_t)multichoice_lfm, (unsigned long)strlen(multichoice_lfm), DlgMultiChoiceProc);
	g_strfreev(gChoice);
	gChoice = NULL;

}

static void ParseOpts(gchar *script, gchar *text)
{
	gchar *output = NULL;
	gboolean request_values = FALSE;
	gboolean log_info = FALSE;

	if (text)
	{
		GKeyFile *langs = OpenTranslations(script);
		gchar **split = g_strsplit(text, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			gchar *string = NULL;

			if (*p[0] != 0)
			{
				if (g_strcmp0(*p, OPT_REQUEST) == 0)
				{
					log_info = FALSE;
					request_values = TRUE;
				}
				else if (g_strcmp0(*p, OPT_LOGINFO) == 0)
				{
					log_info = TRUE;
					request_values = FALSE;
				}
				else if (g_strcmp0(*p, OPT_STATUSINFO) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_STATUSINFO, TRUE);
				else if (g_strcmp0(*p, OPT_GETVALUE) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_GETVALUE, TRUE);
				else if (IsValidOpt(*p, OPT_ENVVAR))
				{
					g_key_file_set_string(gCfg, script, OPT_ENVVAR, STRIP_OPT(*p, OPT_ENVVAR));

					if (gNoise)
					{
						gchar *message = g_strdup_printf("%s value will be set to %s", ENVVAR_NAME, *p + strlen(OPT_ENVVAR) + 1);
						LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
						g_free(message);
					}
				}
				else if (IsValidOpt(*p, OPT_INFORM) && gRequestProc)
				{
					string = TranslateString(langs, STRIP_OPT(*p, OPT_INFORM));

					if (gDialogApi)
						MessageBox(string, script, MB_OK | MB_ICONINFORMATION);
					else
						gRequestProc(gPluginNr, RT_MsgOK, script, string, NULL, 0);
				}
				else if (IsValidOpt(*p, OPT_YESNOMSG) && gRequestProc)
				{
					string = TranslateString(langs, STRIP_OPT(*p, OPT_YESNOMSG));
					gboolean is_yes = FALSE;

					if (gDialogApi)
						is_yes = (MessageBox(string, script, MB_YESNO | MB_ICONQUESTION) == ID_YES);
					else
						is_yes = gRequestProc(gPluginNr, RT_MsgYesNo, script, string, NULL, 0);

					SetOpt(script, STRIP_OPT(*p, OPT_YESNOMSG), is_yes ? "Yes" : "No");
				}
				else if (IsValidOpt(*p, OPT_ASK) && gRequestProc)
				{
					string = TranslateString(langs, STRIP_OPT(*p, OPT_ASK));
					gboolean is_yes = FALSE;
					gchar *key = g_strdup_printf("%s_%s", MARK_BOOL, STRIP_OPT(*p, OPT_ASK));

					if (!g_key_file_has_key(gCfg, script, key, NULL))
					{
						if (gDialogApi)
							is_yes = (MessageBox(string, script, MB_YESNO | MB_ICONQUESTION) == ID_YES);
						else
							is_yes = gRequestProc(gPluginNr, RT_MsgYesNo, script, string, NULL, 0);

						g_key_file_set_boolean(gCfg, script, key, is_yes);
					}
					else
						is_yes = g_key_file_get_boolean(gCfg, script, string, NULL);

					g_free(key);
					SetOpt(script, STRIP_OPT(*p, OPT_ASK), is_yes ? "Yes" : "No");
				}
				else if (IsValidOpt(*p, OPT_PUSH))
				{
					gchar **res = g_strsplit(STRIP_OPT(*p, OPT_PUSH), "\t", 2);

					if (strncmp(res[0], MARK_NOISE, 3) == 0)
						LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Options starting with \"Fs_\" are reserved, ignored");
					else
					{
						if (!g_key_file_has_key(gCfg, script, res[0], NULL))
						{
							g_key_file_set_string(gCfg, script, res[0], res[1]);
							g_strlcpy(gScript, script, PATH_MAX);
							ShowRequestValueDlg(res[0]);
						}
						else
						{
							gchar *value = g_key_file_get_string(gCfg, script, res[0], NULL);
							SetOpt(script, res[0], value);
							g_free(value);
						}
					}

					g_strfreev(res);
				}
				else if (IsValidOpt(*p, OPT_CHOICE))
				{
					g_strlcpy(gScript, script, PATH_MAX);
					ShowMultiChoiceDlg(STRIP_OPT(*p, OPT_CHOICE));
				}
				else if (IsValidOpt(*p, OPT_SELFILE))
				{
					g_strlcpy(gScript, script, PATH_MAX);
					ShowSelectFileDlg(STRIP_OPT(*p, OPT_SELFILE));
				}
				else if (IsValidOpt(*p, OPT_SELDIR))
				{
					g_strlcpy(gScript, script, PATH_MAX);
					ShowSelectDirDlg(STRIP_OPT(*p, OPT_SELDIR));
				}
				else if (strncmp(*p, MARK_NOISE, 3) == 0)
					LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "Options starting with \"Fs_\" are reserved, ignored");
				else if (log_info == TRUE)
				{
					string = TranslateString(langs, *p);
					LogMessage(gPluginNr, MSGTYPE_DETAILS, string);
				}
				else if (request_values == TRUE)
				{
					if (strncasecmp("password", *p, 8) == 0)
					{
						char value[MAX_PATH] = "";

						if (!gCryptProc)
						{
							LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc not initialized");
						}
						else
						{
							int ret = gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_LOAD_PASSWORD_NO_UI, script, value, MAX_PATH);

							if (ret == FS_FILE_NOTFOUND)
								ret = gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_LOAD_PASSWORD, script, value, MAX_PATH);

							LogCryptProc(ret);

							if (gRequestProc && gRequestProc(gPluginNr, RT_Password, script, NULL, value, MAX_PATH))
							{
								ExecuteScript(script, VERB_SETOPT, *p, value, &output, FALSE);


								if (!gCryptProc)
									LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc not initialized");
								else
									LogCryptProc(gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_SAVE_PASSWORD, script, value, MAX_PATH));

								if (output && output[0] != '\0')
									ParseOpts(script, output);

								g_free(output);
								output = NULL;

							}
						}
					}
					else
					{
						g_strlcpy(gScript, script, PATH_MAX);
						ShowRequestValueDlg(*p);
					}
				}
				else
				{
					gchar *message = g_strdup_printf("%s directive not received, Line \"%s\" will be omitted.", OPT_REQUEST, *p);
					LogMessage(gPluginNr, MSGTYPE_IMPORTANTERROR, message);
					g_free(message);
				}
			}

			g_free(string);
		}

		g_strfreev(split);
		CloseTranslations(langs);
	}
}

static void DeInitializeScript(gchar *script)
{
	if (g_key_file_get_boolean(gCfg, script, MARK_INUSE, NULL))
	{
		if (!ExecuteScript(script, VERB_DEINIT, NULL, NULL, NULL, FALSE) && gNoise)
			LogMessage(gPluginNr, MSGTYPE_DETAILS, "Deinitialization not implemented or not completed successfully");

		g_key_file_remove_key(gCfg, script, MARK_INUSE, NULL);
		g_key_file_remove_key(gCfg, script, OPT_ENVVAR, NULL);
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
	gNoise = g_key_file_get_boolean(gCfg, script, MARK_NOISE, NULL);
#ifndef TEMP_PANEL
	gchar *message = g_strdup_printf("CONNECT /%s", script);
	LogMessage(gPluginNr, MSGTYPE_CONNECT, message);
	g_free(message);
#endif

	ExecuteScript(script, VERB_INIT, NULL, NULL, &output, FALSE);
	g_key_file_set_boolean(gCfg, script, MARK_INUSE, TRUE);
	ParseOpts(script, output);
	g_free(output);
}



intptr_t DCPCALL DlgPropertiesProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		gNoise = g_key_file_get_boolean(gCfg, gScript, MARK_NOISE, NULL);
		SendDlgMsg(pDlg, "ckNoise", DM_SETCHECK, (intptr_t)gNoise, 0);
		SendDlgMsg(pDlg, "lScriptName", DM_SETTEXT, (intptr_t)gScript, 0);
		LoadPreview(pDlg, gScript);

		gboolean inuse = g_key_file_get_boolean(gCfg, gScript, MARK_INUSE, NULL);
		SendDlgMsg(pDlg, "btnUnmount", DM_SHOWITEM, (int)inuse, 0);
		SendDlgMsg(pDlg, "btnReset", DM_SHOWITEM, (int)!inuse, 0);

		if (gCaller && gProps)
		{
			gchar *content_type = g_content_type_guess(gCaller, NULL, 0, NULL);
			gchar *descr = g_content_type_get_description(content_type);
			gchar *filename = g_path_get_basename(gCaller);
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
			DeInitializeScript(gScript);
		}
		else if (strcmp(DlgItemName, "btnReset") == 0)
		{
			g_key_file_remove_group(gCfg, gScript, NULL);
			ExecuteScript(gScript, VERB_RESET, NULL, NULL, NULL, FALSE);
		}
		else if (strcmp(DlgItemName, "btnAct") == 0)
		{
			int i = SendDlgMsg(pDlg, "cbAct", DM_LISTGETITEMINDEX, 0, 0);
			if (i > -1)
			{
			char *cmd = g_strdup((char*)SendDlgMsg(pDlg, "lbDataStore", DM_LISTGETITEM, i, 0));

			if (cmd && cmd[0] != '\0')
			{
				SetOpt(gScript, cmd, gCaller);
				SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
			}

			g_free(cmd);
		}
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "ckNoise") == 0)
		{
			gNoise = (gboolean)SendDlgMsg(pDlg, DlgItemName, DM_GETCHECK, 0, 0);
			g_key_file_set_boolean(gCfg, gScript, MARK_NOISE, gNoise);
		}
		else if (strcmp(DlgItemName, "cbAct") == 0)
		{
			if (SendDlgMsg(pDlg, "cbAct", DM_LISTGETITEMINDEX, 0, 0) == -1)
				SendDlgMsg(pDlg, "btnAct", DM_ENABLE, 0, 0);
			else
				SendDlgMsg(pDlg, "btnAct", DM_ENABLE, 1, 0);
		}

		break;

	case DN_KEYUP:
		if (strcmp(DlgItemName, "cbAct") == 0)
		{
			int16_t *key = (int16_t*)wParam;

			if ( *key == 13)
			{
				int i = SendDlgMsg(pDlg, "cbAct", DM_LISTGETITEMINDEX, 0, 0);
				if (i > -1)
				{
				char *cmd = g_strdup((char*)SendDlgMsg(pDlg, "lbDataStore", DM_LISTGETITEM, i, 0));

				if (cmd && cmd[0] != '\0')
				{
					SetOpt(gScript, cmd, gCaller);
					SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
				}

				g_free(cmd);
			}
			}
		}

		break;
	}


	return 0;
}


static void ShowPropertiesDlg(void)
{
	if (gDialogApi && g_file_test(gLFMPath, G_FILE_TEST_EXISTS))
		gDialogApi->DialogBoxLFMFile(gLFMPath, DlgPropertiesProc);
	else  if (gRequestProc && gProps)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, gProps, NULL, 0);
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

static gboolean IsInvalidPath(char *path)
{
	return (g_strcmp0(path, "/.") == 0 || g_strcmp0(path, "/..") == 0 || strncmp(path, "/../", 4) == 0);
}

static gboolean IsRootDir(char *path)
{
	while (*path++)
	{
		if (*path == '/')
			return FALSE;

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
			gchar *src_file = g_strdup_printf("%s/%s", gScriptDir, ent->d_name);

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

		if (gNoise)
		{
			message = g_strdup_printf("Found: %s", string);
			LogMessage(gPluginNr, MSGTYPE_DETAILS, message);
			g_free(message);
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 1);

		if (gNoise)
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

		if (gNoise)
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
		else if (gNoise)
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

					if (gNoise)
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

			if (gNoise)
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "The current datetime has been set");

			//			FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
			//			FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 3);

		if (gNoise)
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

		if (gNoise)
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
		DIR *dir = opendir(gScriptDir);

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
		gchar *script = ExtractScriptFromPath(Path);

		if (!g_key_file_get_boolean(gCfg, script, MARK_INUSE, NULL))
		{
			g_strlcpy(gScript, script, PATH_MAX);
			InitializeScript(script);

			if (gNoise)
				LogMessage(gPluginNr, MSGTYPE_DETAILS, "forced init");
		}

		gchar *output = NULL;
		gchar *list_path = StripScriptFromPath(Path);
		size_t len = strlen(list_path);

		if (len > 1 && list_path[len - 1] == '/')
			list_path[len - 1] = '\0';

		if (!ExecuteScript(script, VERB_LIST, list_path, NULL, &output, FALSE))
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

		dirdata->regex = g_regex_new(REGEXP_LIST, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, NULL);

		if (!g_regex_match(dirdata->regex, output, 0, &(dirdata->match_info)))
		{
			if (gNoise)
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

	if (IsRootDir(RemoteName))
	{
		char path[PATH_MAX];
		g_strlcpy(path, gScriptDir, PATH_MAX);
		strcat(path, RemoteName);

		if (symlink(path, LocalName) != 0)
			return FS_FILE_WRITEERROR;
	}
	else
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *path = StripScriptFromPath(RemoteName);

		if (IsInvalidPath(path) || !ExecuteScript(script, VERB_GET_FILE, path, LocalName, NULL, TRUE))
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

	if (IsRootDir(RemoteName))
		return FS_FILE_NOTSUPPORTED;

	int result = FS_FILE_OK;
	gchar *path = StripScriptFromPath(RemoteName);

	if (IsInvalidPath(path))
	{
		g_free(path);
		return FS_FILE_NOTSUPPORTED;
	}

	gchar *script = ExtractScriptFromPath(RemoteName);

	if (CopyFlags == 0 && ExecuteScript(script, VERB_EXISTS, path, NULL, NULL, FALSE))
		result = FS_FILE_EXISTS;
	else
	{
		if (!ExecuteScript(script, VERB_PUT_FILE, LocalName, path, NULL, TRUE))
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

	if (IsRootDir(NewName))
		return FS_FILE_NOTSUPPORTED;

	gchar *newpath = StripScriptFromPath(NewName);

	if (IsInvalidPath(newpath))
	{
		g_free(newpath);
		return FS_FILE_NOTSUPPORTED;
	}

	int result = FS_FILE_OK;
	gchar *script = ExtractScriptFromPath(OldName);
	gchar *oldpath = StripScriptFromPath(OldName);

	if (!wtf_overwrite && ExecuteScript(script, VERB_EXISTS, newpath, NULL, NULL, FALSE))
		return FS_FILE_EXISTS;

	if (!ExecuteScript(script, Move ? VERB_MOVE : VERB_COPY, oldpath, newpath, NULL, TRUE))
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

	if (!IsRootDir(Path))
	{
		gchar *newpath = StripScriptFromPath(Path);

		if (IsInvalidPath(newpath))
		{
			g_free(newpath);
			return FS_FILE_NOTSUPPORTED;
		}

		gchar *output = NULL;
		gchar *script = ExtractScriptFromPath(Path);
		result = ExecuteScript(script, VERB_MKDIR, newpath, NULL, &output, FALSE);

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

	if (!IsRootDir(RemoteName))
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *newpath = StripScriptFromPath(RemoteName);
		result = ExecuteScript(script, VERB_REMOVE, newpath, NULL, NULL, TRUE);
		g_free(script);
		g_free(newpath);
	}

	return result;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	gboolean result = FALSE;

	if (!IsRootDir(RemoteName))
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *newpath = StripScriptFromPath(RemoteName);
		result = ExecuteScript(script, VERB_RMDIR, newpath, NULL, NULL, TRUE);
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
		if (IsRootDir(RemoteName))
		{
			InitializeScript(script);
			g_strlcpy(gScript, script, PATH_MAX);
			result = FS_EXEC_SYMLINK;
		}
		else
		{
			if (!ExecuteScript(script, VERB_EXEC, path, NULL, &output, FALSE))
				result = FS_EXEC_YOURSELF;
			else if (output && output[0] != '\0')
			{
				if (strncmp(output, MARK_NOISE, 3) == 0)
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
			g_strlcpy(gScript, script, PATH_MAX);

			if (!IsRootDir(RemoteName) && !IsInvalidPath(path))
			{
				gboolean is_ok = ExecuteScript(script, VERB_PROPS, path, NULL, &gProps, FALSE);

				if (!is_ok || (gProps && gProps[0] != '\0'))
				{

					if (strncmp(gProps, MARK_NOISE, 3) == 0)
						ParseOpts(script, gProps);
					else
					{
						gCaller = path;
						ShowPropertiesDlg();
						gCaller = NULL;
					}
				}

				g_free(gProps);
				gProps = NULL;

			}
			else
				ShowPropertiesDlg();

			result = FS_EXEC_OK;
		}

	}
	else if (!IsRootDir(RemoteName) && !IsInvalidPath(path))
	{
		if (strncmp(Verb, "chmod", 5) == 0)
		{
			if (ExecuteScript(script, VERB_CHMOD, path, Verb + 6, NULL, FALSE))
				result = FS_EXEC_OK;
		}
		else if (strncmp(Verb, "quote", 5) == 0)
		{
			ExecuteScript(script, VERB_QUOTE, Verb + 6, path, &output, FALSE);

			if (output && output[0] != '\0')
				ParseOpts(script, output);

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

	if (LastWriteTime && !IsRootDir(RemoteName) && !IsInvalidPath(newpath))
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

				result = ExecuteScript(script, VERB_MODTIME, newpath, date, NULL, FALSE);
				g_free(date);
				g_free(script);
			}
		}
	}

	g_free(newpath);

	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
#ifdef  TEMP_PANEL
{
	g_strlcpy(DefRootName, "Scripts (Temporary Panels)", maxlen - 1);
}
#else
{
	g_strlcpy(DefRootName, "Scripts", maxlen - 1);
}

BOOL DCPCALL FsDisconnect(char* DisconnectRoot)
{
	DeInitializeScript(DisconnectRoot + 1);
	return TRUE;
}
#endif

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		g_strlcpy(FieldName, FIELD_EXTRA, maxlen - 1);
		Units[0] = '\0';
		return ft_string;
	}

	return ft_nomorefields;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = ft_fieldempty;
	gchar *output = NULL;
	gchar *script = ExtractScriptFromPath(FileName);

	if (IsRootDir(FileName))
	{
		if (g_key_file_get_boolean(gCfg, script, MARK_INUSE, NULL))
		{
			g_strlcpy((char*)FieldValue, "*", maxlen - 1);
			g_free(script);
			return ft_string;
		}
	}

	if (!g_key_file_get_boolean(gCfg, script, OPT_GETVALUE, NULL))
	{
		g_free(script);
		return ft_fieldempty;
	}

	gchar *path = StripScriptFromPath(FileName);

	ExecuteScript(script, VERB_GETVALUE, path, NULL, &output, FALSE);

	if (output)
	{
		size_t len = strlen(output);

		if (len > 0)
		{
			output[len - 1] = '\0';
			GKeyFile *langs = OpenTranslations(gScript);
			gchar *string = TranslateString(langs, output);
			CloseTranslations(langs);
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
			g_free(string);
			result = ft_string;
		}
	}

	g_free(output);
	g_free(script);
	g_free(path);

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[DC().GETFILESIZE{}]\\n[DC().GETFILETIME{}]\\n[DC().GETFILEATTR{OCTAL}]     [Plugin(FS)." FIELD_EXTRA "{}] ", maxlen - 1);
	g_strlcpy(ViewHeaders, "Size\\nDate\\nInfo", maxlen - 1);
	g_strlcpy(ViewWidths, "100,15,-25,30,30", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

#ifdef  TEMP_PANEL
BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gboolean result = FALSE;

	if (!IsRootDir(RemoteName))
	{
		gchar *output = NULL;
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *newpath = StripScriptFromPath(RemoteName);
		ExecuteScript(gScript, VERB_REALNAME, newpath, NULL, &output, FALSE);
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
		g_strlcpy(path, gScriptDir, maxlen);
		strcat(path, RemoteName);
		g_strlcpy(RemoteName, path, maxlen - 1);
		result = TRUE;
	}

	return result;
}
#endif

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{

	if (IsRootDir(RemoteDir))
		return;

	gchar *script = ExtractScriptFromPath(RemoteDir);

	gNoise = g_key_file_get_boolean(gCfg, script, MARK_NOISE, NULL);

	if (!g_key_file_get_boolean(gCfg, script, OPT_STATUSINFO, NULL))
	{
		g_free(script);
		return;
	}

	gchar *output = NULL;
	gchar *path = StripScriptFromPath(RemoteDir);

	switch (InfoOperation)
	{
	case FS_STATUS_OP_LIST:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "list start" : "list end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_GET_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_single start" : "get_single end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_GET_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_multi start" : "get_multi end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_PUT_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_single start" : "put_single end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_PUT_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_multi start" : "put_multi end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_GET_MULTI_THREAD:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_multi_thread start" : "get_multi_thread end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_PUT_MULTI_THREAD:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_multi_thread start" : "put_multi_thread end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_RENMOV_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "renmov_single start" : "renmov_single end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_RENMOV_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "renmov_multi start" : "renmov_multi end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_DELETE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "delete start" : "delete end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_ATTRIB:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "attrib start" : "attrib end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_MKDIR:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "mkdir start" : "mkdir end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_EXEC:
		if (InfoStartEnd == FS_STATUS_START)
			g_strlcpy(gExecStart, script, PATH_MAX);
		else if (g_strcmp0(gExecStart, gScript) != 0)
			return;

		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "exec start" : "exec end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_CALCSIZE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "calcsize start" : "calcsize end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_SEARCH:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "search start" : "search end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_SEARCH_TEXT:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "search_text start" : "search_text end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_SYNC_SEARCH:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_search start" : "sync_search end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_SYNC_GET:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_get start" : "sync_get end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_SYNC_PUT:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_put start" : "sync_put end", path, &output, FALSE);
		break;

	case FS_STATUS_OP_SYNC_DELETE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_delete start" : "sync_delete end", path, &output, FALSE);
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

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct * dps)
{
	char def_ini[MAX_PATH];
	g_strlcpy(def_ini, dps->DefaultIniName, PATH_MAX);
	char *pos = strrchr(def_ini, '/');

	if (pos)
		strcpy(pos + 1, "\0");

	snprintf(gHistoryFile, sizeof(gHistoryFile), "%s%s.ini", def_ini, CFG_NAME);

	if (gCfg == NULL)
	{
		gCfg = g_key_file_new();
		g_key_file_load_from_file(gCfg, gHistoryFile, G_KEY_FILE_KEEP_COMMENTS, NULL);

		gchar **groups = g_key_file_get_groups(gCfg, NULL);

		for (char **script = groups; *script != NULL; script++)
		{
			if (g_key_file_get_boolean(gCfg, *script, MARK_INUSE, NULL))
				g_key_file_set_boolean(gCfg, *script, MARK_INUSE, FALSE);
		}
	}
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo * StartupInfo)
{
	const gchar* value = g_getenv("LANG");
	g_strlcpy(gLang, value, sizeof(gLang));
	char *pos = strrchr(gLang, '.');

	if (pos)
		*pos = '\0';

	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
		g_strlcpy(gScriptDir, gDialogApi->PluginDir, PATH_MAX);
		strcat(gScriptDir, EXEC_DIR);
		g_strlcpy(gLFMPath, gDialogApi->PluginDir, PATH_MAX);
		strcat(gLFMPath, "dialog.lfm");
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	gLogProc = NULL;
	gchar **groups = g_key_file_get_groups(gCfg, NULL);

	for (char **script = groups; *script != NULL; script++)
	{
		if (g_key_file_get_boolean(gCfg, *script, MARK_INUSE, NULL))
			DeInitializeScript(*script);
	}

	g_strfreev(groups);

	g_key_file_save_to_file(gCfg, gHistoryFile, NULL);

	if (gCaller != NULL)
		g_free(gCaller);

	if (gProps != NULL)
		g_free(gProps);

	if (gCfg != NULL)
		g_key_file_free(gCfg);

	if (gDialogApi != NULL)
		free(gDialogApi);

	gDialogApi = NULL;
}
