#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gExtensions->SendDlgMsg

typedef struct sVFSDirData
{
	gchar **groups;
	gsize i;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

GKeyFile *gCfg;
gchar *gCfgPath = NULL;
gchar *gInfoPath = NULL;
gchar *gLFMPath = NULL;
static gchar gConnection[PATH_MAX];


void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	gint64 ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

static void SetLastAccessTime(gint64 ll, LPFILETIME ft)
{
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	gsize len;
	gboolean bval;
	gchar *value = NULL;
	const gchar * const *schemes;

	switch (Msg)
	{
	case DN_INITDIALOG:

		SendDlgMsg(pDlg, "lblName", DM_SETTEXT, (intptr_t)gConnection, 0);
		bval = g_key_file_has_group(gCfg, gConnection);

		schemes = g_vfs_get_supported_uri_schemes(g_vfs_get_default());

		for (; *schemes; schemes++)
		{
			SendDlgMsg(pDlg, "cbURI", DM_LISTADD, (intptr_t)*schemes, 0);
		}

		if (!bval)
			SendDlgMsg(pDlg, "cbURI", DM_SHOWITEM, TRUE, 0);
		else
		{
			value = g_key_file_get_string(gCfg, gConnection, "URI", NULL);

			if (value)
			{
				if (value[0] == '\0')
					SendDlgMsg(pDlg, "cbURI", DM_SHOWITEM, TRUE, 0);

				SendDlgMsg(pDlg, "edURI", DM_SETTEXT, (intptr_t)value, 0);
				g_free(value);
			}
			else
				SendDlgMsg(pDlg, "cbURI", DM_SHOWITEM, TRUE, 0);

			value = g_key_file_get_string(gCfg, gConnection, "User", NULL);

			if (value)
			{
				SendDlgMsg(pDlg, "edUser", DM_SETTEXT, (intptr_t)value, 0);
				g_free(value);
			}

			value = g_key_file_get_string(gCfg, gConnection, "Password", NULL);

			if (value)
			{
				gchar *dec = (gchar*)g_base64_decode(value, &len);
				SendDlgMsg(pDlg, "edPasswd", DM_SETTEXT, (intptr_t)dec, 0);
				g_free(value);
				g_free(dec);
			}

			bval = g_key_file_get_boolean(gCfg, gConnection, "Anonymous", NULL);
			SendDlgMsg(pDlg, "chkAnon", DM_SETCHECK, (intptr_t)bval, 0);

			bval = g_key_file_has_key(gCfg, gConnection, "Domain", NULL);
			SendDlgMsg(pDlg, "chkDomain", DM_SETCHECK, (intptr_t)bval, 0);
			SendDlgMsg(pDlg, "edDomain", DM_ENABLE, (intptr_t)bval, 0);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			value = (gchar*)SendDlgMsg(pDlg, "edURI", DM_GETTEXT, 0, 0);
			g_key_file_set_string(gCfg, gConnection, "URI", value);
			value = (gchar*)SendDlgMsg(pDlg, "edUser", DM_GETTEXT, 0, 0);
			g_key_file_set_string(gCfg, gConnection, "User", value);
			value = (gchar*)SendDlgMsg(pDlg, "edPasswd", DM_GETTEXT, 0, 0);
			gchar *base64 = g_base64_encode((guchar*)value, strlen(value));
			g_key_file_set_string(gCfg, gConnection, "Password", base64);
			g_free(base64);
			bval = (gboolean)SendDlgMsg(pDlg, "chkAnon", DM_GETCHECK, 0, 0);

			if (bval)
				g_key_file_set_boolean(gCfg, gConnection, "Anonymous", bval);
			else if (g_key_file_has_key(gCfg, gConnection, "Anonymous", NULL))
				g_key_file_set_boolean(gCfg, gConnection, "Anonymous", bval);

			bval = (gboolean)SendDlgMsg(pDlg, "chkDomain", DM_GETCHECK, 0, 0);

			if (bval)
			{
				value = (gchar*)SendDlgMsg(pDlg, "edDomain", DM_GETTEXT, 0, 0);
				g_key_file_set_string(gCfg, gConnection, "Domain", value);
			}
			else
				g_key_file_remove_key(gCfg, gConnection, "Domain", NULL);

			SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}
		else if (strcmp(DlgItemName, "btnCancel") == 0)
			SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_CANCEL, 0);

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "chkAnon") == 0)
		{
			bval = (gboolean)SendDlgMsg(pDlg, "chkAnon", DM_GETCHECK, 0, 0);
			SendDlgMsg(pDlg, "edUser", DM_ENABLE, (intptr_t)!bval, 0);
			SendDlgMsg(pDlg, "edPasswd", DM_ENABLE, (intptr_t)!bval, 0);
		}
		else if (strcmp(DlgItemName, "chkDomain") == 0)
		{
			bval = (gboolean)SendDlgMsg(pDlg, "chkDomain", DM_GETCHECK, 0, 0);
			SendDlgMsg(pDlg, "edDomain", DM_ENABLE, (intptr_t)bval, 0);
		}
		else if (strcmp(DlgItemName, "cbURI") == 0)
		{
			value = (gchar*)SendDlgMsg(pDlg, "cbURI", DM_GETTEXT, 0, 0);
			gchar *uri = g_strdup_printf("%s://", value);
			SendDlgMsg(pDlg, "edURI", DM_SETTEXT, (intptr_t)uri, 0);
			g_free(uri);
		}
		else if (strcmp(DlgItemName, "edURI") == 0)
		{
			value = (gchar*)SendDlgMsg(pDlg, "edURI", DM_GETTEXT, 0, 0);

			if (value[0] == '\0')
				SendDlgMsg(pDlg, "cbURI", DM_SHOWITEM, TRUE, 0);
			else
				SendDlgMsg(pDlg, "cbURI", DM_SHOWITEM, FALSE, 0);
		}

		break;
	}

	return 0;
}

