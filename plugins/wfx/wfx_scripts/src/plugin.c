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
#define PICTURE_MAX_SIZE 3000000

#ifdef  TEMP_PANEL
#define EXEC_DIR "scripts/temppanel"
#define CFG_NAME "wfx_scripts_temppanel"
#define ROOTNAME "Scripts (Temporary Panels)"
#else
#define EXEC_DIR "scripts"
#define CFG_NAME "wfx_scripts"
#define ROOTNAME "Scripts"
#endif

#define ENVVAR_REMOTENAME "DC_WFX_SCRIPT_REMOTENAME"
#define ENVVAR_MULTIFILEOP "DC_WFX_SCRIPT_MULTIFILEOP"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define MessageBox gDialogApi->MessageBox
#define SendDlgMsg gDialogApi->SendDlgMsg

#define EXEC_SEP "< < < < < < < < < < < < < < < < < < < < < < < < < > > > > > > > > > > > > > > > > > > > > > > > > >"

#define REGEXP_LIST "([0-9cbdflrstwxST\\-]+)\\s+(\\d{4}\\-?\\d{2}\\-?\\d{2}[\\stT]\\d{2}:?\\d{2}:?\\d?\\d?\\.?[0-9]*Z?)\\s+([0-9\\-]+)\\s+([^\\n]+)"
#define REGEXP_ENVVAAR "^(" OPT_ENVVAR "[A-Z0-9_]+)\\s"
#define REGEXP_STRING "E?N?V?_?WFX_SCRIPT_STR_[A-Z0-9_]+"

#define STRIP_OPT(S, O) S + strlen(O) + 1

#define OPT_EXECFEEDBK "Fs_ExecFeedback_Needed"
#define OPT_NOFAKEDATES "Fs_DisableFakeDates"
#define OPT_STATUSINFO "Fs_StatusInfo_Needed"
#define OPT_GETVALUES "Fs_GetValues_Needed"
#define OPT_GETVALUE "Fs_GetValue_Needed"
#define OPT_CONNECT "Fs_CONNECT_Needed"
#define OPT_REQUEST "Fs_Request_Options"
#define OPT_YESNOMSG "Fs_YesNo_Message"
#define OPT_ASKONCE "Fs_RequestOnce"
#define OPT_INFORM "Fs_Info_Message"
#define OPT_CHOICE "Fs_MultiChoice"
#define OPT_SELFILE "Fs_SelectFile"
#define OPT_SELDIR "Fs_SelectDir"
#define OPT_LOGINFO "Fs_LogInfo"
#define OPT_OUT  "Fs_ShowOutput"
#define OPT_ACTS "Fs_PropsActs"
#define OPT_PUSH "Fs_PushValue"
#define OPT_CLR "Fs_ClearValue"
#define OPT_EDIT "Fs_EditLine"
#define OPT_TERM  "Fs_RunTerm"
#define OPT_RUN  "Fs_RunAsync"
#define OPT_ENVVAR "Fs_Set_"
#define OPT_OPEN  "Fs_Open"

#define MARK_PIDS  "Fs_PIDStorage"
#define MARK_NOISE "Fs_DebugMode"
#define MARK_INUSE "Fs_InUse"
#define MARK_BOOL  "Fs_Bool"
#define MARK_PREV  "Fs_Old"
#define MARK_VALS  "Fs_GetValuesFrom"

#define MARK_LINK  "<.>.-->"
#define MARK_DEBG  "######.log"

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
#define VERB_GETVALS  "getvalues"
#define VERB_RESET    "reset"

#define FIELD_EXTRA "customfield"

typedef struct sVFSDirData
{
	DIR *dir;
	gchar *output;
	GRegex *regex;
	gboolean empty_dates;
	gboolean debug_check;
	char script[PATH_MAX];
	GMatchInfo *match_info;
} tVFSDirData;

typedef struct sProgressData
{
	gchar *script;
	gchar *verb;
	gchar *arg1;
	gchar *arg2;
	int *pid;
	int *progress;
	gboolean *is_running;
	gboolean *result;
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
static gboolean gExtraNoise = FALSE;
static char gScript[MAX_PATH];
static char gExecStart[MAX_PATH];
static char gScriptDir[PATH_MAX];
static char gHistoryFile[PATH_MAX];
static char gLang[10];

static gchar *gTermCmdOpen = NULL;
static gchar *gTermCmdClose = NULL;
static gchar *gViewerFont = NULL;
static gchar *gMemoText = NULL;
static gchar *gCaption = NULL;

int gDebugFd = -1;
gchar *gDebugFileName = NULL;
gchar **gLastValues = NULL;

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

static gchar *ReplaceTemplate(GKeyFile *KeyFile, char *Template)
{
	if (!KeyFile)
		return NULL;

	gchar *string = g_key_file_get_string(KeyFile, gLang, Template, NULL);

	if (!string)
		string = g_key_file_get_string(KeyFile, "Default", Template, NULL);

	return string;
}

static gchar *TranslateString(GKeyFile *KeyFile, char *String)
{
	gchar *result = g_strdup(String);

	if (!KeyFile)
		return result;

	GMatchInfo *match_info = NULL;
	GRegex *regex = g_regex_new(REGEXP_STRING, 0, 0, NULL);

	if (g_regex_match(regex, String, 0, &match_info))
	{
		while (g_match_info_matches(match_info))
		{
			gchar *template = g_match_info_fetch(match_info, 0);
			gchar *string = ReplaceTemplate(KeyFile, template);

			if (!string && gNoise)
				dprintf(gDebugFd, "ERR: Translation for %s not found!\n", template);
			else if (string)
			{
				if (gNoise && result[0] == '\0')
					dprintf(gDebugFd, "ERR: Translation for %s is empty!\n", template);

				result = ReplaceString(result, template, string);
			}

			g_free(string);
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

static gboolean ExecuteScript(gchar * script_name, gchar * verb, char *arg1, char *arg2, gchar **output, tProgressData *data)
{
	gchar *argv[4];
	gsize len, term;
	gchar *line = NULL;
	GPid pid;
	gint status = 0;
	gint stdout_fp, stderr_fp;
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

	envp = g_environ_setenv(g_get_environ(), "DC_TERMCMD_CLOSE", gTermCmdClose ? gTermCmdClose : "", TRUE);
	envp = g_environ_setenv(envp, "DC_TERMCMD_OPEN", gTermCmdOpen ? gTermCmdOpen : "", TRUE);
	GKeyFile *langs = OpenTranslations(script_name);

	if (langs)
	{
		gchar **items = g_key_file_get_keys(langs, "Default", NULL, NULL);

		if (items)
		{
			for (gsize i = 0; items[i] != NULL; i++)
			{
				if (strncmp(items[i], "ENV_WFX_", 8) == 0)
				{
					gchar *string = TranslateString(langs, items[i]);
					envp = g_environ_setenv(envp, items[i], string, TRUE);
					g_free(string);
				}
			}
		}

		g_strfreev(items);
		CloseTranslations(langs);
	}

	gchar **keys = g_key_file_get_keys(gCfg, script_name, NULL, NULL);

	if (keys)
	{
		for (gsize i = 0; keys[i] != NULL; i++)
		{
			if (strncmp(keys[i], OPT_ENVVAR, strlen(OPT_ENVVAR)) == 0)
			{
				gchar *env_data = g_key_file_get_string(gCfg, script_name, keys[i], NULL);

				if (env_data)
				{
					envp = g_environ_setenv(envp, keys[i] + strlen(OPT_ENVVAR), env_data, TRUE);
					g_free(env_data);
				}

			}
		}

		g_strfreev(keys);
	}

	if (gNoise)
	{
		dprintf(gDebugFd, "%s\n", EXEC_SEP);

		if (gExtraNoise)
		{
			for (gsize i = 0; envp[i] != NULL; i++)
				dprintf(gDebugFd, "%s\n", envp[i]);
		}

		dprintf(gDebugFd, "%s %s %s %s\n", script, verb, arg1 ? arg1 : "", arg2 ? arg2 : "");
		dprintf(gDebugFd, "%s\n", EXEC_SEP);
	}

	result = g_spawn_async_with_pipes(gScriptDir, argv, envp, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL, &stdout_fp, &stderr_fp, &err);
	g_free(script);
	g_strfreev(envp);
	gboolean is_logvisible = g_key_file_get_boolean(gCfg, script_name, OPT_CONNECT, NULL);

	if (err)
	{
		if (is_logvisible)
			gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);

		dprintf(gDebugFd, "ERR (g_spawn_async_with_pipes): %s\n", (err)->message);
	}
	else if (result)
	{
		gchar *pid_string = g_strdup_printf("%d;", (int)pid);
		gchar *pids = g_key_file_get_string(gCfg, script_name, MARK_PIDS, NULL);

		if (!pids)
			g_key_file_set_string(gCfg, script_name, MARK_PIDS, pid_string);
		else
		{
			gchar *string = g_strdup_printf("%s%s", pids, pid_string);
			g_key_file_set_string(gCfg, script_name, MARK_PIDS, string);
			g_free(string);

			if (gNoise && pids[0] != '\0')
				dprintf(gDebugFd, "INFO: another instance of the script is running (%s)\n", pids);

			g_free(pids);
		}

		if (data)
			*data->pid = (int)pid;

		GIOChannel *stdout_chan = g_io_channel_unix_new(stdout_fp);
		GString *lines = g_string_new(NULL);

		while (G_IO_STATUS_NORMAL == g_io_channel_read_line(stdout_chan, &line, &len, &term, &err))
		{
			if (line)
			{
				lines = g_string_append(lines, line);
				line[term] = '\0';

				if (gNoise)
					dprintf(gDebugFd, "STDOUT (%s [%d]]): %s\n", script_name, (int)pid, line);

				if (data)
				{
					gint64 ret = g_ascii_strtoll(line, NULL, 0);

					if (ret >= 0 || ret <= 100)
						*data->progress = (int)ret;
				}

				g_free(line);
			}
		}

		if (err)
		{
			if (is_logvisible)
				gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);

			dprintf(gDebugFd, "ERR (g_io_channel_read_line): %s\n", (err)->message);
			g_clear_error(&err);
		}

		GIOChannel *stderr_chan = g_io_channel_unix_new(stderr_fp);

		while (G_IO_STATUS_NORMAL == g_io_channel_read_line(stderr_chan, &line, &len, &term, &err))
		{
			if (line)
			{
				line[term] = '\0';

				if (is_logvisible)
					gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, line);

				if (gNoise || data)
					dprintf(gDebugFd, "STDERR (%s [%d]]): %s\n", script_name, (int)pid, line);

				g_free(line);
			}
		}

		if (err)
		{
			if (is_logvisible)
				gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);

			dprintf(gDebugFd, "ERR (g_io_channel_read_line): %s\n", (err)->message);
			g_clear_error(&err);
		}

		waitpid(pid, &status, 0);
		g_spawn_close_pid(pid);
		g_io_channel_shutdown(stdout_chan, TRUE, &err);

		if (err)
		{
			if (is_logvisible)
				gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);

			dprintf(gDebugFd, "ERR (g_io_channel_shutdown): %s\n", (err)->message);
			g_clear_error(&err);
		}

		g_io_channel_unref(stdout_chan);

		g_io_channel_shutdown(stderr_chan, TRUE, &err);

		if (err)
		{
			if (is_logvisible)
				gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, (err)->message);

			dprintf(gDebugFd, "ERR (g_io_channel_shutdown): %s\n", (err)->message);
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

		pids = g_key_file_get_string(gCfg, script_name, MARK_PIDS, NULL);

		pids = ReplaceString(pids, pid_string, "");

		if (pids)
			g_key_file_set_string(gCfg, script_name, MARK_PIDS, pids);
		else
			g_key_file_remove_key(gCfg, script_name, MARK_PIDS, NULL);

		g_free(pid_string);
	}

	if (gNoise)
		dprintf(gDebugFd, "INFO (%s [%d]]): exit status %d\n", script_name, (int)pid, WEXITSTATUS(status));

	if (err)
		g_error_free(err);

	if (status != 0)
		result = FALSE;

	return result;
}

static gpointer ExecFileOP(gpointer userdata)
{
	tProgressData *data = (tProgressData*)userdata;
	*data->result = ExecuteScript(data->script, data->verb, data->arg1, data->arg2, NULL, data);
	*data->is_running = FALSE;
	return NULL;
}

