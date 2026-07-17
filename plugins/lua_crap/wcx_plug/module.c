#define _DEFAULT_SOURCE
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <glib.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "wcxplugin.h"
#include "extension.h"

#define DEFINE_INT(name, value) lua_pushinteger(L, value); lua_setfield(L, -2, name)
#define DEFINE_STR(name, value) lua_pushstring(L, value);  lua_setfield(L, -2, name)

typedef void (*tExtensionInitialize)(tExtensionStartupInfo* StartupInfo);
typedef void (*tExtensionFinalize)(void* Reserved);
typedef void (*tPackSetDefaultParams)(PackDefaultParamStruct* dps);
typedef int (*tGetPackerCaps)(void);
typedef int (*tGetBackgroundFlags)(void);
typedef BOOL (*tCanYouHandleThisFile)(char *FileName);
typedef BOOL (*tCanYouHandleThisFileW)(WCHAR *FileName);
typedef void* (*tOpenArchive)(tOpenArchiveData* ArchiveData);
typedef void* (*tOpenArchiveW)(tOpenArchiveDataW* ArchiveData);
typedef int (*tReadHeader)(void* hArcData, tHeaderData* HeaderData);
typedef int (*tReadHeaderEx)(void* hArcData, tHeaderDataEx* HeaderData);
typedef int (*tReadHeaderExW)(void* hArcData, tHeaderDataExW* HeaderData);
typedef int (*tProcessFile)(void* hArcData, int Operation, char* DestPath, char* DestName);
typedef int (*tProcessFileW)(void* hArcData, int Operation, WCHAR* DestPath, WCHAR* DestName);
typedef int (*tCloseArchive)(void* hArcData);
typedef void (*tSetChangeVolProc)(HANDLE hArcData, tChangeVolProc pChangeVolProc1);
typedef void (*tSetProcessDataProc)(HANDLE hArcData, tProcessDataProc pProcessDataProc);
typedef void (*tSetChangeVolProcW)(HANDLE hArcData, tChangeVolProcW pChangeVolProc1);
typedef void (*tSetProcessDataProcW)(HANDLE hArcData, tProcessDataProcW pProcessDataProc);
typedef void (*tPkSetCryptCallback)(tPkCryptProc pPkCryptProc, int CryptoNr, int Flags);
typedef void (*tPkSetCryptCallbackW)(tPkCryptProcW pPkCryptProc, int CryptoNr, int Flags);
typedef int (*tPackFiles)(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags);
typedef int (*tDeleteFiles)(char *PackedFile, char *DeleteList);
typedef int (*tPackFilesW)(WCHAR *PackedFile, WCHAR *SubPath, WCHAR *SrcPath, WCHAR *AddList, int Flags);
typedef int (*tDeleteFilesW)(WCHAR *PackedFile, WCHAR *DeleteList);
typedef int (*tStartMemPack)(int Options, char *FileName);
typedef int (*tStartMemPackW)(int Options, WCHAR *FileName);
typedef int (*tPackToMem)(int hMemPack, char* BufIn, int InLen, int* Taken, char* BufOut, int OutLen, int* Written, int SeekBy);
typedef int (*tDoneMemPack)(int hMemPack);


typedef struct
{
	void* handle;
	void* archive_handle;
	gboolean is_process_file;
	gboolean is_point_at_dir;
	gboolean is_extract_mode;
	gchar *filename;
	int packer_caps;
	int flags;
	tCanYouHandleThisFile CanYouHandleThisFile;
	tCanYouHandleThisFileW CanYouHandleThisFileW;
	tOpenArchive OpenArchive;
	tOpenArchiveW OpenArchiveW;
	tReadHeader ReadHeader;
	tReadHeaderEx ReadHeaderEx;
	tReadHeaderExW ReadHeaderExW;
	tProcessFile ProcessFile;
	tProcessFileW ProcessFileW;
	tCloseArchive CloseArchive;
	tSetChangeVolProc SetChangeVolProc;
	tSetChangeVolProcW SetChangeVolProcW;
	tSetProcessDataProc SetProcessDataProc;
	tSetProcessDataProcW SetProcessDataProcW;
	tPackFiles PackFiles;
	tPackFilesW PackFilesW;
	tDeleteFiles DeleteFiles;
	tDeleteFilesW DeleteFilesW;
	tStartMemPack StartMemPack;
	tStartMemPackW StartMemPackW;
	tPackToMem PackToMem;
	tDoneMemPack DoneMemPack;
	tExtensionFinalize ExtensionFinalize;
} PlugData;

