#include <glib.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "extension.h"
#include "wfxplugin.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

#define PICTURE_MAX_SIZE 3000000

typedef struct sVFSDirData
{
	GList *list;
} tVFSDirData;


int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

GtkRecentManager *gManager = NULL;
GList *gItems = NULL;
GData *gFileList;


gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

unsigned long FileTimeToUnixTime(LPFILETIME ft)
{
	gint64 ll = ft->dwHighDateTime;
	ll = (ll << 32) | ft->dwLowDateTime;
	ll = (ll - 116444736000000000) / 10000000;
	return (unsigned long)ll;
}

static gchar* ReplaceString(gchar *Text, gchar *String, gchar *Repl)
{
	gchar **split = g_strsplit(Text, String, -1);
	gchar *result = g_strjoinv(Repl, split);
	g_strfreev(split);

	return result;
}

static gchar* ArrayToString(gchar **array)
{
	if (!array)
		return NULL;

	gsize i = 0;
	GString *data = g_string_new(NULL);

	while (array[i] != NULL)
	{
		g_string_append(data, array[i++]);
		g_string_append(data, "\n");
	}

	if (data->str)
	{
		char *pos = strrchr(data->str, '\n');
		
		if (pos)
			*pos = '\0';
	}

	g_strfreev(array);

	return g_string_free(data, FALSE);
}

