#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <string.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include "wcxplugin.h"
#include "extension.h"

typedef void *HINSTANCE;

tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;
static char gLFMPath[PATH_MAX];
static char gCfgPath[PATH_MAX];
static char gLastCMD[PATH_MAX];
static char gLastExt[PATH_MAX];
static char gLastMask[PATH_MAX];
gboolean gQuoteFN = TRUE;
gboolean gCfgMode = FALSE;
GKeyFile *gCfg = NULL;


void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
		g_strlcpy(gLFMPath, gDialogApi->PluginDir, PATH_MAX);
		strncat(gLFMPath, "dialog.lfm", PATH_MAX - 11);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gDialogApi != NULL)
	{
		free(gDialogApi);
	}

	if (gCfg != NULL)
		g_key_file_free(gCfg);

	gDialogApi = NULL;
	gCfg = NULL;
}

static gchar *str_replace(gchar *text, gchar *str, gchar *repl, gboolean quote)
{
	gchar *result = NULL;

	if (!str || !repl || !text)
		return result;

	gchar **split = g_strsplit(text, str, -1);

	if (quote)
	{
		gchar *quoted_repl = g_shell_quote(repl);
		result = g_strjoinv(quoted_repl, split);
		g_free(quoted_repl);
	}
	else
		result = g_strjoinv(repl, split);

	g_strfreev(split);

	return result;
}

static gchar *str_replace_templ(gchar *in_file, gchar *out_file)
{
	gchar *tmp = str_replace(gLastCMD, "$FILE", in_file, gQuoteFN);
	gchar *result = str_replace(tmp, "$OUTPUT", out_file, gQuoteFN);
	g_free(tmp);
	return result;
}
static void edcmd_add_templ(uintptr_t pDlg, char *templ)
{
	char result[PATH_MAX];
	g_strlcpy(result, (char*)gDialogApi->SendDlgMsg(pDlg, "edCmd", DM_GETTEXT, 0, 0), PATH_MAX);

	if (result[strlen(result) - 1] != ' ')
		strncat(result, " ", PATH_MAX - strlen(result));

	strncat(result, templ, PATH_MAX - strlen(result));
	gDialogApi->SendDlgMsg(pDlg, "edCmd", DM_SETTEXT, (intptr_t)result, 0);
}

static void preset_get_data(uintptr_t pDlg, int index)
{
	gDialogApi->SendDlgMsg(pDlg, "lblCmdTst", DM_SETTEXT, 0, 0);
	gchar *key = g_strdup_printf("Preset_%d_Quote", index);
	gboolean quote_val = g_key_file_get_boolean(gCfg, gLastExt, key, NULL);
	gDialogApi->SendDlgMsg(pDlg, "chkQuote", DM_SETCHECK, (gboolean)quote_val, 0);
	key = g_strdup_printf("Preset_%d_Cmd", index);
	gchar *value = g_key_file_get_string(gCfg, gLastExt, key, NULL);
	gDialogApi->SendDlgMsg(pDlg, "edCmd", DM_SETTEXT, (intptr_t)value, 0);
	g_free(key);
	key = g_strdup_printf("Preset_%d_Glob", index);
	value = g_key_file_get_string(gCfg, gLastExt, key, NULL);
	gDialogApi->SendDlgMsg(pDlg, "edGlob", DM_SETTEXT, (intptr_t)value, 0);
	g_free(key);
}

static void preset_remove_data(int index)
{
	gchar *key = g_strdup_printf("Preset_%d_Name", index);
	g_key_file_remove_key(gCfg, gLastExt, key, NULL);
	g_free(key);
	key = g_strdup_printf("Preset_%d_Cmd", index);
	g_key_file_remove_key(gCfg, gLastExt, key, NULL);
	g_free(key);
	key = g_strdup_printf("Preset_%d_Quote", index);
	g_key_file_remove_key(gCfg, gLastExt, key, NULL);
	g_free(key);
	key = g_strdup_printf("Preset_%d_Glob", index);
	g_key_file_remove_key(gCfg, gLastExt, key, NULL);
	g_free(key);
}