static gboolean RunFileOP(gchar *script_name, gchar *verb, char *arg1, char *arg2)
{
	tProgressData *data = g_new0(tProgressData, 1);
	int pid = 0;
	int progress = 0;
	gboolean result = FALSE;
	gboolean is_running = TRUE;
	data->script  = script_name;
	data->verb  = verb;
	data->arg1  = arg1;
	data->arg2  = arg2;
	data->pid  = &pid;
	data->progress  = &progress;
	data->result  = &result;
	data->is_running  = &is_running;

	GThread *thread = g_thread_new("FILEOP", ExecFileOP, data);

	while (is_running)
	{
		if (gProgressProc(gPluginNr, arg1, arg2, progress))
		{
			if (pid > 0)
				kill(pid, SIGTERM);

			break;
		}

		sleep(0.1);
	}

	g_thread_join(thread);
	g_thread_unref(thread);
	g_free(data);

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

	gchar *src_file = g_strdup_printf("%s/%s_readme[%s].txt", gScriptDir, file, gLang);

	if (!g_file_test(src_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		g_free(src_file);
		src_file = g_strdup_printf("%s/%s_readme.txt", gScriptDir, file);
	}

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

static gchar* BuildPictureData(char *FileName, char *LinePrefix, char *Class)
{
	unsigned char buff[32];
	struct stat buf;

	if (stat(FileName, &buf) != 0 || buf.st_size == 0 || buf.st_size > PICTURE_MAX_SIZE)
		return NULL;

	ssize_t len = strlen(Class);
	unsigned char class_size = (unsigned char)len;
	GString *data = g_string_new(NULL);
	g_string_append(data, LinePrefix);
	g_string_append_printf(data, "%02X", class_size);

	for (int i = 0; i < len; i++)
		g_string_append_printf(data, "%02X", Class[i]);

	unsigned char size[4];
	unsigned long s = (unsigned long)buf.st_size;
	size[3] = (s >> 24) & 0xFF;
	size[2] = (s >> 16) & 0xFF;
	size[1] = (s >> 8) & 0xFF;
	size[0] = s & 0xFF;

	for (int i = 0; i < 4; i++)
		g_string_append_printf(data, "%02X", size[i]);

	int fd = open(FileName, O_RDONLY);

	if (fd > -1)
	{
		int first_size = sizeof(buff) - (len + 5);

		if ((len = read(fd, buff, first_size)) > 0)
		{
			for (int i = 0; i < len; i++)
				g_string_append_printf(data, "%02X", buff[i]);
		}

		g_string_append(data, "\n");

		while ((len = read(fd, buff, sizeof(buff))) > 0)
		{
			g_string_append(data, LinePrefix);

			for (int i = 0; i < len; i++)
				g_string_append_printf(data, "%02X", buff[i]);

			g_string_append(data, "\n");
		}

		close(fd);
	}

	if (strlen(data->str) < strlen(LinePrefix) + strlen(Class) + 6)
	{
		g_string_free(data, TRUE);
		return NULL;
	}

	return g_string_free(data, FALSE);
}

static void AddPropLabels(GString *lfm_string)
{
	int i = 0;

	if (gProps && gProps[0] != '\0')
	{
		GKeyFile *langs = OpenTranslations(gScript);
		gchar **split = g_strsplit(gProps, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			if (strncmp(*p, MARK_NOISE, 3) != 0)
			{
				gchar **res = g_strsplit(*p, "\t", 2);

				if (!res)
					continue;

				if (!res[0] || !res[1])
				{
					g_strfreev(res);
					continue;
				}

				i++;

				if (g_strcmp0(res[0], "filetype") == 0 || g_strcmp0(res[0], "filetype") == 0 || g_strcmp0(res[0], "path") == 0 || g_strcmp0(res[0], "url") == 0)
					continue;

				if (strncmp(res[0], "png:", 4) == 0)
				{
					gchar *picture = NULL;
					const char label_body[] =  "      object lProp%d: TLabel\n"
					                           "        Left = 0\n"
					                           "        Height = 72\n"
					                           "        Top = 305\n"
					                           "        Width = 202\n"
					                           "        Alignment = taRightJustify\n"
					                           "        Caption = 'Label1'\n"
					                           "        Layout = tlCenter\n"
					                           "        ParentColor = False\n"
					                           "        ShowAccelChar = False\n"
					                           "        Visible = False\n"
					                           "        WordWrap = True\n"
					                           "      end\n"
					                           "      object iValue%d: TImage\n"
					                           "        Left = 212\n"
					                           "        Height = 72\n"
					                           "        Top = 305\n"
					                           "        Width = 295\n"
					                           "        AntialiasingMode = amOn\n"
					                           "        Constraints.MaxHeight = 256\n"
					                           "        Picture.Data = {\n"
					                           "        %s"
					                           "        }\n"
					                           "        Proportional = True\n"
					                           "        Stretch = True\n"
					                           "        Transparent = True\n"
					                           "        Visible = False\n"
					                           "      end\n";

					if (res[1][0] == '/')
						picture = BuildPictureData(res[1], "      ", "TPortableNetworkGraphic");

					/*else
					{
						GtkIconTheme *theme = gtk_icon_theme_get_default();
						GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon(theme, res[1], 48, 0);
						GdkPixbuf *pixbuf = gtk_icon_info_load_icon(icon_info, NULL);
						gtk_icon_info_free(icon_info);
						gchar *tmpdir = g_dir_make_tmp("gtkrecent_XXXXXX", NULL);
						gchar *img = g_strdup_printf("%s/image,png", tmpdir);
						gdk_pixbuf_save(pixbuf, img, "png", NULL, NULL);
						g_object_unref(pixbuf);
						picture = BuildPictureData(img, "      ", "TPortableNetworkGraphic");
						remove(img);
						remove(tmpdir);
						g_free(tmpdir);
						g_free(img);
					}
					*/
					if (picture)
					{
						g_string_append_printf(lfm_string, label_body, i, i, picture);
						g_free(picture);
					}
				}
				else
				{
					const char label_body[] =  "      object lProp%d: TLabel\n"
					                           "        Left = 0\n"
					                           "        Height = 17\n"
					                           "        Top = 129\n"
					                           "        Width = 202\n"
					                           "        Alignment = taRightJustify\n"
					                           "        Caption = ''\n"
					                           "        ParentColor = False\n"
					                           "        ParentFont = False\n"
					                           "        ShowAccelChar = False\n"
					                           "        Visible = False\n"
					                           "        WordWrap = True\n"
					                           "      end\n"
					                           "      object lValue%d: TLabel\n"
					                           "        Left = 212\n"
					                           "        Height = 17\n"
					                           "        Top = 129\n"
					                           "        Width = 295\n"
					                           "        Caption = ''\n"
					                           "        ParentColor = False\n"
					                           "        ParentFont = False\n"
					                           "        ShowAccelChar = False\n"
					                           "        Visible = False\n"
					                           "        WordWrap = True\n"
					                           "      end\n";

					g_string_append_printf(lfm_string, label_body, i, i);
				}
			}
		}

		g_strfreev(split);
		CloseTranslations(langs);
	}
}

static void StoreEnvVar(char *line, char *script)
{
	GMatchInfo *match_info = NULL;
	GRegex *regex = g_regex_new(REGEXP_ENVVAAR, 0, 0, NULL);

	if (g_regex_match(regex, line, 0, &match_info))
	{
		if (g_match_info_matches(match_info))
		{
			gchar *key = g_match_info_fetch(match_info, 1);

			if (key)
			{
				gchar *envvar = key + strlen(OPT_ENVVAR);

				if (strcmp(envvar, ENVVAR_REMOTENAME) == 0)
					dprintf(gDebugFd, "ERR (%s): " ENVVAR_REMOTENAME " is reserved, ignored.\n", script);
				else if (strcmp(envvar, ENVVAR_MULTIFILEOP) == 0)
					dprintf(gDebugFd, "ERR (%s): " ENVVAR_MULTIFILEOP " is reserved, ignored.\n", script);
				else if (strncmp(envvar, "ENV_WFX_SCRIPT_STR_", 19) == 0)
					dprintf(gDebugFd, "ERR (%s): ENV_WFX_SCRIPT_STR_* is reserved, ignored.\n", script);
				else
				{
					gchar *value = STRIP_OPT(line, key);

					if (value[0] == '\0')
					{
						g_key_file_remove_key(gCfg, script, key, NULL);

						if (gNoise)
							dprintf(gDebugFd, "INFO (%s): $%s value will be unset.\n", script, envvar);
					}
					else
					{
						g_key_file_set_string(gCfg, script, key, value);

						if (gNoise)
							dprintf(gDebugFd, "INFO (%s): $%s value will be set to \"%s\".\n", script, envvar, value);
					}
				}

				g_free(key);
			}
			else
				dprintf(gDebugFd, "ERR (%s): Failed to fetch envvar name.\n", script);
		}
	}

	if (match_info)
		g_match_info_free(match_info);

	if (regex)
		g_regex_unref(regex);
}

static void FillProps(uintptr_t pDlg)
{
	int i = 0;

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

					SendDlgMsg(pDlg, "lblAct", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "cbAct", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "btnAct", DM_SHOWITEM, 1, 0);
				}
			}
			else if (IsValidOpt(*p, OPT_ENVVAR))
			{
				StoreEnvVar(*p, gScript);
			}
			else
			{
				gchar **res = g_strsplit(*p, "\t", 2);

				if (!res)
					continue;

				if (!res[0] || !res[1])
				{
					g_strfreev(res);
					continue;
				}

				i++;

				if (g_ascii_strcasecmp(res[0], "filetype") == 0)
				{
					gchar *value = TranslateString(langs, g_strstrip(res[1]));
					SendDlgMsg(pDlg, "lType", DM_SETTEXT, (intptr_t)value, 0);
					g_free(value);
				}
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
				else if (strncmp(res[0], "png:", 4) == 0 && strlen(res[0]) > 4)
				{
					gchar *prop = TranslateString(langs, g_strstrip(res[0] + 4));
					gchar *item = g_strdup_printf("lProp%d", i);
					SendDlgMsg(pDlg, item, DM_SETTEXT, (intptr_t)prop, 0);
					SendDlgMsg(pDlg, item, DM_SHOWITEM, 1, 0);
					g_free(item);
					item = g_strdup_printf("iValue%d", i);
					SendDlgMsg(pDlg, item, DM_SHOWITEM, 1, 0);
					g_free(item);
					g_free(prop);

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
			gLogProc(gPluginNr, MSGTYPE_DETAILS, "CryptProc: Success");

		break;

	case FS_FILE_NOTSUPPORTED:
		gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc: Encrypt/Decrypt failed");
		break;

	case FS_FILE_WRITEERROR:
		gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, "CryptProc: Could not write password to password store");
		break;

	case FS_FILE_READERROR:
		if (gNoise)
			gLogProc(gPluginNr, MSGTYPE_DETAILS, "CryptProc: Password not found in password store");

		break;

	case FS_FILE_NOTFOUND:
		if (gNoise)
			gLogProc(gPluginNr, MSGTYPE_DETAILS, "CryptProc: No master password entered yet");

		break;
	}
}

static void ParseOpts(gchar *script, gchar *text, gboolean thread);

static void SetOpt(gchar *script, gchar *opt, gchar *value)
{
	gchar *output = NULL;
	ExecuteScript(script, VERB_SETOPT, opt, value, &output, NULL);

	if (output && output[0] != '\0')
		ParseOpts(script, output, FALSE);

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

static void DlgRequestSetOption(uintptr_t pDlg)
{
	gchar *output = NULL;
	char *text = g_strdup((char*)SendDlgMsg(pDlg, "mOption", DM_LISTGETITEM, 0, 0));
	char *res = g_strdup((char*)SendDlgMsg(pDlg, "edValue", DM_GETTEXT, 0, 0));
	SendDlgMsg(pDlg, "edValue", DM_SHOWDIALOG, 0, 0);

	if (res)
	{
		SaveHistory(pDlg, text, res);
		ExecuteScript(gScript, VERB_SETOPT, text, res, &output, NULL);

		if (output && output[0] != '\0')
			ParseOpts(gScript, output, FALSE);

		g_free(output);
	}

	gchar *mark = g_strdup_printf(OPT_PUSH "_%s", text);

	if (g_key_file_has_key(gCfg, gScript, mark, NULL))
		g_key_file_remove_key(gCfg, gScript, mark, NULL);

	g_free(mark);

	g_free(res);
	g_free(text);
	SendDlgMsg(pDlg, "edValue", DM_CLOSE, 1, 0);
}

intptr_t DCPCALL DlgRequestValueProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
	{
		if (gDialogApi->VersionAPI > 1 && gCaption)
			SendDlgMsg(pDlg, NULL, DM_SETTEXT, (intptr_t)gCaption, 0);

		char *text = (char*)SendDlgMsg(pDlg, "mOption", DM_LISTGETITEM, 0, 0);
		gchar *key = g_strdup_printf("%s_%s_0", MARK_PREV, text);
		gboolean history = g_key_file_has_key(gCfg, gScript, key, NULL);
		char *value = g_strdup((char*)SendDlgMsg(pDlg, "edValue", DM_GETTEXT, 0, 0));

		if (value && value[0] != '\0')
			SendDlgMsg(pDlg, "lbHistory", DM_LISTADDSTR, (intptr_t)value, 0);

		if (history)
		{
			SendDlgMsg(pDlg, "lbHistory", DM_SHOWITEM, (int)history, 0);

			int num = 0;

			do
			{
				g_free(key);
				key = g_strdup_printf("%s_%s_%d", MARK_PREV, text, num++);
				gchar *oldvalue = g_key_file_get_string(gCfg, gScript, key, NULL);

				if (oldvalue && (!value || strcmp(oldvalue, value) != 0))
					SendDlgMsg(pDlg, "lbHistory", DM_LISTADDSTR, (intptr_t)oldvalue, 0);

				g_free(oldvalue);
			}
			while (g_key_file_has_key(gCfg, gScript, key, NULL));

		}

		g_free(key);
		g_free(value);
	}

	break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
			DlgRequestSetOption(pDlg);
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

	case DN_DBLCLICK:
		if (strcmp(DlgItemName, "lbHistory") == 0)
			DlgRequestSetOption(pDlg);

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

				gchar *text = g_strdup((char*)SendDlgMsg(pDlg, "mOption", DM_LISTGETITEM, 0, 0));
				SaveHistory(pDlg, text, value);
				g_free(value);
				g_free(text);
			}
		}

		break;
	}

	return 0;
}

static gboolean IsCanShowDlg(char *text, gboolean request_once)
{
	if (strncmp(text, MARK_NOISE, 3) == 0)
	{
		dprintf(gDebugFd, "ERR: Options starting with \"Fs_\" are reserved, \"%s\" ignored,\n", text);
		return FALSE;
	}

	if (!gDialogApi)
	{
		dprintf(gDebugFd, "ERR: DialogApi not initialized,\n");
		return FALSE;
	}

	if (request_once)
	{
		gchar **res = g_strsplit(text, "\t", -1);

		if (res)
		{
			if (res[0])
			{
				gchar *mark = g_strdup_printf(OPT_PUSH "_%s", res[0]);

				if (!g_key_file_has_key(gCfg, gScript, mark, NULL) && g_key_file_has_key(gCfg, gScript, res[0], NULL))
				{
					gchar *value = g_key_file_get_string(gCfg, gScript, res[0], NULL);
					SetOpt(gScript, res[0], value);
					g_free(mark);
					g_free(value);
					g_strfreev(res);
					return FALSE;
				}

				g_free(mark);
			}

			g_strfreev(res);
		}
	}

	return TRUE;
}

static gchar *BuildRequestLFMData(const char *lfmdata_templ, char *text)
{
	GKeyFile *langs = OpenTranslations(gScript);
	gchar *string = TranslateString(langs, text);
	CloseTranslations(langs);
	string = ReplaceString(string, "'", "''");
	gchar *prev = g_key_file_get_string(gCfg, gScript, text, NULL);
	prev = ReplaceString(prev, "'", "''");
	gchar *lfmdata = g_strdup_printf(lfmdata_templ, text, string, prev ? prev : "");
	g_free(prev);
	g_free(string);

	return lfmdata;
}

static void ShowSelectFileDlg(char *text, gboolean request_once)
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
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object mOption: TMemo\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideBottom.Control = Owner\n"
	                       "    AnchorSideBottom.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 189\n"
	                       "    Width = 21\n"
	                       "    Anchors = [akLeft, akBottom]\n"
	                       "    TabStop = False\n"
	                       "    TabOrder = 3\n"
	                       "    Lines.Strings = (\n"
	                       "      '%s'\n"
	                       "    )\n"
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
	                       "    OnDblClick = ListBoxDblClick\n"
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
	                       "    DefaultExt = '%s'\n"
	                       "    DialogKind = %s\n"
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

	if (!IsCanShowDlg(text, request_once))
		return;

	gchar **split = g_strsplit(text, "\t", -1);
	guint len = g_strv_length(split);

	if (len > 0)
	{
		gchar *filer = NULL;
		gchar *ext = NULL;
		gboolean is_opendlg = TRUE;
		GKeyFile *langs = OpenTranslations(gScript);
		gchar *string = TranslateString(langs, split[0]);

		if (split[1] && strlen(split[1]) > 0)
			filer = TranslateString(langs, split[1]);

		CloseTranslations(langs);

		if (g_strv_length(split) > 2)
		{
			for (guint i = 2; i < len; i++)
			{
				if (strlen(split[i]) > 0)
				{
					if (g_ascii_strcasecmp(split[i], "save") == 0)
						is_opendlg = FALSE;
					else
						ext = split[i];
				}
			}
		}

		gchar *prev = g_key_file_get_string(gCfg, gScript, split[0], NULL);
		filer = ReplaceString(filer, "'", "''");
		string = ReplaceString(string, "'", "''");
		prev = ReplaceString(prev, "'", "''");
		gchar *opt = ReplaceString(g_strdup(split[0]), "'", "''");

		gchar *lfmdata = g_strdup_printf(lfmdata_templ, opt, string, ext ? ext : "", is_opendlg ? "dkOpen" : "dkSave", filer ? filer : "*.*|*.*", prev ? prev : "");
		g_free(prev);
		g_free(string);
		g_free(filer);

		gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgRequestValueProc);
		g_free(lfmdata);
	}

	if (split)
		g_strfreev(split);
}

static void ShowSelectDirDlg(char *text, gboolean request_once)
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
	                             "  OnShow = DialogBoxShow\n"
	                             "  Position = poOwnerFormCenter\n"
	                             "  LCLVersion = '2.2.4.0'\n"
	                             "  object mOption: TMemo\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideBottom.Control = Owner\n"
	                             "    AnchorSideBottom.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 17\n"
	                             "    Top = 189\n"
	                             "    Width = 21\n"
	                             "    Anchors = [akLeft, akBottom]\n"
	                             "    TabStop = False\n"
	                             "    TabOrder = 4\n"
	                             "    Lines.Strings = (\n"
	                             "      '%s'\n"
	                             "    )\n"
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
	                             "    OnDblClick = ListBoxDblClick\n"
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

	if (!IsCanShowDlg(text, request_once))
		return;

	gchar *lfmdata = BuildRequestLFMData(lfmdata_templ, text);

	gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgRequestValueProc);
	g_free(lfmdata);
}