typedef struct
{
	char arcname[1024 * 2];
	char filename[1024 * 2];
	int64_t pk_size;
	int64_t unpk_size;
	uint64_t filetime;
	int mode;
} FileItem;

static WCHAR* utf8_to_wchar(char* string)
{
	if (!string)
		return NULL;

	gsize bytes_written = 0;
	return (WCHAR*)g_convert(string, -1, "UTF-16LE", "UTF-8", NULL, &bytes_written, NULL);
}

static gchar* wchar_to_utf8(WCHAR* string)
{
	if (!string)
		return NULL;

	gsize u16_len = 0;

	while (string[u16_len] != 0)
		u16_len++;

	return g_convert((const char*)string, u16_len * 2, "UTF-8", "UTF-16LE", NULL, NULL, NULL);
}

static int DCPCALL progress_cb(char *filename, int size)
{
	g_printerr("[ProcessDataProc] (%d): %s\n", size, filename);
	return 1;
}

static int DCPCALL progress_utf16_cb(WCHAR *filename_utf16, int size)
{
	gchar *filename = wchar_to_utf8(filename_utf16);
	progress_cb(filename, size);
	g_free(filename);
	return 1;
}

static int DCPCALL changevol_cb(char *archive, int mode)
{
	g_printerr("[ChangeVolProc] (%s): %s\n", (mode == PK_VOL_NOTIFY) ? "PK_VOL_NOTIFY" : "PK_VOL_ASK", archive);
	return (mode == PK_VOL_NOTIFY) ? 1 : 0;
}

static int DCPCALL changevol_utf16_cb(WCHAR *archive_utf16, int mode)
{
	gchar *archive = wchar_to_utf8(archive_utf16);
	changevol_cb(archive, mode);
	g_free(archive);
	return 0;
}

static int DCPCALL crypto_cb(int num, int mode, char* archive, char* passwd, int maxlen)
{
	g_printerr("[PkCryptProc] (%d): %s\n", mode, archive);
	return E_ECREATE;
}

static int DCPCALL crypto_utf16_cb(int num, int mode, WCHAR* archive_utf16, WCHAR* passwd, int maxlen)
{
	gchar *archive = wchar_to_utf8(archive_utf16);
	crypto_cb(num, mode, archive, (char*)passwd, maxlen);
	g_free(archive);
	return E_ECREATE;
}

static int DCPCALL msgbox_cb(char* text, char* caption, long flags)
{
	g_printerr("[MessageBox] (%s %ld): %s\n", caption, flags, text);
	return ID_CANCEL;
}

static BOOL DCPCALL inputbox_cb(char* caption, char* prompt, BOOL mask, char* value, int maxlen)
{
	g_printerr("[InputBox] (%s %s): %s\n", caption, value, prompt);
	return FALSE;
}

static void set_callbacks(PlugData *data, void *arc_handle)
{
	if (data->SetProcessDataProcW)
		data->SetProcessDataProcW(arc_handle, progress_utf16_cb);
	else if (data->SetProcessDataProc)
		data->SetProcessDataProc(arc_handle, progress_cb);

	if (data->SetChangeVolProcW)
		data->SetChangeVolProcW(arc_handle, changevol_utf16_cb);
	else if (data->SetChangeVolProc)
		data->SetChangeVolProc(arc_handle, changevol_cb);
}

