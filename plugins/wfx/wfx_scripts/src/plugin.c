#include <stdio.h>
#include <glib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define MAX_LINES 10
#define EXEC_DIR "scripts"
#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define LIST_REGEXP "([drwx\\-]{10})\\s+(\\d{4}\\-?\\d{2}\\-?\\d{2}[\\stT]\\d{2}:?\\d{2}:?\\d{2}Z)\\s+([0-9\\-]+)\\s+([^\\n]+)"

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


typedef struct sVFSDirData
{
	gchar *output;
	GRegex *regex;
	GMatchInfo *match_info;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;

static GKeyFile *g_cfg = NULL;
static gboolean g_noise = FALSE;
static gboolean g_no_dialog = FALSE;
static char g_script[PATH_MAX] = "default.run";
static char g_scripts_dir[PATH_MAX];
static char g_lfm_path[PATH_MAX];
static char g_history_file[PATH_MAX];



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

static gboolean ExecuteScript(gchar *verb, char *arg1, char *arg2, gchar **output)
{
	gint status = 0;
	gchar *command =  NULL;
	gboolean result = TRUE;

	if (g_script[0] == '\0')
		return FALSE;

	gchar *path = g_strdup_printf("%s/%s", g_scripts_dir, g_script);
	gchar *script = g_shell_quote(path);
	g_free(path);

	if (!arg1 && !arg2)
		command = g_strdup_printf("%s %s", script, verb);
	else
	{
		gchar *quoted_arg1 = g_shell_quote(arg1);

		if (!arg2)
			command = g_strdup_printf("%s %s %s", script, verb, quoted_arg1);
		else
		{
			gchar *quoted_arg2 = g_shell_quote(arg2);
			command = g_strdup_printf("%s %s %s %s", script, verb, quoted_arg1, quoted_arg2);
		}

		g_free(quoted_arg1);
	}

	g_free(script);

	if (g_noise)
		g_print("---> %s\n", command);

	result = g_spawn_command_line_sync(command, output, NULL, &status, NULL);

	if (g_noise)
		g_print("---\n%s---\nexit status %d\n", output ? *output : "", status);

	g_free(command);


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

	gchar *src_file = g_strdup_printf("%s/%s", g_scripts_dir, file);

	if ((fp = fopen(src_file, "r")) != NULL)
	{
		while ((read = getline(&line, &len, fp)) != -1 && count < MAX_LINES)
		{
			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			gDialogApi->SendDlgMsg(pDlg, "mPreview", DM_LISTADD, (intptr_t)line, 0);

			count++;
		}

		g_free(line);
		fclose(fp);
	}

	g_free(src_file);
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	DIR *dir;
	char *line = NULL;
	struct dirent *ent;
	gchar *src_file;

	switch (Msg)
	{
	case DN_INITDIALOG:
		if ((dir = opendir(g_scripts_dir)) != NULL)
		{
			while ((ent = readdir(dir)) != NULL)
			{
				if (ent->d_type == DT_REG)
				{
					src_file = g_strdup_printf("%s/%s", g_scripts_dir, ent->d_name);

					if (g_file_test(src_file, G_FILE_TEST_EXISTS))
						gDialogApi->SendDlgMsg(pDlg, "cbScript", DM_LISTADD, (intptr_t)ent->d_name, 0);

					g_free(src_file);
				}
			}

			closedir(dir);

			src_file = g_key_file_get_string(g_cfg, "wfx_scripts", "last_script", NULL);
			gDialogApi->SendDlgMsg(pDlg, "cbScript", DM_SETTEXT, (intptr_t)src_file, 0);

			if (gDialogApi->SendDlgMsg(pDlg, "cbScript", DM_LISTGETITEMINDEX, 0, 0) != -1)
				LoadPreview(pDlg, src_file);

			g_free(src_file);
		}

		gDialogApi->SendDlgMsg(pDlg, "ckNoise", DM_SETCHECK, (intptr_t)g_noise, 0);


		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			ExecuteScript(VERB_DEINIT, NULL, NULL, NULL);

			line = (char*)gDialogApi->SendDlgMsg(pDlg, "cbScript", DM_GETTEXT, 0, 0);

			if (line)
			{
				g_strlcpy(g_script, line, PATH_MAX);
				g_key_file_set_string(g_cfg, "wfx_scripts", "last_script", g_script);
			}

			g_noise = (gboolean)gDialogApi->SendDlgMsg(pDlg, "ckNoise", DM_GETCHECK, 0, 0);
			g_key_file_set_boolean(g_cfg, "wfx_scripts", "noise_on_stdout", g_noise);

			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}
		else if (strcmp(DlgItemName, "btnCancel") == 0)
			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_CANCEL, 0);

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "cbScript") == 0)
		{
			gDialogApi->SendDlgMsg(pDlg, "mPreview", DM_SETTEXT, 0, 0);
			line = (char*)gDialogApi->SendDlgMsg(pDlg, "cbScript", DM_GETTEXT, 0, 0);
			LoadPreview(pDlg, line);
		}

		break;
	}

	return 0;
}

