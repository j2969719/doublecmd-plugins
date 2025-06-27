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
#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))
#define FIELDCOUNT ARRAY_SIZE(gFields)
#define PICTURE_MAX_SIZE 3000000

typedef struct sVFSDirData
{
	GList *list;
	time_t now;
} tVFSDirData;

typedef struct sField
{
	char *name;
	int type;
	char *unit;
} tField;

tField gFields[] =
{
	{"Basename",		ft_string,	""},
	{"BasenameNoExt",	ft_string,	""},
	{"Dirname",		ft_string,	""},
	{"Path",		ft_string,	""},
	{"Description",		ft_string,	""},
	{"Apps",		ft_string,	""},
	{"LastApp",		ft_string,	""},
	{"Groups",		ft_string,	""},
	{"Visited",		ft_datetime,	""},
	{"Added",		ft_datetime,	""},
	{"Exists",		ft_boolean,	""},
	{"Private",		ft_boolean,	""},
};

enum
{
	NAME_MODE0   = 1 << 0,
	NAME_MODE1   = 1 << 1,
	NAME_MODE2   = 1 << 2,
	NAME_DIFFNOW = 1 << 3,
};

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;

GtkRecentManager *gManager = NULL;
GList *gItems = NULL;
GData *gFileList;
int gNameOpts = 0;
gboolean gOptsInit = FALSE;

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

gboolean Translate(const char *string, char *output, int maxlen)
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

static void AddPropLabels(GString *lfm_string, int Num, gchar *PropName, gchar *Value)
{
	char loc_buf[PATH_MAX];
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
		Translate(PropName, loc_buf, sizeof(loc_buf) - 1);
		char *p = strchr(loc_buf, '\'');

		if (p)
		{
			g_print("%s: PropName: \"%s\", Translation: \"%s\", contains ' char, string truncated", PLUGNAME, PropName, loc_buf);
			*p = '\0';
		}

		g_string_append_printf(lfm_string, label_body, "Prop", Num, "taRightJustify", loc_buf);
		g_string_append_printf(lfm_string, label_body, "Value", Num, "taLeftJustify", string);
		g_free(string);
	}
}

gchar* MakeFileName(gchar *name, gchar *ext, time_t atime, int num)
{
	char dupl[16] = "";
	gchar *filename = NULL;

	if (num > 0)
		snprintf(dupl, sizeof(dupl) - 1, " (#%d)", num);

	if (gNameOpts & NAME_MODE1)
		filename = g_strdup_printf("[%ld] %s%s%s%s", atime, name, dupl, ext ? "." : "", ext ? ext : "");
	else if (gNameOpts & NAME_MODE2)
		filename = g_strdup_printf("%s [%ld]%s%s%s", name, atime, dupl, ext ? "." : "", ext ? ext : "");
	else
		filename = g_strdup_printf("%s%s%s%s", name, dupl, ext ? "." : "", ext ? ext : "");

	return filename;
}

void MakeTestPreview(uintptr_t pDlg)
{
	time_t atime = 1751003394;

	if (gNameOpts & NAME_DIFFNOW)
		atime = time(0) - atime;

	gchar *filename1 = MakeFileName("readme", "txt", atime, 0);
	gchar *filename2 = MakeFileName("readme", "txt", atime, 1);
	gchar *test = g_strdup_printf("%s\n%s", filename1, filename2);
	g_free(filename1);
	g_free(filename2);
	SendDlgMsg(pDlg, "lblTest", DM_SETTEXT, (intptr_t)test, 0);
	g_free(test);
}

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		gOptsInit = TRUE;

		if (gNameOpts & NAME_DIFFNOW)
			SendDlgMsg(pDlg, "chkDiffNow", DM_SETCHECK, 1, 0);

		if (gNameOpts & NAME_MODE1)
			SendDlgMsg(pDlg, "cbForm", DM_LISTSETITEMINDEX, 1, 0);
		else if (gNameOpts & NAME_MODE2)
			SendDlgMsg(pDlg, "cbForm", DM_LISTSETITEMINDEX, 2, 0);
		else
		{
			SendDlgMsg(pDlg, "cbForm", DM_LISTSETITEMINDEX, 0, 0);
			SendDlgMsg(pDlg, "chkDiffNow", DM_ENABLE, 0, 0);
		}

		MakeTestPreview(pDlg);
		gOptsInit = FALSE;
		break;

	case DN_CHANGE:
		if (!gOptsInit)
		{
			gNameOpts = 0;
			gboolean form = (gboolean)SendDlgMsg(pDlg, "chkDiffNow", DM_GETCHECK, 0, 0);
			int index = (int)SendDlgMsg(pDlg, "cbForm", DM_LISTGETITEMINDEX, 0, 0);

			if (index == 1)
				gNameOpts = NAME_MODE1;
			else if (index == 2)
				gNameOpts = NAME_MODE2;

			if (form)
				gNameOpts |= NAME_DIFFNOW;

			SendDlgMsg(pDlg, "chkDiffNow", DM_ENABLE, (index != 0), 0);
			MakeTestPreview(pDlg);
		}

		break;

	}

	return 0;
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	return 0;
}