static void print_wcx_error(const char *what, gchar *filename, int error)
{
	if (error == E_SUCCESS)
		return;

	char *msg = NULL;

	switch (error)
	{
	case E_END_ARCHIVE:
		msg = "No more files in archive";
		break;

	case E_NO_MEMORY:
		msg = "Not enough memory";
		break;

	case E_BAD_DATA:
		msg = "Data is bad";
		break;

	case E_BAD_ARCHIVE:
		msg = "CRC error in archive data";
		break;

	case E_UNKNOWN_FORMAT:
		msg = "Archive format unknown";
		break;

	case E_EOPEN:
		msg = "Cannot open existing file";
		break;

	case E_ECREATE:
		msg = "Cannot create file";
		break;

	case E_ECLOSE:
		msg = "Error closing file";
		break;

	case E_EREAD:
		msg = "Error reading from file";
		break;

	case E_EWRITE:
		msg = "Error writing to file";
		break;

	case E_SMALL_BUF:
		msg = "Buffer too small";
		break;

	case E_EABORTED:
		msg = "Function aborted by user";
		break;

	case E_NO_FILES:
		msg = "No files founde";
		break;

	case E_TOO_MANY_FILES:
		msg = "Too many files to pack";
		break;

	case E_NOT_SUPPORTED:
		msg = "Function not supported";
		break;

	default:
		g_printerr("Error code %d, ", error);
		msg = "wtf";
	}

	g_printerr("Error (%s %s): %s\n", what, filename, msg);
}

static gboolean call_probe_file(PlugData *data, const char *archive)
{
	gboolean result = FALSE;
	gchar *filename = g_canonicalize_filename(archive, NULL);

	if (data->CanYouHandleThisFileW)
	{
		WCHAR *filenamew = utf8_to_wchar(filename);
		result = data->CanYouHandleThisFileW(filenamew);
		g_free(filenamew);
	}
	else if (data->CanYouHandleThisFile)
		result = data->CanYouHandleThisFile(filename);

	g_free(filename);

	return result;
}

static void* call_open_archive(PlugData *data, const char *filename)
{
	void *result = NULL;
	data->filename = g_canonicalize_filename(filename, NULL);
	data->is_process_file = FALSE;

	if (data->OpenArchiveW)
	{
		tOpenArchiveDataW ArchiveData;
		memset(&ArchiveData, 0, sizeof(tOpenArchiveDataW));
		ArchiveData.ArcName = utf8_to_wchar(data->filename);
		ArchiveData.OpenMode = data->is_extract_mode ? PK_OM_EXTRACT : PK_OM_LIST;
		result = data->OpenArchiveW(&ArchiveData);
		g_free(ArchiveData.ArcName);

		if (ArchiveData.OpenResult != E_SUCCESS)
		{
			result = NULL;
			print_wcx_error("OpenArchive", data->filename, ArchiveData.OpenResult);
			g_free(data->filename);
			data->filename = NULL;
		}
	}
	else if (data->OpenArchive)
	{
		tOpenArchiveData ArchiveData;
		memset(&ArchiveData, 0, sizeof(tOpenArchiveData));
		ArchiveData.ArcName = data->filename;
		ArchiveData.OpenMode = data->is_extract_mode ? PK_OM_EXTRACT : PK_OM_LIST;
		result = data->OpenArchive(&ArchiveData);

		if (ArchiveData.OpenResult != E_SUCCESS)
		{
			result = NULL;
			print_wcx_error("OpenArchive", data->filename, ArchiveData.OpenResult);
			g_free(data->filename);
			data->filename = NULL;
		}
	}

	if (result)
		set_callbacks(data, result);

	return result;
}