static void ShowCfgDlg(void)
{
	if (gDialogApi && !g_no_dialog)
	{
		if (g_file_test(g_lfm_path, G_FILE_TEST_EXISTS))
			gDialogApi->DialogBoxLFMFile(g_lfm_path, DlgProc);
	}
	else
	{
		gchar *last_script = g_key_file_get_string(g_cfg, "wfx_scripts", "last_script", NULL);

		if (last_script)
		{
			g_strlcpy(g_script, last_script, PATH_MAX);
			g_free(last_script);
		}
	}

	gchar *output = NULL;
	ExecuteScript(VERB_INIT, NULL, NULL, &output);

	if (output)
	{
		gchar **split = g_strsplit(output, "\n", -1);
		g_free(output);
		gchar **p = split;

		while (*p)
		{
			char value[MAX_PATH] = "";
			gchar *prev = g_key_file_get_string(g_cfg, g_script, *p, NULL);

			if (prev)
			{
				g_strlcpy(value, prev, MAX_PATH);
				g_free(prev);
			}


			if (*p[0] != 0)
			{
				if (gRequestProc && gRequestProc(gPluginNr, RT_Other, g_script, *p, value, MAX_PATH))
				{
					ExecuteScript(VERB_SETOPT, *p, value, &output);
					g_key_file_set_string(g_cfg, g_script, *p, value);
				}
			}

			*p++;
		}

		g_strfreev(split);
	}
}