static void ShowRequestValueDlg(char *text, gboolean request_once)
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
	                             "  OnShow = DialogBoxShow\n"
	                             "  Position = poOwnerFormCenter\n"
	                             "  LCLVersion = '2.2.4.0'\n"
	                             "  object mOption: TMemo\n"
	                             "    AnchorSideLeft.Control = Owner\n"
	                             "    AnchorSideBottom.Control = Owner\n"
	                             "    AnchorSideBottom.Side = asrBottom\n"
	                             "    Left = 10\n"
	                             "    Height = 17\n"
	                             "    Top = 189\n"
	                             "    Width = 21\n"
	                             "    Anchors = [akLeft, akBottom]\n"
	                             "    TabStop = False\n"
	                             "    TabOrder = 4\n"
	                             "    Lines.Strings = (\n"
	                             "      '%s'\n"
	                             "    )\n"
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
	                             "    OnDblClick = ListBoxDblClick\n"
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

	if (!IsCanShowDlg(text, request_once))
		return;

	gchar *lfmdata = BuildRequestLFMData(lfmdata_templ, text);
	gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgRequestValueProc);
	g_free(lfmdata);
}

intptr_t DCPCALL DlgTextOutputProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG && gMemoText != NULL)
	{
		gchar **split = g_strsplit(gMemoText, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			SendDlgMsg(pDlg, "mText", DM_LISTADDSTR, (intptr_t)*p, 0);
		}

		g_strfreev(split);
		g_free(gMemoText);
		gMemoText = NULL;
	}

	return 0;
}

static void ShowTextOutput(gboolean Log)
{
	const char lfmdata_templ[] = ""
	                             "object MemoDialog: TMemoDialog\n"
	                             "  Left = 356\n"
	                             "  Height = 480\n"
	                             "  Top = 162\n"
	                             "  Width = 640\n"
	                             "  AutoSize = True\n"
	                             "  BorderStyle = bsDialog\n"
	                             "  ChildSizing.LeftRightSpacing = 10\n"
	                             "  ChildSizing.TopBottomSpacing = 10\n"
	                             "  ChildSizing.VerticalSpacing = 20\n"
	                             "  ClientHeight = 480\n"
	                             "  Caption = 'View'\n"
	                             "  ClientWidth = 640\n"
	                             "  OnShow = DialogBoxShow\n"
	                             "  Position = poOwnerFormCenter\n"
	                             "  LCLVersion = '2.2.0.3'\n"
	                             "  object mText: TMemo\n"
	                             "    Left = 8\n"
	                             "    Height = 400\n"
	                             "    Top = 8\n"
	                             "    Width = 620\n"
	                             "    Font.Name = '%s'\n"
	                             "    ReadOnly = True\n"
	                             "    ScrollBars = ssAutoBoth\n"
	                             "    TabOrder = 0\n"
	                             "    WordWrap = False\n"
	                             "  end\n"
	                             "  object btnClose: TBitBtn\n"
	                             "    AnchorSideTop.Control = mText\n"
	                             "    AnchorSideTop.Side = asrBottom\n"
	                             "    AnchorSideRight.Control = mText\n"
	                             "    AnchorSideRight.Side = asrBottom\n"
	                             "    Left = 528\n"
	                             "    Height = 30\n"
	                             "    Top = 428\n"
	                             "    Width = 100\n"
	                             "    Anchors = [akTop, akRight]\n"
	                             "    AutoSize = True\n"
	                             "    Cancel = True\n"
	                             "    Constraints.MinHeight = 30\n"
	                             "    Constraints.MinWidth = 97\n"
	                             "    Default = True\n"
	                             "    DefaultCaption = True\n"
	                             "    Kind = bkClose\n"
	                             "    ModalResult = 11\n"
	                             "    OnClick = ButtonClick\n"
	                             "    TabOrder = 1\n"
	                             "  end\n"
	                             "end\n";

	gchar *lfmdata = g_strdup_printf(lfmdata_templ, gViewerFont ? gViewerFont : "Monospace");

	gDialogApi->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), DlgTextOutputProc);

	g_free(lfmdata);
}

static void DlgMultiChoiceUpdLabel(uintptr_t pDlg)
{
	int i = SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEMINDEX, 0, 0);

	if (i != -1)
	{
		char *text = (char*)SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEM, i, 0);
		SendDlgMsg(pDlg, "lblChoice", DM_SETTEXT, (intptr_t)text, 0);
	}
}

static void DlgMultiChoiceSetOption(uintptr_t pDlg)
{
	gchar *output = NULL;
	char *text = g_strdup((char*)SendDlgMsg(pDlg, "lbDataStore", DM_LISTGETITEM, 0, 0));
	int i = SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEMINDEX, 0, 0);
	char *res = g_strdup((char*)SendDlgMsg(pDlg, "lbDataStore", DM_LISTGETITEM, ++i, 0));
	g_key_file_set_string(gCfg, gScript, text, res);
	SendDlgMsg(pDlg, "lbChoice", DM_SHOWDIALOG, 0, 0);

	if (res && res[0] != '\0')
	{
		ExecuteScript(gScript, VERB_SETOPT, text, res, &output, NULL);

		if (output && output[0] != '\0')
			ParseOpts(gScript, output, FALSE);

		g_free(output);
	}
	else
		dprintf(gDebugFd, "ERR: empty argument selected.\n");

	gchar *mark = g_strdup_printf(OPT_PUSH "_%s", text);

	if (g_key_file_has_key(gCfg, gScript, mark, NULL))
		g_key_file_remove_key(gCfg, gScript, mark, NULL);

	g_free(mark);

	g_free(text);
	g_free(res);
	SendDlgMsg(pDlg, "lbChoice", DM_CLOSE, 1, 0);
}

static void DlgListSearch(uintptr_t pDlg, char *text, int start, int end, int inc)
{
	gboolean is_found = FALSE;

	for (int i = start; i != end; i += inc)
	{
		char *item = (char*)SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEM, i, 0);

		if (strcasestr(item, text) != NULL)
		{
			SendDlgMsg(pDlg, "lbChoice", DM_LISTSETITEMINDEX, i, 0);
			is_found = TRUE;
			break;
		}
	}

	if (is_found)
	{
		if (inc > 0)
			SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 1, 0);
		else
			SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 1, 0);
	}
	else
	{
		if (inc > 0)
			SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 0, 0);
		else
			SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 0, 0);
	}
}

static void DlgListStartSearch(uintptr_t pDlg, intptr_t wParam)
{
	char *text = (char*)wParam;
	SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 0, 0);
	SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 0, 0);
	SendDlgMsg(pDlg, "lbChoice", DM_LISTSETITEMINDEX, 0, 0);

	if (text[0] != '\0')
	{
		int count = SendDlgMsg(pDlg, "lbChoice", DM_LISTGETCOUNT, 0, 0);
		DlgListSearch(pDlg, text, 0, count - 1, 1);
	}
}

static void DlgListSearchUp(uintptr_t pDlg)
{
	int i = SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEMINDEX, 0, 0);

	if (i > 0)
	{
		SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 1, 0);
		char *text = g_strdup((char*)SendDlgMsg(pDlg, "edSearch", DM_GETTEXT, 0, 0));
		DlgListSearch(pDlg, text, i - 1, -1, -1);
		g_free(text);
	}
	else
		SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 0, 0);
}

static void DlgListSearchDown(uintptr_t pDlg)
{
	int i = SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEMINDEX, 0, 0);
	int count = SendDlgMsg(pDlg, "lbChoice", DM_LISTGETCOUNT, 0, 0);

	if (i < count - 1)
	{
		SendDlgMsg(pDlg, "btnUp", DM_ENABLE, 1, 0);
		char *text = g_strdup((char*)SendDlgMsg(pDlg, "edSearch", DM_GETTEXT, 0, 0));
		DlgListSearch(pDlg, text, i + 1, count, 1);
		g_free(text);
	}
	else
		SendDlgMsg(pDlg, "btnDown", DM_ENABLE, 0, 0);
}

intptr_t DCPCALL DlgMultiChoiceProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		if (gDialogApi->VersionAPI > 1 && gCaption)
			SendDlgMsg(pDlg, NULL, DM_SETTEXT, (intptr_t)gCaption, 0);

		if (gChoice)
		{
			gsize count = 0;
			GKeyFile *langs = OpenTranslations(gScript);
			gchar *string = TranslateString(langs, gChoice[0]);
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
						SendDlgMsg(pDlg, "lbChoice", DM_LISTADDSTR, (intptr_t)string, 0);
						SendDlgMsg(pDlg, "lbDataStore", DM_LISTADDSTR, (intptr_t)*p, 0);
						g_free(string);

						if (prev_choice && g_strcmp0(*p, prev_choice) == 0)
							SendDlgMsg(pDlg, "lbChoice", DM_LISTSETITEMINDEX, (intptr_t)count, 0);

						count++;
					}
				}

				if (gNoise && count == 1)
					dprintf(gDebugFd, "ERR: " OPT_CHOICE " - there is no choice.\n");
				else if (count < 1)
					dprintf(gDebugFd, "ERR: " OPT_CHOICE " - there is no choice.\n");
				else if (count > 10)
				{
					SendDlgMsg(pDlg, "edSearch", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "btnUp", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "btnDown", DM_SHOWITEM, 1, 0);
				}

				int index = (int)SendDlgMsg(pDlg, "lbChoice", DM_LISTGETITEMINDEX, 0, 0);

				if (index == -1)
					SendDlgMsg(pDlg, "lbChoice", DM_LISTSETITEMINDEX, 0, 0);

				DlgMultiChoiceUpdLabel(pDlg);
				g_strfreev(gChoice);
				gChoice = NULL;
			}
			else
				dprintf(gDebugFd, "ERR: " OPT_CHOICE " - there is no choice.\n");

			g_free(prev_choice);
			CloseTranslations(langs);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
			DlgMultiChoiceSetOption(pDlg);
		else if (strcmp(DlgItemName, "lbChoice") == 0)
			SendDlgMsg(pDlg, "edSearch", DM_SETTEXT, 0, 0);
		else if (strcmp(DlgItemName, "btnUp") == 0)
			DlgListSearchUp(pDlg);
		else if (strcmp(DlgItemName, "btnDown") == 0)
			DlgListSearchDown(pDlg);

		break;

	case DN_DBLCLICK:
		if (strcmp(DlgItemName, "lbChoice") == 0)
			DlgMultiChoiceSetOption(pDlg);

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "lbChoice") == 0)
			DlgMultiChoiceUpdLabel(pDlg);
		else if (strcmp(DlgItemName, "edSearch") == 0)
			DlgListStartSearch(pDlg, wParam);

		break;

	case DN_KEYDOWN:
		if (strcmp(DlgItemName, "edSearch") == 0)
		{
			int16_t *key = (int16_t*)wParam;

			if (*key == 38)
				DlgListSearchUp(pDlg);
			else if (*key == 40)
				DlgListSearchDown(pDlg);
		}
	}

	return 0;
}