static int call_read_header(PlugData *data, FileItem *item)
{
	int result = E_NOT_SUPPORTED;

	if (data->ReadHeaderExW)
	{
		tHeaderDataExW HeaderData;
		memset(&HeaderData, 0, sizeof(tHeaderDataExW));
		result = data->ReadHeaderExW(data->archive_handle, &HeaderData);

		if (result == E_SUCCESS && item)
		{
			gchar *string = wchar_to_utf8(HeaderData.FileName);
			g_strlcpy(item->filename, string, sizeof(item->filename));
			g_free(string);
			string = wchar_to_utf8(HeaderData.ArcName);
			g_strlcpy(item->arcname, string, sizeof(item->arcname));
			g_free(string);
			item->pk_size = ((int64_t)HeaderData.PackSizeHigh << 32) | HeaderData.PackSize;
			item->unpk_size = ((int64_t)HeaderData.UnpSizeHigh << 32) | HeaderData.UnpSize;

			if (HeaderData.MfileTime >= 0x019DB1DED53E8000ULL)
			{
				item->filetime = HeaderData.MfileTime - 0x019DB1DED53E8000ULL;
				item->filetime /= 10000000ULL;
			}
			else if (HeaderData.FileTime > 0)
				item->filetime = HeaderData.FileTime;

			item->mode = HeaderData.FileAttr;
		}
	}
	else if (data->ReadHeaderEx)
	{
		tHeaderDataEx HeaderData;
		memset(&HeaderData, 0, sizeof(tHeaderDataEx));
		result = data->ReadHeaderEx(data->archive_handle, &HeaderData);

		if (result == E_SUCCESS && item)
		{
			g_strlcpy(item->filename, HeaderData.FileName, sizeof(item->filename));
			g_strlcpy(item->arcname, HeaderData.ArcName, sizeof(item->arcname));
			item->pk_size = ((int64_t)HeaderData.PackSizeHigh << 32) | HeaderData.PackSize;
			item->unpk_size = ((int64_t)HeaderData.UnpSizeHigh << 32) | HeaderData.UnpSize;
			item->filetime = (uint16_t)HeaderData.FileTime;
			item->mode = HeaderData.FileAttr;
		}
	}
	else if (data->ReadHeader)
	{
		tHeaderData HeaderData;
		memset(&HeaderData, 0, sizeof(tHeaderData));
		result = data->ReadHeader(data->archive_handle, &HeaderData);

		if (result == E_SUCCESS && item)
		{
			g_strlcpy(item->filename, HeaderData.FileName, sizeof(item->filename));
			g_strlcpy(item->arcname, HeaderData.ArcName, sizeof(item->arcname));
			item->pk_size = (int64_t)HeaderData.PackSize;
			item->unpk_size = (int64_t)HeaderData.UnpSize;
			item->filetime = (uint16_t)HeaderData.FileTime;
			item->mode = HeaderData.FileAttr;
		}
	}

	if (result != E_SUCCESS && result != E_END_ARCHIVE)
		print_wcx_error("ReadHeader", data->filename, result);
	else if (result == E_SUCCESS)
	{
		data->is_point_at_dir = (item->filename[strlen(item->filename) - 1] == '/' || S_ISDIR(item->mode));
		data->is_process_file = TRUE;
	}


	return result;
}

static int call_process_file(PlugData *data, int mode, const char *dst)
{
	int result = E_NOT_SUPPORTED;

	gchar *filename = NULL;

	if (dst)
		filename = g_canonicalize_filename(dst, NULL);

	if (data->ProcessFileW)
	{
		WCHAR *filenamew = NULL;

		if (filename)
			filenamew = utf8_to_wchar(filename);

		result = data->ProcessFileW(data->archive_handle, mode, NULL, filenamew);
		g_free(filenamew);
	}
	else if (data->ProcessFileW)
	{
		result = data->ProcessFile(data->archive_handle, mode, NULL, filename);
	}

	g_free(filename);

	if (result != E_SUCCESS)
		print_wcx_error("ProcessFile", data->filename, result);
	else
		data->is_process_file = FALSE;

	return result;
}

static void call_close_archive(PlugData *data)
{
	if (data->CloseArchive)
		print_wcx_error("CloseArchive", data->filename, data->CloseArchive(data->archive_handle));

	g_free(data->filename);
	data->archive_handle = NULL;
	data->filename = NULL;
}