static void listbox_get_extentions(uintptr_t pDlg)
{
	gint i;
	gsize length;

	length = gDialogApi->SendDlgMsg(pDlg, "lbExt", DM_LISTGETCOUNT, 0, 0);

	if (length > 0)
	{
		for (i = length - 1; i != -1; i--)
			gDialogApi->SendDlgMsg(pDlg, "lbExt", DM_LISTDELETE, i, 0);
	}

	gchar **groups = g_key_file_get_groups(gCfg, &length);

	for (i = 0; i < length; i++)
	{
		if (groups[i][0] == '.')
			gDialogApi->SendDlgMsg(pDlg, "lbExt", DM_LISTADDSTR, (intptr_t)groups[i], 0);
	}

	if (groups)
		g_strfreev(groups);
}

static void combobox_get_presets(uintptr_t pDlg)
{
	gint i, length;

	length = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETCOUNT, 0, 0);

	if (length > 0)
	{
		for (i = length - 1; i != -1; i--)
			gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTDELETE, i, 0);
	}

	if (gLastExt[0] == '.' && gLastExt[1] != '\0')
	{
		length = g_key_file_get_integer(gCfg, gLastExt, "Presets", NULL);

		for (i = 0; i < length; i++)
		{
			gchar *key = g_strdup_printf("Preset_%d_Name", i);
			gchar *value = g_key_file_get_string(gCfg, gLastExt, key, NULL);

			if (value && value[0] != '\0')
				gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTADD, (intptr_t)value, 0);
			else
				gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTADD, (intptr_t)"<UNNAMED>", 0);

			g_free(key);
		}
	}
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		//g_print("DlgProc(%s): DN_INITDIALOG\n", DlgItemName);
		memset(gLastCMD, 0, PATH_MAX);
		memset(gLastMask, 0, PATH_MAX);
		g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);
		listbox_get_extentions(pDlg);

		if (gLastExt[0] != '\0')
			gDialogApi->SendDlgMsg(pDlg, "edExt", DM_SETTEXT, (intptr_t)gLastExt, 0);

		break;

	case DN_CLOSE:
		g_print("DlgProc(%s): DN_CLOSE not implemented\n", DlgItemName);

		break;

	case DN_CLICK:

		//g_print("DlgProc(%s): DN_CLICK\n", DlgItemName);

		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			int index = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETITEMINDEX, 0, 0);

			if (index > -1)
				g_key_file_set_integer(gCfg, gLastExt, "LastUsed", index);

			g_key_file_save_to_file(gCfg, gCfgPath, NULL);

			if (!gCfgMode && (gLastCMD[0] == '\0' || strstr(gLastCMD, "$FILE") == NULL || strstr(gLastCMD, "$OUTPUT") == NULL))
			{
				memset(gLastCMD, 0, PATH_MAX);
				gDialogApi->MessageBox("Missing or incorrect command, aborting.", NULL, MB_OK | MB_ICONERROR);
			}
		}
		else if (strcmp(DlgItemName, "btnCancel") == 0)
		{
			memset(gLastCMD, 0, PATH_MAX);
			memset(gLastMask, 0, PATH_MAX);
			g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL);
		}
		else if (strcmp(DlgItemName, "btnSave") == 0)
		{
			if (gLastExt[0] == '.' && gLastExt[1] != '\0')
			{
				int count = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETCOUNT, 0, 0);
				int index = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETITEMINDEX, 0, 0);

				if (index == -1)
				{
					index = count;
					count++;
				}

				g_key_file_set_integer(gCfg, gLastExt, "Presets", count);
				gchar *key = g_strdup_printf("Preset_%d_Name", index);
				char *value = (char*)gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_GETTEXT, 0, 0);
				g_key_file_set_string(gCfg, gLastExt, key, value);
				g_free(key);
				key = g_strdup_printf("Preset_%d_Cmd", index);
				value = (char*)gDialogApi->SendDlgMsg(pDlg, "edCmd", DM_GETTEXT, 0, 0);
				g_key_file_set_string(gCfg, gLastExt, key, value);
				g_free(key);
				key = g_strdup_printf("Preset_%d_Quote", index);
				gboolean quote_val = (gboolean)gDialogApi->SendDlgMsg(pDlg, "chkQuote", DM_GETCHECK, 0, 0);
				g_key_file_set_boolean(gCfg, gLastExt, key, quote_val);
				g_free(key);
				key = g_strdup_printf("Preset_%d_Glob", index);
				value = (char*)gDialogApi->SendDlgMsg(pDlg, "edGlob", DM_GETTEXT, 0, 0);
				g_key_file_set_string(gCfg, gLastExt, key, value);
				g_free(key);
			}
			else
				gDialogApi->MessageBox("Missing or incorrect file extension.", NULL, MB_OK | MB_ICONERROR);

			listbox_get_extentions(pDlg);
			combobox_get_presets(pDlg);
		}
		else if (strcmp(DlgItemName, "btnDel") == 0)
		{
			int i, u;
			int count = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETCOUNT, 0, 0);
			int index = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETITEMINDEX, 0, 0);

			if (count == 1 && index == 0)
			{
				g_key_file_remove_group(gCfg, gLastExt, NULL);
				gDialogApi->SendDlgMsg(pDlg, "edExt", DM_SETTEXT, 0, 0);
			}
			else if (index != -1)
			{
				preset_remove_data(index);

				for (i = index; i < count - 1; i++)
				{
					u = i + 1;
					gchar *key = g_strdup_printf("Preset_%d_Name", u);
					gchar *newkey = g_strdup_printf("Preset_%d_Name", i);
					gchar *value = g_key_file_get_string(gCfg, gLastExt, key, NULL);

					if (value)
						g_key_file_set_string(gCfg, gLastExt, newkey, value);

					g_free(key);
					g_free(newkey);
					key = g_strdup_printf("Preset_%d_Cmd", u);
					newkey = g_strdup_printf("Preset_%d_Cmd", i);
					value = g_key_file_get_string(gCfg, gLastExt, key, NULL);

					if (value)
						g_key_file_set_string(gCfg, gLastExt, newkey, value);

					g_free(key);
					g_free(newkey);
					key = g_strdup_printf("Preset_%d_Quote", u);
					newkey = g_strdup_printf("Preset_%d_Quote", i);
					gboolean quote_val = g_key_file_get_boolean(gCfg, gLastExt, key, NULL);
					g_key_file_set_boolean(gCfg, gLastExt, newkey, quote_val);
					g_free(key);
					g_free(newkey);
					key = g_strdup_printf("Preset_%d_Glob", u);
					newkey = g_strdup_printf("Preset_%d_Glob", i);
					value = g_key_file_get_string(gCfg, gLastExt, key, NULL);

					if (value)
						g_key_file_set_string(gCfg, gLastExt, newkey, value);

					g_free(key);
					g_free(newkey);
				}

				preset_remove_data(count - 1);

				g_key_file_set_integer(gCfg, gLastExt, "Presets", count - 1);

				u = g_key_file_get_integer(gCfg, gLastExt, "LastUsed", NULL);

				if (u >= count - 1)
					g_key_file_set_integer(gCfg, gLastExt, "LastUsed", 0);
			}

			gDialogApi->SendDlgMsg(pDlg, "edCmd", DM_SETTEXT, 0, 0);
			gDialogApi->SendDlgMsg(pDlg, "edGlob", DM_SETTEXT, 0, 0);
			gDialogApi->SendDlgMsg(pDlg, "chkQuote", DM_SETCHECK, 1, 0);

			listbox_get_extentions(pDlg);
			combobox_get_presets(pDlg);
		}
		else if (strcmp(DlgItemName, "btnAddIn") == 0)
		{
			edcmd_add_templ(pDlg, "$FILE");
		}
		else if (strcmp(DlgItemName, "btnAddOut") == 0)
		{
			edcmd_add_templ(pDlg, "$OUTPUT");
		}
		else if (strcmp(DlgItemName, "lbExt") == 0)
		{
			int index = gDialogApi->SendDlgMsg(pDlg, "lbExt", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
			{
				char *ext = (char*)gDialogApi->SendDlgMsg(pDlg, "lbExt", DM_LISTGETITEM, index, 0);
				gDialogApi->SendDlgMsg(pDlg, "edExt", DM_SETTEXT, (intptr_t)ext, 0);
			}
		}


		break;

	case DN_DBLCLICK:
		g_print("DlgProc(%s): DN_DBLCLICK not implemented\n", DlgItemName);

		break;

	case DN_CHANGE:

		//g_print("DlgProc(%s): DN_CHANGE\n", DlgItemName);

		if (strcmp(DlgItemName, "edExt") == 0)
		{
			char *ext = (char*)gDialogApi->SendDlgMsg(pDlg, "edExt", DM_GETTEXT, 0, 0);

			if (strcmp(gLastExt, ext) != 0)
				g_strlcpy(gLastExt, ext, PATH_MAX);

			combobox_get_presets(pDlg);

			int count = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETCOUNT, 0, 0);

			if (count > 0)
			{
				gint last = g_key_file_get_integer(gCfg, gLastExt, "LastUsed", NULL);

				if (last < count)
				{
					gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTSETITEMINDEX, (intptr_t)last, 0);
					preset_get_data(pDlg, last);
				}
			}
		}
		else if (strcmp(DlgItemName, "cbPreset") == 0)
		{
			int count = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETCOUNT, 0, 0);
			int index = gDialogApi->SendDlgMsg(pDlg, "cbPreset", DM_LISTGETITEMINDEX, 0, 0);

			if (count > 0 && index != -1)
			{
				gQuoteFN = TRUE;
				preset_get_data(pDlg, index);
			}
		}
		else if (strcmp(DlgItemName, "edCmd") == 0)
		{
			g_strlcpy(gLastCMD, (char*)gDialogApi->SendDlgMsg(pDlg, "edCmd", DM_GETTEXT, 0, 0), PATH_MAX);

			/*
			gchar *testcmd = str_replace_templ("/home/user/some file.ext", "/tmp/some file.out");
			gDialogApi->SendDlgMsg(pDlg, "lblCmdTst", DM_SETTEXT, (intptr_t)testcmd, 0);
			g_free(testcmd);
			*/
			gDialogApi->SendDlgMsg(pDlg, "lblCmdTst", DM_SETTEXT, 0, 0);
		}
		else if (strcmp(DlgItemName, "edGlob") == 0)
		{
			g_strlcpy(gLastMask, (char*)gDialogApi->SendDlgMsg(pDlg, "edGlob", DM_GETTEXT, 0, 0), PATH_MAX);
		}
		else if (strcmp(DlgItemName, "chkQuote") == 0)
		{
			gQuoteFN = (gboolean)gDialogApi->SendDlgMsg(pDlg, "chkQuote", DM_GETCHECK, 0, 0);

			gchar *testcmd = str_replace_templ("/home/user/some file.ext", "/tmp/some file.out");
			gDialogApi->SendDlgMsg(pDlg, "lblCmdTst", DM_SETTEXT, (intptr_t)testcmd, 0);
			g_free(testcmd);
		}

		break;

	case DN_GOTFOCUS:
		g_print("DlgProc(%s): DN_GOTFOCUS not implemented\n", DlgItemName);

		break;

	case DN_KILLFOCUS:
		g_print("DlgProc(%s): DN_KILLFOCUS not implemented\n", DlgItemName);

		break;

	case DN_KEYDOWN:
		g_print("DlgProc(%s): DN_KEYDOWN not implemented\n", DlgItemName);

		break;

	case DN_KEYUP:
		g_print("DlgProc(%s): DN_KEYUP not implemented\n", DlgItemName);

		break;
	}

	return 0;
}