static void ShowMultiChoiceDlg(char *text, gboolean request_once)
{
	if (!IsCanShowDlg(text, request_once))
		return;

	GString *lfm_string = g_string_new(NULL);
	g_string_append(lfm_string, "object MultichoiceDialogBox: TMultichoiceDialogBox\n"
	                "  Left = 329\n"
	                "  Height = 243\n"
	                "  Top = 107\n"
	                "  Width = 519\n"
	                "  AutoSize = True\n"
	                "  BorderStyle = bsDialog\n"
	                "  Caption = 'Confirmation of parameter'\n"
	                "  ChildSizing.LeftRightSpacing = 10\n"
	                "  ChildSizing.TopBottomSpacing = 10\n"
	                "  ClientHeight = 243\n"
	                "  ClientWidth = 519\n"
	                "  OnShow = DialogBoxShow\n"
	                "  Position = poOwnerFormCenter\n"
	                "  LCLVersion = '3.0.0.3'\n"
	                "  object lblText: TLabel\n"
	                "    AnchorSideLeft.Control = Owner\n"
	                "    AnchorSideTop.Control = Owner\n"
	                "    AnchorSideRight.Control = lbChoice\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    Left = 10\n"
	                "    Height = 1\n"
	                "    Top = 10\n"
	                "    Width = 500\n"
	                "    Anchors = [akTop, akLeft, akRight]\n"
	                "    BorderSpacing.Left = 10\n"
	                "    BorderSpacing.Top = 10\n"
	                "    WordWrap = True\n"
	                "  end\n"
	                "  object lbChoice: TListBox\n"
	                "    AnchorSideLeft.Control = Owner\n"
	                "    AnchorSideTop.Control = lblText\n"
	                "    AnchorSideTop.Side = asrBottom\n"
	                "    Left = 10\n"
	                "    Height = 92\n"
	                "    Top = 21\n"
	                "    Width = 500\n"
	                "    BorderSpacing.Top = 10\n"
	                "    ItemHeight = 0\n"
	                "    TabOrder = 0\n"
	                "    TopIndex = -1\n"
	                "    OnClick = ListBoxClick\n"
	                "    OnDblClick = ListBoxDblClick\n"
	                "    OnSelectionChange = ListBoxSelectionChange\n"
	                "  end\n");

	g_string_append(lfm_string, "  object edSearch: TEdit\n"
	                "    AnchorSideLeft.Control = lbChoice\n"
	                "    AnchorSideTop.Control = lbChoice\n"
	                "    AnchorSideTop.Side = asrBottom\n"
	                "    AnchorSideRight.Control = btnUp\n"
	                "    Left = 10\n"
	                "    Height = 28\n"
	                "    Top = 118\n"
	                "    Width = 448\n"
	                "    Anchors = [akTop, akLeft, akRight]\n"
	                "    BorderSpacing.Top = 5\n"
	                "    TabOrder = 1\n"
	                "    OnChange = EditChange\n"
	                "    OnKeyDown = EditKeyDown\n"
	                "    Visible = False\n"
	                "  end\n"
	                "  object btnUp: TBitBtn\n"
	                "    AnchorSideLeft.Side = asrBottom\n"
	                "    AnchorSideTop.Control = edSearch\n"
	                "    AnchorSideRight.Control = btnDown\n"
	                "    AnchorSideBottom.Control = edSearch\n"
	                "    AnchorSideBottom.Side = asrBottom\n"
	                "    Left = 458\n"
	                "    Height = 28\n"
	                "    Top = 118\n"
	                "    Width = 26\n"
	                "    Anchors = [akTop, akRight, akBottom]\n"
	                "    AutoSize = True\n"
	                "    Enabled = False\n"
	                "    Glyph.Data = {\n"
	                "      36040000424D3604000000000000360000002800000010000000100000000100\n"
	                "      2000000000000004000064000000640000000000000000000000FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004743AE804733AFF04733AFF04733AFF04733AFF0473\n"
	                "      3AFF04733AFF04743BE8FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFA5DFC1FF81D5AAFF81D7ABFF82D9ACFF82DA\n"
	                "      ADFF82D9ADFF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFA9E2C5FF16B665FF0FBA63FF10BE66FF11C1\n"
	                "      67FF83DEB0FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFADE4C8FF1FBD6DFF10BF66FF12C56AFF13C9\n"
	                "      6DFF84E3B3FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFB0E5CAFF31C378FF11C067FF13C86CFF15CF\n"
	                "      71FF85E5B4FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF0004753BC50473\n"
	                "      3AFF04733AFF04733AFF04783CF7B2E6CBFF4BC989FF10BD65FF11C369FF12C7\n"
	                "      6BFF84E1B2FF057A3EF704733AFF04733AFF04733AFF04753BC504733A65288B\n"
	                "      58F6B5E0CAFFBCE5D0FFB8E5CEFFB3E5CBFF63CD97FF19BA68FF0FBC64FF10BE\n"
	                "      65FF83DCAFFF82DBAEFF82D9ACFF7BD3A6FF18844CF504733A65FFFFFF000474\n"
	                "      3BAC56A97FF9B0DFC7FF77CBA0FF6CCA9AFF63CA96FF43C181FF0DB35EFF0DB5\n"
	                "      5FFF0DB45FFF0FB25FFF6ACE9BFF359C67F804753BABFFFFFF00FFFFFF000473\n"
	                "      3A0C04743AE180C4A1FE97D5B5FF69C596FF60C591FF56C38BFF1DB066FF0BAC\n"
	                "      59FF0BAB59FF4DC185FF55B484FE04753BE104733A0CFFFFFF00FFFFFF00FFFF\n"
	                "      FF0004733A2D0D7840F69CD4B7FF7AC9A0FF5ABE8BFF51BC86FF38B474FF08A3\n"
	                "      53FF2CAF6CFF6EC397FF0A773EF604733A2DFFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF0004733A65288A57F6A0D7BBFF5FBD8CFF4DB680FF44B37AFF1AA2\n"
	                "      5CFF71C49AFF1D844FF504733A65FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004743BAC4EA578FA91D2B0FF48B47DFF3FB176FF6AC2\n"
	                "      95FF3B9C6AF904743AABFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733A0C04743AE172BD96FE72C59AFF6AC295FF5BB3\n"
	                "      86FE04743AE104733A0CFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF0004733A2D0F7941F687CAA7FF7EC7A2FF0E79\n"
	                "      41F604733A2DFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF0004733A65288957F5268956F50473\n"
	                "      3A65FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF0004753B9D04753B9DFFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00\n"
	                "    }\n"
	                "    OnClick = ButtonClick\n"
	                "    TabOrder = 2\n"
	                "    Visible = False\n"
	                "  end\n");

	g_string_append(lfm_string, "  object btnDown: TBitBtn\n"
	                "    AnchorSideTop.Control = edSearch\n"
	                "    AnchorSideRight.Control = lbChoice\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    AnchorSideBottom.Control = edSearch\n"
	                "    AnchorSideBottom.Side = asrBottom\n"
	                "    Left = 484\n"
	                "    Height = 28\n"
	                "    Top = 118\n"
	                "    Width = 26\n"
	                "    Anchors = [akTop, akRight, akBottom]\n"
	                "    AutoSize = True\n"
	                "    Enabled = False\n"
	                "    Glyph.Data = {\n"
	                "      36040000424D3604000000000000360000002800000010000000100000000100\n"
	                "      2000000000000004000064000000640000000000000000000000FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF0004763C9D04763C9DFFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF00FFFFFF0004733A65248B56F5248B56F50473\n"
	                "      3A65FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF00FFFFFF0004733A2D0E7A42F674D1A2FF75D3A3FF0E7B\n"
	                "      42F604733A2DFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733A0C04763BE15DBF8DFE4CD18DFF4ED590FF5FC5\n"
	                "      92FE04763CE104733A0CFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004753BAC4CAA7AFA6DD6A0FF12C469FF14CB6EFF6CE0\n"
	                "      A5FF3FA872F904763CABFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF0004733A65268C57F594DBB7FF31C379FF11C067FF12C56AFF1FC9\n"
	                "      73FF78DBA8FF1E8952F504733A65FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF0004733A2D0D7840F690D3B1FF65CC98FF3BC27DFF0FB962FF0FBC65FF10BD\n"
	                "      65FF32C57BFF71CE9FFF0A783FF604733A2DFFFFFF00FFFFFF00FFFFFF000473\n"
	                "      3A0C04743AE175BF99FE83D1A9FF52C289FF4DC387FF1EB669FF0DB45FFF0DB4\n"
	                "      5FFF0DB35EFF4EC589FF55B784FE04753BE104733A0CFFFFFF00FFFFFF000474\n"
	                "      3BAC4FA57AF89BD6B8FF5DBE8DFF58C08AFF51C087FF42BC7EFF0CAB5AFF0BAB\n"
	                "      59FF0AAA59FF0CA958FF68C797FF349A65F804743BABFFFFFF0004733A652789\n"
	                "      56F6AADBC2FFAFDEC6FFADDDC5FFAADCC2FF56BD88FF4DBA82FF1FAA63FF08A2\n"
	                "      53FF7FCEA5FF7ECDA4FF7ECCA4FF78C79FFF18824BF504733A6504753BC50473\n"
	                "      3AFF04733AFF04733AFF04763BF7ADDDC5FF59BC89FF50B882FF34AD6FFF069A\n"
	                "      4EFF7ECAA3FF04763BF704733AFF04733AFF04733AFF04753BC5FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFB0DEC7FF5DBD8CFF54B985FF47B47CFF069A\n"
	                "      4EFF7ECAA3FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFB1DFC7FF5FBE8DFF55BA86FF41B278FF069A\n"
	                "      4EFF7ECAA3FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFB1DFC8FF5FBE8DFF55BA86FF37AE71FF069A\n"
	                "      4EFF7ECAA3FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004733AFFB0DEC7FFABDCC3FFA6DABFFF91D2B1FF7ECA\n"
	                "      A3FF7ECAA3FF04733AFFFFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFFFF00FFFF\n"
	                "      FF00FFFFFF00FFFFFF0004743AE804733AFF04733AFF04733AFF04733AFF0473\n"
	                "      3AFF04733AFF04743AE8FFFFFF00FFFFFF00FFFFFF00FFFFFF00\n"
	                "    }\n"
	                "    OnClick = ButtonClick\n"
	                "    TabOrder = 3\n"
	                "    Visible = False\n"
	                "  end\n");

	g_string_append(lfm_string, "  object lblChoice: TMemo\n"
	                "    Alignment = taCenter\n"
	                "    AnchorSideLeft.Control = lbChoice\n"
	                "    AnchorSideTop.Control = edSearch\n"
	                "    AnchorSideTop.Side = asrBottom\n"
	                "    AnchorSideRight.Control = lbChoice\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    Left = 10\n"
	                "    Height = 42\n"
	                "    Top = 151\n"
	                "    Width = 500\n"
	                "    Anchors = [akTop, akLeft, akRight]\n"
	                "    BorderSpacing.Top = 10\n"
	                "    BorderStyle = bsNone\n"
	                "    Color = clForm\n"
	                "    ReadOnly = True\n"
	                "    ScrollBars = ssAutoVertical\n"
	                "    TabOrder = 4\n"
	                "    TabStop = False\n"
	                "  end\n"
	                "  object lbDataStore: TListBox\n"
	                "    AnchorSideLeft.Control = Owner\n"
	                "    AnchorSideTop.Control = btnCancel\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    AnchorSideBottom.Side = asrBottom\n"
	                "    Left = 10\n"
	                "    Height = 27\n"
	                "    Top = 203\n"
	                "    Width = 164\n"
	                "    ItemHeight = 0\n"
	                "    TabOrder = 5\n"
	                "    TabStop = False\n"
	                "    TopIndex = -1\n"
	                "    Visible = False\n"
	                "  end\n"
	                "  object btnCancel: TBitBtn\n"
	                "    AnchorSideTop.Control = btnOK\n"
	                "    AnchorSideRight.Control = btnOK\n"
	                "    Left = 306\n"
	                "    Height = 30\n"
	                "    Top = 203\n"
	                "    Width = 97\n"
	                "    Anchors = [akTop, akRight]\n"
	                "    AutoSize = True\n"
	                "    BorderSpacing.Right = 10\n"
	                "    Cancel = True\n"
	                "    Constraints.MinHeight = 30\n"
	                "    Constraints.MinWidth = 97\n"
	                "    DefaultCaption = True\n"
	                "    Kind = bkCancel\n"
	                "    ModalResult = 2\n"
	                "    OnClick = ButtonClick\n"
	                "    ParentFont = False\n"
	                "    TabOrder = 7\n"
	                "  end\n"
	                "  object btnOK: TBitBtn\n"
	                "    AnchorSideTop.Control = lblChoice\n"
	                "    AnchorSideTop.Side = asrBottom\n"
	                "    AnchorSideRight.Control = lbChoice\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    Left = 413\n"
	                "    Height = 30\n"
	                "    Top = 203\n"
	                "    Width = 97\n"
	                "    Anchors = [akTop, akRight]\n"
	                "    AutoSize = True\n"
	                "    BorderSpacing.Top = 10\n"
	                "    Constraints.MinHeight = 30\n"
	                "    Constraints.MinWidth = 97\n"
	                "    Default = True\n"
	                "    DefaultCaption = True\n"
	                "    Kind = bkOK\n"
	                "    ModalResult = 1\n"
	                "    OnClick = ButtonClick\n"
	                "    ParentFont = False\n"
	                "    TabOrder = 6\n"
	                "  end\n"
	                "end\n");

	gChoice = g_strsplit(text, "\t", -1);
	gDialogApi->DialogBoxLFM((intptr_t)lfm_string->str, (unsigned long)strlen(lfm_string->str), DlgMultiChoiceProc);
	g_string_free(lfm_string, TRUE);
}