static gchar* UnixTimeToISO8601String(time_t time)
{
	gchar *result = NULL;
	GDateTime *date = g_date_time_new_from_unix_local((gint64)time);

	if (date)
	{
		result = g_date_time_format_iso8601(date);
		g_date_time_unref(date);
	}

	return result;
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

static void AddPropLabels(GString *lfm_string, int Num, gchar *PropName, gchar *Value)
{
	const char label_body[] =  "    object lbl%s%d: TLabel\n"
	                           "      Left = 208\n"
	                           "      Height = 17\n"
	                           "      Top = 0\n"
	                           "      Width = 202\n"
	                           "      Alignment = %s\n"
	                           "      Caption = '%s'\n"
	                           "      Layout = tlCenter\n"
	                           "      ParentColor = False\n"
	                           "    end\n";


	if (Value)
	{
		gchar *string = ReplaceString(Value, "'", "''");
		gchar *tmp = string;
		string = ReplaceString(tmp, "\n", "'#10'");
		g_free(tmp);
		g_string_append_printf(lfm_string, label_body, "Prop", Num, "taRightJustify", PropName);
		g_string_append_printf(lfm_string, label_body, "Value", Num, "taLeftJustify", string);
		g_free(string);
	}	
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	return 0;
}

static BOOL PropertiesDialog(char *FileName, GtkRecentInfo *info)
{
	GString *lfm_string = g_string_new(NULL);

	g_string_append(lfm_string, "object PropsDialogbox: TPropsDialogbox\n"
	                "  Left = 509\n"
	                "  Height = 547\n"
	                "  Top = 230\n"
	                "  Width = 681\n"
	                "  AutoSize = True\n"
	                "  BorderStyle = bsDialog\n"
	                "  Caption = 'Properties'\n"
	                "  ChildSizing.LeftRightSpacing = 10\n"
	                "  ChildSizing.TopBottomSpacing = 10\n"
	                "  ChildSizing.HorizontalSpacing = 5\n"
	                "  ChildSizing.VerticalSpacing = 5\n"
	                "  ClientHeight = 547\n"
	                "  ClientWidth = 681\n"
	                "  DesignTimePPI = 100\n"
	                "  LCLVersion = '2.2.4.0'\n"
	                "  Position = poOwnerFormCenter\n"
	                "  object Image: TImage\n"
	                "    AnchorSideLeft.Control = Owner\n"
	                "    AnchorSideTop.Control = Owner\n"
	                "    Left = 10\n"
	                "    Height = 48\n"
	                "    Top = 10\n"
	                "    Width = 48\n"
	                "    Center = True\n");

	GdkPixbuf *pixbuf = gtk_recent_info_get_icon(info, 48);

	if (pixbuf)
	{
		gchar *tmpdir = g_dir_make_tmp("gtkrecent_XXXXXX", NULL);
		gchar *icon = g_strdup_printf("%s/icon,png", tmpdir);
		gdk_pixbuf_save(pixbuf, icon, "png", NULL, NULL);
		g_object_unref(pixbuf);
		gchar *picture = BuildPictureData(icon, "      ", "TPortableNetworkGraphic");

		if (picture)
		{
			g_string_append(lfm_string, "    Picture.Data = {\n");
			g_string_append(lfm_string, picture);
			g_string_append(lfm_string, "    }\n");
			g_free(picture);
		}

		remove(icon);
		remove(tmpdir);
		g_free(tmpdir);
		g_free(icon);
	}

	g_string_append(lfm_string, "    Proportional = True\n"
	                "    Stretch = True\n"
	                "  end\n"
	                "  object edName: TEdit\n"
	                "    AnchorSideLeft.Control = Image\n"
	                "    AnchorSideLeft.Side = asrBottom\n"
	                "    AnchorSideTop.Control = Image\n"
	                "    AnchorSideTop.Side = asrCenter\n"
	                "    AnchorSideRight.Control = ScrollBox\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    Left = 63\n"
	                "    Height = 36\n"
	                "    Top = 16\n"
	                "    Width = 361\n"
	                "    Alignment = taCenter\n"
	                "    Anchors = [akTop, akLeft, akRight]\n"
	                "    BorderStyle = bsNone\n"
	                "    Color = clForm\n"
	                "    Font.Style = [fsBold]\n"
	                "    ParentFont = False\n"
	                "    ReadOnly = True\n"
	                "    TabStop = False\n"
	                "    TabOrder = 0\n"
	                "    Text = '");

	g_string_append(lfm_string, FileName);

	g_string_append(lfm_string, "'\n"
	                "  end\n"
	                "  object ScrollBox: TScrollBox\n"
	                "    AnchorSideLeft.Control = Owner\n"
	                "    AnchorSideTop.Control = Image\n"
	                "    AnchorSideTop.Side = asrBottom\n"
	                "    Left = 10\n"
	                "    Height = 281\n"
	                "    Top = 63\n"
	                "    Width = 414\n"
	                "    HorzScrollBar.Increment = 4\n"
	                "    HorzScrollBar.Page = 47\n"
	                "    HorzScrollBar.Smooth = True\n"
	                "    HorzScrollBar.Tracking = True\n"
	                "    VertScrollBar.Increment = 1\n"
	                "    VertScrollBar.Page = 17\n"
	                "    VertScrollBar.Smooth = True\n"
	                "    VertScrollBar.Tracking = True\n"
	                "    BorderSpacing.Top = 5\n"
	                "    ChildSizing.HorizontalSpacing = 5\n"
	                "    ChildSizing.VerticalSpacing = 5\n"
	                "    ChildSizing.EnlargeHorizontal = crsHomogenousChildResize\n"
	                "    ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
	                "    ChildSizing.ControlsPerLine = 2\n"
	                "    ClientHeight = 277\n"
	                "    ClientWidth = 410\n"
	                "    TabOrder = 1\n");

	int i = 1;
	gsize length;
	gchar *string = NULL;
	const char *name = gtk_recent_info_get_display_name(info);
	AddPropLabels(lfm_string, i++, "Name", (char*)name);
	string = gtk_recent_info_get_short_name(info);
	AddPropLabels(lfm_string, i++, "", string);
	g_free(string);
	const char *descr = gtk_recent_info_get_description(info);
	AddPropLabels(lfm_string, i++, "Description", (char*)descr);
	const char *uri = gtk_recent_info_get_uri(info);
	AddPropLabels(lfm_string, i++, "URI", (char*)uri);
	string = gtk_recent_info_get_uri_display(info);
	AddPropLabels(lfm_string, i++, "", string);
	g_free(string);
	const char *mime = gtk_recent_info_get_mime_type(info);
	AddPropLabels(lfm_string, i++, "MIME", (char*)mime);
	string = ArrayToString(gtk_recent_info_get_applications(info, &length));
	AddPropLabels(lfm_string, i++, "Apps", string);
	g_free(string);
	string = gtk_recent_info_last_application(info);
	AddPropLabels(lfm_string, i++, "Last App", string);
	g_free(string);
	string = ArrayToString(gtk_recent_info_get_groups(info, &length));
	AddPropLabels(lfm_string, i++, "Gropus", string);
	g_free(string);
	string = UnixTimeToISO8601String(gtk_recent_info_get_added(info));
	AddPropLabels(lfm_string, i++, "Added", string);
	g_free(string);
	string = UnixTimeToISO8601String(gtk_recent_info_get_modified(info));
	AddPropLabels(lfm_string, i++, "Modified", string);
	g_free(string);
	string = UnixTimeToISO8601String(gtk_recent_info_get_visited(info));
	AddPropLabels(lfm_string, i++, "Visited", string);
	g_free(string);
	string = g_strdup_printf("%d day(s) elapsed since the last update of the resource", gtk_recent_info_get_age(info));
	AddPropLabels(lfm_string, i++, "Age", string);
	g_free(string);
	AddPropLabels(lfm_string, i++, "Local", gtk_recent_info_is_local(info) ? "True" : "False");
	AddPropLabels(lfm_string, i++, "Private", gtk_recent_info_get_private_hint(info) ? "True" : "False");
	AddPropLabels(lfm_string, i++, "Exists", gtk_recent_info_exists(info) ? "True" : "False");

	g_string_append(lfm_string, "  end\n"
	                "  object btnClose: TBitBtn\n"
	                "    AnchorSideTop.Control = ScrollBox\n"
	                "    AnchorSideTop.Side = asrBottom\n"
	                "    AnchorSideRight.Control = ScrollBox\n"
	                "    AnchorSideRight.Side = asrBottom\n"
	                "    Left = 323\n"
	                "    Height = 31\n"
	                "    Top = 349\n"
	                "    Width = 101\n"
	                "    Anchors = [akTop, akRight]\n"
	                "    Cancel = True\n"
	                "    AutoSize = True\n"
	                "    Constraints.MinHeight = 31\n"
	                "    Constraints.MinWidth = 101\n"
	                "    DefaultCaption = True\n"
	                "    Kind = bkClose\n"
	                "    ModalResult = 11\n"
	                "    TabOrder = 2\n"
	                "  end\n"
	                "end\n");

	//g_print("%s", lfm_string->str);
	return gExtensions->DialogBoxLFM((intptr_t)lfm_string->str, (unsigned long)strlen(lfm_string->str), PropertiesDlgProc);
	g_string_free(lfm_string, TRUE);
}

gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	gchar *path = NULL;
	gchar *filename = NULL;
	unsigned long atime;
	struct stat buf;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	GList *list = dirdata->list;

	if (list != NULL)
	{
		do
		{
			gchar *file = gtk_recent_info_get_short_name(list->data);
			atime = (unsigned long)gtk_recent_info_get_visited(list->data);
			UnixTimeToFileTime(atime, &FindData->ftLastWriteTime);
			filename = g_strdup_printf("[%ld] %s", atime, file);
			path = g_filename_from_uri(gtk_recent_info_get_uri(list->data), NULL, NULL);
			g_datalist_set_data(&gFileList, filename, (gpointer)gtk_recent_info_get_uri(list->data));
			g_free(file);
			list = list->next;
		}
		while (!filename && list != NULL);

		if (!filename)
			return FALSE;

		g_strlcpy(FindData->cFileName, filename, MAX_PATH - 1);
		g_free(filename);

		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;

		if (path && stat(path, &buf) == 0)
		{
			FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;

			if (!S_ISDIR(buf.st_mode))
			{
				FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
				FindData->dwReserved0 = buf.st_mode;
			}
		}
		else
		{
			FindData->nFileSizeHigh = 0xFFFFFFFF;
			FindData->nFileSizeLow = 0xFFFFFFFE;
		}

		dirdata->list = list;
		g_free(path);

		return TRUE;
	}

	return FALSE;
}