static BOOL OptionsDioalog()
{
	const char lfm_string[] = "object DialogBox: TOptDialogBox\n"
	                          "  Left = 472\n"
	                          "  Height = 194\n"
	                          "  Top = 372\n"
	                          "  Width = 411\n"
	                          "  AutoSize = True\n"
	                          "  BorderStyle = bsDialog\n"
	                          "  Caption = 'Options'\n"
	                          "  ChildSizing.LeftRightSpacing = 10\n"
	                          "  ChildSizing.TopBottomSpacing = 10\n"
	                          "  ChildSizing.VerticalSpacing = 5\n"
	                          "  ClientHeight = 194\n"
	                          "  ClientWidth = 411\n"
	                          "  Position = poOwnerFormCenter\n"
	                          "  LCLVersion = '4.0.0.4'\n"
	                          "  OnClose = DialogBoxClose\n"
	                          "  OnShow = DialogBoxShow\n"
	                          "  object lblForm: TLabel\n"
	                          "    Left = 32\n"
	                          "    Height = 16\n"
	                          "    Top = 16\n"
	                          "    Width = 91\n"
	                          "    Caption = 'File name form:'\n"
	                          "  end\n"
	                          "  object cbForm: TComboBox\n"
	                          "    AnchorSideLeft.Control = lblForm\n"
	                          "    AnchorSideTop.Control = lblForm\n"
	                          "    AnchorSideTop.Side = asrBottom\n"
	                          "    Left = 32\n"
	                          "    Height = 30\n"
	                          "    Top = 37\n"
	                          "    Width = 352\n"
	                          "    ItemHeight = 0\n"
	                          "    Items.Strings = (\n"
	                          "      'filename.ext'\n"
	                          "      '[unixtime] filename.ext'\n"
	                          "      'filename [unixtime].ext'\n"
	                          "    )\n"
	                          "    Style = csDropDownList\n"
	                          "    TabOrder = 0\n"
	                          "    OnChange = ComboBoxChange\n"
	                          "  end\n"
	                          "  object chkDiffNow: TCheckBox\n"
	                          "    AnchorSideLeft.Control = cbForm\n"
	                          "    AnchorSideTop.Control = cbForm\n"
	                          "    AnchorSideTop.Side = asrBottom\n"
	                          "    Left = 32\n"
	                          "    Height = 23\n"
	                          "    Top = 77\n"
	                          "    Width = 304\n"
	                          "    BorderSpacing.Top = 10\n"
	                          "    Caption = '[unixtime] is the difference from the current time'\n"
	                          "    TabOrder = 1\n"
	                          "    OnChange = CheckBoxChange\n"
	                          "  end\n"
	                          "  object lblTest: TLabel\n"
	                          "    AnchorSideLeft.Control = cbForm\n"
	                          "    AnchorSideTop.Control = chkDiffNow\n"
	                          "    AnchorSideTop.Side = asrBottom\n"
	                          "    AnchorSideRight.Control = cbForm\n"
	                          "    AnchorSideRight.Side = asrBottom\n"
	                          "    Left = 32\n"
	                          "    Height = 16\n"
	                          "    Top = 110\n"
	                          "    Width = 352\n"
	                          "    Alignment = taCenter\n"
	                          "    Anchors = [akTop, akLeft, akRight]\n"
	                          "    BorderSpacing.Top = 10\n"
	                          "    Caption = 'Test'\n"
	                          "    Font.Style = [fsBold]\n"
	                          "    ParentFont = False\n"
	                          "  end\n"
	                          "  object btnClose: TBitBtn\n"
	                          "    AnchorSideTop.Control = lblTest\n"
	                          "    AnchorSideTop.Side = asrBottom\n"
	                          "    AnchorSideRight.Control = cbForm\n"
	                          "    AnchorSideRight.Side = asrBottom\n"
	                          "    Left = 288\n"
	                          "    Height = 30\n"
	                          "    Top = 141\n"
	                          "    Width = 96\n"
	                          "    Anchors = [akTop, akRight]\n"
	                          "    BorderSpacing.Top = 15\n"
	                          "    DefaultCaption = True\n"
	                          "    Kind = bkClose\n"
	                          "    ModalResult = 11\n"
	                          "    TabOrder = 2\n"
	                          "    OnClick = ButtonClick\n"
	                          "  end\n"
	                          "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfm_string, (unsigned long)strlen(lfm_string), OptionsDlgProc);
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
		gchar *icon = g_strdup_printf("%s/icon.png", tmpdir);
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

	char loc_days[256];
	char loc_true[16];
	char loc_false[16];
	Translate("days ago", loc_days, sizeof(loc_days) - 1);
	Translate("True", loc_true, sizeof(loc_true) - 1);
	Translate("False", loc_false, sizeof(loc_false) - 1);

	string = g_strdup_printf("%d %s", gtk_recent_info_get_age(info), loc_days);
	AddPropLabels(lfm_string, i++, "Last update", string);
	g_free(string);
	AddPropLabels(lfm_string, i++, "Local", gtk_recent_info_is_local(info) ? loc_true : loc_false);
	AddPropLabels(lfm_string, i++, "Private", gtk_recent_info_get_private_hint(info) ? loc_true : loc_false);
	AddPropLabels(lfm_string, i++, "Exists", gtk_recent_info_exists(info) ? loc_true : loc_false);

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
	BOOL ret = gExtensions->DialogBoxLFM((intptr_t)lfm_string->str, (unsigned long)strlen(lfm_string->str), PropertiesDlgProc);
	g_string_free(lfm_string, TRUE);
	return ret;
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

			if (gNameOpts & NAME_DIFFNOW)
				atime = dirdata->now - atime;

			int num = 0;
			char *ext = NULL;
			char *dot = strrchr(file, '.');

			if (file[0] != '\0' && dot)
			{
				*dot = '\0';

				if (file[0] == '\0')
					file[0] = '.';
				else
					ext = dot + 1;
			}

			do
			{
				g_free(filename);
				filename = MakeFileName(file, ext, atime, num);
				num++;
			}
			while ((gchar *)g_datalist_get_data(&gFileList, filename) != NULL);

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

			FindData->dwFileAttributes |= FILE_ATTRIBUTE_UNIX_MODE;
			FindData->dwReserved0 = buf.st_mode;

			if (!S_ISDIR(buf.st_mode))
				FindData->dwReserved0 = buf.st_mode;
			else
				FindData->dwReserved0 = buf.st_mode & ~S_IFDIR;
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
	dirdata->now = time(0);
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

	if (strcmp(Verb, "properties") == 0 && (RemoteName[1] == '\0' || strcmp(RemoteName, "/..") == 0))
	{
		OptionsDioalog();
		return FS_EXEC_OK;
	}

	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, RemoteName + 1);

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
	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, RemoteName + 1);

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

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= FIELDCOUNT)
		return ft_nomorefields;

	g_strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);
	g_strlcpy(Units, gFields[FieldIndex].unit, maxlen - 1);
	return gFields[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = ft_fieldempty;

	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, FileName + 1);

	if (uri)
	{
		if (FieldIndex < 4)
		{
			gchar *path = g_filename_from_uri(uri, NULL, NULL);

			if (path)
			{
				if (FieldIndex == 0 || FieldIndex == 1)
				{
					gchar *name = g_path_get_basename(path);

					if (name)
					{
						if (FieldIndex == 0)
							g_strlcpy((char *)FieldValue, name, maxlen - 1);
						else
						{
							char *dot = strrchr(name, '.');

							if (name[0] != '\0' && dot)
							{
								*dot = '\0';

								if (name[0] == '\0')
									name[0] = '.';
							}

							g_strlcpy((char*)FieldValue, name, maxlen - 1);
						}

						result = ft_string;
						g_free(name);
					}
				}
				else if (FieldIndex == 2)
				{
					gchar *dir = g_path_get_dirname(path);
					g_strlcpy((char*)FieldValue, dir, maxlen - 1);
					g_free(dir);
					result = ft_string;
				}
				else if (FieldIndex == 3)
				{
					g_strlcpy((char*)FieldValue, path, maxlen - 1);
					result = ft_string;
				}

				g_free(path);
			}
		}
		else
		{
			GtkRecentInfo *info = gtk_recent_manager_lookup_item(gManager, uri, NULL);

			if (FieldIndex == 4)
			{
				const char *string = gtk_recent_info_get_description(info);
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
				result = ft_string;
			}
			else if (FieldIndex == 5)
			{
				gchar *string = ArrayToString(gtk_recent_info_get_applications(info, NULL));
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
				result = ft_string;
				g_free(string);
			}
			else if (FieldIndex == 6)
			{
				gchar *string = gtk_recent_info_last_application(info);
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
				result = ft_string;
				g_free(string);
			}
			else if (FieldIndex == 7)
			{
				gchar *string = ArrayToString(gtk_recent_info_get_groups(info, NULL));
				g_strlcpy((char*)FieldValue, string, maxlen - 1);
				result = ft_string;
				g_free(string);
			}
			else if (FieldIndex == 8)
			{
				if (UnixTimeToFileTime(gtk_recent_info_get_visited(info), (FILETIME*)FieldValue))
					result = ft_datetime;
			}
			else if (FieldIndex == 9)
			{
				if (UnixTimeToFileTime(gtk_recent_info_get_added(info), (FILETIME*)FieldValue))
					result = ft_datetime;
			}
			else if (FieldIndex == 10)
			{
				*(int*)FieldValue = (int)gtk_recent_info_exists(info);
				result = ft_boolean;
			}
			else if (FieldIndex == 11)
			{
				*(int*)FieldValue = (int)gtk_recent_info_get_private_hint(info);
				result = ft_boolean;
			}

			gtk_recent_info_unref(info);
		}
	}

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	char buff[MAX_PATH];
	GString *headers = g_string_new(NULL);
	Translate("Size", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	Translate("Visited", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	Translate("Last App", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	Translate("Exists", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	g_strlcpy(ViewHeaders, headers->str, maxlen - 1);
	g_string_free(headers, TRUE);
	g_strlcpy(ViewContents, "[DC().GETFILESIZE{}]\\n[Plugin(FS).Visited{}]\\n[Plugin(FS).LastApp{}]\\n[Plugin(FS).Exists{}]", maxlen - 1);
	g_strlcpy(ViewWidths, "100,20,-30,45,60,-15", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

int DCPCALL FsExtractCustomIcon(char* RemoteName, int ExtractFlags, PWfxIcon TheIcon)
{
	int result = FS_ICON_USEDEFAULT;
	gchar *uri = (gchar*)g_datalist_get_data(&gFileList, RemoteName + 1);
	GtkRecentInfo *info = gtk_recent_manager_lookup_item(gManager, uri, NULL);
	GdkPixbuf *pixbuf = gtk_recent_info_get_icon(info, 48);

	if (pixbuf)
	{
		gchar *tmpdir = g_dir_make_tmp(PLUGNAME "_XXXXXX", NULL);
		gchar *icon = g_strdup_printf("%s/icon.png", tmpdir);
		gdk_pixbuf_save(pixbuf, icon, "png", NULL, NULL);
		g_object_unref(pixbuf);
		gsize len;
		gchar *data = NULL;
		if (g_file_get_contents(icon, &data, &len, NULL))
		{
			TheIcon->Pointer = (void*)data;
			TheIcon->Size = (uintptr_t)len;
			TheIcon->Format = FS_ICON_FORMAT_BINARY;
			TheIcon->Free = (tFreeProc)g_free;
			result = FS_ICON_EXTRACTED_DESTROY;
		}
		remove(icon);
		remove(tmpdir);
		g_free(tmpdir);
		g_free(icon);
	}

	gtk_recent_info_unref(info);
	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	Translate("Recent", DefRootName, maxlen - 1);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo * StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));

		GKeyFile *cfg = g_key_file_new();
		gchar *filename = g_strdup_printf("%s/j2969719.ini", gExtensions->PluginConfDir);

		if (g_key_file_load_from_file(cfg, filename, G_KEY_FILE_KEEP_COMMENTS, NULL))
			gNameOpts = g_key_file_get_integer(cfg, PLUGNAME, "Options", NULL);

		g_free(filename);
		g_key_file_free(cfg);
	}

	if (gtk_main_level() == 0)
		gtk_init_check(0, NULL);
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
	{
		GKeyFile *cfg = g_key_file_new();
		gchar *filename = g_strdup_printf("%s/j2969719.ini", gExtensions->PluginConfDir);

		if (g_key_file_load_from_file(cfg, filename, G_KEY_FILE_KEEP_COMMENTS, NULL))
		{
			g_key_file_set_integer(cfg, PLUGNAME, "Options", gNameOpts);
			g_key_file_save_to_file(cfg, filename, NULL);
		}

		g_free(filename);
		g_key_file_free(cfg);
		free(gExtensions);
	}

	gExtensions = NULL;

	FreeDataArrays();
}