static void ParseOpts(gchar *script, gchar *text, gboolean thread)
{
	gchar *output = NULL;
	gboolean request_once = FALSE;
	gboolean request_values = FALSE;
	gboolean log_info = FALSE;

	if (text)
	{
		GKeyFile *langs = OpenTranslations(script);
		gchar *caption = ReplaceTemplate(langs, "WFX_SCRIPT_NAME");
		gchar **split = g_strsplit(text, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			gchar *string = NULL;

			if (*p[0] != 0)
			{
				if (g_strcmp0(*p, OPT_REQUEST) == 0)
				{
					log_info = FALSE;
					request_once = FALSE;
					request_values = TRUE;
				}
				else if (g_strcmp0(*p, OPT_ASKONCE) == 0)
				{
					log_info = FALSE;
					request_once = TRUE;
					request_values = TRUE;
				}
				else if (g_strcmp0(*p, OPT_LOGINFO) == 0)
				{
					log_info = TRUE;
					request_once = FALSE;
					request_values = FALSE;
					gboolean is_logvisible = g_key_file_get_boolean(gCfg, script, OPT_CONNECT, NULL);

					if (!is_logvisible)
						dprintf(gDebugFd, "ERR (%s): " OPT_CONNECT " directive not received, the log window may be not visible.\n", script);
				}
				else if (g_strcmp0(*p, OPT_CONNECT) == 0)
				{

					gchar *message = g_strdup_printf("CONNECT /%s", script);
					gLogProc(gPluginNr, MSGTYPE_CONNECT, message);
					g_key_file_set_boolean(gCfg, script, OPT_CONNECT, TRUE);
					g_free(message);
				}
				else if (g_strcmp0(*p, OPT_STATUSINFO) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_STATUSINFO, TRUE);
				else if (g_strcmp0(*p, OPT_GETVALUE) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_GETVALUE, TRUE);
				else if (g_strcmp0(*p, OPT_GETVALUES) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_GETVALUES, TRUE);
				else if (g_strcmp0(*p, OPT_EXECFEEDBK) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_EXECFEEDBK, TRUE);
				else if (IsValidOpt(*p, OPT_ENVVAR))
				{
					StoreEnvVar(*p, script);
				}
				else if (IsValidOpt(*p, OPT_INFORM) && gRequestProc)
				{
					string = TranslateString(langs, STRIP_OPT(*p, OPT_INFORM));

					if (gDialogApi && !thread)
						MessageBox(string, caption, MB_OK | MB_ICONINFORMATION);
					else
						gRequestProc(gPluginNr, RT_MsgOK, caption, string, NULL, 0);

				}
				else if (IsValidOpt(*p, OPT_YESNOMSG) && gRequestProc)
				{
					string = TranslateString(langs, STRIP_OPT(*p, OPT_YESNOMSG));
					gboolean is_yes = FALSE;
					gchar *key = g_strdup_printf("%s_%s", MARK_BOOL, STRIP_OPT(*p, OPT_YESNOMSG));

					if (g_key_file_has_key(gCfg, script, key, NULL) && request_once)
						is_yes = g_key_file_get_boolean(gCfg, script, key, NULL);
					else
					{
						if (gDialogApi && !thread)
							is_yes = (MessageBox(string, caption, MB_YESNO | MB_ICONQUESTION) == ID_YES);
						else
							is_yes = gRequestProc(gPluginNr, RT_MsgYesNo, caption, string, NULL, 0);

						g_key_file_set_boolean(gCfg, script, key, is_yes);
					}

					g_free(key);

					SetOpt(script, STRIP_OPT(*p, OPT_YESNOMSG), is_yes ? "Yes" : "No");
				}
				else if (IsValidOpt(*p, OPT_PUSH))
				{
					gchar **res = g_strsplit(STRIP_OPT(*p, OPT_PUSH), "\t", 2);

					if (strncmp(res[0], MARK_NOISE, 3) == 0)
						dprintf(gDebugFd, "ERR (%s): Options starting with \"Fs_\" are reserved, \"%s\" ignored.\n", script, res[0]);
					else if (res[1] && !g_key_file_has_key(gCfg, script, res[0], NULL))
					{
						gchar *mark = g_strdup_printf(OPT_PUSH "_%s", res[0]);
						g_key_file_set_boolean(gCfg, script, mark, TRUE);
						g_free(mark);
						g_key_file_set_string(gCfg, script, res[0], res[1]);

						if (gNoise)
							dprintf(gDebugFd, "INFO (%s): The stored value of option \"%s\" will be set to \"%s\".\n", script, res[0], res[1]);

					}
					else if (!res[1])
						dprintf(gDebugFd, "ERR (%s): Failed to get value for \"%s\", recived string: \"%s\".\n", script, res[0], STRIP_OPT(*p, OPT_PUSH));

					g_strfreev(res);
				}
				else if (IsValidOpt(*p, OPT_CLR))
				{
					gchar *option = STRIP_OPT(*p, OPT_CLR);

					if (strncmp(option, MARK_NOISE, 3) == 0)
						dprintf(gDebugFd, "ERR (%s): Options starting with \"Fs_\" are reserved, \"%s\" ignored.\n", script, option);
					else if (strlen(option) > 0)
					{
						g_key_file_remove_key(gCfg, script, option, NULL);

						if (gNoise)
							dprintf(gDebugFd, "INFO (%s): The stored value of option \"%s\" has been cleared.\n", script, option);

						gchar *key = g_strdup_printf("%s_%s_0", MARK_PREV, option);

						if (g_key_file_has_key(gCfg, gScript, key, NULL))
						{
							g_key_file_remove_key(gCfg, script, key, NULL);

							if (gNoise)
								dprintf(gDebugFd, "INFO (%s): The stored value of option \"%s\" has been cleared.\n", script, key);

							int num = 1;

							do
							{
								g_free(key);
								key = g_strdup_printf("%s_%s_%d", MARK_PREV, option, num++);
								g_key_file_remove_key(gCfg, script, key, NULL);

								if (gNoise)
									dprintf(gDebugFd, "INFO (%s): The stored value of option \"%s\" has been cleared.\n", script, key);
							}
							while (g_key_file_has_key(gCfg, gScript, key, NULL));
						}

						g_free(key);
					}
				}
				else if (IsValidOpt(*p, OPT_CHOICE))
				{
					if (thread)
						gRequestProc(gPluginNr, RT_MsgOK, caption, OPT_CHOICE " is not supported here.", NULL, 0);
					else
					{
						gCaption = caption;
						g_strlcpy(gScript, script, MAX_PATH);
						ShowMultiChoiceDlg(STRIP_OPT(*p, OPT_CHOICE), request_once);
					}
				}
				else if (IsValidOpt(*p, OPT_SELFILE))
				{
					if (thread)
						gRequestProc(gPluginNr, RT_MsgOK, caption, OPT_SELFILE " is not supported here.", NULL, 0);
					else
					{
						gCaption = caption;
						g_strlcpy(gScript, script, MAX_PATH);
						ShowSelectFileDlg(STRIP_OPT(*p, OPT_SELFILE), request_once);
					}
				}
				else if (IsValidOpt(*p, OPT_SELDIR))
				{
					if (thread)
						gRequestProc(gPluginNr, RT_MsgOK, caption, OPT_SELDIR " is not supported here.", NULL, 0);
					else
					{
						gCaption = caption;
						g_strlcpy(gScript, script, MAX_PATH);

						if (gDialogApi->VersionAPI > 0)
							ShowSelectDirDlg(STRIP_OPT(*p, OPT_SELDIR), request_once);
						else
							ShowRequestValueDlg(STRIP_OPT(*p, OPT_SELDIR), request_once);
					}
				}
				else if (IsValidOpt(*p, OPT_OUT))
				{
					GError *err = NULL;
					gboolean feedback = g_key_file_get_boolean(gCfg, script, OPT_EXECFEEDBK, NULL);

					if (!g_spawn_command_line_sync(STRIP_OPT(*p, OPT_OUT), &gMemoText, NULL, NULL, &err))
					{
						dprintf(gDebugFd, "ERR (%s): %s (%s)\n", script, err->message, STRIP_OPT(*p, OPT_OUT));
						g_error_free(err);

						if (feedback)
							SetOpt(script, *p, "Error");
					}
					else if (gMemoText != NULL)
					{
						if (strlen(gMemoText) > 1 && g_utf8_validate(gMemoText, -1, NULL))
							ShowTextOutput(FALSE);
						else
						{
							if (gNoise)
								dprintf(gDebugFd, "INFO (%s): failed to show output (%s)\n", script, STRIP_OPT(*p, OPT_OUT));

							g_free(gMemoText);
							gMemoText = NULL;

							if (feedback)
								SetOpt(script, *p, "Error");
						}
					}
				}
				else if (IsValidOpt(*p, OPT_RUN))
				{
					GError *err = NULL;
					gboolean feedback = g_key_file_get_boolean(gCfg, script, OPT_EXECFEEDBK, NULL);

					if (!g_spawn_command_line_async(STRIP_OPT(*p, OPT_RUN), &err))
					{
						dprintf(gDebugFd, "ERR (%s): %s (%s)\n", script, err->message, STRIP_OPT(*p, OPT_RUN));
						g_error_free(err);

						if (feedback)
							SetOpt(script, *p, "Error");
					}
					else if (feedback)
						SetOpt(script, *p, "OK");
				}
				else if (IsValidOpt(*p, OPT_TERM))
				{
					GError *err = NULL;
					gboolean feedback = g_key_file_get_boolean(gCfg, script, OPT_EXECFEEDBK, NULL);
					gboolean is_open = (strncmp(STRIP_OPT(*p, OPT_TERM) - 1, "Keep ", 5) == 0);
					gchar *command = is_open ? STRIP_OPT(*p, OPT_TERM) + 4 : STRIP_OPT(*p, OPT_TERM);
					command = ReplaceString(g_strdup(is_open ? gTermCmdOpen : gTermCmdClose), "{command}", command);
					g_spawn_command_line_async(command, &err);

					if (err)
					{
						dprintf(gDebugFd, "ERR (%s): %s (%s)\n", script, err->message, command);
						g_error_free(err);

						if (feedback)
							SetOpt(script, *p, "Error");
					}
					else if (feedback)
						SetOpt(script, *p, "OK");

					g_free(command);
				}
				else if (IsValidOpt(*p, OPT_EDIT))
				{
					char value[MAX_PATH];
					gchar **res = g_strsplit(STRIP_OPT(*p, OPT_EDIT), "\t", 2);
					string = TranslateString(langs, res[0]);
					g_strlcpy(value, res[1], MAX_PATH);

					if (gRequestProc(gPluginNr, RT_Other, caption, string, value, MAX_PATH))
					{
						gchar *opt = g_strdup_printf("%s %s", OPT_EDIT, res[0]);
						ExecuteScript(script, VERB_SETOPT, opt, value, &output, NULL);

						if (output && output[0] != '\0')
							ParseOpts(script, output, FALSE);

						g_free(opt);
						g_free(output);
						output = NULL;
					}

					g_strfreev(res);
				}
				else if (IsValidOpt(*p, OPT_OPEN))
				{
					GError *err = NULL;
					gchar *command = NULL;
					gboolean is_open = FALSE;
					gboolean is_term = (strncmp(STRIP_OPT(*p, OPT_OPEN) - 1, "Term ", 5) == 0);
					gchar *file = is_term ? STRIP_OPT(*p, OPT_OPEN) + 4 : STRIP_OPT(*p, OPT_OPEN);
					gboolean feedback = g_key_file_get_boolean(gCfg, script, OPT_EXECFEEDBK, NULL);
					gchar *quoted = g_shell_quote(file);

					if (g_file_test(file, G_FILE_TEST_IS_EXECUTABLE))
					{
						if (is_term)
						{
							command = ReplaceString(g_strdup(gTermCmdOpen), "{command}", file);
							is_open = g_spawn_command_line_async(command, &err);
						}
						else
							is_open = g_spawn_command_line_async(quoted, &err);
					}

					if (!is_open)
					{
						if (err)
						{
							dprintf(gDebugFd, "ERR (%s): %s.\n", script, err->message);
							g_clear_error(&err);
						}

						command = g_strdup_printf("xdg-open %s", quoted);
						g_spawn_command_line_async(command, &err);
					}

					g_free(quoted);

					if (err)
					{
						dprintf(gDebugFd, "ERR (%s): %s (%s)\n", script, err->message, command ? command : file);
						g_error_free(err);

						if (feedback)
							SetOpt(script, *p, "Error");
					}
					else if (feedback)
						SetOpt(script, *p, "OK");

					g_free(command);
				}
				else if (strcmp(*p, OPT_NOFAKEDATES) == 0)
					g_key_file_set_boolean(gCfg, script, OPT_NOFAKEDATES, TRUE);
				else if (strncmp(*p, MARK_NOISE, 3) == 0)
					dprintf(gDebugFd, "ERR (%s): Options starting with \"Fs_\" are reserved, \"%s\" ignored.\n", script, *p);
				else if (log_info == TRUE)
				{
					string = TranslateString(langs, *p);
					gLogProc(gPluginNr, MSGTYPE_DETAILS, string);
				}
				else if (request_values == TRUE)
				{
					char value[MAX_PATH] = "";

					if (strncasecmp("password", *p, 8) == 0)
					{
						if (!gCryptProc)
							dprintf(gDebugFd, "ERR: CryptProc is not initialized.\n");
						else
						{
							int ret = gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_LOAD_PASSWORD_NO_UI, script, value, MAX_PATH);

							if (ret == FS_FILE_NOTFOUND)
								ret = gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_LOAD_PASSWORD, script, value, MAX_PATH);

							LogCryptProc(ret);

							if (gRequestProc && gRequestProc(gPluginNr, RT_Password, caption, NULL, value, MAX_PATH))
							{
								ExecuteScript(script, VERB_SETOPT, *p, value, &output, NULL);
								LogCryptProc(gCryptProc(gPluginNr, gCryptoNr, FS_CRYPT_SAVE_PASSWORD, script, value, MAX_PATH));

								if (output && output[0] != '\0')
									ParseOpts(script, output, FALSE);

								g_free(output);
								output = NULL;

							}
						}
					}
					else
					{
						if (thread && IsCanShowDlg(*p, request_once))
						{
							if (gNoise)
								dprintf(gDebugFd, "INFO (%s): %s supports only basic dialogs here.\n", script, VERB_STATUS);

							string = TranslateString(langs, *p);
							gchar *prev = g_key_file_get_string(gCfg, script, *p, NULL);

							if (prev)
							{
								g_strlcpy(value, prev, MAX_PATH);
								g_free(prev);
							}

							if (gRequestProc(gPluginNr, RT_Other, caption, string, value, MAX_PATH))
							{
								g_key_file_set_string(gCfg, script, *p, value);
								gchar *mark = g_strdup_printf(OPT_PUSH "_%s", *p);

								if (g_key_file_has_key(gCfg, gScript, mark, NULL))
									g_key_file_remove_key(gCfg, gScript, mark, NULL);

								g_free(mark);

								ExecuteScript(script, VERB_SETOPT, *p, value, &output, NULL);

								if (output && output[0] != '\0')
									ParseOpts(script, output, FALSE);

								g_free(output);
								output = NULL;
							}

						}
						else
						{
							gCaption = caption;
							g_strlcpy(gScript, script, MAX_PATH);
							ShowRequestValueDlg(*p, request_once);
						}
					}
				}
				else if (gNoise)
					dprintf(gDebugFd, "INFO (%s): " OPT_REQUEST " directive not received, Line \"%s\" will be omitted.\n", script, *p);
			}

			g_free(string);
		}

		g_strfreev(split);
		gCaption = NULL;
		g_free(caption);
		CloseTranslations(langs);
	}
}

static void DeInitializeScript(gchar *script)
{
	if (g_key_file_get_boolean(gCfg, script, MARK_INUSE, NULL))
	{
		gchar *output = NULL;

		if (!ExecuteScript(script, VERB_DEINIT, NULL, NULL, &output, NULL))
		{
			if (gNoise)
				dprintf(gDebugFd, "INFO: Deinitialization not implemented or not completed successfully.\n");
		}
		else
		{
			ParseOpts(script, output, FALSE);
			g_free(output);
		}

		gsize len;
		gchar **pids = g_key_file_get_string_list(gCfg, script, MARK_PIDS, &len, NULL);

		for (gsize i = 0; i < len; i++)
		{
			int pid = atoi(pids[i]);

			if (pid > 0)
			{
				dprintf(gDebugFd, "INFO: PID %d: SIGTERM.\n", pid);
				kill(pid, SIGTERM);
			}
		}

		g_strfreev(pids);

		if (g_key_file_get_boolean(gCfg, script, OPT_CONNECT, NULL))
		{
			gchar *message = g_strdup_printf("DISCONNECT /%s", script);

			if (gLogProc)
				gLogProc(gPluginNr, MSGTYPE_DISCONNECT, message);

			g_free(message);
		}

		gchar **keys = g_key_file_get_keys(gCfg, script, NULL, NULL);

		if (keys)
		{
			for (gsize i = 0; keys[i] != NULL; i++)
			{
				if (!gExtraNoise && strncmp(keys[i], MARK_NOISE, 3) == 0)
				{
					if (strncmp(keys[i], MARK_PREV, strlen(MARK_PREV)) != 0 && strncmp(keys[i], MARK_BOOL, strlen(MARK_BOOL)) != 0 && strcmp(keys[i], MARK_NOISE) != 0)
						g_key_file_remove_key(gCfg, script, keys[i], NULL);
					else if (strcmp(keys[i], MARK_NOISE) == 0 && !gNoise)
						g_key_file_remove_key(gCfg, script, keys[i], NULL);
				}
			}

			g_strfreev(keys);
		}
	}
}

static void InitializeScript(gchar *script)
{
	gchar *output = NULL;

	DeInitializeScript(script);
	gNoise = g_key_file_get_boolean(gCfg, script, MARK_NOISE, NULL);
	ExecuteScript(script, VERB_INIT, NULL, NULL, &output, NULL);
	g_key_file_set_boolean(gCfg, script, MARK_INUSE, TRUE);
	ParseOpts(script, output, FALSE);
	g_free(output);
}