static void FreeDataArrays(void)
{
	while (gItems != NULL)
	{
		gtk_recent_info_unref(gItems->data);
		gItems = gItems->next;
	}

	if (gItems != NULL)
		g_list_free(gItems);

	gItems = NULL;

	g_datalist_clear(&gFileList);

}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	gManager = gtk_recent_manager_get_default();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	FreeDataArrays();
	gItems = gtk_recent_manager_get_items(gManager);
	dirdata->list = gItems;
	g_datalist_init(&gFileList);


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

	g_free(dirdata);

	return 0;
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	GError *err = NULL;
	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, RemoteName + 1);

	if (uri)
	{
		if (!gtk_recent_manager_remove_item(gManager, uri, &err))
			MessageBox((char*)(err)->message, NULL,  MB_OK | MB_ICONERROR);
		else
			return TRUE;
	}

	if (err)
		g_error_free(err);

	return FALSE;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	int err = gProgressProc(gPluginNr, RemoteName, LocalName, 0);
	gchar *uri = g_filename_to_uri(LocalName, NULL, NULL);

	if (err)
		return FS_FILE_USERABORT;

	if (!gtk_recent_manager_add_item(gManager, uri))
	{
		g_free(uri);
		return FS_FILE_WRITEERROR;
	}

	g_free(uri);

	return FS_FILE_OK;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;
	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, g_basename(RemoteName));

	if (!uri)
		return result;

	GtkRecentInfo *info = gtk_recent_manager_lookup_item(gManager, uri, NULL);

	if (strcmp(Verb, "open") == 0)
	{
		gchar *app_name = gtk_recent_info_last_application(info);
		const gchar *app_exec;
		guint count;
		time_t time_;

		if (gtk_recent_info_get_application_info(info, app_name, &app_exec, &count, &time_))
			system(app_exec);

		result = FS_FILE_OK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		PropertiesDialog(RemoteName + 1, info);
		result = FS_EXEC_OK;
	}

	gtk_recent_info_unref(info);

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	return TRUE;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, g_basename(RemoteName));

	if (uri)
	{
		gchar *path = g_filename_from_uri(uri, NULL, NULL);

		if (path)
		{
			g_strlcpy(RemoteName, path, maxlen - 1);
			g_free(path);
			return TRUE;
		}
	}

	return FALSE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "GTKRecent", maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo * StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}

	if (gtk_main_level() == 0)
		gtk_init_check(0, NULL);
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;

	FreeDataArrays();
}