static void ShowCFGDlg(void)
{
	if (gDialogApi && gLFMPath[0] != '\0')
	{
		if (g_file_test(gLFMPath, G_FILE_TEST_EXISTS))
			gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
		else
			gDialogApi->MessageBox("LFM file missing!", NULL, 0x00000010);
	}
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
	return E_SUCCESS;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	return E_NOT_SUPPORTED;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	gProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	return;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	return FALSE;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_HIDE;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	gCfgMode = TRUE;
	memset(gLastExt, 0, PATH_MAX);
	ShowCFGDlg();
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	int result = E_SUCCESS;
	char fname[PATH_MAX];
	gchar *in_file = NULL, *out_file = NULL;

	char *ext = strrchr(PackedFile, '.');

	if (ext != NULL)
		g_strlcpy(gLastExt, ext, PATH_MAX);

	gchar *target_path = g_path_get_dirname(PackedFile);

	gCfgMode = FALSE;
	ShowCFGDlg();

	if (gLastCMD[0] == '\0')
		return E_EABORTED;

	if (gLastExt[0] != '.' || gLastExt[1] == '\0')
		return E_NOT_SUPPORTED;

	while (*AddList)
	{
		if (AddList[strlen(AddList) - 1] != '/')
		{

			g_strlcpy(fname, AddList, PATH_MAX);
			in_file = g_strdup_printf("%s%s", SrcPath, fname);

			ext = strrchr(fname, '.');

			if (ext != NULL)
				strcpy(ext, gLastExt);
			else
				strcat(fname, gLastExt);

			out_file = g_strdup_printf("%s/%s", target_path, fname);

			if (gProcessDataProc(out_file, -100) == 0)
			{
				result = E_EABORTED;
				break;
			}

			if (gLastMask[0] != '\0' && fnmatch(gLastMask, in_file, FNM_CASEFOLD | FNM_EXTMATCH) != 0)
			{
				//g_print("Skipping file \"%s\"\n", in_file);
			}
			else
			{

				gchar *out_dir = g_path_get_dirname(out_file);

				if (!g_file_test(out_dir, G_FILE_TEST_EXISTS))
					g_mkdir_with_parents(out_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

				g_free(out_dir);

				int status;
				gchar *command = str_replace_templ(in_file, out_file);

				if ((status = system(command)) != 0)
				{
					gchar *msg = g_strdup_printf("Error executing command \"%s\". Exit status: %d.", command, status);

					if (gDialogApi->MessageBox((char*)msg, NULL, MB_OKCANCEL | MB_ICONERROR) == ID_CANCEL)
						result = E_EABORTED;

					g_free(msg);
				}

				g_free(command);
			}
		}

		if (in_file)
		{
			g_free(in_file);
			in_file = NULL;
		}

		if (out_file)
		{
			g_free(out_file);
			out_file = NULL;
		}

		if (result != E_SUCCESS)
			break;

		while (*AddList++);
	}

	g_free(target_path);

	return result;
}


void DCPCALL PackSetDefaultParams(PackDefaultParamStruct* dps)
{
	gCfg = g_key_file_new();
	gchar *ini_dirname = g_path_get_dirname(dps->DefaultIniName);
	g_strlcpy(gCfgPath, ini_dirname, PATH_MAX);
	strncat(gCfgPath, "/cmdconv_crap.ini", PATH_MAX - 18);
	g_free(ini_dirname);

	if (!g_file_test(gCfgPath, G_FILE_TEST_EXISTS))
	{
		Dl_info dlinfo;
		static char ini_default[PATH_MAX];

		memset(&dlinfo, 0, sizeof(dlinfo));

		if (dladdr(ini_default, &dlinfo) != 0)
		{
			g_strlcpy(ini_default, dlinfo.dli_fname, PATH_MAX);
			char *pos = strrchr(ini_default, '/');

			if (pos)
				strcpy(pos + 1, "settings_default.ini");
		}

		if (!g_key_file_load_from_file(gCfg, ini_default, G_KEY_FILE_KEEP_COMMENTS, NULL))
		{
			g_key_file_set_integer(gCfg, ".png", "Presets", 1);
			g_key_file_set_string(gCfg, ".png", "Preset_0_Name", "Convert images to png");
			g_key_file_set_string(gCfg, ".png", "Preset_0_Cmd", "convert $FILE $OUTPUT");
			g_key_file_set_boolean(gCfg, ".png", "Preset_0_Quote", TRUE);
			g_key_file_set_string(gCfg, ".png", "Preset_0_Glob", "+(*.jpg|*.svg|*.webp)");
			g_key_file_set_integer(gCfg, ".png", "LastUsed", 0);
		}

		g_key_file_save_to_file(gCfg, gCfgPath, NULL);
	}
}