static void ShowCFGDlg(void)
{
	if (gExtensions && gLFMPath)
	{
		if (g_file_test(gLFMPath, G_FILE_TEST_EXISTS))
			gExtensions->DialogBoxLFMFile(gLFMPath, DlgProc);
		else
			gExtensions->MessageBox("LFM file missing!", NULL, 0x00000010);
	}
}

static void ask_password_cb(GMountOperation *op, const char *msg, const char *user, const char *domain, GAskPasswordFlags flags, gpointer user_data)
{
	gsize len;
	gchar *value = NULL;
	char input[MAX_PATH];

	if (user && strncmp(user, "ABORT", 5) == 0)
	{
		g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
		return;
	}

	if (flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)
	{
		gboolean anon = g_key_file_get_boolean(gCfg, gConnection, "Anonymous", NULL);
		g_mount_operation_set_anonymous(op, anon);

		if (anon)
		{
			g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
			return;
		}
	}

	if (flags & G_ASK_PASSWORD_NEED_USERNAME)
	{
		value = g_key_file_get_string(gCfg, gConnection, "User", NULL);

		if (value && value[0] != '\0')
			g_mount_operation_set_username(op, value);
		else
		{
			g_strlcpy(input, user, MAX_PATH);

			if (gRequestProc(gPluginNr, RT_UserName, (char*)msg, NULL, input, MAX_PATH))
				g_mount_operation_set_username(op, input);
			else
			{
				g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
				g_free(value);
				return;
			}
		}

		g_free(value);
	}

	if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
	{
		value = g_key_file_get_string(gCfg, gConnection, "Password", NULL);

		if (value && value[0] != '\0')
		{
			gchar *dec = (gchar*)g_base64_decode(value, &len);
			g_mount_operation_set_password(op, dec);
			g_free(dec);
		}
		else
		{
			input[0] = '\0';

			if (gRequestProc(gPluginNr, RT_Password, (char*)msg, NULL, input, MAX_PATH))
				g_mount_operation_set_password(op, input);
			else
			{
				g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
				g_free(value);
				return;
			}
		}

		g_free(value);
	}

	if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
	{
		value = g_key_file_get_string(gCfg, gConnection, "Domain", NULL);

		if (value && value[0] != '\0')
			g_mount_operation_set_domain(op, value);
		else
		{
			g_strlcpy(input, domain, MAX_PATH);

			if (gRequestProc(gPluginNr, RT_Other, (char*)msg, "Domain:", input, MAX_PATH))
				g_mount_operation_set_domain(op, input);
			else
			{
				g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
				g_free(value);
				return;
			}
		}

		g_free(value);
	}

	g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);

}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;

	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(gConnection, &dlinfo) != 0)
	{
		gInfoPath = g_strdup_printf("%s/info.txt", g_path_get_dirname(dlinfo.dli_fname));
		gLFMPath = g_strdup_printf("%s/dialog.lfm", g_path_get_dirname(dlinfo.dli_fname));
	}

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;
	struct stat buf;

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->i = 0;
	dirdata->groups = g_key_file_get_groups(gCfg, NULL);

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	g_strlcpy(FindData->cFileName, "!ReadMe.txt", MAX_PATH - 1);

	if (stat(gInfoPath, &buf) == 0)
	{
		FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
		FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
		UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
	}

	return (HANDLE)dirdata;
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (!dirdata->groups[dirdata->i])
		return FALSE;
	else
	{
		g_strlcpy(FindData->cFileName, dirdata->groups[dirdata->i], MAX_PATH - 1);
		FindData->dwFileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		gint64 value = g_key_file_get_int64(gCfg, dirdata->groups[dirdata->i], "LastAccess", NULL);

		if (value > 0)
		{
			SetLastAccessTime(value, &FindData->ftLastWriteTime);
			SetLastAccessTime(value, &FindData->ftLastAccessTime);
		}

		dirdata->i++;

		return TRUE;
	}
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->groups != NULL)
		g_strfreev(dirdata->groups);

	g_free(dirdata);

	return 0;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strncmp(RemoteName, "/!ReadMe.txt", 12) == 0 && strncmp(Verb, "open", 5) == 0)
		return FS_EXEC_YOURSELF;
	else if (RemoteName[1] != '\0' && strncmp(RemoteName, "/!ReadMe.txt", 12) != 0)
	{
		if (strncmp(Verb, "open", 5) == 0)
		{
			g_strlcpy(gConnection, RemoteName + 1, PATH_MAX);
			gchar *uri = g_key_file_get_string(gCfg, gConnection, "URI", NULL);

			if (uri && uri[0] != '\0')
			{
				GFile *f = g_file_new_for_uri(uri);
				g_strlcpy(RemoteName, uri, MAX_PATH - 1);
				g_free(uri);

				GMountOperation *op = g_mount_operation_new();
				g_signal_connect(op, "ask_password", G_CALLBACK(ask_password_cb), NULL);
				g_file_mount_enclosing_volume(f, 0, op, NULL, NULL, NULL);

				g_object_unref(op);
				g_object_unref(f);
				g_key_file_set_int64(gCfg, gConnection, "LastAccess", g_get_real_time());

				return FS_EXEC_SYMLINK;
			}
		}
		else if (strncmp(Verb, "properties", 10) == 0)
		{
			g_strlcpy(gConnection, RemoteName + 1, PATH_MAX);
			ShowCFGDlg();
			return FS_EXEC_OK;
		}
	}
	else
		gExtensions->MessageBox((char*)strerror(EOPNOTSUPP), NULL, 0x00000010);

	return FS_EXEC_ERROR;
}


