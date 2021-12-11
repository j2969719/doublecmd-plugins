#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <utime.h>
#include <fcntl.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define BUFF_SIZE 8192
#define GROUP_NAME "Files"

typedef struct sVFSDirData
{
	gsize ifile;
	gchar **files;
	gchar *folder;
	GDir *localdir;
} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
GKeyFile *gKeyFile = NULL;
GtkClipboard *gClipboard = NULL;

static gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

static unsigned long FileTimeToUnixTime(LPFILETIME ft)
{
	gint64 ll = ft->dwHighDateTime;
	ll = (ll << 32) | ft->dwLowDateTime;
	ll = (ll - 116444736000000000) / 10000000;
	return (unsigned long)ll;
}

static void GetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

static void FillKeyFile(gchar *text)
{
	g_key_file_remove_group(gKeyFile, GROUP_NAME, NULL);
	gchar **lines = g_strsplit(text, "\n", -1);

	for (gchar **line = lines; *line != NULL; line++)
	{
		gchar *path = NULL;
		size_t len = strlen(*line);

		if (len > 7 && strncmp(*line, "file://", 7) == 0)
			path = g_filename_from_uri(*line, NULL, NULL);
		else if (len > 2 && *line[0] == '/' && (strncmp(*line, "//", 2) != 0 || strncmp(*line, "/*", 2) != 0))
			path = g_strdup(*line);

		if (path)
		{
			gchar *filename = g_path_get_basename(path);
			g_key_file_set_string(gKeyFile, GROUP_NAME, filename, path);
			g_free(filename);
			g_free(path);
		}
	}

	g_strfreev(lines);
}