intptr_t DCPCALL DlgPropertiesProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		gNoise = g_key_file_get_boolean(gCfg, gScript, MARK_NOISE, NULL);
		SendDlgMsg(pDlg, "ckNoise", DM_SETCHECK, (intptr_t)gNoise, 0);
		SendDlgMsg(pDlg, "ckExtraNoise", DM_ENABLE, (intptr_t)gNoise, 0);
		SendDlgMsg(pDlg, "ckExtraNoise", DM_SETCHECK, (intptr_t)gExtraNoise, 0);

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
			SendDlgMsg(pDlg, "ScrollBox", DM_SHOWITEM, 1, 0);
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
			ExecuteScript(gScript, VERB_RESET, NULL, NULL, NULL, NULL);
		}
		else if (strcmp(DlgItemName, "btnAct") == 0)
		{
			int i = SendDlgMsg(pDlg, "cbAct", DM_LISTGETITEMINDEX, 0, 0);

			if (i > -1)
			{
				//SendDlgMsg(pDlg, "cbAct", DM_SHOWDIALOG, 0, 0);
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
			SendDlgMsg(pDlg, "ckExtraNoise", DM_ENABLE, (intptr_t)gNoise, 0);
			g_key_file_set_boolean(gCfg, gScript, MARK_NOISE, gNoise);

			if (!gNoise)
			{
				gExtraNoise = FALSE;
				SendDlgMsg(pDlg, "ckExtraNoise", DM_SETCHECK, (intptr_t)gExtraNoise, 0);
			}
		}
		else if (strcmp(DlgItemName, "ckExtraNoise") == 0)
		{
			gExtraNoise = (gboolean)SendDlgMsg(pDlg, DlgItemName, DM_GETCHECK, 0, 0);
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

			if (*key == 13)
			{
				int i = SendDlgMsg(pDlg, "cbAct", DM_LISTGETITEMINDEX, 0, 0);

				if (i > -1)
				{
					SendDlgMsg(pDlg, "cbAct", DM_SHOWDIALOG, 0, 0);
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
	if (gDialogApi)
	{
		GString *lfm_string = g_string_new(NULL);
		g_string_append(lfm_string, "object DialogBox: TDialogBox\n"
		                "  Left = 662\n"
		                "  Height = 742\n"
		                "  Top = 171\n"
		                "  Width = 540\n"
		                "  ActiveControl = lFileName\n"
		                "  AutoSize = True\n"
		                "  BorderStyle = bsDialog\n"
		                "  Caption = 'Properties'\n"
		                "  ChildSizing.LeftRightSpacing = 10\n"
		                "  ChildSizing.TopBottomSpacing = 10\n"
		                "  ClientHeight = 742\n"
		                "  ClientWidth = 540\n"
		                "  DesignTimePPI = 100\n"
		                "  OnShow = DialogBoxShow\n"
		                "  Position = poOwnerFormCenter\n"
		                "  LCLVersion = '2.2.6.0'\n"
		                "  object lFileName: TEdit\n"
		                "    AnchorSideLeft.Control = mPreview\n"
		                "    AnchorSideTop.Control = Owner\n"
		                "    AnchorSideRight.Control = mPreview\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 10\n"
		                "    Height = 36\n"
		                "    Top = 10\n"
		                "    Width = 521\n"
		                "    Alignment = taCenter\n"
		                "    Anchors = [akTop, akLeft, akRight]\n"
		                "    AutoSelect = False\n"
		                "    BorderStyle = bsNone\n"
		                "    Color = clForm\n"
		                "    Font.Style = [fsBold]\n"
		                "    ParentFont = False\n"
		                "    ReadOnly = True\n"
		                "    TabStop = False\n"
		                "    TabOrder = 1\n"
		                "    Text = 'File'\n"
		                "    Visible = False\n"
		                "  end\n"
		                "  object ScrollBox: TScrollBox\n"
		                "    AnchorSideLeft.Control = lFileName\n"
		                "    AnchorSideTop.Control = lFileName\n"
		                "    AnchorSideTop.Side = asrBottom\n"
		                "    AnchorSideRight.Control = lFileName\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 10\n"
		                "    Height = 236\n"
		                "    Top = 46\n"
		                "    Width = 521\n"
		                "    HorzScrollBar.Page = 1\n"
		                "    HorzScrollBar.Visible = False\n"
		                "    VertScrollBar.Increment = 23\n"
		                "    VertScrollBar.Page = 236\n"
		                "    VertScrollBar.Smooth = True\n"
		                "    VertScrollBar.Tracking = True\n"
		                "    Anchors = [akTop, akLeft, akRight]\n"
		                "    AutoSize = True\n"
		                "    BorderStyle = bsNone\n"
		                "    ClientHeight = 200\n"
		                "    ClientWidth = 507\n"
		                "    Constraints.MaxHeight = 200\n"
		                "    TabOrder = 2\n"
		                "    Visible = False\n");

		g_string_append(lfm_string, "    object pProperties: TPanel\n"
		                "      AnchorSideLeft.Control = ScrollBox\n"
		                "      AnchorSideTop.Control = ScrollBox\n"
		                "      AnchorSideRight.Control = ScrollBox\n"
		                "      AnchorSideRight.Side = asrBottom\n"
		                "      Left = 0\n"
		                "      Height = 377\n"
		                "      Top = 10\n"
		                "      Width = 507\n"
		                "      Anchors = [akTop, akLeft, akRight]\n"
		                "      AutoSize = True\n"
		                "      BorderSpacing.Top = 10\n"
		                "      BevelOuter = bvNone\n"
		                "      ChildSizing.HorizontalSpacing = 10\n"
		                "      ChildSizing.VerticalSpacing = 5\n"
		                "      ChildSizing.EnlargeHorizontal = crsHomogenousChildResize\n"
		                "      ChildSizing.EnlargeVertical = crsHomogenousChildResize\n"
		                "      ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
		                "      ChildSizing.ControlsPerLine = 2\n"
		                "      ClientHeight = 377\n"
		                "      ClientWidth = 507\n"
		                "      ParentFont = False\n"
		                "      TabOrder = 0\n"
		                "      Visible = False\n"
		                "      object lTypeLabel: TLabel\n"
		                "        Left = 0\n"
		                "        Height = 17\n"
		                "        Top = 0\n"
		                "        Width = 202\n"
		                "        Alignment = taRightJustify\n"
		                "        Caption = 'Type'\n"
		                "        ParentColor = False\n"
		                "        ParentFont = False\n"
		                "        ShowAccelChar = False\n"
		                "        Visible = False\n"
		                "        WordWrap = True\n"
		                "      end\n"
		                "      object lType: TLabel\n"
		                "        Left = 212\n"
		                "        Height = 17\n"
		                "        Top = 0\n"
		                "        Width = 295\n"
		                "        Caption = 'Value'\n"
		                "        ParentColor = False\n"
		                "        ParentFont = False\n"
		                "        ShowAccelChar = False\n"
		                "        Visible = False\n"
		                "        WordWrap = True\n"
		                "      end\n"
		                "      object lPath: TLabel\n"
		                "        Left = 0\n"
		                "        Height = 36\n"
		                "        Top = 22\n"
		                "        Width = 202\n"
		                "        Alignment = taRightJustify\n"
		                "        Caption = 'Path'\n"
		                "        Layout = tlCenter\n"
		                "        ParentColor = False\n"
		                "        ParentFont = False\n"
		                "        ShowAccelChar = False\n"
		                "        Visible = False\n"
		                "      end\n"
		                "      object ePath: TEdit\n"
		                "        Left = 212\n"
		                "        Height = 36\n"
		                "        Top = 22\n"
		                "        Width = 295\n"
		                "        AutoSelect = False\n"
		                "        BorderStyle = bsNone\n"
		                "        Color = clForm\n"
		                "        ParentFont = False\n"
		                "        ReadOnly = True\n"
		                "        TabStop = False\n"
		                "        TabOrder = 0\n"
		                "        Text = ''\n"
		                "        Visible = False\n"
		                "      end\n"
		                "      object lURLLabel: TLabel\n"
		                "        Left = 0\n"
		                "        Height = 17\n"
		                "        Top = 63\n"
		                "        Width = 202\n"
		                "        Alignment = taRightJustify\n"
		                "        Caption = 'URL'\n"
		                "        ParentColor = False\n"
		                "        ParentFont = False\n"
		                "        ShowAccelChar = False\n"
		                "        Visible = False\n"
		                "      end\n"
		                "      object lURL: TLabel\n"
		                "        Cursor = crHandPoint\n"
		                "        Left = 212\n"
		                "        Height = 17\n"
		                "        Top = 63\n"
		                "        Width = 295\n"
		                "        Caption = ''\n"
		                "        Font.Color = clHighlight\n"
		                "        Font.Style = [fsUnderline]\n"
		                "        ParentColor = False\n"
		                "        ParentFont = False\n"
		                "        ShowAccelChar = False\n"
		                "        Visible = False\n"
		                "        OnClick = ButtonClick\n"
		                "      end\n");

		AddPropLabels(lfm_string);

		g_string_append(lfm_string, "    end\n"
		                "  end\n"
		                "  object lblAct: TLabel\n"
		                "    AnchorSideLeft.Control = ScrollBox\n"
		                "    AnchorSideTop.Control = cbAct\n"
		                "    AnchorSideTop.Side = asrCenter\n"
		                "    Left = 10\n"
		                "    Height = 16\n"
		                "    Top = 113\n"
		                "    Width = 37\n"
		                "    Caption = '&Action'\n"
		                "    Visible = False\n"
		                "  end\n"
		                "  object cbAct: TComboBox\n"
		                "    AnchorSideLeft.Control = lblAct\n"
		                "    AnchorSideLeft.Side = asrBottom\n"
		                "    AnchorSideTop.Control = ScrollBox\n"
		                "    AnchorSideTop.Side = asrBottom\n"
		                "    AnchorSideRight.Control = btnAct\n"
		                "    Left = 10\n"
		                "    Height = 36\n"
		                "    Top = 287\n"
		                "    Width = 389\n"
		                "    Anchors = [akTop, akLeft, akRight]\n"
		                "    BorderSpacing.Left = 10\n"
		                "    BorderSpacing.Top = 5\n"
		                "    BorderSpacing.Right = 10\n"
		                "    ItemHeight = 17\n"
		                "    OnChange = ComboBoxChange\n"
		                "    OnKeyUp = ComboBoxKeyUp\n"
		                "    Style = csDropDownList\n"
		                "    TabOrder = 3\n"
		                "    Visible = False\n"
		                "  end\n"
		                "  object btnAct: TBitBtn\n"
		                "    AnchorSideTop.Control = cbAct\n"
		                "    AnchorSideRight.Control = ScrollBox\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    AnchorSideBottom.Control = cbAct\n"
		                "    AnchorSideBottom.Side = asrBottom\n"
		                "    Left = 409\n"
		                "    Height = 36\n"
		                "    Top = 287\n"
		                "    Width = 80\n"
		                "    Anchors = [akTop, akRight, akBottom]\n"
		                "    AutoSize = True\n"
		                "    Caption = 'Start'\n"
		                "    Enabled = False\n"
		                "    Glyph.Data = {\n"
		                "      36040000424D3604000000000000360000002800000010000000100000000100\n"
		                "      200000000000000400006400000064000000000000000000000000000000066A\n"
		                "      1D8B03651A4E0060200800000000000000000000000000000000000000000000\n"
		                "      0000000000000000000000000000000000000000000000000000000000000B78\n"
		                "      2CC02DA25CF30E7D31B504661841006633050000000000000000000000000000\n"
		                "      0000000000000000000000000000000000000000000000000000000000000C79\n"
		                "      2EC05ACB8FFF50C385FF2CA15CEC0E7B30AA0560133500800002000000000000\n"
		                "      0000000000000000000000000000000000000000000000000000000000000D7A\n"
		                "      30C05CCE93FF3AC47EFF55CD8FFF4EC485FF289E57E50F7A319D0059122B0000\n"
		                "      0001000000000000000000000000000000000000000000000000000000000F7C\n"
		                "      31C05ED197FF31C57AFF32C67CFF40CB85FF52D091FF47C281FF239750DD0D78\n"
		                "      2C8A005D0F21000000000000000000000000000000000000000000000000107D\n"
		                "      34C05CD298FF35C980FF37CB82FF38CC83FF3ACD85FF44D08CFF4FD292FF3EBE\n"
		                "      7DFF1F954CD80D752C7A005C141900000000000000000000000000000000117E\n"
		                "      35C05AD598FF39CD86FF3BCF88FF3CD089FF3DD18BFF3ED28BFF3FD28CFF47D4\n"
		                "      90FF4FD594FF3EBD7BFD1D9147D10C71266A00550E1200000000000000001380\n"
		                "      36C059D79AFF3ED28BFF3FD38DFF41D58FFF42D691FF42D691FF43D792FF42D6\n"
		                "      91FF43D691FF4DD797FF56D69AFF6DCA99FC219048CA0C6F2455000000001481\n"
		                "      38C057D99CFF42D691FF44D893FF45D995FF46DA97FF47DB98FF47DB98FF47DB\n"
		                "      97FF47DA97FF84E6B9FFAAECCDFF6FCD9BFB21904ACA0C6F2455000000001582\n"
		                "      3AC055DB9DFF46DA96FF48DC99FF4ADE9BFF4BDF9DFF4CE09EFF4DE09EFF58E2\n"
		                "      A4FFADF0D1FF82D8ADFE2F9B58D30E732B6A00550E1200000000000000001783\n"
		                "      3CC05ADFA2FF4ADE9BFF4CE09EFF4EE2A1FF51E3A3FF5BE5A9FFAEF2D4FF8EE1\n"
		                "      B9FF3BA769DB137A327B005C1419000000000000000000000000000000001885\n"
		                "      3DC05FE2A7FF4DE19FFF50E4A3FF5CE6ABFFADF3D5FF9BE7C4FF4AB176E11880\n"
		                "      398A005D0F210000000000000000000000000000000000000000000000001885\n"
		                "      3EC065E5ACFF58E5A8FFA7F2D2FFA3EBCBFF59BB86E91D87439D0059122B0000\n"
		                "      0001000000000000000000000000000000000000000000000000000000001986\n"
		                "      3EC0B8F4DAFFA7EDCEFF62C490EF218C47AA0560183500800002000000000000\n"
		                "      0000000000000000000000000000000000000000000000000000000000001986\n"
		                "      40C069CA97F5238C49B608662341006633050000000000000000000000000000\n"
		                "      0000000000000000000000000000000000000000000000000000000000000F74\n"
		                "      2C8B0A6C244E0060200800000000000000000000000000000000000000000000\n"
		                "      0000000000000000000000000000000000000000000000000000\n"
		                "    }\n"
		                "    OnClick = ButtonClick\n"
		                "    TabOrder = 4\n"
		                "    Visible = False\n"
		                "  end\n");

		g_string_append(lfm_string, "  object lScriptName: TEdit\n"
		                "    AnchorSideLeft.Control = mPreview\n"
		                "    AnchorSideTop.Control = cbAct\n"
		                "    AnchorSideTop.Side = asrBottom\n"
		                "    AnchorSideRight.Control = mPreview\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 10\n"
		                "    Height = 36\n"
		                "    Top = 344\n"
		                "    Width = 521\n"
		                "    Alignment = taCenter\n"
		                "    Anchors = [akTop, akLeft, akRight]\n"
		                "    AutoSelect = False\n"
		                "    BorderSpacing.Top = 21\n"
		                "    BorderStyle = bsNone\n"
		                "    Color = clForm\n"
		                "    Font.Style = [fsBold]\n"
		                "    ParentFont = False\n"
		                "    ReadOnly = True\n"
		                "    TabStop = False\n"
		                "    TabOrder = 5\n"
		                "    Text = 'Script'\n"
		                "  end\n"
		                "  object lScriptType: TLabel\n"
		                "    AnchorSideLeft.Control = mPreview\n"
		                "    AnchorSideTop.Control = ckNoise\n"
		                "    AnchorSideTop.Side = asrCenter\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 10\n"
		                "    Height = 17\n"
		                "    Top = 393\n"
		                "    Width = 33\n"
		                "    Caption = 'Type'\n"
		                "    ParentColor = False\n"
		                "    ParentFont = False\n"
		                "    Visible = False\n"
		                "  end\n"
		                "  object pSrciptActs: TPanel\n"
		                "    AnchorSideLeft.Control = mPreview\n"
		                "    AnchorSideLeft.Side = asrCenter\n"
		                "    AnchorSideTop.Control = ckNoise\n"
		                "    AnchorSideBottom.Control = ckNoise\n"
		                "    AnchorSideBottom.Side = asrBottom\n"
		                "    Left = 202\n"
		                "    Height = 23\n"
		                "    Top = 390\n"
		                "    Width = 137\n"
		                "    Anchors = [akTop, akLeft, akBottom]\n"
		                "    AutoSize = True\n"
		                "    BevelOuter = bvNone\n"
		                "    ChildSizing.HorizontalSpacing = 2\n"
		                "    ChildSizing.EnlargeHorizontal = crsHomogenousSpaceResize\n"
		                "    ChildSizing.ShrinkVertical = crsHomogenousChildResize\n"
		                "    ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
		                "    ChildSizing.ControlsPerLine = 3\n"
		                "    ClientHeight = 23\n"
		                "    ClientWidth = 137\n"
		                "    TabOrder = 6\n"
		                "    object btnUnmount: TButton\n"
		                "      Left = 0\n"
		                "      Height = 23\n"
		                "      Top = 0\n"
		                "      Width = 80\n"
		                "      AutoSize = True\n"
		                "      Caption = 'Unmount'\n"
		                "      ModalResult = 11\n"
		                "      OnClick = ButtonClick\n"
		                "      TabOrder = 0\n"
		                "    end\n"
		                "    object btnReset: TButton\n"
		                "      Left = 82\n"
		                "      Height = 23\n"
		                "      Top = 0\n"
		                "      Width = 55\n"
		                "      AutoSize = True\n"
		                "      Caption = 'Reset'\n"
		                "      ModalResult = 11\n"
		                "      OnClick = ButtonClick\n"
		                "      TabOrder = 1\n"
		                "    end\n"
		                "  end\n"
		                "  object ckNoise: TCheckBox\n"
		                "    AnchorSideLeft.Control = lScriptType\n"
		                "    AnchorSideLeft.Side = asrBottom\n"
		                "    AnchorSideTop.Control = lScriptName\n"
		                "    AnchorSideTop.Side = asrBottom\n"
		                "    AnchorSideRight.Control = ckExtraNoise\n"
		                "    Left = 368\n"
		                "    Height = 23\n"
		                "    Top = 390\n"
		                "    Width = 121\n"
		                "    Anchors = [akTop, akRight]\n"
		                "    BorderSpacing.Top = 10\n"
		                "    Caption = 'De&bug mode'\n"
		                "    OnChange = CheckBoxChange\n"
		                "    ParentFont = False\n"
		                "    TabOrder = 7\n"
		                "    TabStop = False\n"
		                "  end\n"
		                "  object ckExtraNoise: TCheckBox\n"
		                "    AnchorSideTop.Control = ckNoise\n"
		                "    AnchorSideTop.Side = asrCenter\n"
		                "    AnchorSideRight.Control = mPreview\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 489\n"
		                "    Height = 23\n"
		                "    Top = 390\n"
		                "    Width = 42\n"
		                "    Anchors = [akTop, akRight]\n"
		                "    Caption = '+'\n"
		                "    Enabled = False\n"
		                "    OnChange = CheckBoxChange\n"
		                "    TabOrder = 8\n"
		                "  end\n");

		g_string_append(lfm_string, "  object mPreview: TMemo\n"
		                "    AnchorSideLeft.Control = Owner\n"
		                "    AnchorSideTop.Control = ckNoise\n"
		                "    AnchorSideTop.Side = asrBottom\n"
		                "    AnchorSideRight.Control = Owner\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 10\n"
		                "    Height = 153\n"
		                "    Top = 418\n"
		                "    Width = 521\n"
		                "    BorderSpacing.Top = 5\n"
		                "    Enabled = False\n"
		                "    Font.Name = 'Monospace'\n"
		                "    ParentFont = False\n"
		                "    ReadOnly = True\n"
		                "    ScrollBars = ssAutoBoth\n"
		                "    TabOrder = 9\n"
		                "    TabStop = False\n"
		                "    WordWrap = False\n"
		                "  end\n"
		                "  object btnClose: TBitBtn\n"
		                "    AnchorSideTop.Control = mPreview\n"
		                "    AnchorSideTop.Side = asrBottom\n"
		                "    AnchorSideRight.Control = mPreview\n"
		                "    AnchorSideRight.Side = asrBottom\n"
		                "    Left = 430\n"
		                "    Height = 31\n"
		                "    Top = 581\n"
		                "    Width = 101\n"
		                "    Anchors = [akTop, akRight]\n"
		                "    AutoSize = True\n"
		                "    BorderSpacing.Top = 10\n"
		                "    Cancel = True\n"
		                "    Constraints.MinHeight = 31\n"
		                "    Constraints.MinWidth = 101\n"
		                "    Default = True\n"
		                "    DefaultCaption = True\n"
		                "    Kind = bkClose\n"
		                "    ModalResult = 11\n"
		                "    OnClick = ButtonClick\n"
		                "    ParentFont = False\n"
		                "    TabOrder = 0\n"
		                "  end\n"
		                "  object lbDataStore: TListBox\n"
		                "    AnchorSideLeft.Control = Owner\n"
		                "    AnchorSideRight.Control = Owner\n"
		                "    AnchorSideBottom.Control = Owner\n"
		                "    AnchorSideBottom.Side = asrBottom\n"
		                "    Left = 10\n"
		                "    Height = 83\n"
		                "    Top = 649\n"
		                "    Width = 164\n"
		                "    Anchors = [akLeft, akBottom]\n"
		                "    ItemHeight = 0\n"
		                "    TabOrder = 10\n"
		                "    TabStop = False\n"
		                "    Visible = False\n"
		                "  end\n"
		                "end\n");

		gDialogApi->DialogBoxLFM((intptr_t)lfm_string->str, (unsigned long)strlen(lfm_string->str), DlgPropertiesProc);
		g_string_free(lfm_string, TRUE);
	}
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
	return (path == NULL || path[0] != '/' || g_strcmp0(path, "/.") == 0 || g_strcmp0(path, "/..") == 0 || strncmp(path, "/../", 4) == 0);
}

static gboolean IsRootDir(char *path)
{
	if (path[1] == '\0')
		return TRUE;

	while (*path++)
	{
		if (*path == '/')
			return FALSE;
	}

	return TRUE;
}

static void CleanScriptName(char *fn)
{
	while (*fn++)
	{
		if (*fn == '/' || *fn == '[' || *fn == ']')
			*fn = '_';
	}
}

static gchar *StripScriptFromPath(char *path)
{
	if (path[0] != '/')
	{
		dprintf(gDebugFd, "ERR (StripScriptFromPath): path is invalid (%s).\n", path);
		return NULL;
	}

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
				gchar *filename = NULL;
				char *dot = strrchr(ent->d_name, '.');
				GKeyFile *langs = OpenTranslations(ent->d_name);
				gchar *string = ReplaceTemplate(langs, "WFX_SCRIPT_NAME");
				CloseTranslations(langs);

				if (string && string[0] != '\0')
					filename = g_strdup_printf("%s%s", string, (dot != NULL) ? dot : "");

				g_free(string);

				if (filename && !g_key_file_has_key(gCfg, "/", filename, NULL))
					g_strlcpy(FindData->cFileName, filename, sizeof(FindData->cFileName));
				else
					g_strlcpy(FindData->cFileName, ent->d_name, sizeof(FindData->cFileName));

				CleanScriptName(FindData->cFileName);
				g_key_file_set_string(gCfg, "/", FindData->cFileName, ent->d_name);
				g_free(filename);

				if (g_strcmp0(FindData->cFileName, MARK_LINK) == 0 || g_strcmp0(FindData->cFileName, MARK_DEBG) == 0)
					dprintf(gDebugFd, "ERR: filename \"%s\" is reserved!\n", FindData->cFileName);

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

				if (g_key_file_get_boolean(gCfg, ent->d_name, MARK_INUSE, NULL))
					FindData->dwReserved0 |= S_IWUSR;

				return TRUE;
			}
		}
	}

	if (!dirdata->debug_check)
	{
		dirdata->debug_check = TRUE;

		if (fstat(gDebugFd, &buf) == 0)
		{
			g_strlcpy(FindData->cFileName, MARK_DEBG, sizeof(FindData->cFileName));
			UnixTimeToFileTime((time_t)buf.st_mtime, &FindData->ftLastWriteTime);
			UnixTimeToFileTime((time_t)buf.st_atime, &FindData->ftLastAccessTime);
			FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
			FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;
			FindData->dwReserved0 = buf.st_mode;

			if (buf.st_size > 0)
				return TRUE;
		}
	}

	return FALSE;
}

