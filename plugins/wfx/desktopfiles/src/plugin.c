#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

#define ROOTNAME "Desktop Files"
#define NEWFILE "<Create New>"

typedef struct sVFSDirData
{
	GList *list;
	GList *iter;
} tVFSDirData;

typedef struct sField
{
	char *name;
	int type;
} tField;

#define fieldcount (sizeof(gFields)/sizeof(tField))

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

static char gLFMPath[EXT_MAX_PATH];

static char gRootName[MAX_PATH];
static char gHomeAppDir[MAX_PATH];
static char gLastID[MAX_PATH];
static char gLastDesktopFile[EXT_MAX_PATH];
gchar **gCategories = NULL;
gsize gCategoriesCount = 0;
GKeyFile *gKeyFile = NULL;

tField gFields[] =
{
	{"filename",		ft_string},
	{"geneticname",		ft_string},
	{"hidden",	       ft_boolean},
	{"nodisplay",	       ft_boolean},
	{"custom",	       ft_boolean},
};

static GAppInfo* GetAppInfoByID(char *id)
{
	GAppInfo *result = NULL;

	GList *list = g_app_info_get_all();

	for (GList *l = list; l != NULL; l = l->next)
	{
		if (strcmp(id, g_app_info_get_id(l->data)) == 0)
		{
			result = g_app_info_dup(l->data);
			break;
		}
	}

	g_list_free_full(list, g_object_unref);

	return result;
}

static BOOL Translate(const char *string, char *output, int maxlen)
{
	char id[256];

	if (gExtensions->Translation != NULL)
	{
		snprintf(id, sizeof(id) - 1, "#: MiscStr.%s", string);

		if (gExtensions->TranslateString(gExtensions->Translation, id, string, output, maxlen - 1) > 0)
			return TRUE;
	}

	g_strlcpy(output, string, maxlen - 1);
	return FALSE;
}

static void ErrMsg(char *string, char *file)
{
	char msg[PATH_MAX];

	Translate(string, msg, sizeof(msg));

	if (!file)
		MessageBox(msg, gRootName, MB_OK | MB_ICONERROR);
	else
	{
		gchar *message = g_strdup_printf(msg, file);
		MessageBox(message, gRootName, MB_OK | MB_ICONERROR);
		g_free(message);
	}
}

static gchar* GetLocalestringKey(gchar *key, char *group)
{
	gchar *result = NULL;
	const gchar * const *lang_names = g_get_language_names();

	while (*lang_names)
	{
		result = g_strdup_printf("%s[%s]", key, *lang_names);

		if (g_key_file_has_key(gKeyFile, group, result, NULL))
			break;
		else
		{
			g_free(result);
			result = NULL;
		}

		*lang_names++;
	}

	if (!result)
		result = g_strdup(key);

	return result;
}

static void DesktopFileGetLocalestring(gchar *key, uintptr_t pDlg, char* DlgItemName, gchar *group)
{
	gchar *localekey = GetLocalestringKey(key, group);
	gchar *value = g_key_file_get_string(gKeyFile, group, localekey, NULL);

	if (value)
	{
		SendDlgMsg(pDlg, DlgItemName, DM_SETTEXT, (intptr_t)value, 0);
		g_free(value);
	}

	g_free(localekey);
}

static void DesktopFileSetValue(gchar *key, uintptr_t pDlg, char* DlgItemName, gchar *group)
{
	if (g_str_has_prefix(DlgItemName, "ch"))
	{
		gboolean value = (gboolean)SendDlgMsg(pDlg, DlgItemName, DM_GETCHECK, 0, 0);
		g_key_file_set_boolean(gKeyFile, group, key, value);
	}
	else if (g_str_has_prefix(DlgItemName, "ed"))
	{
		char *value = (char*)SendDlgMsg(pDlg, DlgItemName, DM_GETTEXT, 0, 0);

		if (value && value[0] != '\0')
			g_key_file_set_string(gKeyFile, group, key, value);
		else if (g_key_file_has_key(gKeyFile, group, key, NULL))
			g_key_file_set_string(gKeyFile, group, key, value);
	}
}