int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	if (strncmp(RemoteName, "/!ReadMe.txt", 12) != 0)
		return FS_FILE_USERABORT;

	if ((CopyFlags == 0) && (g_file_test(LocalName, G_FILE_TEST_EXISTS)))
		return FS_FILE_EXISTS;

	gchar *contents = NULL;

	if (g_file_get_contents(gInfoPath, &contents, NULL, NULL))
		g_file_set_contents(LocalName, contents, -1, NULL);
	else
		return FS_FILE_READERROR;

	g_free(contents);

	return FS_FILE_OK;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
	if (strncmp(OldName, "/!ReadMe.txt", 12) == 0)
		return FS_FILE_USERABORT;

	if (g_key_file_has_group(gCfg, NewName + 1))
		return FS_FILE_EXISTS;

	gchar **keys = g_key_file_get_keys(gCfg, OldName + 1, NULL, NULL);

	for (gsize i = 0; keys[i]; i++)
	{
		gchar *value = g_key_file_get_string(gCfg, OldName + 1, keys[i], NULL);
		g_key_file_set_string(gCfg, NewName + 1, keys[i], value);
		g_free(value);
	}

	if (keys)
		g_strfreev(keys);

	g_key_file_remove_group(gCfg, OldName + 1, NULL);

	return FS_FILE_OK;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	if (strncmp(RemoteName, "/!ReadMe.txt", 12) == 0)
		return FALSE;

	return g_key_file_remove_group(gCfg, RemoteName + 1, NULL);
}