static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while (g_match_info_matches(dirdata->match_info))
	{
		gchar *word = g_match_info_fetch(dirdata->match_info, 0);

		if (g_noise)
			g_print("Found: %s\n", word);

		g_free(word);

		word = g_match_info_fetch(dirdata->match_info, 1);

		if (g_noise)
			g_print("mode: %s\n", word);

		int i = 0;
		mode_t mode = S_IFREG;
		FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;

		if (word[i++] == 'd')
			mode = S_IFDIR;

		if (word[i++] == 'r')
			mode |= S_IRUSR;

		if (word[i++] == 'w')
			mode |= S_IWUSR;

		if (word[i++] == 'x')
			mode |= S_IXUSR;

		if (word[i++] == 'r')
			mode |= S_IRGRP;

		if (word[i++] == 'w')
			mode |= S_IWGRP;

		if (word[i++] == 'x')
			mode |= S_IXGRP;

		if (word[i++] == 'r')
			mode |= S_IROTH;

		if (word[i++] == 'w')
			mode |= S_IWOTH;

		if (word[i++] == 'x')
			mode |= S_IXOTH;

		FindData->dwReserved0 = mode;
		g_free(word);

		word = g_match_info_fetch(dirdata->match_info, 2);

		if (g_noise)
			g_print("date: %s\n", word);

		gint64 filetime = 0;
		GDateTime *dt = g_date_time_new_from_iso8601(word, NULL);

		if (dt)
		{
			filetime = g_date_time_to_unix(dt);
			g_date_time_unref(dt);
		}

		if (filetime > 0)
		{
			UnixTimeToFileTime((time_t)filetime, &FindData->ftCreationTime);
			UnixTimeToFileTime((time_t)filetime, &FindData->ftLastAccessTime);
			UnixTimeToFileTime((time_t)filetime, &FindData->ftLastWriteTime);
		}
		else
		{
			SetCurrentFileTime(&FindData->ftCreationTime);
			SetCurrentFileTime(&FindData->ftLastAccessTime);
			SetCurrentFileTime(&FindData->ftLastWriteTime);
		}

		g_free(word);

		word = g_match_info_fetch(dirdata->match_info, 3);

		if (g_noise)
			g_print("size: %s\n", word);

		gdouble filesize = g_ascii_strtod(word, NULL);
		FindData->nFileSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
		FindData->nFileSizeLow = (int64_t)filesize & 0x00000000FFFFFFFF;
		g_free(word);

		word = g_match_info_fetch(dirdata->match_info, 4);

		if (g_noise)
			g_print("name: %s\n", word);

		g_strlcpy(FindData->cFileName, word, sizeof(FindData->cFileName));
		g_free(word);

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

	ShowCfgDlg();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;
	gchar *output = NULL;

	if (!ExecuteScript(VERB_LIST, Path, NULL, &output))
	{
		g_free(output);
		return (HANDLE)(-1);
	}

	dirdata = g_new0(tVFSDirData, 1);

	if (!dirdata)
	{
		g_free(output);
		return (HANDLE)(-1);
	}

	dirdata->regex = g_regex_new(LIST_REGEXP, G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, NULL);

	if (!g_regex_match(dirdata->regex, output, 0, &(dirdata->match_info)))
	{
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

	return dirdata;

}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	g_match_info_free(dirdata->match_info);
	g_regex_unref(dirdata->regex);
	g_free(dirdata->output);
	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	if (!ExecuteScript(VERB_GET_FILE, RemoteName, LocalName, NULL))
		return FS_FILE_NOTSUPPORTED;

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	if (!g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_WRITEERROR;

	return FS_FILE_OK;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	if (gProgressProc(gPluginNr, LocalName, RemoteName, 0))
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && ExecuteScript(VERB_EXISTS, RemoteName, NULL, NULL))
		return FS_FILE_EXISTS;

	if (!ExecuteScript(VERB_PUT_FILE, LocalName, RemoteName, NULL))
		return FS_FILE_NOTSUPPORTED;

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return FS_FILE_OK;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
	if (gProgressProc(gPluginNr, OldName, NewName, 0))
		return FS_FILE_USERABORT;

	if (!OverWrite && ExecuteScript(VERB_EXISTS, NewName, NULL, NULL))
		return FS_FILE_EXISTS;

	if (!ExecuteScript(Move ? VERB_MOVE : VERB_COPY, OldName, NewName, NULL))
		return FS_FILE_NOTSUPPORTED;

	gProgressProc(gPluginNr, OldName, NewName, 100);

	return FS_FILE_OK;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	return ExecuteScript(VERB_MKDIR, Path, NULL, NULL);
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	return ExecuteScript(VERB_REMOVE, RemoteName, NULL, NULL);
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	return ExecuteScript(VERB_RMDIR, RemoteName, NULL, NULL);
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;

	if (strcmp(Verb, "open") == 0)
		result = FS_EXEC_YOURSELF;
	else if (strcmp(Verb, "properties") == 0)
	{
		ShowCfgDlg();
		result = FS_EXEC_OK;
	}
	else if (strncmp(Verb, "chmod", 5) == 0)
	{
		if (ExecuteScript(VERB_CHMOD, RemoteName, Verb + 6, NULL))
			result = FS_EXEC_OK;
	}
	else if (strncmp(Verb, "quote", 5) == 0)
	{
		ExecuteScript(VERB_QUOTE, Verb + 6, NULL, NULL);
		result = FS_EXEC_OK;
	}

	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Scripts", maxlen - 1);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	g_strlcpy(g_history_file, dps->DefaultIniName, PATH_MAX);

	char *pos = strrchr(g_history_file, '/');

	if (pos)
		strcpy(pos + 1, "wfx_scripts.ini");

	if (g_cfg == NULL)
	{
		g_cfg = g_key_file_new();
		g_key_file_load_from_file(g_cfg, g_history_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

		if (!g_key_file_has_key(g_cfg, "wfx_scripts", "noise_on_stdout", NULL))
			g_key_file_set_boolean(g_cfg, "wfx_scripts", "noise_on_stdout", g_noise);
		else
			g_noise = g_key_file_get_boolean(g_cfg, "wfx_scripts", "noise_on_stdout", NULL);

		if (!g_key_file_has_key(g_cfg, "wfx_scripts", "no_dialog", NULL))
			g_key_file_set_boolean(g_cfg, "wfx_scripts", "no_dialog", g_no_dialog);
		else
			g_no_dialog = g_key_file_get_boolean(g_cfg, "wfx_scripts", "no_dialog", NULL);
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
	ExecuteScript(VERB_DEINIT, NULL, NULL, NULL);

	g_key_file_save_to_file(g_cfg, g_history_file, NULL);

	if (g_cfg != NULL)
		g_key_file_free(g_cfg);

	if (gDialogApi != NULL)
		free(gDialogApi);

	gDialogApi = NULL;
}