static PlugData* load_plugin(const char *filename)
{
	void* handle = dlopen(filename, RTLD_LAZY);

	if (handle)
	{
		PlugData *data = (PlugData*)g_new0(PlugData, 1);

		data->handle = handle;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
		data->CanYouHandleThisFile = (tCanYouHandleThisFile)dlsym(handle, "CanYouHandleThisFile");
		data->CanYouHandleThisFileW = (tCanYouHandleThisFileW)dlsym(handle, "CanYouHandleThisFileW");
		data->OpenArchive = (tOpenArchive)dlsym(handle, "OpenArchive");
		data->OpenArchiveW = (tOpenArchiveW)dlsym(handle, "OpenArchiveW");
		data->ReadHeader = (tReadHeader)dlsym(handle, "ReadHeader");
		data->ReadHeaderEx = (tReadHeaderEx)dlsym(handle, "ReadHeaderEx");
		data->ReadHeaderExW = (tReadHeaderExW)dlsym(handle, "ReadHeaderExW");
		data->ProcessFile = (tProcessFile)dlsym(handle, "ProcessFile");
		data->ProcessFileW = (tProcessFileW)dlsym(handle, "ProcessFileW");
		data->CloseArchive = (tCloseArchive)dlsym(handle, "CloseArchive");
		data->SetChangeVolProc = (tSetChangeVolProc)dlsym(handle, "SetChangeVolProc");
		data->SetChangeVolProcW = (tSetChangeVolProcW)dlsym(handle, "SetChangeVolProcW");
		data->SetProcessDataProc = (tSetProcessDataProc)dlsym(handle, "SetProcessDataProc");
		data->SetProcessDataProcW = (tSetProcessDataProcW)dlsym(handle, "SetProcessDataProcW");
		data->PackFiles = (tPackFiles)dlsym(handle, "PackFiles");
		data->PackFilesW = (tPackFilesW)dlsym(handle, "PackFilesW");
		data->DeleteFiles = (tDeleteFiles)dlsym(handle, "DeleteFiles");
		data->DeleteFilesW = (tDeleteFilesW)dlsym(handle, "DeleteFilesW");
		data->StartMemPack = (tStartMemPack)dlsym(handle, "StartMemPack");
		data->StartMemPackW = (tStartMemPackW)dlsym(handle, "StartMemPackW");
		data->PackToMem = (tPackToMem)dlsym(handle, "PackToMem");
		data->DoneMemPack = (tDoneMemPack)dlsym(handle, "DoneMemPack");
		data->ExtensionFinalize = (tExtensionFinalize)dlsym(handle, "ExtensionFinalize");

		tExtensionInitialize ExtensionInitialize = (tExtensionInitialize)dlsym(handle, "ExtensionInitialize");
		tPackSetDefaultParams PackSetDefaultParams = (tPackSetDefaultParams)dlsym(handle, "PackSetDefaultParams");
		tGetPackerCaps GetPackerCaps = (tGetPackerCaps)dlsym(handle, "GetPackerCaps");
		tGetBackgroundFlags GetBackgroundFlags = (tGetBackgroundFlags)dlsym(handle, "GetBackgroundFlags");
		tPkSetCryptCallback PkSetCryptCallback = (tPkSetCryptCallback)dlsym(handle, "PkSetCryptCallback");
		tPkSetCryptCallbackW PkSetCryptCallbackW = (tPkSetCryptCallbackW)dlsym(handle, "PkSetCryptCallbackW");
#pragma GCC diagnostic pop

		if (!data->OpenArchive || !data->CloseArchive || (!data->ReadHeaderEx && !data->ReadHeaderExW) || (!data->ProcessFile && !data->ProcessFileW))
		{
			dlclose(handle);
			g_free(data);
			g_printerr("mandatory api fail (%s)\n", filename);
		}
		else
		{
			if (ExtensionInitialize)
			{
				tExtensionStartupInfo StartupInfo;
				memset(&StartupInfo, 0, sizeof(tExtensionStartupInfo));
				gchar *plug_dir = g_path_get_dirname(filename);
				g_strlcpy(StartupInfo.PluginDir, plug_dir, EXT_MAX_PATH);
				g_free(plug_dir);
				snprintf(StartupInfo.PluginConfDir, EXT_MAX_PATH, "%s/doublecmd/wcx.ini", g_get_user_config_dir());
				StartupInfo.MessageBox = msgbox_cb;
				StartupInfo.InputBox = inputbox_cb;
				StartupInfo.StructSize = sizeof(tExtensionStartupInfo);
				ExtensionInitialize(&StartupInfo);
			}

			if (PackSetDefaultParams)
			{
				PackDefaultParamStruct dps;
				memset(&dps, 0, sizeof(PackDefaultParamStruct));
				snprintf(dps.DefaultIniName, MAX_PATH, "%s/doublecmd/wcx.ini", g_get_user_config_dir());
				dps.PluginInterfaceVersionHi = 2;
				dps.PluginInterfaceVersionLow = 22;
				dps.size = sizeof(PackDefaultParamStruct);
				PackSetDefaultParams(&dps);
			}

			if (GetPackerCaps)
				data->packer_caps = GetPackerCaps();

			if (GetBackgroundFlags)
				data->flags = GetBackgroundFlags();

			if (PkSetCryptCallbackW)
				PkSetCryptCallbackW(crypto_utf16_cb, 666, 0);
			else if (PkSetCryptCallback)
				PkSetCryptCallback(crypto_cb, 666, 0);

			set_callbacks(data, (HANDLE) -1);

			return data;
		}
	}

	return NULL;
}