BOOL DCPCALL FsMkDir(char* Path)
{
	g_strlcpy(gConnection, Path + 1, PATH_MAX);
	ShowCFGDlg();
	return TRUE;
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	GKeyFile *legacycfg;
	gchar *key = NULL, *value = NULL, *group = NULL;
	gchar *lcfgpath = NULL;

	gCfgPath = g_strdup_printf("%s/gvfs_qickmount.ini", g_path_get_dirname(dps->DefaultIniName));
	gCfg = g_key_file_new();

	if (!g_key_file_load_from_file(gCfg, gCfgPath, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		legacycfg = g_key_file_new();
		gchar *cfgdir = g_path_get_dirname(dps->DefaultIniName);
		lcfgpath = g_strdup_printf("%s/gvfs.ini", cfgdir);
		g_free(cfgdir);

		if (g_key_file_load_from_file(legacycfg, lcfgpath, G_KEY_FILE_KEEP_COMMENTS, NULL))
		{
			gint count = g_key_file_get_integer(legacycfg, "gvfs", "ConnectionCount", NULL);

			if (count > 0)
			{
				for (gint i = 1; i <= count; i++)
				{
					key = g_strdup_printf("Connection%dName", i);
					group = g_key_file_get_string(legacycfg, "gvfs", key, NULL);
					g_free(key);

					if (group)
					{
						key = g_strdup_printf("Connection%dHost", i);
						value = g_key_file_get_string(legacycfg, "gvfs", key, NULL);
						g_free(key);

						if (value)
						{
							key = g_strdup_printf("Connection%dPath", i);
							gchar *path = g_key_file_get_string(legacycfg, "gvfs", key, NULL);
							g_free(key);

							if (path)
							{
								gchar *host = value;

								if (value[strlen(host) - 1] == '/' && host[0] == '/')
									value = g_strdup_printf("%s%s", host, path + 1);
								else
									value = g_strdup_printf("%s%s", host, path);

								g_free(host);
								g_free(path);
							}

							g_key_file_set_string(gCfg, group, "URI", value);
							g_free(value);
						}

						key = g_strdup_printf("Connection%dUserName", i);
						value = g_key_file_get_string(legacycfg, "gvfs", key, NULL);
						g_free(key);

						if (value)
						{
							g_key_file_set_string(gCfg, group, "User", value);
							g_free(value);
						}

						key = g_strdup_printf("Connection%dPassword", i);
						value = g_key_file_get_string(legacycfg, "gvfs", key, NULL);
						g_free(key);

						if (value)
						{
							gchar *base64 = g_base64_encode((guchar*)value, strlen(value));
							g_key_file_set_string(gCfg, group, "Password", base64);
							g_free(base64);
							g_free(value);
						}

						key = g_strdup_printf("Connection%dType", i);
						value = g_key_file_get_string(legacycfg, "gvfs", key, NULL);
						g_free(key);

						if (value)
						{
							g_key_file_set_string(gCfg, group, "Type", value);
							g_free(value);
						}

						g_free(group);
					}
				}
			}
		}

		g_free(lcfgpath);
		g_key_file_save_to_file(gCfg, gCfgPath, NULL);

		if (legacycfg)
			g_key_file_free(legacycfg);
	}
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gCfg)
	{
		g_key_file_save_to_file(gCfg, gCfgPath, NULL);
		g_key_file_free(gCfg);
	}

	g_free(gCfgPath);
	g_free(gInfoPath);
	g_free(gLFMPath);

	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	return FALSE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "GVFS Quick Mount", maxlen - 1);
}