static void UpdateClipboardText(void)
{
	gsize length;
	gchar **files = g_key_file_get_keys(gKeyFile, GROUP_NAME, &length, NULL);
	GString *urilist = g_string_new(NULL);

	for (gsize i = 0; i < length; i++)
	{
		gchar *filepath = g_key_file_get_string(gKeyFile, GROUP_NAME, files[i], NULL);
		gchar *temp = g_filename_to_uri(filepath, NULL, NULL);
		gchar *uri = g_uri_unescape_string(temp, NULL);
		g_string_append_printf(urilist, "%s\n", uri);
		g_free(filepath);
		g_free(uri);
		g_free(temp);
	}

	gchar *list = g_string_free(urilist, FALSE);
	gtk_clipboard_set_text(gClipboard, list, -1);
	g_free(list);
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

static gchar *ExtractFolderFromPath(char *path)
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

static gchar *StripFolderFromPath(char *path)
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

static gchar *GetLocalName(gchar *remotename)
{
	gchar *localname = NULL;

	if (RootDir(remotename))
	{
		localname = g_key_file_get_string(gKeyFile, GROUP_NAME, remotename + 1, NULL);
	}
	else
	{
		gchar *folder = ExtractFolderFromPath(remotename);
		gchar *path = g_key_file_get_string(gKeyFile, GROUP_NAME, folder, NULL);
		g_free(folder);
		gchar *subdir = StripFolderFromPath(remotename);

		if (path && subdir)
			localname = g_strjoin(NULL, path, subdir, NULL);

		g_free(subdir);
		g_free(path);
	}

	return localname;
}

static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	gchar *filepath = NULL;
	gchar *filename = NULL;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (!dirdata->localdir)
	{
		if (!dirdata->files)
			return FALSE;

		filename = dirdata->files[dirdata->ifile++];

		if (!filename)
			return FALSE;

		filepath = g_key_file_get_string(gKeyFile, GROUP_NAME, filename, NULL);
	}
	else
	{
		filename = (gchar*)g_dir_read_name(dirdata->localdir);

		if (!filename)
			return FALSE;

		filepath = g_strdup_printf("%s/%s", dirdata->folder, filename);
	}

	if (filepath)
	{
		GStatBuf buf;

		g_strlcpy(FindData->cFileName, filename, MAX_PATH);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;


		if (g_stat(filepath, &buf) == 0)
		{
			if (S_ISDIR(buf.st_mode))
				FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

			FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;
			FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
			FindData->dwReserved0 = buf.st_mode;
			UnixTimeToFileTime(buf.st_mtime, &FindData->ftLastWriteTime);
			UnixTimeToFileTime(buf.st_atime, &FindData->ftLastAccessTime);
		}
		else
		{
			GetCurrentFileTime(&FindData->ftLastAccessTime);
			GetCurrentFileTime(&FindData->ftLastWriteTime);
		}

		g_free(filepath);
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

	gClipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gKeyFile = g_key_file_new();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;
	gchar *target = NULL;

	if (Path[1] == '\0')
	{
		gchar *text = gtk_clipboard_wait_for_text(gClipboard);

		if (!text)
			return (HANDLE)(-1);

		FillKeyFile(text);
		g_free(text);
	}
	else
	{
		target = GetLocalName(Path);
	}

	if (Path[1] != '\0' && !target)
		return (HANDLE)(-1);

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->localdir = NULL;

	if (target)
	{
		dirdata->localdir = g_dir_open(target, 0, NULL);

		if (!dirdata->localdir)
		{
			g_free(target);
			g_free(dirdata);
			return (HANDLE)(-1);
		}

		dirdata->folder = target;
	}
	else
	{
		dirdata->files = g_key_file_get_keys(gKeyFile, GROUP_NAME, NULL, NULL);
		dirdata->ifile = 0;
	}

	if (SetFindData(dirdata, FindData))
		return (HANDLE)dirdata;

	g_free(dirdata);

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

	if (dirdata->localdir)
		g_dir_close(dirdata->localdir);

	if (dirdata->files)
		g_strfreev(dirdata->files);

	g_free(dirdata->folder);
	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char buff[BUFF_SIZE];
	struct stat buf;
	struct utimbuf ubuf;
	int result = FS_FILE_OK;


	if (CopyFlags == 0 && access(LocalName, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	gchar *localname = GetLocalName(RemoteName);

	if (strcmp(localname, LocalName) == 0)
	{
		g_free(localname);
		return FS_FILE_WRITEERROR;
	}

	ifd = open(localname, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(LocalName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{
		size_t rsize = (size_t)((int64_t)ri->SizeHigh << 32 | ri->SizeLow);

		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				result = FS_FILE_WRITEERROR;
				break;
			}

			total += len;

			if (rsize > 0)
				done = total * 100 / rsize;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProgressProc(gPluginNr, RemoteName, LocalName, done) == 1)
			{
				result = FS_FILE_USERABORT;
				break;
			}
		}

		close(ofd);

		if (ri->Attr > 0)
			chmod(LocalName, ri->Attr);

		if (stat(localname, &buf) == 0)
		{
			ubuf.actime = buf.st_atime;
			ubuf.modtime = buf.st_mtime;
			utime(LocalName, &ubuf);
		}

	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);
	g_free(localname);

	return result;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	if (!RootDir(RemoteName))
		return FS_FILE_NOTSUPPORTED;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0) == 1)
		return FS_FILE_USERABORT;

	gchar *filename = g_path_get_basename(RemoteName);

	if (CopyFlags == 0 && g_key_file_has_key(gKeyFile, GROUP_NAME, filename, NULL))
	{
		g_free(filename);
		return FS_FILE_EXISTS;
	}

	g_key_file_set_string(gKeyFile, GROUP_NAME, filename, LocalName);
	g_free(filename);
	gProgressProc(gPluginNr, RemoteName, LocalName, 50);

	UpdateClipboardText();

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return FS_FILE_OK;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	return FsDeleteFile(RemoteName);
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	if (!RootDir(RemoteName))
		return TRUE;

	gchar *filename = g_path_get_basename(RemoteName);
	g_key_file_remove_key(gKeyFile, GROUP_NAME, filename, NULL);
	g_free(filename);
	UpdateClipboardText();
	return TRUE;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;

	if (strcmp(Verb, "open") == 0)
	{
		result = FS_EXEC_YOURSELF;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		if (RemoteName[1] == '\0' || strcmp(RemoteName, "/..") == 0)
			return FS_EXEC_ERROR;

		gchar *filename = GetLocalName(RemoteName);
		gchar *msg = g_strdup_printf("Location: '%s'", filename);
		g_free(filename);
		gRequestProc(gPluginNr, RT_MsgOK, NULL, msg, NULL, 0);
		g_free(msg);
		result = FS_FILE_OK;
	}
	else if (strncmp(Verb, "chmod", 5) == 0)
	{
		int mode = strtoll(Verb + 6, 0, 8);
		gchar *filename = GetLocalName(RemoteName);
		if (chmod(filename, mode) == 0)
			result = FS_FILE_OK;

		g_free(filename);
	}

	return result;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME *CreationTime, FILETIME *LastAccessTime, FILETIME *LastWriteTime)
{
	struct stat buf;
	struct utimbuf ubuf;
	gboolean result = FALSE;

	gchar *filename = GetLocalName(RemoteName);

	if (LastAccessTime != NULL || LastWriteTime != NULL)
	{

		if (stat(filename, &buf) == 0)
		{
			if (LastAccessTime != NULL)
				ubuf.actime = FileTimeToUnixTime(LastAccessTime);
			else
				ubuf.actime = buf.st_atime;

			if (LastWriteTime != NULL)
				ubuf.modtime = FileTimeToUnixTime(LastWriteTime);
			else
				ubuf.modtime = buf.st_mtime;

			if (utime(filename, &ubuf) == 0)
				result = TRUE;
		}
	}

	g_free(filename);

	return result;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gchar *localname = GetLocalName(RemoteName);

	if (localname)
	{
		g_strlcpy(RemoteName, localname, maxlen - 1);
		g_free(localname);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	return FALSE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Files in the clipboard", maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{

}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gKeyFile)
		g_key_file_free(gKeyFile);
}