static int lua_load_plug(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	if (filename)
	{
		PlugData *data = load_plugin(filename);

		if (data)
		{
			lua_pushlightuserdata(L, data);
			return 1;
		}
	}

	lua_pushnil(L);

	return 1;
}

static int lua_unload_plug(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);

	if (data->archive_handle)
		call_close_archive(data);

	if (data->ExtensionFinalize)
		data->ExtensionFinalize(NULL);

	dlclose(data->handle);
	g_free(data);

	return 0;
}

static void push_mode_str(lua_State *L, int mode)
{
	int i = 0;
	char attr[] = "----------";

	if (S_ISDIR(mode))
		attr[i] = 'd';
	else if (S_ISCHR(mode))
		attr[i] = 'c';
	else if (S_ISBLK(mode))
		attr[i] = 'b';
	else if (S_ISFIFO(mode))
		attr[i] = 'f';
	else if (S_ISLNK(mode))
		attr[i] = 'l';
	else if (S_ISSOCK(mode))
		attr[i] = 's';

	i++;

	if (mode & S_IRUSR)
		attr[i] = 'r';

	i++;

	if (mode & S_IWUSR)
		attr[i] = 'w';

	i++;

	if (mode & S_ISUID)
		attr[i] = 'S';
	else if (mode & S_IXUSR)
		attr[i] = 'x';

	i++;

	if (mode & S_IRGRP)
		attr[i] = 'r';

	i++;

	if (mode & S_IWGRP)
		attr[i] = 'w';

	i++;

	if (mode & S_ISGID)
		attr[i] = 'S';
	else if (mode & S_IXGRP)
		attr[i] = 'x';

	i++;

	if (mode & S_IROTH)
		attr[i] = 'r';

	i++;

	if (mode & S_IWOTH)
		attr[i] = 'w';

	i++;

	if (mode & S_ISVTX)
		attr[i] = 'T';
	else if (mode & S_IXOTH)
		attr[i] = 'x';

	lua_pushstring(L, attr);
}

static int archive_iterator(lua_State *L)
{
	PlugData *data = (PlugData*)lua_touserdata(L, lua_upvalueindex(1));

	if (!data || !data->archive_handle)
		return 0;

	if (data->is_process_file && call_process_file(data, PK_SKIP, NULL) != E_SUCCESS)
	{
		call_close_archive(data);
		return 0;
	}

	FileItem item;
	memset(&item, 0, sizeof(FileItem));

	int ret = call_read_header(data, &item);

	if (ret != E_SUCCESS)
	{
		call_close_archive(data);
		return 0;
	}

	lua_pushstring(L, item.filename);
	lua_createtable(L, 0, 5);
	lua_pushstring(L, item.arcname);
	lua_setfield(L, -2, "arcname");
	lua_pushnumber(L, item.unpk_size);
	lua_setfield(L, -2, "unpk_size");
	lua_pushnumber(L, item.pk_size);
	lua_setfield(L, -2, "pk_size");
	lua_pushnumber(L, item.filetime);
	lua_setfield(L, -2, "filetime");
	push_mode_str(L, item.mode);
	lua_setfield(L, -2, "mode");

	return 2;
}