static void DesktopFileSetLocalestring(gchar *key, uintptr_t pDlg, char* DlgItemName, gchar *group)
{
	gchar *localekey = GetLocalestringKey(key, group);
	DesktopFileSetValue(localekey, pDlg, DlgItemName, group);
	g_free(localekey);
}

static void DesktopFileGetString(gchar *key, uintptr_t pDlg, char* DlgItemName, gchar *group)
{
	gchar *value = g_key_file_get_string(gKeyFile, group, key, NULL);

	if (value)
	{
		SendDlgMsg(pDlg, DlgItemName, DM_SETTEXT, (intptr_t)value, 0);
		g_free(value);
	}
}

static void DesktopFileSetStringListFromListBox(gchar *key, uintptr_t pDlg, char* DlgItemName)
{
	gsize count = (gsize)SendDlgMsg(pDlg, DlgItemName, DM_LISTGETCOUNT, 0, 0);

	if (count > 0)
	{
		gchar *types = NULL;

		for (gsize i = 0; i < count; i++)
		{
			char *type = (char*)SendDlgMsg(pDlg, DlgItemName, DM_LISTGETITEM, i, 0);

			if (type && strlen(type) > 0)
			{
				if (!types)
					types = g_strdup_printf("%s;", type);
				else
				{
					gchar *tmp = g_strdup_printf("%s%s;", types, type);
					g_free(types);
					types = tmp;
				}
			}
		}

		if (types)
		{
			g_key_file_set_string(gKeyFile, "Desktop Entry", key, types);
			g_free(types);
		}
	}
	else if (g_key_file_has_key(gKeyFile, "Desktop Entry", key, NULL))
		g_key_file_set_string(gKeyFile, "Desktop Entry", key, "");
}

