#include <glib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <udisks/udisks.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define ROOTNAME "LoopDev Mount"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

#define UDISK_BLOCKDEV_PATH "/org/freedesktop/UDisks2/block_devices"
#define ANNOYANCE "to setup loopdev, copy *.img or *.iso file to the panel (it dosent matter where). to mount, press enter on the partition in \"subdir\" of the loopdev."

typedef struct sVFSDirData
{
	GList *list;
	GList *objects;
	char *blockdev;
	guint index;
	const gchar *const *partitions;

} tVFSDirData;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

static UDisksClient *gUdisksClient = NULL;
const char *gAllowedExts[] = { ".img", ".iso", NULL };
GString *gLogString = NULL;

static gboolean IsRootDir(char *path)
{
	while (*path++)
	{
		if (*path == '/')
			return FALSE;
	}

	return TRUE;
}

static UDisksObject *GetUDisksObjectFromPath(char *Path)
{
	gchar *name = g_path_get_basename(Path);
	gchar *devPath = g_strdup_printf(UDISK_BLOCKDEV_PATH "/%s", name);
	UDisksObject *object = udisks_client_peek_object(gUdisksClient, devPath);
	g_free(devPath);
	g_free(name);

	return object;
}

static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;

	if (dirdata->objects)
	{
		for (GList *l = dirdata->objects; l != NULL; l = l->next)
		{
			UDisksObject *object = UDISKS_OBJECT(l->data);
			UDisksLoop *loop = udisks_object_peek_loop(object);
			UDisksBlock *block = udisks_object_peek_block(object);

			if (loop && block)
			{
				gchar *filename = g_path_get_basename(udisks_block_get_device(block));
				g_strlcpy(FindData->cFileName, filename, MAX_PATH);
				FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				g_free(filename);
				dirdata->objects = l->next;
				return TRUE;
			}
		}
	}
	else if (dirdata->partitions && dirdata->partitions[dirdata->index])
	{
		gchar *filename = g_path_get_basename(dirdata->partitions[dirdata->index]);
		g_strlcpy(FindData->cFileName, filename, MAX_PATH);
		g_free(filename);
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
		dirdata->index++;
		return TRUE;
	}
	else if (dirdata->blockdev)
	{
		g_strlcpy(FindData->cFileName, dirdata->blockdev, MAX_PATH);
		dirdata->blockdev = NULL;
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
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

	if (!gUdisksClient)
	{
		GError *err = NULL;
		gUdisksClient = udisks_client_new_sync(NULL, &err);

		if (err)
		{
			MessageBox(err->message, NULL,  MB_OK | MB_ICONERROR);
			g_error_free(err);
		}
		else
			MessageBox(ANNOYANCE, NULL, MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	if (gUdisksClient)
	{
		dirdata = g_new0(tVFSDirData, 1);

		if (dirdata == NULL)
			return (HANDLE)(-1);

		if (strcmp(Path, "/") == 0)
		{
			GDBusObjectManager *manager = udisks_client_get_object_manager(gUdisksClient);
			dirdata->objects = g_dbus_object_manager_get_objects(manager);
			dirdata->list = dirdata->objects;
		}
		else
		{
			UDisksObject *object = GetUDisksObjectFromPath(Path);

			if (object)
			{
				UDisksPartitionTable *table = udisks_object_peek_partition_table(object);

				if (UDISKS_IS_PARTITION_TABLE(table))
					dirdata->partitions = udisks_partition_table_get_partitions(table);
				else
				{
					UDisksBlock *block = udisks_object_peek_block(object);

					if (block && udisks_block_get_size(block) > 0)
					{
						char *pos = strrchr(Path, '/');

						if (pos)
							dirdata->blockdev = pos + 1;
					}
				}
			}
		}
	}
	else
		return (HANDLE)(-1);

	if (!SetFindData(dirdata, FindData))
	{
		if (dirdata->list)
		{
			g_list_foreach(dirdata->list, (GFunc) g_object_unref, NULL);
			g_list_free(dirdata->list);
		}

		g_free(dirdata);
		return (HANDLE)(-1);
	}

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
	{
		g_list_foreach(dirdata->list, (GFunc) g_object_unref, NULL);
		g_list_free(dirdata->list);
	}

	g_free(dirdata);

	return 0;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (!gUdisksClient)
		return FS_EXEC_ERROR;

	GError *err = NULL;

	if (strcmp(Verb, "open") == 0)
	{
		gchar *mountPath = NULL;
		UDisksObject *object = GetUDisksObjectFromPath(RemoteName);

		if (object)
		{

			UDisksFilesystem *filesystem = udisks_object_peek_filesystem(object);

			if (filesystem)
				udisks_filesystem_call_mount_sync(filesystem, g_variant_new("a{sv}", NULL), &mountPath, NULL, &err);

			if (err)
			{
				MessageBox(err->message, NULL,  MB_OK | MB_ICONERROR);
				g_error_free(err);
			}

			if (mountPath)
			{
				g_strlcpy(RemoteName, mountPath, MAX_PATH);
				return FS_EXEC_SYMLINK;
			}
		}

		return FS_EXEC_OK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		UDisksObject *object = GetUDisksObjectFromPath(RemoteName);

		if (object)
		{
			UDisksLoop *loop = udisks_object_peek_loop(object);
			UDisksPartition *partition = udisks_object_peek_partition(object);
			UDisksBlock *block = udisks_object_peek_block(object);

			if (loop)
			{
				if (IsRootDir(RemoteName))
				{
					if (block && udisks_block_get_size(block) > 0 && MessageBox("Detach loop device?", NULL,  MB_YESNO | MB_ICONQUESTION) == ID_YES)
					{
						GVariantBuilder opts;
						g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);
						udisks_loop_call_delete_sync(loop, g_variant_builder_end(&opts), NULL, &err);

						if (err)
						{
							MessageBox(err->message, NULL,  MB_OK | MB_ICONERROR);
							g_error_free(err);
						}
					}
				}
				else
				{
					gchar *size = g_format_size(udisks_block_get_size(block));
					gchar *info = g_strdup_printf("device:\t%s\nlabel:\t%s\nuuid:\t%s\nsize:\t%s\ntype:\t%s %s\nversion:\t%s",
					                              udisks_block_get_device(block),
					                              udisks_block_get_id_label(block),
					                              udisks_block_get_id_uuid(block),
					                              size,
					                              udisks_block_get_id_usage(block), udisks_block_get_id_type(block),
					                              udisks_block_get_id_version(block));
					MessageBox(info, NULL,  MB_OK | MB_ICONINFORMATION);
					g_free(size);
					g_free(info);
				}
			}
			else if (partition)
			{
				gchar *size = g_format_size(udisks_partition_get_size(partition));
				gchar *info = g_strdup_printf("device:\t%s\nlabel:\t%s\nname:\t%s\nuuid:\t%s\nsize:\t%s\ntype:\t%s %s (%s)\nversion:\t%s\ntable:\t%s",
				                              udisks_block_get_device(block),
				                              udisks_block_get_id_label(block),
				                              udisks_partition_get_name(partition),
				                              udisks_partition_get_uuid(partition),
				                              size,
				                              udisks_block_get_id_usage(block), udisks_block_get_id_type(block), udisks_partition_get_type_(partition),
				                              udisks_block_get_id_version(block),
				                              udisks_partition_get_table(partition));
				MessageBox(info, NULL,  MB_OK | MB_ICONINFORMATION);
				g_free(size);
				g_free(info);
			}
		}

		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	if (!gUdisksClient)
	{
		MessageBox("Failed to get UDisks client.", NULL,  MB_OK | MB_ICONERROR);
		return FS_FILE_WRITEERROR;
	}

	gboolean isOK = FALSE;
	char *dot = strrchr(LocalName, '.');

	if (dot)
	{
		int i = 0;

		while (gAllowedExts[i])
		{
			if (strcmp(dot, gAllowedExts[i++]) == 0)
				isOK = TRUE;
		}
	}

	if (isOK)
	{
		gboolean isReadOnly = (MessageBox("Set Read Only?", NULL,  MB_YESNO | MB_ICONQUESTION) == ID_YES);
		gchar *loopPath = NULL;
		int fd = open(LocalName, isReadOnly ? O_RDONLY : O_RDWR);

		if (fd == -1)
			return FS_FILE_READERROR;
		else
		{
			GError *err = NULL;
			GVariantBuilder opts;

			GUnixFDList *fd_list = g_unix_fd_list_new_from_array(&fd, 1);
			g_variant_builder_init(&opts, G_VARIANT_TYPE_VARDICT);

			if (isReadOnly)
				g_variant_builder_add(&opts, "{sv}", "read-only", g_variant_new_boolean(TRUE));

			if (!udisks_manager_call_loop_setup_sync(udisks_client_get_manager(gUdisksClient), g_variant_new_handle(0), g_variant_builder_end(&opts), fd_list, &loopPath, NULL, NULL, &err))
			{
				MessageBox(err->message, NULL,  MB_OK | MB_ICONERROR);
				g_error_free(err);
			}
			else
			{
				UDisksObject *object = udisks_client_peek_object(gUdisksClient, loopPath);

				if (object)
				{
					UDisksLoop *loop = udisks_object_peek_loop(object);

					if (loop)
						udisks_loop_set_autoclear(loop, TRUE);
				}
			}

			g_string_append_printf(gLogString, "%s -> %s\n", LocalName, loopPath);
			g_clear_object(&fd_list);
			g_free(loopPath);
		}
	}
	else
		g_string_append_printf(gLogString, "SKIP %s\n", LocalName);

	return FS_FILE_OK;
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex > 0)
		return ft_nomorefields;

	g_strlcpy(FieldName, "Info", maxlen - 1);
	Units[0] = '\0';

	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (!gUdisksClient)
		return ft_fileerror;

	UDisksObject *object = GetUDisksObjectFromPath(FileName);

	if (object)
	{
		UDisksObjectInfo *info = udisks_client_get_object_info(gUdisksClient, object);
		const gchar *value = udisks_object_info_get_one_liner(info);

		if (value)
			g_strlcpy((char*)FieldValue, value, maxlen - 1);
		else
			return ft_fieldempty;

		g_object_unref(info);
	}
	else
		return ft_fileerror;

	return ft_string;
}

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{
	if (InfoOperation == FS_STATUS_OP_LIST)
	{
		if (gLogString)
		{
			MessageBox(gLogString->str, NULL, MB_OK);
			g_string_free(gLogString, TRUE);
			gLogString = NULL;
		}
	}
	else if (InfoOperation > FS_STATUS_OP_LIST && InfoOperation < FS_STATUS_OP_RENMOV_SINGLE)
	{
		if (InfoStartEnd == FS_STATUS_START)
		{
			gLogString = g_string_new(NULL);
		}
	}
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[Plugin(FS).Info{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "Info", maxlen - 1);
	g_strlcpy(ViewWidths, "100,0,200", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
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
	if (gUdisksClient)
		g_object_unref(gUdisksClient);

	gUdisksClient = NULL;

	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, ROOTNAME, maxlen - 1);
}