static int lua_list_archive(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);

	if (data->archive_handle)
		return luaL_error(L, "archive already open (%s)", data->filename);

	const char *path = luaL_checkstring(L, 2);
	data->is_extract_mode = lua_toboolean(L, 3);
	data->archive_handle = call_open_archive(data, path);

	if (!data->archive_handle)
		return luaL_error(L, "archive open fail (%s)", path);

	lua_pushlightuserdata(L, data);
	lua_pushcclosure(L, archive_iterator, 1);

	return 1;
}

static int lua_snatch_current(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);

	if (!data->is_process_file)
		return luaL_error(L, "better luck next time");

	if (data->is_point_at_dir)
	{
		lua_pushboolean(L, FALSE);
		return 1;
	}

	const char *dst = luaL_checkstring(L, 2);
	lua_pushboolean(L, call_process_file(data, PK_EXTRACT, dst) == E_SUCCESS);

	return 1;
}

static int lua_test_current(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);

	if (!data->is_process_file)
		return luaL_error(L, "better luck next time");

	if (data->is_point_at_dir)
	{
		lua_pushboolean(L, FALSE);
		return 1;
	}

	lua_pushboolean(L, call_process_file(data, PK_TEST, NULL));

	return 1;
}

static int lua_probe_archive(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);
	const char *file = luaL_checkstring(L, 2);
	lua_pushboolean(L, call_probe_file(data, file));

	return 1;
}

static int lua_packer_casps(lua_State *L)
{
	if (!lua_islightuserdata(L, 1))
		return 0;

	PlugData *data = (PlugData*)lua_touserdata(L, 1);
	lua_pushnumber(L, data->packer_caps);

	return 1;
}

static const struct luaL_Reg shitcode[] =
{
	{"load_plug",		     lua_load_plug},
	{"unload_plug",		   lua_unload_plug},
	{"packer_caps",		  lua_packer_casps},
	{"probe_archive",	 lua_probe_archive},
	{"list_archive",	  lua_list_archive},
	{"snatch_current",	lua_snatch_current},
	{"test_current",	  lua_test_current},
	{NULL,				      NULL}
};

int LUAOPEN_NAME(lua_State *L)
{
#if LUA_VERSION_NUM == 501
	lua_newtable(L);
	luaL_register(L, NULL, shitcode);
#else
	luaL_newlib(L, shitcode);
#endif

	DEFINE_INT("PK_CAPS_NEW", PK_CAPS_NEW);
	DEFINE_INT("PK_CAPS_MODIFY", PK_CAPS_MODIFY);
	DEFINE_INT("PK_CAPS_MULTIPLE", PK_CAPS_MULTIPLE);
	DEFINE_INT("PK_CAPS_DELETE", PK_CAPS_DELETE);
	DEFINE_INT("PK_CAPS_OPTIONS", PK_CAPS_OPTIONS);
	DEFINE_INT("PK_CAPS_MEMPACK", PK_CAPS_MEMPACK);
	DEFINE_INT("PK_CAPS_BY_CONTENT", PK_CAPS_BY_CONTENT);
	DEFINE_INT("PK_CAPS_SEARCHTEXT", PK_CAPS_SEARCHTEXT);
	DEFINE_INT("PK_CAPS_HIDE", PK_CAPS_HIDE);
	DEFINE_INT("PK_CAPS_ENCRYPT", PK_CAPS_ENCRYPT);

	DEFINE_INT("BACKGROUND_UNPACK", BACKGROUND_UNPACK);
	DEFINE_INT("BACKGROUND_PACK", BACKGROUND_PACK);
	DEFINE_INT("BACKGROUND_MEMPACK", BACKGROUND_MEMPACK);

	DEFINE_INT("RHDF_ENCRYPTED", RHDF_ENCRYPTED);
	return 1;
}