static void SaveDesktopFile(uintptr_t pDlg, char *file)
{

	DesktopFileSetLocalestring("Name", pDlg, "edName", "Desktop Entry");
	DesktopFileSetLocalestring("GenericName", pDlg, "edGenericName", "Desktop Entry");
	DesktopFileSetLocalestring("Comment", pDlg, "edComment", "Desktop Entry");
	DesktopFileSetLocalestring("Keywords", pDlg, "edKeywords", "Desktop Entry");
	DesktopFileSetValue("Icon", pDlg, "edIcon", "Desktop Entry");
	DesktopFileSetValue("Exec", pDlg, "edExec", "Desktop Entry");
	DesktopFileSetValue("TryExec", pDlg, "edTryExec", "Desktop Entry");
	DesktopFileSetValue("Path", pDlg, "edPath", "Desktop Entry");
	DesktopFileSetValue("StartupNotify", pDlg, "chStartupNotify", "Desktop Entry");
	DesktopFileSetValue("Terminal", pDlg, "chTerminal", "Desktop Entry");
	DesktopFileSetValue("NoDisplay", pDlg, "chNoDisplay", "Desktop Entry");
	DesktopFileSetValue("Hidden", pDlg, "chHidden", "Desktop Entry");
	DesktopFileSetStringListFromListBox("Categories", pDlg, "lbCategories");
	DesktopFileSetStringListFromListBox("MimeType", pDlg, "lbMimeType");

	if (!g_key_file_save_to_file(gKeyFile, file, NULL))
		ErrMsg("Failed to save %s", file);
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		SendDlgMsg(pDlg, "edFileName", DM_SETTEXT, (intptr_t)gLastID, 0);
		DesktopFileGetLocalestring("Name", pDlg, "edName", "Desktop Entry");
		DesktopFileGetLocalestring("GenericName", pDlg, "edGenericName", "Desktop Entry");
		DesktopFileGetLocalestring("Comment", pDlg, "edComment", "Desktop Entry");
		DesktopFileGetLocalestring("Keywords", pDlg, "edKeywords", "Desktop Entry");

		DesktopFileGetString("Icon", pDlg, "edIcon", "Desktop Entry");
		DesktopFileGetString("Exec", pDlg, "edExec", "Desktop Entry");
		DesktopFileGetString("TryExec", pDlg, "edTryExec", "Desktop Entry");
		DesktopFileGetString("Path", pDlg, "edPath", "Desktop Entry");

		if (g_key_file_get_boolean(gKeyFile, "Desktop Entry", "StartupNotify", NULL))
			SendDlgMsg(pDlg, "chStartupNotify", DM_SETCHECK, 1, 0);

		if (g_key_file_get_boolean(gKeyFile, "Desktop Entry", "Terminal", NULL))
			SendDlgMsg(pDlg, "chTerminal", DM_SETCHECK, 1, 0);

		if (g_key_file_get_boolean(gKeyFile, "Desktop Entry", "NoDisplay", NULL))
			SendDlgMsg(pDlg, "chNoDisplay", DM_SETCHECK, 1, 0);

		if (g_key_file_get_boolean(gKeyFile, "Desktop Entry", "Hidden", NULL))
			SendDlgMsg(pDlg, "chHidden", DM_SETCHECK, 1, 0);

		gsize len = 0;
		gchar **list = g_key_file_get_string_list(gKeyFile, "Desktop Entry", "Categories", &len, NULL);

		for (gsize i = 0; i < gCategoriesCount; i++)
		{
			if (list)
			{
				if (strlen(gCategories[i]) > 0 && !g_strv_contains((const gchar * const*)list, gCategories[i]))
					SendDlgMsg(pDlg, "cbAll", DM_LISTADDSTR, (intptr_t)gCategories[i], 0);
			}
			else if (strlen(gCategories[i]) > 0)
				SendDlgMsg(pDlg, "cbAll", DM_LISTADDSTR, (intptr_t)gCategories[i], 0);
		}

		if (list)
		{
			for (gsize i = 0; i < len; i++)
				SendDlgMsg(pDlg, "lbCategories", DM_LISTADDSTR, (intptr_t)list[i], 0);

			g_strfreev(list);
		}

		list = g_key_file_get_string_list(gKeyFile, "Desktop Entry", "Actions", &len, NULL);

		if (list)
		{
			for (gsize i = 0; i < len; i++)
				SendDlgMsg(pDlg, "lbActions", DM_LISTADDSTR, (intptr_t)list[i], 0);

			g_strfreev(list);
		}

		list = g_key_file_get_string_list(gKeyFile, "Desktop Entry", "MimeType", &len, NULL);

		GList *mimes = g_content_types_get_registered();

		for (GList *l = mimes; l != NULL; l = l->next)
		{
			if (list != NULL && g_strv_contains((const gchar * const*)list, l->data))
				SendDlgMsg(pDlg, "lbMimeType", DM_LISTADDSTR, (intptr_t)l->data, 0);
			else
				SendDlgMsg(pDlg, "cbMimeAll", DM_LISTADDSTR, (intptr_t)l->data, 0);
		}

		if (list)
			g_strfreev(list);

		g_list_free_full(mimes, g_free);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnSave") == 0)
		{

			char file[PATH_MAX];
			snprintf(file, sizeof(file), "%s/%s", gHomeAppDir, gLastID);

			SaveDesktopFile(pDlg, file);
		}
		else if (strcmp(DlgItemName, "btnSaveAs") == 0)
		{
			char newfile[MAX_PATH];
			g_strlcpy(newfile, gLastID, MAX_PATH);
			char msg[PATH_MAX];
			Translate("Enter filename:", msg, sizeof(msg));

			if (InputBox(gRootName, msg, FALSE, newfile, sizeof(newfile)))
			{
				gchar *filename = NULL;

				if (!g_str_has_suffix(newfile, ".desktop"))
					filename = g_strdup_printf("%s/%s.desktop", gHomeAppDir, newfile);
				else
					filename = g_strdup_printf("%s/%s", gHomeAppDir, newfile);

				if (g_file_test(filename, G_FILE_TEST_EXISTS))
				{
					Translate("File %s exists, overwrite?", msg, sizeof(msg));
					gchar *message = g_strdup_printf(msg, filename);

					if (MessageBox(message, gRootName, MB_YESNO | MB_ICONQUESTION) == ID_YES)
						SaveDesktopFile(pDlg, filename);

					g_free(message);
				}
				else
					SaveDesktopFile(pDlg, filename);

				g_free(filename);
			}
		}
		else if (strcmp(DlgItemName, "btnAdd") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "cbAll", DM_LISTGETITEMINDEX, 0, 0);

			gchar *content = g_strdup((char*)SendDlgMsg(pDlg, "cbAll", DM_GETTEXT, 0, 0));

			if (content && content[0] != '\0')
			{
				SendDlgMsg(pDlg, "lbCategories", DM_LISTADDSTR, (intptr_t)content, 0);

				if (i != -1)
					SendDlgMsg(pDlg, "cbAll", DM_LISTDELETE, i, 0);
				else
					SendDlgMsg(pDlg, "cbAll", DM_SETTEXT, 0, 0);
			}

			SendDlgMsg(pDlg, "cbAll", DM_LISTSETITEMINDEX, -1, 0);
			g_free(content);

		}
		else if (strcmp(DlgItemName, "btnDel") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbCategories", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				gchar *content = g_strdup((char*)SendDlgMsg(pDlg, "lbCategories", DM_LISTGETITEM, i, 0));
				SendDlgMsg(pDlg, "cbAll", DM_LISTADDSTR, (intptr_t)content, 0);
				SendDlgMsg(pDlg, "lbCategories", DM_LISTDELETE, i, 0);
				g_free(content);
			}
		}
		else if (strcmp(DlgItemName, "btnActLaunch") == 0)
		{
			char *command = (char*)SendDlgMsg(pDlg, "edActExec", DM_GETTEXT, 0, 0);

			if (command && command[0] != '\0')
				g_spawn_command_line_async(command, NULL);
		}
		else if (strcmp(DlgItemName, "btnActDel") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *action = ((char*)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEM, i, 0));

				if (action && action[0] != '\0')
				{
					gchar *group = g_strdup_printf("Desktop Action %s", action);

					if (g_key_file_has_group(gKeyFile, group))
						g_key_file_remove_group(gKeyFile, group, NULL);

					g_free(group);
				}

				SendDlgMsg(pDlg, "lbActions", DM_LISTDELETE, i, 0);
				DesktopFileSetStringListFromListBox("Actions", pDlg, "lbActions");
			}
		}
		else if (strcmp(DlgItemName, "btnActAdd") == 0)
		{
			char newact[MAX_PATH] = "";
			char msg[PATH_MAX];
			Translate("New action:", msg, sizeof(msg));

			if (InputBox(gRootName, msg, FALSE, newact, sizeof(newact)))
			{
				gboolean is_ok = TRUE;
				if (strlen(newact) < 1)
				{
					is_ok = FALSE;
					ErrMsg("No data has been entered.", NULL);
				}
				else
				{
					gsize count = (gsize)SendDlgMsg(pDlg, "lbActions", DM_LISTGETCOUNT, 0, 0);

					if (count > 0)
					{

						for (gsize i = 0; i < count; i++)
						{
							char *action = (char*)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEM, i, 0);
							if (strcmp(action, newact) == 0)
							{
								is_ok = FALSE;
								ErrMsg("Action already exist.", NULL);
							}
						}
					}
				}

				if (is_ok)
				{
					SendDlgMsg(pDlg, "lbActions", DM_LISTADDSTR, (intptr_t)newact, 0);
					DesktopFileSetStringListFromListBox("Actions", pDlg, "lbActions");
				}
			}
		}
		else if (strcmp(DlgItemName, "btnApply") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *action = ((char*)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEM, i, 0));

				if (action && action[0] != '\0')
				{
					gchar *group = g_strdup_printf("Desktop Action %s", action);
					DesktopFileSetLocalestring("Name", pDlg, "edActName", group);
					DesktopFileSetValue("Exec", pDlg, "edActExec", group);
					DesktopFileSetValue("Icon", pDlg, "edActIcon", group);
					g_free(group);
				}
			}
		}
		else if (strcmp(DlgItemName, "btnMimeAdd") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "cbMimeAll", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				gchar *content = g_strdup((char*)SendDlgMsg(pDlg, "cbMimeAll", DM_LISTGETITEM, i, 0));
				SendDlgMsg(pDlg, "lbMimeType", DM_LISTADDSTR, (intptr_t)content, 0);
				SendDlgMsg(pDlg, "cbMimeAll", DM_LISTDELETE, i, 0);
				g_free(content);
			}
		}
		else if (strcmp(DlgItemName, "btnMimeDel") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbMimeType", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				gchar *content = g_strdup((char*)SendDlgMsg(pDlg, "lbMimeType", DM_LISTGETITEM, i, 0));
				SendDlgMsg(pDlg, "cbMimeAll", DM_LISTADDSTR, (intptr_t)content, 0);
				SendDlgMsg(pDlg, "lbMimeType", DM_LISTDELETE, i, 0);
				g_free(content);
			}
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "lbActions") == 0)
		{
			SendDlgMsg(pDlg, "edActName", DM_SETTEXT, 0, 0);
			SendDlgMsg(pDlg, "edActExec", DM_SETTEXT, 0, 0);
			SendDlgMsg(pDlg, "edActIcon", DM_SETTEXT, 0, 0);
			int i = (int)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *action = ((char*)SendDlgMsg(pDlg, "lbActions", DM_LISTGETITEM, i, 0));

				if (action && action[0] != '\0')
				{
					gchar *group = g_strdup_printf("Desktop Action %s", action);

					if (g_key_file_has_group(gKeyFile, group))
					{
						DesktopFileGetLocalestring("Name", pDlg, "edActName", group);
						DesktopFileGetString("Exec", pDlg, "edActExec", group);
						DesktopFileGetString("Icon", pDlg, "edActIcon", group);
					}

					g_free(group);

				}
			}
		}

		break;
	}

	return 0;
}