static gboolean SetFindData(tVFSDirData * dirdata, WIN32_FIND_DATAA * FindData)
{
	gchar *string = NULL;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while (g_match_info_matches(dirdata->match_info))
	{
		string = g_match_info_fetch(dirdata->match_info, 0);

		if (gExtraNoise)
			dprintf(gDebugFd, "INFO: Found: %s\n", string);

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 1);

		if (gExtraNoise)
			dprintf(gDebugFd, "INFO: mode: %s\n", string);

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

		if (gExtraNoise)
			dprintf(gDebugFd, "INFO: date: %s\n", string);

		gint64 filetime = 0;
		GDateTime *dt = g_date_time_new_from_iso8601(string, NULL);

		if (dt)
		{
			filetime = g_date_time_to_unix(dt);
			g_date_time_unref(dt);
		}
		else if (gExtraNoise)
			dprintf(gDebugFd, "INFO: \"%s\" is not a valid ISO 8601 UTC formatted string.\n", string);

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

					if (gExtraNoise)
						dprintf(gDebugFd, "INFO: Used %s to parse the date.\n", *p);

					break;
				}
			}
		}

		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;

		if (filetime > 0)
		{
			UnixTimeToFileTime((time_t)filetime, &FindData->ftLastWriteTime);
		}
		else if (gDialogApi->VersionAPI > 0 && dirdata->empty_dates)
		{
			FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
			FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
		}
		else
		{
			SetCurrentFileTime(&FindData->ftLastWriteTime);

			if (gExtraNoise)
				dprintf(gDebugFd, "INFO: The current datetime has been set.\n");
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 3);

		if (gExtraNoise)
			dprintf(gDebugFd, "INFO: size: %s\n", string);

		gdouble filesize = -1;

		if (string[0] != '-')
			filesize = g_ascii_strtod(string, NULL);

		if (!S_ISDIR(mode))
		{
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
		}

		g_free(string);

		string = g_match_info_fetch(dirdata->match_info, 4);

		if (gExtraNoise)
			dprintf(gDebugFd, "INFO: name: %s\n", string);

		g_strlcpy(FindData->cFileName, string, sizeof(FindData->cFileName));
		g_free(string);

		g_match_info_next(dirdata->match_info, NULL);

		if (strchr(FindData->cFileName, '/') != NULL)
			dprintf(gDebugFd, "ERR (%s): filename \"%s\" is invalid, ignored.\n", dirdata->script, FindData->cFileName);
		else
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
		g_key_file_remove_group(gCfg, "/", NULL);
		g_strlcpy(FindData->cFileName, MARK_LINK, sizeof(FindData->cFileName));
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
		SetCurrentFileTime(&FindData->ftLastWriteTime);
		FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;
		FindData->dwReserved0 = S_IFLNK | S_IREAD | S_IEXEC;
	}
	else
	{
		gchar *script = ExtractScriptFromPath(Path);

		if (!script)
			return (HANDLE)(-1);

		if (!g_key_file_get_boolean(gCfg, script, MARK_INUSE, NULL))
		{
			g_strlcpy(gScript, script, MAX_PATH);
			InitializeScript(script);

			if (gNoise)
				dprintf(gDebugFd, "INFO: forced init.\n");
		}

		gchar *output = NULL;
		gchar *list_path = StripScriptFromPath(Path);
		size_t len = strlen(list_path);

		if (len > 1 && list_path[len - 1] == '/')
			list_path[len - 1] = '\0';

		if (!ExecuteScript(script, VERB_LIST, list_path, NULL, &output, NULL))
		{
			if (list_path[1] == 0 && !gNoise)
				DeInitializeScript(script);

			g_free(list_path);
			g_free(script);
			g_free(output);
			return (HANDLE)(-1);
		}

		dirdata = g_new0(tVFSDirData, 1);

		if (!dirdata)
		{
			g_free(list_path);
			g_free(output);
			g_free(script);
			return (HANDLE)(-1);
		}

		g_strlcpy(dirdata->script, script, MAX_PATH);
		gchar **split = g_strsplit(output, "\n", -1);

		for (gchar **p = split; *p != NULL; p++)
		{
			if (IsValidOpt(*p, OPT_ENVVAR))
				StoreEnvVar(*p, script);
		}

		g_strfreev(split);
		g_free(script);
		dirdata->regex = g_regex_new(REGEXP_LIST, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, NULL);
		dirdata->empty_dates = g_key_file_get_boolean(gCfg, dirdata->script, OPT_NOFAKEDATES, NULL);

		if (!g_regex_match(dirdata->regex, output, 0, &(dirdata->match_info)))
		{
			if (gNoise)
				dprintf(gDebugFd, "INFO: %s: no file list received\n", Path);

			g_regex_unref(dirdata->regex);
			g_free(list_path);
			g_free(output);
			return (HANDLE)(-1);
		}

		dirdata->output = output;

		if (!SetFindData(dirdata, FindData))
		{
			g_match_info_free(dirdata->match_info);
			g_regex_unref(dirdata->regex);
			g_free(list_path);
			g_free(output);
			g_free(dirdata);
			return (HANDLE)(-1);
		}

		if (g_key_file_get_boolean(gCfg, dirdata->script, OPT_GETVALUES, NULL) && !g_key_file_has_key(gCfg, dirdata->script, "Fs_Set_" ENVVAR_MULTIFILEOP, NULL))
			g_key_file_set_string(gCfg, dirdata->script, MARK_VALS, list_path);

		g_free(list_path);

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
	int result = FS_FILE_OK;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (strcmp("/" MARK_LINK, RemoteName) == 0)
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	if (strcmp("/" MARK_LINK, RemoteName) == 0)
		return FS_FILE_USERABORT;
	else if (strcmp("/" MARK_DEBG, RemoteName) == 0)
	{
		if (symlink(gDebugFileName, LocalName) != 0)
			return FS_FILE_WRITEERROR;
		else
			return FS_FILE_OK;
	}

	if (IsRootDir(RemoteName))
	{
		char path[PATH_MAX];
		g_strlcpy(path, gScriptDir, PATH_MAX);
		gchar *script_name = g_key_file_get_string(gCfg, "/", RemoteName + 1, NULL);
		strcat(path, "/");
		strcat(path, script_name);
		g_free(script_name);

		if (symlink(path, LocalName) != 0)
			return FS_FILE_WRITEERROR;
	}
	else
	{
		gchar *script = ExtractScriptFromPath(RemoteName);
		gchar *path = StripScriptFromPath(RemoteName);

		if (IsInvalidPath(path) || !RunFileOP(script, VERB_GET_FILE, path, LocalName))
			result = FS_FILE_NOTSUPPORTED;

		if (result == FS_FILE_OK && g_key_file_get_boolean(gCfg, script, OPT_CONNECT, NULL))
		{
			gchar *message = g_strdup_printf("%s -> %s", path, LocalName);
			gLogProc(gPluginNr, MSGTYPE_TRANSFERCOMPLETE, message);
			g_free(message);
		}

		g_free(script);
		g_free(path);
	}

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

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

	if (CopyFlags == 0 && ExecuteScript(script, VERB_EXISTS, path, NULL, NULL, NULL))
		result = FS_FILE_EXISTS;
	else
	{
		if (!RunFileOP(script, VERB_PUT_FILE, LocalName, path))
			result = FS_FILE_NOTSUPPORTED;

		if (result == FS_FILE_OK && g_key_file_get_boolean(gCfg, script, OPT_CONNECT, NULL))
		{
			gchar *message = g_strdup_printf("%s -> %s", path, LocalName);
			gLogProc(gPluginNr, MSGTYPE_TRANSFERCOMPLETE, message);
			g_free(message);
		}

		gProgressProc(gPluginNr, RemoteName, path, 100);
	}

	g_free(script);
	g_free(path);

	return result;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct * ri)
{
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

	if (!OverWrite && ExecuteScript(script, VERB_EXISTS, newpath, NULL, NULL, NULL))
		return FS_FILE_EXISTS;

	if (!RunFileOP(script, Move ? VERB_MOVE : VERB_COPY, oldpath, newpath))
		return FS_FILE_NOTSUPPORTED;

	if (result == FS_FILE_OK && g_key_file_get_boolean(gCfg, script, OPT_CONNECT, NULL))
	{
		gchar *message = g_strdup_printf("%s -> %s", oldpath, newpath);
		gLogProc(gPluginNr, MSGTYPE_TRANSFERCOMPLETE, message);
		g_free(message);
	}

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
		result = ExecuteScript(script, VERB_MKDIR, newpath, NULL, &output, NULL);

		if (output && output[0] != '\0')
			ParseOpts(script, output, FALSE);

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
		result = RunFileOP(script, VERB_REMOVE, newpath, NULL);
		g_free(script);
		g_free(newpath);
	}
	else if (strcmp("/" MARK_DEBG, RemoteName) == 0)
	{
		ftruncate(gDebugFd, 0);
		lseek(gDebugFd, 0, SEEK_SET);
		result = TRUE;
	}
	else
	{
		gchar *script_name = g_key_file_get_string(gCfg, "/", RemoteName + 1, NULL);

		if (strcmp("/" MARK_LINK, RemoteName) != 0 && g_key_file_get_boolean(gCfg, script_name, MARK_INUSE, NULL))
		{

			DeInitializeScript(script_name);

			if (MessageBox("The script has been deinitialized, do you also want to reset the script settings?", script_name, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == ID_YES)
			{
				ExecuteScript(script_name, VERB_RESET, NULL, NULL, NULL, NULL);
				g_key_file_remove_group(gCfg, script_name, NULL);
			}

			result = TRUE;
		}

		g_free(script_name);
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
		result = RunFileOP(script, VERB_RMDIR, newpath, NULL);
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
	gboolean is_root = IsRootDir(RemoteName);

	if (!is_root)
		g_key_file_set_string(gCfg, script, "Fs_Set_" ENVVAR_REMOTENAME, path);

	if (strcmp(Verb, "open") == 0)
	{
		if (is_root)
		{
			if (strcmp("/" MARK_LINK, RemoteName) == 0)
				g_strlcpy(RemoteName, gScriptDir, MAX_PATH);
			else if (strcmp("/" MARK_DEBG, RemoteName) == 0)
			{
				g_free(script);
				g_free(path);
				return FS_EXEC_YOURSELF;
			}
			else
			{
				gchar *script_name = g_key_file_get_string(gCfg, "/", script, NULL);

				if (!g_key_file_get_boolean(gCfg, script_name, MARK_INUSE, NULL))
					InitializeScript(script_name);

				g_strlcpy(gScript, script_name, MAX_PATH);
				gchar *remote = g_strdup_printf("/%s", script_name);
				g_strlcpy(RemoteName, remote, MAX_PATH);
				g_free(remote);
				g_free(script_name);
			}

			result = FS_EXEC_SYMLINK;
		}
		else
		{
			if (!ExecuteScript(script, VERB_EXEC, path, NULL, &output, NULL))
				result = FS_EXEC_YOURSELF;
			else if (output && output[0] != '\0')
			{
				if (strncmp(output, OPT_OPEN, 3) == 0)
				{
					ParseOpts(script, output, FALSE);
					result = FS_EXEC_OK;
				}
				else
				{
					size_t len = strlen(output);

					if (len > 0)
						output[len - 1] = '\0';

					if (output[0] == '/' && output[1] == '\0')
						dprintf(gDebugFd, "ERR (%s): local path '/' is not supported", script);
					else if (output[0] == '/')
					{
						DIR *dir = opendir(output);

						if (dir != NULL)
						{
							closedir(dir);
							g_strlcpy(RemoteName, output, MAX_PATH - 1);
							result = FS_EXEC_SYMLINK;
						}
						else
							dprintf(gDebugFd, "ERR (%s): failed to change dir to local path\n%s\n%s\n%s\n", script, EXEC_SEP, output, EXEC_SEP);
					}
					else if (output[0] == '.')
					{
						gchar *scr_path = g_strdup_printf("/%s/%s", script, (output[1] == '/') ? output + 2 : output + 1);
						g_strlcpy(RemoteName, scr_path, MAX_PATH - 1);
						g_free(scr_path);
						result = FS_EXEC_SYMLINK;
					}
					else
						dprintf(gDebugFd, "ERR (%s): unexpected output\n%s\n%s\n%s\n", script, EXEC_SEP, output, EXEC_SEP);
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
			if (is_root)
			{
				gchar *script_name = g_key_file_get_string(gCfg, "/", script, NULL);
				g_strlcpy(gScript, script_name, MAX_PATH);
				g_free(script_name);
			}
			else
				g_strlcpy(gScript, script, MAX_PATH);

			if (!is_root && !IsInvalidPath(path))
			{
				gboolean is_ok = ExecuteScript(script, VERB_PROPS, path, NULL, &gProps, NULL);

				if (!is_ok || (gProps && gProps[0] != '\0'))
				{

					if (strncmp(gProps, MARK_NOISE, 3) == 0)
						ParseOpts(script, gProps, FALSE);
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
			else if (strcmp("/" MARK_LINK, RemoteName) == 0)
				MessageBox(gScriptDir, ROOTNAME, MB_OK | MB_ICONINFORMATION);
			else if (strcmp("/" MARK_DEBG, RemoteName) == 0)
				MessageBox(gDebugFileName, ROOTNAME, MB_OK | MB_ICONINFORMATION);
			else
				ShowPropertiesDlg();

			result = FS_EXEC_OK;
		}
	}
	else if (!is_root && !IsInvalidPath(path))
	{
		if (strncmp(Verb, "chmod", 5) == 0)
		{
			if (ExecuteScript(script, VERB_CHMOD, path, Verb + 6, NULL, NULL))
				result = FS_EXEC_OK;
		}
		else if (strncmp(Verb, "quote", 5) == 0)
		{
			ExecuteScript(script, VERB_QUOTE, Verb + 6, path, &output, NULL);

			if (output && output[0] != '\0')
				ParseOpts(script, output, FALSE);

			result = FS_EXEC_OK;
		}
	}

	if (!is_root)
		g_key_file_remove_key(gCfg, script, "Fs_Set_" ENVVAR_REMOTENAME, NULL);

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

				result = ExecuteScript(script, VERB_MODTIME, newpath, date, NULL, NULL);
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
	g_strlcpy(DefRootName, ROOTNAME, maxlen - 1);
}

BOOL DCPCALL FsDisconnect(char* DisconnectRoot)
{
	DeInitializeScript(DisconnectRoot + 1);
	return TRUE;
}

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
	gchar *path = NULL;
	gchar *output = NULL;
	int result = ft_fieldempty;
	gchar *script = ExtractScriptFromPath(FileName);

	if (IsRootDir(FileName))
	{
		memset((char*)FieldValue, 0, maxlen);

		if (strcmp("/" MARK_LINK, FileName) == 0)
		{
			g_strlcpy((char*)FieldValue, "@", maxlen - 1);
			result = ft_string;
		}
		else if (strcmp("/" MARK_DEBG, FileName) == 0)
		{
			g_strlcpy((char*)FieldValue, "!!!", maxlen - 1);
			result = ft_string;
		}
		else if (strcmp("/..", FileName) == 0)
		{

		}
		else
		{
			gchar *script_name = g_key_file_get_string(gCfg, "/", script, NULL);

			if (g_key_file_get_boolean(gCfg, script_name, MARK_NOISE, NULL))
			{
				g_strlcpy((char*)FieldValue, "# ", maxlen - 1);
				result = ft_string;
			}

			if (g_key_file_get_boolean(gCfg, script_name, MARK_INUSE, NULL))
			{
				strcat((char*)FieldValue, "* ");
				result = ft_string;

				gchar *pids = g_key_file_get_string(gCfg, script_name, MARK_PIDS, NULL);

				if (pids && pids[0] != '\0')
				{
					strcat((char*)FieldValue, "! ");
					strncat((char*)FieldValue, pids, 100);

					if (strlen(pids) > 100)
						strcat((char *)FieldValue, "...");

					g_free(pids);

				}
			}

			g_free(script_name);
		}

		g_free(script);
		return result;
	}

	gboolean is_get_single = g_key_file_get_boolean(gCfg, script, OPT_GETVALUE, NULL);
	gboolean is_get_multi = g_key_file_get_boolean(gCfg, script, OPT_GETVALUES, NULL);

	if (!is_get_single && !is_get_multi)
	{
		g_free(script);
		return ft_fieldempty;
	}

	if (is_get_multi)
	{
		path = g_key_file_get_string(gCfg, script, MARK_VALS, NULL);

		if (path)
		{
			g_strfreev(gLastValues);
			gLastValues = NULL;

			ExecuteScript(script, VERB_GETVALS, path, NULL, &output, NULL);

			if (output)
			{
				size_t len = strlen(output);

				if (len > 0)
					gLastValues = g_strsplit(output, "\n", -1);
			}

			g_key_file_remove_key(gCfg, script, MARK_VALS, NULL);
		}

		if (gLastValues)
		{
			g_free(path);
			path = StripScriptFromPath(FileName);
			gchar *file = g_path_get_basename(path);

			for (char **p = gLastValues; *p != NULL; p++)
			{
				gchar **res = g_strsplit(*p, "\t", 2);

				if (!res)
					continue;

				if (!res[0] || !res[1])
				{
					g_strfreev(res);
					continue;
				}
				else if (strcmp(file, res[0]) == 0)
				{
					GKeyFile *langs = OpenTranslations(gScript);
					gchar *string = TranslateString(langs, res[1]);
					CloseTranslations(langs);
					g_strlcpy((char*)FieldValue, string, maxlen - 1);
					g_free(string);
					result = ft_string;
					g_strfreev(res);
					break;
				}

				g_strfreev(res);
			}

			g_free(file);
		}
	}
	else
	{
		path = StripScriptFromPath(FileName);
		ExecuteScript(script, VERB_GETVALUE, path, NULL, &output, NULL);

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
		ExecuteScript(gScript, VERB_REALNAME, newpath, NULL, &output, NULL);
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
	else if (strcmp("/" MARK_DEBG, RemoteName) == 0)
	{
		g_strlcpy(RemoteName, gDebugFileName, maxlen - 1);
		result = TRUE;
	}
	else
	{
		gchar *script_name = g_key_file_get_string(gCfg, "/", RemoteName + 1, NULL);
		gchar *path = g_strdup_printf("%s/%s", gScriptDir, script_name);
		g_free(script_name);
		g_strlcpy(RemoteName, path, maxlen - 1);
		g_free(path);
		result = TRUE;
	}

	return result;
}
#endif

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{
	if (InfoOperation != FS_STATUS_OP_LIST && IsRootDir(RemoteDir))
		return;
	else if (InfoOperation == FS_STATUS_OP_LIST && RemoteDir[1] == '\0')
		return;

	gchar *script = ExtractScriptFromPath(RemoteDir);

	if (!script)
		return;

	gNoise = g_key_file_get_boolean(gCfg, script, MARK_NOISE, NULL);
	gchar *path = StripScriptFromPath(RemoteDir);
	size_t len = strlen(path);

	if (len > 1 && path[len - 1] == '/')
		path[len - 1] = '\0';

	if (InfoOperation != FS_STATUS_OP_EXEC)
	{
		if (InfoStartEnd == FS_STATUS_START)
		{
			g_key_file_set_string(gCfg, script, "Fs_Set_" ENVVAR_REMOTENAME, path);

			if (InfoOperation == FS_STATUS_OP_GET_MULTI_THREAD)
				g_key_file_set_string(gCfg, script, "Fs_Set_" ENVVAR_MULTIFILEOP, "get_multi_thread");
			else if (InfoOperation == FS_STATUS_OP_PUT_MULTI_THREAD)
				g_key_file_set_string(gCfg, script, "Fs_Set_" ENVVAR_MULTIFILEOP, "put_multi_thread");
			else if (InfoOperation == FS_STATUS_OP_RENMOV_MULTI)
				g_key_file_set_string(gCfg, script, "Fs_Set_" ENVVAR_MULTIFILEOP, "renmov_multi");
			else if (InfoOperation == FS_STATUS_OP_DELETE)
				g_key_file_set_string(gCfg, script, "Fs_Set_" ENVVAR_MULTIFILEOP, "delete");
		}
		else
		{
			g_key_file_remove_key(gCfg, script, "Fs_Set_" ENVVAR_REMOTENAME, NULL);
			g_key_file_remove_key(gCfg, script, "Fs_Set_" ENVVAR_MULTIFILEOP, NULL);
		}
	}

	if (!g_key_file_get_boolean(gCfg, script, OPT_STATUSINFO, NULL))
	{
		g_free(path);
		g_free(script);
		return;
	}

	gboolean thread = FALSE;
	gchar *output = NULL;

	switch (InfoOperation)
	{
	case FS_STATUS_OP_LIST:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "list start" : "list end", path, &output, NULL);
		break;

	case FS_STATUS_OP_GET_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_single start" : "get_single end", path, &output, NULL);
		break;

	case FS_STATUS_OP_GET_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_multi start" : "get_multi end", path, &output, NULL);
		break;

	case FS_STATUS_OP_PUT_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_single start" : "put_single end", path, &output, NULL);
		break;

	case FS_STATUS_OP_PUT_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_multi start" : "put_multi end", path, &output, NULL);
		break;

	case FS_STATUS_OP_GET_MULTI_THREAD:
		thread = TRUE;
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "get_multi_thread start" : "get_multi_thread end", path, &output, NULL);
		break;

	case FS_STATUS_OP_PUT_MULTI_THREAD:
		thread = TRUE;
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "put_multi_thread start" : "put_multi_thread end", path, &output, NULL);
		break;

	case FS_STATUS_OP_RENMOV_SINGLE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "renmov_single start" : "renmov_single end", path, &output, NULL);
		break;

	case FS_STATUS_OP_RENMOV_MULTI:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "renmov_multi start" : "renmov_multi end", path, &output, NULL);
		break;

	case FS_STATUS_OP_DELETE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "delete start" : "delete end", path, &output, NULL);
		break;

	case FS_STATUS_OP_ATTRIB:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "attrib start" : "attrib end", path, &output, NULL);
		break;

	case FS_STATUS_OP_MKDIR:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "mkdir start" : "mkdir end", path, &output, NULL);
		break;

	case FS_STATUS_OP_EXEC:
		if (InfoStartEnd == FS_STATUS_START)
			g_strlcpy(gExecStart, script, MAX_PATH);
		else if (g_strcmp0(gExecStart, gScript) != 0)
			return;

		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "exec start" : "exec end", path, &output, NULL);
		break;

	case FS_STATUS_OP_CALCSIZE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "calcsize start" : "calcsize end", path, &output, NULL);
		break;

	case FS_STATUS_OP_SEARCH:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "search start" : "search end", path, &output, NULL);
		break;

	case FS_STATUS_OP_SEARCH_TEXT:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "search_text start" : "search_text end", path, &output, NULL);
		break;

	case FS_STATUS_OP_SYNC_SEARCH:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_search start" : "sync_search end", path, &output, NULL);
		break;

	case FS_STATUS_OP_SYNC_GET:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_get start" : "sync_get end", path, &output, NULL);
		break;

	case FS_STATUS_OP_SYNC_PUT:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_put start" : "sync_put end", path, &output, NULL);
		break;

	case FS_STATUS_OP_SYNC_DELETE:
		ExecuteScript(script, VERB_STATUS, (InfoStartEnd == FS_STATUS_START) ? "sync_delete start" : "sync_delete end", path, &output, NULL);
		break;
	}

	if (output && output[0] != '\0')
		ParseOpts(script, output, thread);

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
	strcpy(pos, "/doublecmd.xml");
	gchar *command = g_strdup_printf("sh -c 'grep -A1 \"<RunInTerminalCloseCmd>\" \"%s\" | sed \"s/\\s*<[^>]*>//g\" | tr \"\\n\\r\" \" \"'", def_ini);
	g_spawn_command_line_sync(command, &gTermCmdClose, NULL, NULL, NULL);
	g_free(command);
	command = g_strdup_printf("sh -c 'grep -A1 \"<RunInTerminalStayOpenCmd>\" \"%s\" | sed \"s/\\s*<[^>]*>//g\" | tr \"\\n\\r\" \" \"'", def_ini);
	g_spawn_command_line_sync(command, &gTermCmdOpen, NULL, NULL, NULL);
	g_free(command);
	command = g_strdup_printf("sh -c 'grep -A1 \"    <Viewer>\" \"%s\" | sed \"s/\\s*<[^>]*>//g\" | tr -d \"\\n\\r\"'", def_ini);
	g_spawn_command_line_sync(command, &gViewerFont, NULL, NULL, NULL);
	g_free(command);

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
		gDebugFd = g_file_open_tmp(CFG_NAME "_XXXXXX.log", &gDebugFileName, NULL);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	gLogProc = NULL;
	g_key_file_remove_group(gCfg, "/", NULL);
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

	if (gTermCmdOpen != NULL)
		g_free(gTermCmdOpen);

	if (gTermCmdClose != NULL)
		g_free(gTermCmdClose);

	if (gViewerFont != NULL)
		g_free(gViewerFont);

	if (gCfg != NULL)
		g_key_file_free(gCfg);

	if (gDialogApi != NULL)
		free(gDialogApi);

	if (gLastValues)
	{
		g_strfreev(gLastValues);
		gLastValues = NULL;
	}

	if (gDebugFd != -1)
	{
		close(gDebugFd);
		remove(gDebugFileName);
		g_free(gDebugFileName);
	}

	gDialogApi = NULL;
}