static BOOL PropertiesDialog(void)
{
	gKeyFile = g_key_file_new();

	if (g_key_file_load_from_file(gKeyFile, gLastDesktopFile, G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
		return gExtensions->DialogBoxLFMFile(gLFMPath, PropertiesDlgProc);
	else
		ErrMsg("Failed to open %s", gLastDesktopFile);

	g_key_file_free(gKeyFile);
}

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	int64_t ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}


static BOOL SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	GList *iter = dirdata->iter;

	if (iter)
	{
		const char *id = g_app_info_get_id(iter->data);
		g_strlcpy(FindData->cFileName, id, sizeof(FindData->cFileName) - 1);
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;

		GDesktopAppInfo *info = g_desktop_app_info_new(id);

		if (info)
		{
			struct stat buf;
			const char *filename = g_desktop_app_info_get_filename(info);

			if (lstat(filename, &buf) == 0)
			{
				FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
				FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
				UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
				UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
				FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
				FindData->dwReserved0 = buf.st_mode;
			}

			g_object_unref(info);
		}

		dirdata->iter = iter->next;
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

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->list = g_app_info_get_all();
	dirdata->iter = dirdata->list;

	char addaction[MAX_PATH];
	Translate(NEWFILE, addaction, sizeof(addaction));
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	g_strlcpy(FindData->cFileName, addaction, sizeof(FindData->cFileName) - 1);
	FindData->nFileSizeHigh = 0xFFFFFFFF;
	FindData->nFileSizeLow = 0xFFFFFFFE;
	FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->dwFileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;

	return (HANDLE)dirdata;
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->list)
		g_list_free(dirdata->list);

	g_free(dirdata);

	return 0;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0 && strlen(RemoteName) > 1)
	{
		char addaction[MAX_PATH];
		g_strlcpy(addaction, NEWFILE, MAX_PATH);
		Translate(NEWFILE, addaction, sizeof(addaction));

		if (strcmp(RemoteName + 1, addaction) == 0)
		{
			char newfile[MAX_PATH] = "";
			char msg[PATH_MAX];
			Translate("Enter filename:", msg, sizeof(msg));

			if (InputBox(gRootName, msg, FALSE, newfile, sizeof(newfile)))
			{
				if (strlen(newfile) < 1)
					ErrMsg("No data has been entered.", NULL);
				else
				{
					gchar *filename;

					if (!g_str_has_suffix(newfile, ".desktop"))
						filename = g_strdup_printf("%s/%s.desktop", gHomeAppDir, newfile);
					else
					{
						filename = g_strdup_printf("%s/%s", gHomeAppDir, newfile);
						char *pos = strrchr(newfile, '.');

						if (pos)
							*pos = '\0';
					}

					GKeyFile *desktopfile = g_key_file_new();
					g_key_file_set_string(desktopfile, "Desktop Entry", "Version", "1.0");
					g_key_file_set_string(desktopfile, "Desktop Entry", "Type", "Application");
					g_key_file_set_string(desktopfile, "Desktop Entry", "Name", newfile);
					g_key_file_set_string(desktopfile, "Desktop Entry", "Comment", "");
					g_key_file_set_string(desktopfile, "Desktop Entry", "Icon", newfile);
					g_key_file_set_string(desktopfile, "Desktop Entry", "Exec", newfile);
					g_key_file_set_string(desktopfile, "Desktop Entry", "Terminal", "false");

					if (!g_key_file_save_to_file(desktopfile, filename, NULL))
						ErrMsg("Failed to save %s", filename);
					else
					{
						g_strlcpy(gLastDesktopFile, filename, PATH_MAX);
						gchar *name = g_path_get_basename(gLastDesktopFile);
						g_strlcpy(gLastID, name, PATH_MAX);
						g_free(name);
						PropertiesDialog();
					}

					g_key_file_free(desktopfile);
				}
			}
		}
		else
		{

			GAppInfo *appinfo = GetAppInfoByID(RemoteName + 1);

			if (appinfo)
			{
				g_app_info_launch(appinfo, NULL, NULL, NULL);
				g_object_unref(appinfo);
			}
		}

		return FS_EXEC_OK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{

		g_strlcpy(gLastDesktopFile, RemoteName, PATH_MAX);

		if (FsGetLocalName(gLastDesktopFile, PATH_MAX))
		{
			g_strlcpy(gLastID, RemoteName + 1, PATH_MAX);
			PropertiesDialog();
		}

		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}


int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int result = FS_FILE_OK;
	char filename[PATH_MAX];

	if (gProgressProc(gPluginNr, RemoteName, RemoteName, 0) == 1)
		return FS_FILE_USERABORT;

	if ((CopyFlags == 0) && (g_file_test(LocalName, G_FILE_TEST_EXISTS)))
		return FS_FILE_EXISTS;


	GKeyFile *desktopfile = g_key_file_new();

	g_strlcpy(filename, RemoteName, PATH_MAX);

	if (FsGetLocalName(filename, PATH_MAX) && g_key_file_load_from_file(desktopfile, filename, G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
	{
		if (!g_key_file_save_to_file(desktopfile, LocalName, NULL))
			result = FS_FILE_WRITEERROR;
	}
	else
		result = FS_FILE_READERROR;

	g_key_file_free(desktopfile);
	gProgressProc(gPluginNr, RemoteName, RemoteName, 100);
	return result;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	gboolean result = FALSE;
	GAppInfo *appinfo = GetAppInfoByID(RemoteName + 1);

	if (appinfo)
	{
		result = g_app_info_delete(appinfo);
		g_object_unref(appinfo);
	}

	return result;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	GDesktopAppInfo *info = g_desktop_app_info_new(RemoteName + 1);

	if (info)
	{
		const char *filename = g_desktop_app_info_get_filename(info);
		g_strlcpy(RemoteName, filename, maxlen - 1);
		g_object_unref(info);
		return TRUE;
	}
	else
		return FALSE;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);
	Units[0] = '\0';
	return gFields[FieldIndex].type;
}

int DCPCALL FsContentGetDefaultSortOrder(int FieldIndex)
{
	if (gFields[FieldIndex].type  == ft_boolean)
		return -1;

	return 1;
}


int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (strcmp(FileName, "/..") == 0)
		return ft_fileerror;


	int result = gFields[FieldIndex].type;
	GDesktopAppInfo *info = g_desktop_app_info_new(FileName + 1);

	if (!info)
		return ft_fieldempty;

	const char *string;

	switch (FieldIndex)
	{
	case 0:
		string = g_desktop_app_info_get_filename(info);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			result = ft_fieldempty;

		break;

	case 1:
		string = g_desktop_app_info_get_generic_name(info);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			result = ft_fieldempty;

		break;

	case 2:
		*(int*)FieldValue = (int)g_desktop_app_info_get_is_hidden(info);
		break;

	case 3:
		*(int*)FieldValue = (int)g_desktop_app_info_get_nodisplay(info);
		break;

	case 4:
		string = g_desktop_app_info_get_filename(info);

		if (strncmp(string, "/home", 5) != 0)
			*(int*)FieldValue = 0;
		else
			*(int*)FieldValue = 1;

		break;

	default:
		result = ft_nosuchfield;
	}

	if (info)
		g_object_unref(info);

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	Translate("GeneticName\\nHidden\\nNoDisplay\\nCustom", ViewHeaders, maxlen - 1);
	g_strlcpy(ViewContents, "[Plugin(FS).geneticname{}]\\n[Plugin(FS).hidden{}]\\n[Plugin(FS).nodisplay{}]\\n[Plugin(FS).custom{}]", maxlen - 1);
	g_strlcpy(ViewWidths, "100,0,70,10,10,10", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, gRootName, maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
		Translate(ROOTNAME, gRootName, sizeof(gRootName));
		snprintf(gHomeAppDir, sizeof(gHomeAppDir), "%s/.local/share/applications", g_getenv("HOME"));

		snprintf(gLFMPath, sizeof(gLFMPath), "%sdialog.lfm", gExtensions->PluginDir);

		GList *list = g_app_info_get_all();
		GKeyFile *temp = g_key_file_new();

		for (GList *l = list; l != NULL; l = l->next)
		{
			const char *id = g_app_info_get_id(l->data);
			GDesktopAppInfo *info = g_desktop_app_info_new(id);

			if (info)
			{
				if (g_desktop_app_info_has_key(info, "Categories"))
				{
					gsize len = 0;
					gchar **list = g_desktop_app_info_get_string_list(info, "Categories", &len);

					if (list)
					{
						for (gsize i = 0; i < len; i++)
						{
							if (gCategories)
							{
								if (!g_strv_contains((const gchar * const*)gCategories, list[i]))
								{
									gchar *oldvalue = g_key_file_get_string(temp, "wtf", "dunno", NULL);
									gchar *value = g_strdup_printf("%s;%s", oldvalue, list[i]);
									g_free(oldvalue);
									g_key_file_set_string(temp, "wtf", "dunno", value);
									g_free(value);
									g_strfreev(gCategories);
									gCategories = g_key_file_get_string_list(temp, "wtf", "dunno", &gCategoriesCount, NULL);
								}

							}
							else
							{
								gchar *value = g_strdup_printf("%s;", list[i]);
								g_key_file_set_string(temp, "wtf", "dunno", value);
								g_free(value);
								gCategories = g_key_file_get_string_list(temp, "wtf", "dunno", &gCategoriesCount, NULL);
							}
						}

						g_strfreev(list);
					}
				}

				g_object_unref(info);
			}
		}

		g_list_free_full(list, g_object_unref);
		g_key_file_free(temp);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;

	if (gCategories)
		g_strfreev(gCategories);

	gCategories = NULL;
}
