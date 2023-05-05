#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <tag_c.h>
#include <errno.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox

#define ROOTNAME "Taglib"
#define ININAME "j2969719_wfx.ini"
#define MAXINIITEMS 256


typedef struct sVFSDirData
{
	DIR *cur;
	char path[PATH_MAX];
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

GKeyFile *gCfg = NULL;
static gchar *gCfgPath = NULL;
gchar *gLastFile = NULL;
TagLib_Tag *gLastTag = NULL;
TagLib_File *gLastTagFile = NULL;
static char gStartPath[PATH_MAX] = "/";

static char gOutExt[6];
gchar *gTerm = NULL;
gchar *gCommand = NULL;


tField gFields[] =
{
	{"title",	    ft_string},
	{"artist",	    ft_string},
	{"album",	    ft_string},
	{"comment",	    ft_string},
	{"genre",	    ft_string},
	{"year",	ft_numeric_32},
	{"track",	ft_numeric_32},
	{"bitrate",	ft_numeric_32},
	{"sample rate",	ft_numeric_32},
	{"channels",	ft_numeric_32},
	{"lenght",	    ft_string},
};

void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	int64_t ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

BOOL Translate(const char *string, char *output, int maxlen)
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

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	char *string;
	unsigned int value;

	switch (Msg)
	{
	case DN_INITDIALOG:
		if (gLastTag)
		{
			char *pos = strrchr(gLastFile, '/');

			if (pos)
				SendDlgMsg(pDlg, "edFileName", DM_SETTEXT, (intptr_t)pos + 1, 0);

			string = taglib_tag_title(gLastTag);

			if (string)
				SendDlgMsg(pDlg, "edTitle", DM_SETTEXT, (intptr_t)string, 0);

			string = taglib_tag_artist(gLastTag);

			if (string)
				SendDlgMsg(pDlg, "edArtist", DM_SETTEXT, (intptr_t)string, 0);

			string = taglib_tag_album(gLastTag);

			if (string)
				SendDlgMsg(pDlg, "edAlbum", DM_SETTEXT, (intptr_t)string, 0);

			string = taglib_tag_comment(gLastTag);

			if (string)
				SendDlgMsg(pDlg, "edComment", DM_SETTEXT, (intptr_t)string, 0);

			string = taglib_tag_genre(gLastTag);

			if (string)
				SendDlgMsg(pDlg, "edGenre", DM_SETTEXT, (intptr_t)string, 0);

			value = taglib_tag_year(gLastTag);

			if (value != 0)
			{
				char year[5];
				snprintf(year, sizeof(year), "%d", value);
				SendDlgMsg(pDlg, "edYear", DM_SETTEXT, (intptr_t)year, 0);
			}

			value = taglib_tag_track(gLastTag);

			if (value != 0)
			{
				char track[5];
				snprintf(track, sizeof(track), "%d", value);
				SendDlgMsg(pDlg, "edTrack", DM_SETTEXT, (intptr_t)track, 0);
			}
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			string = (char*)SendDlgMsg(pDlg, "edTitle", DM_GETTEXT, 0, 0);

			if (string)
				taglib_tag_set_title(gLastTag, string);

			string = (char*)SendDlgMsg(pDlg, "edArtist", DM_GETTEXT, 0, 0);

			if (string)
				taglib_tag_set_artist(gLastTag, string);

			string = (char*)SendDlgMsg(pDlg, "edAlbum", DM_GETTEXT, 0, 0);

			if (string)
				taglib_tag_set_album(gLastTag, string);

			string = (char*)SendDlgMsg(pDlg, "edComment", DM_GETTEXT, 0, 0);

			if (string)
				taglib_tag_set_comment(gLastTag, string);

			string = (char*)SendDlgMsg(pDlg, "edGenre", DM_GETTEXT, 0, 0);

			if (string)
				taglib_tag_set_genre(gLastTag, string);

			string = (char*)SendDlgMsg(pDlg, "edYear", DM_GETTEXT, 0, 0);

			if (string)
			{
				if (string[0] == '\0')
					value = 0;
				else
					value = (unsigned int)atoi(string);

				taglib_tag_set_year(gLastTag, value);
			}

			string = (char*)SendDlgMsg(pDlg, "edTrack", DM_GETTEXT, 0, 0);

			if (string)
			{
				if (string[0] == '\0')
					value = 0;
				else
					value = (unsigned int)atoi(string);

				taglib_tag_set_track(gLastTag, value);
			}

			if (!taglib_file_save(gLastTagFile))
			{
				char msg[MAX_PATH];
				Translate("Failed to write tag.", msg, MAX_PATH);
				MessageBox(msg, ROOTNAME, MB_OK | MB_ICONERROR);
			}
		}
		else if (strcmp(DlgItemName, "btnAutoPaste") == 0)
		{
			gchar *filename = g_strdup((char*)SendDlgMsg(pDlg, "edFileName", DM_GETTEXT, 0, 0));
			char *pos = strrchr(filename, '.');

			if (pos)
				*pos = '\0';

			char **split = g_strsplit(filename, " - ", -1);
			guint len = g_strv_length(split);
			char question[MAX_PATH], artist_str[MAX_PATH], title_str[MAX_PATH];
			Translate("Fill in these fields with the following values?", question, MAX_PATH);
			Translate("Artist", artist_str, MAX_PATH);
			Translate("Title", title_str, MAX_PATH);
			gchar *message = NULL;

			if (len > 1)
				message = g_strdup_printf("%s\n\t%s: %s\n\t%s: %s", question, artist_str, split[0], title_str, split[len - 1]);
			else
				message = g_strdup_printf("%s\n\t%s: %s", question, title_str, split[0]);

			int ret = MessageBox((char*)message, NULL, MB_YESNO | MB_ICONQUESTION);
			g_free(message);

			if (ret == ID_YES)
			{
				if (len > 1)
				{
					SendDlgMsg(pDlg, "edArtist", DM_SETTEXT, (intptr_t)split[0], 0);
					SendDlgMsg(pDlg, "edTitle", DM_SETTEXT, (intptr_t)split[len - 1], 0);
				}
				else
					SendDlgMsg(pDlg, "edTitle", DM_SETTEXT, (intptr_t)split[0], 0);
			}

			g_strfreev(split);
			g_free(filename);
		}

		break;
	}

	return 0;
}

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);
		gchar *path = g_key_file_get_string(gCfg, ROOTNAME, "StartPath", NULL);

		if (path)
		{
			SendDlgMsg(pDlg, "fnSelectPath", DM_SETTEXT, (intptr_t)path, 0);
			g_free(path);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			char *path = (char*)SendDlgMsg(pDlg, "fnSelectPath", DM_GETTEXT, 0, 0);

			if (path && strlen(path) > 0)
			{

				g_strlcpy(gStartPath, path, sizeof(gStartPath));
				g_key_file_set_string(gCfg, ROOTNAME, "StartPath", path);
				g_key_file_save_to_file(gCfg, gCfgPath, NULL);
			}
		}

		break;
	}

	return 0;
}

intptr_t DCPCALL ConvertDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);

		gchar **items = g_key_file_get_keys(gCfg, ROOTNAME, NULL, NULL);

		if (items)
		{
			for (gsize i = 0; items[i] != NULL; i++)
			{
				if (strncmp(items[i], "Command", 6) == 0)
				{
					gchar *item = g_key_file_get_string(gCfg, ROOTNAME, items[i], NULL);

					if (item && strlen(item) > 0)
						SendDlgMsg(pDlg, "cbCommand", DM_LISTADDSTR, (intptr_t)item, 0);

					g_free(item);
				}
				else if (strncmp(items[i], "Ext", 3) == 0)
				{
					gchar *item = g_key_file_get_string(gCfg, ROOTNAME, items[i], NULL);

					if (item && strlen(item) > 0)
						SendDlgMsg(pDlg, "cbExt", DM_LISTADDSTR, (intptr_t)item, 0);

					g_free(item);
				}
			}
		}

		g_strfreev(items);

		if (gOutExt[0] != '\0')
		{
			SendDlgMsg(pDlg, "edTerm", DM_SETTEXT, (intptr_t)gOutExt, 0);
			memset(gOutExt, 0, sizeof(gOutExt));
		}

		if (gCommand)
		{
			SendDlgMsg(pDlg, "edTerm", DM_SETTEXT, (intptr_t)gCommand, 0);
			g_free(gCommand);
			gCommand = NULL;
		}

		if (!gTerm)
			gTerm = g_key_file_get_string(gCfg, ROOTNAME, "Termminal", NULL);

		if (gTerm)
			SendDlgMsg(pDlg, "edTerm", DM_SETTEXT, (intptr_t)gTerm, 0);

		g_free(gTerm);
		gTerm = NULL;
		SendDlgMsg(pDlg, "cbExt", DM_LISTSETITEMINDEX, 0, 0);
		SendDlgMsg(pDlg, "cbCommand", DM_LISTSETITEMINDEX, 0, 0);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			g_strlcpy(gOutExt, (char*)SendDlgMsg(pDlg, "cbExt", DM_GETTEXT, 0, 0), sizeof(gOutExt));
			gCommand = g_strdup((char*)SendDlgMsg(pDlg, "cbCommand", DM_GETTEXT, 0, 0));
			gTerm = g_strdup((char*)SendDlgMsg(pDlg, "edTerm", DM_GETTEXT, 0, 0));
			g_key_file_set_string(gCfg, ROOTNAME, "Ext0", gOutExt);
			g_key_file_set_string(gCfg, ROOTNAME, "Command0", gCommand);
			g_key_file_set_string(gCfg, ROOTNAME, "Termminal", gTerm);

			gsize count = (gsize)SendDlgMsg(pDlg, "cbExt", DM_LISTGETCOUNT, 0, 0);
			int num = 1;

			for (gsize i = 0; i < count; i++)
			{
				char *line = (char*)SendDlgMsg(pDlg, "cbExt", DM_LISTGETITEM, i, 0);

				if (g_strcmp0(line, gOutExt) != 0)
				{
					gchar *key = g_strdup_printf("Ext%d", num++);
					g_key_file_set_string(gCfg, ROOTNAME, key, line);
					g_free(key);

					if (num == MAXINIITEMS)
						break;
				}
			}

			count = (gsize)SendDlgMsg(pDlg, "cbCommand", DM_LISTGETCOUNT, 0, 0);
			num = 1;

			for (gsize i = 0; i < count; i++)
			{
				char *line = (char*)SendDlgMsg(pDlg, "cbCommand", DM_LISTGETITEM, i, 0);

				if (g_strcmp0(line, gCommand) != 0)
				{
					gchar *key = g_strdup_printf("Command%d", num++);
					g_key_file_set_string(gCfg, ROOTNAME, key, line);
					g_free(key);

					if (num == MAXINIITEMS)
						break;
				}
			}

			g_key_file_save_to_file(gCfg, gCfgPath, NULL);
		}

		break;

	case DN_CHANGE:
		SendDlgMsg(pDlg, "lblErr", DM_SHOWITEM, 0, 0);
		SendDlgMsg(pDlg, "lblTemplate", DM_SHOWITEM, 0, 0);
		SendDlgMsg(pDlg, "lblTemplate", DM_SETTEXT, 0, 0);
		SendDlgMsg(pDlg, "lblPreview", DM_SETTEXT, 0, 0);

		gchar *cmd = g_strdup((char*)SendDlgMsg(pDlg, "cbCommand", DM_GETTEXT, 0, 0));

		if (!g_strrstr(cmd, "{infile}"))
		{
			g_free(cmd);
			SendDlgMsg(pDlg, "lblErr", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lblTemplate", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lblTemplate", DM_SETTEXT, (intptr_t)"{infile}", 0);
			return 0;
		}
		else if (!g_strrstr(cmd, "{outfilenoext.ext}"))
		{
			g_free(cmd);
			SendDlgMsg(pDlg, "lblErr", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lblTemplate", DM_SHOWITEM, 1, 0);
			SendDlgMsg(pDlg, "lblTemplate", DM_SETTEXT, (intptr_t)"{outfilenoext.ext}", 0);
			return 0;
		}

		gchar *term = g_strdup((char*)SendDlgMsg(pDlg, "edTerm", DM_GETTEXT, 0, 0));
		gchar *ext = g_strdup((char*)SendDlgMsg(pDlg, "cbExt", DM_GETTEXT, 0, 0));

		gchar *temp = str_replace(cmd, "{infile}", "/home/user/test.mp3", TRUE);
		g_free(cmd);
		cmd = temp;
		gchar *newname = g_strdup_printf("/home/user/test.%s", ext);
		temp = str_replace(cmd, "{outfilenoext.ext}", newname, TRUE);
		g_free(newname);
		g_free(cmd);

		if (term && strlen(term) > 0)
		{
			if (g_strrstr(term, "{command}"))
			{
				cmd = temp;
				temp = str_replace(term, "{command}", cmd, FALSE);
				g_free(cmd);
			}
			else
			{
				SendDlgMsg(pDlg, "lblErr", DM_SHOWITEM, 1, 0);
				SendDlgMsg(pDlg, "lblTemplate", DM_SHOWITEM, 1, 0);
				SendDlgMsg(pDlg, "lblTemplate", DM_SETTEXT, (intptr_t)"{command}", 0);
			}
		}

		SendDlgMsg(pDlg, "lblPreview", DM_SETTEXT, (intptr_t)temp, 0);
		g_free(temp);
		g_free(ext);
		g_free(term);

		break;
	}

	return 0;
}

static BOOL PropertiesDialog(char* FileName)
{
	const char lfmdata[] = ""
	                       "object PropDialogBox: TPropDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 466\n"
	                       "  Top = 307\n"
	                       "  Width = 468\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Properties'\n"
	                       "  ChildSizing.LeftRightSpacing = 15\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ChildSizing.VerticalSpacing = 10\n"
	                       "  ClientHeight = 466\n"
	                       "  ClientWidth = 468\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object edFileName: TEdit\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    AnchorSideRight.Control = btnAutoPaste\n"
	                       "    Left = 15\n"
	                       "    Height = 36\n"
	                       "    Top = 15\n"
	                       "    Width = 341\n"
	                       "    Alignment = taCenter\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderStyle = bsNone\n"
	                       "    Color = clForm\n"
	                       "    Font.Style = [fsBold]\n"
	                       "    ParentFont = False\n"
	                       "    ReadOnly = True\n"
	                       "    TabStop = False\n"
	                       "    TabOrder = 0\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = gbTag\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = gbTag\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 286\n"
	                       "    Height = 31\n"
	                       "    Top = 383\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Top = 20\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkOK\n"
	                       "    ModalResult = 1\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 3\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 180\n"
	                       "    Height = 31\n"
	                       "    Top = 383\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Cancel = True\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkCancel\n"
	                       "    ModalResult = 2\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 4\n"
	                       "  end\n"
	                       "  object gbTag: TGroupBox\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = edFileName\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 302\n"
	                       "    Top = 61\n"
	                       "    Width = 372\n"
	                       "    AutoSize = True\n"
	                       "    ChildSizing.LeftRightSpacing = 10\n"
	                       "    ChildSizing.TopBottomSpacing = 10\n"
	                       "    ChildSizing.HorizontalSpacing = 10\n"
	                       "    ChildSizing.VerticalSpacing = 5\n"
	                       "    ChildSizing.EnlargeHorizontal = crsHomogenousChildResize\n"
	                       "    ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
	                       "    ChildSizing.ControlsPerLine = 2\n"
	                       "    ClientHeight = 302\n"
	                       "    ClientWidth = 356\n"
	                       "    TabOrder = 2\n"
	                       "    object lblArtist: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 10\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Artist'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edArtist: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 10\n"
	                       "      Width = 256\n"
	                       "      TabOrder = 0\n"
	                       "    end\n"
	                       "    object lblTitle: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 51\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Title'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edTitle: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 51\n"
	                       "      Width = 256\n"
	                       "      Constraints.MinWidth = 256\n"
	                       "      TabOrder = 1\n"
	                       "    end\n"
	                       "    object lblAlbum: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 92\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Album'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edAlbum: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 92\n"
	                       "      Width = 256\n"
	                       "      TabOrder = 2\n"
	                       "    end\n"
	                       "    object lblTrack: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 133\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Track'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edTrack: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 133\n"
	                       "      Width = 256\n"
	                       "      MaxLength = 4\n"
	                       "      NumbersOnly = True\n"
	                       "      OnChange = EditChange\n"
	                       "      TabOrder = 3\n"
	                       "    end\n"
	                       "    object lblYear: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 174\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Year'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edYear: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 174\n"
	                       "      Width = 256\n"
	                       "      MaxLength = 4\n"
	                       "      NumbersOnly = True\n"
	                       "      OnChange = EditChange\n"
	                       "      TabOrder = 4\n"
	                       "    end\n"
	                       "    object lblGenre: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 215\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Genre'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edGenre: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 215\n"
	                       "      Width = 256\n"
	                       "      TabOrder = 5\n"
	                       "    end\n"
	                       "    object lblComment: TLabel\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 256\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Comment'\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edComment: TEdit\n"
	                       "      Left = 90\n"
	                       "      Height = 36\n"
	                       "      Top = 256\n"
	                       "      Width = 256\n"
	                       "      TabOrder = 6\n"
	                       "    end\n"
	                       "  end\n"
	                       "  object btnAutoPaste: TButton\n"
	                       "    AnchorSideLeft.Control = edFileName\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = edFileName\n"
	                       "    AnchorSideRight.Control = gbTag\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    AnchorSideBottom.Control = edFileName\n"
	                       "    AnchorSideBottom.Side = asrBottom\n"
	                       "    Left = 356\n"
	                       "    Height = 36\n"
	                       "    Top = 15\n"
	                       "    Width = 31\n"
	                       "    Anchors = [akTop, akRight, akBottom]\n"
	                       "    AutoSize = True\n"
	                       "    Caption = '📋'\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 1\n"
	                       "  end\n"
	                       "end\n";

	gchar *localpath = g_strdup_printf("%s%s", gStartPath, FileName);

	if (!gLastFile || g_strcmp0(localpath, gLastFile) != 0)
	{
		g_free(gLastFile);
		gLastFile = localpath;

		if (gLastTagFile != NULL)
		{
			taglib_tag_free_strings();
			taglib_file_free(gLastTagFile);
		}

		gLastTagFile = taglib_file_new(localpath);

		if (gLastTagFile != NULL && taglib_file_is_valid(gLastTagFile))
			gLastTag = taglib_file_tag(gLastTagFile);
	}

	if (!gLastTagFile || !taglib_file_is_valid(gLastTagFile))
	{
		char msg[MAX_PATH];
		Translate("Failed to fetch tag, possibly wrong file selected.", msg, MAX_PATH);
		MessageBox(msg, ROOTNAME, MB_OK | MB_ICONERROR);

		return FALSE;
	}

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), PropertiesDlgProc);
}

static BOOL OptionsDialog(void)
{
	const char lfmdata[] = ""
	                       "object OptDialogBox: TOptDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 163\n"
	                       "  Top = 307\n"
	                       "  Width = 672\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Options'\n"
	                       "  ChildSizing.LeftRightSpacing = 15\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ChildSizing.VerticalSpacing = 10\n"
	                       "  ClientHeight = 163\n"
	                       "  ClientWidth = 672\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblStartPath: TLabel\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    Left = 15\n"
	                       "    Height = 49\n"
	                       "    Top = 15\n"
	                       "    Width = 638\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Caption = 'Please select the folder you want to work with.'#10#10'You can re-open this dialog by clicking plugin properties or \"..\" properties in the root folder.'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object fnSelectPath: TDirectoryEdit\n"
	                       "    AnchorSideLeft.Control = lblStartPath\n"
	                       "    AnchorSideTop.Control = lblStartPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lblStartPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 23\n"
	                       "    Top = 74\n"
	                       "    Width = 638\n"
	                       "    DialogTitle = 'Select folder'\n"
	                       "    ButtonWidth = 24\n"
	                       "    NumGlyphs = 1\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 0\n"
	                       "    Text = '/'\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 446\n"
	                       "    Height = 31\n"
	                       "    Top = 112\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Cancel = True\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkCancel\n"
	                       "    ModalResult = 2\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = fnSelectPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = fnSelectPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 552\n"
	                       "    Height = 31\n"
	                       "    Top = 112\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Top = 15\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkOK\n"
	                       "    ModalResult = 1\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 1\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), OptionsDlgProc);
}

static BOOL ConvertDialog(void)
{
	const char lfmdata[] = ""
	                       "object ConvDialogBox: TConvDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 188\n"
	                       "  Top = 307\n"
	                       "  Width = 652\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Convert'\n"
	                       "  ChildSizing.LeftRightSpacing = 10\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ClientHeight = 188\n"
	                       "  ClientWidth = 652\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object lblExt: TLabel\n"
	                       "    AnchorSideLeft.Control = cbExt\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 15\n"
	                       "    Width = 67\n"
	                       "    Caption = 'Extension'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object cbExt: TComboBox\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = lblExt\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lblExt\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 23\n"
	                       "    Top = 32\n"
	                       "    Width = 129\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    ItemHeight = 17\n"
	                       "    MaxLength = 5\n"
	                       "    OnChange = ComboBoxChange\n"
	                       "    TabOrder = 0\n"
	                       "    Text = 'ogg'\n"
	                       "  end\n"
	                       "  object lblCommand: TLabel\n"
	                       "    AnchorSideLeft.Control = cbCommand\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    Left = 149\n"
	                       "    Height = 17\n"
	                       "    Top = 15\n"
	                       "    Width = 72\n"
	                       "    Caption = 'Command'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object cbCommand: TComboBox\n"
	                       "    AnchorSideLeft.Control = cbExt\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = cbExt\n"
	                       "    Left = 149\n"
	                       "    Height = 23\n"
	                       "    Top = 32\n"
	                       "    Width = 491\n"
	                       "    BorderSpacing.Left = 10\n"
	                       "    ItemHeight = 17\n"
	                       "    OnChange = ComboBoxChange\n"
	                       "    TabOrder = 1\n"
	                       "    Text = 'ffmpeg -i {infile} {outfilenoext.ext}'\n"
	                       "  end\n"
	                       "  object lblTerm: TLabel\n"
	                       "    AnchorSideLeft.Control = edTerm\n"
	                       "    AnchorSideTop.Control = cbExt\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 17\n"
	                       "    Top = 65\n"
	                       "    Width = 73\n"
	                       "    BorderSpacing.Top = 10\n"
	                       "    Caption = 'Termminal'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object edTerm: TEdit\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = lblTerm\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = cbCommand\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 23\n"
	                       "    Top = 82\n"
	                       "    Width = 630\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    OnChange = EditChange\n"
	                       "    TabOrder = 2\n"
	                       "    Text = 'xterm -e sh -c \"{command}\"'\n"
	                       "  end\n"
	                       "  object lblPreview: TLabel\n"
	                       "    AnchorSideLeft.Control = edTerm\n"
	                       "    AnchorSideTop.Control = edTerm\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = edTerm\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 12\n"
	                       "    Height = 1\n"
	                       "    Top = 115\n"
	                       "    Width = 628\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Left = 2\n"
	                       "    BorderSpacing.Top = 10\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object lblErr: TLabel\n"
	                       "    AnchorSideLeft.Control = lblPreview\n"
	                       "    AnchorSideTop.Control = lblPreview\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 12\n"
	                       "    Height = 17\n"
	                       "    Top = 116\n"
	                       "    Width = 243\n"
	                       "    Caption = 'A required template is missing'\n"
	                       "    Font.Style = [fsBold, fsItalic]\n"
	                       "    ParentColor = False\n"
	                       "    ParentFont = False\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object lblTemplate: TLabel\n"
	                       "    AnchorSideLeft.Control = lblErr\n"
	                       "    AnchorSideLeft.Side = asrBottom\n"
	                       "    AnchorSideTop.Control = lblErr\n"
	                       "    Left = 260\n"
	                       "    Height = 1\n"
	                       "    Top = 116\n"
	                       "    Width = 1\n"
	                       "    BorderSpacing.Left = 5\n"
	                       "    Font.Style = [fsBold]\n"
	                       "    ParentColor = False\n"
	                       "    ParentFont = False\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = lblPreview\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = cbCommand\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 539\n"
	                       "    Height = 31\n"
	                       "    Top = 136\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Top = 20\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    Default = True\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkOK\n"
	                       "    ModalResult = 1\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 3\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 433\n"
	                       "    Height = 31\n"
	                       "    Top = 136\n"
	                       "    Width = 101\n"
	                       "    Anchors = [akTop, akRight]\n"
	                       "    AutoSize = True\n"
	                       "    BorderSpacing.Right = 5\n"
	                       "    Cancel = True\n"
	                       "    Constraints.MinHeight = 31\n"
	                       "    Constraints.MinWidth = 101\n"
	                       "    DefaultCaption = True\n"
	                       "    Kind = bkCancel\n"
	                       "    ModalResult = 2\n"
	                       "    OnClick = ButtonClick\n"
	                       "    TabOrder = 4\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), ConvertDlgProc);
}

static BOOL SetFindData(DIR *cur, char *path, WIN32_FIND_DATAA *FindData)
{
	struct dirent *ent;
	char file[PATH_MAX];

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	while ((ent = readdir(cur)) != NULL)
	{

		snprintf(file, sizeof(file), "%s/%s", path, ent->d_name);
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
		GFile *gfile = g_file_new_for_path(file);

		if (!gfile)
			continue;

		GFileInfo *fileinfo = g_file_query_info(gfile, "standard::*,time::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

		if (!gfile)
		{
			g_object_unref(gfile);
			continue;
		}

		if (g_file_info_get_file_type(fileinfo) == G_FILE_TYPE_DIRECTORY)
			FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		else
		{
			const gchar *content_type = g_file_info_get_content_type(fileinfo);

			if (strncmp(content_type, "audio", 5) != 0)
			{
				g_object_unref(fileinfo);
				g_object_unref(gfile);
				continue;
			}

			int64_t size = (int64_t)g_file_info_get_size(fileinfo);
			FindData->nFileSizeHigh = (size & 0xFFFFFFFF00000000) >> 32;
			FindData->nFileSizeLow = size & 0x00000000FFFFFFFF;
		}

		int64_t timeinfo = (int64_t)g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED);
		UnixTimeToFileTime(timeinfo, &FindData->ftLastWriteTime);
		timeinfo = (int64_t)g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_TIME_ACCESS);
		UnixTimeToFileTime(timeinfo, &FindData->ftLastAccessTime);
		g_strlcpy(FindData->cFileName, ent->d_name, MAX_PATH - 1);
		g_object_unref(fileinfo);
		g_object_unref(gfile);

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

	g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);
	gchar *path = g_key_file_get_string(gCfg, ROOTNAME, "StartPath", NULL);

	if (path)
	{
		g_strlcpy(gStartPath, path, sizeof(gStartPath));
		g_free(path);
	}
	else
		OptionsDialog();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	DIR *dir;
	tVFSDirData *dirdata;

	dirdata = malloc(sizeof(tVFSDirData));

	if (dirdata == NULL)
		return (HANDLE)(-1);

	memset(dirdata, 0, sizeof(tVFSDirData));
	snprintf(dirdata->path, sizeof(dirdata->path), "%s%s", gStartPath, Path);

	if ((dir = opendir(dirdata->path)) == NULL)
	{
		int errsv = errno;
		MessageBox(strerror(errsv), ROOTNAME, MB_OK | MB_ICONERROR);
		free(dirdata);
		return (HANDLE)(-1);
	}

	dirdata->cur = dir;

	if (!SetFindData(dirdata->cur, dirdata->path, FindData))
	{
		closedir(dir);
		free(dirdata);
		return (HANDLE)(-1);
	}

	return (HANDLE)dirdata;
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata->cur, dirdata->path, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->cur != NULL)
		closedir(dirdata->cur);

	if (dirdata != NULL)
		free(dirdata);

	return 0;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	gchar *localpath = g_strdup_printf("%s%s", gStartPath, RemoteName);
	g_strlcpy(RemoteName, localpath, maxlen - 1);
	g_free(localpath);

	return TRUE;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	int status;
	char str[MAX_PATH];
	int result = FS_FILE_WRITEERROR;

	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (!gCommand || gCommand[0] == '\0' || gOutExt[0] == '\0')
		return FS_FILE_USERABORT;

	gchar *localpath = g_strdup_printf("%s%s", gStartPath, RemoteName);
	gchar *outdir = g_path_get_dirname(LocalName);
	gchar *file = g_path_get_basename(LocalName);
	char *pos = strrchr(file, '.');

	if (pos)
		*pos = '\0';

	gchar *outfile = g_strdup_printf("%s/%s.%s", outdir, file, gOutExt);
	g_free(outdir);
	g_free(file);

	if (g_strcmp0(localpath, outfile) == 0)
	{
		g_free(localpath);
		g_free(outfile);
		return FS_FILE_NOTSUPPORTED;
	}

	gProgressProc(gPluginNr, localpath, outfile, 0);

	if (CopyFlags == 0 && g_file_test(outfile, G_FILE_TEST_EXISTS))
	{
		Translate("File %s already exists, overwrite?", str, MAX_PATH);
		gchar *msg = g_strdup_printf(str, outfile);
		int ret = MessageBox((char*)msg, NULL, MB_YESNOCANCEL | MB_ICONQUESTION);
		g_free(msg);

		if (ret != ID_YES)
		{
			g_free(localpath);
			g_free(outfile);

			if (ret == ID_NO)
				return FS_FILE_OK;
			else
				return FS_FILE_USERABORT;
		}
	}

	gchar *temp = str_replace(gCommand, "{infile}", localpath, TRUE);
	gchar *command = temp;
	temp = str_replace(command, "{outfilenoext.ext}", outfile, TRUE);
	g_free(command);
	command = temp;
	temp = str_replace(command, "\"", "\\\"", FALSE);
	g_free(command);
	command = temp;

	if (!gTerm || gTerm[0] != '\0')
	{
		temp = str_replace(gTerm, "{command}", command, FALSE);
		g_free(command);
		command = temp;
	}

	if ((status = system(command)) != 0)
	{
		Translate("Error executing command", str, MAX_PATH);
		gchar *msg = g_strdup_printf("%s \"%s\" (%d)", str, command, status);

		if (MessageBox((char*)msg, NULL, MB_OKCANCEL | MB_ICONERROR) == ID_CANCEL)
			result = FS_FILE_USERABORT;

		g_free(msg);
	}
	else
		result = FS_FILE_OK;


	g_free(command);
	gProgressProc(gPluginNr, localpath, outfile, 100);
	g_free(localpath);
	g_free(outfile);

	return result;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct * ri)
{
	int result = FS_FILE_WRITEERROR;

	if (gProgressProc(gPluginNr, OldName, NewName, 0))
		return FS_FILE_USERABORT;

	if (!Move)
		return FS_FILE_NOTSUPPORTED;

	gchar *oldlocal = g_strdup_printf("%s%s", gStartPath, OldName);
	gchar *newlocal = g_strdup_printf("%s%s", gStartPath, NewName);

	if (!OverWrite && g_file_test(newlocal, G_FILE_TEST_EXISTS))
	{
		g_free(oldlocal);
		g_free(newlocal);
		return FS_FILE_EXISTS;
	}

	if (rename(oldlocal, newlocal) == 0)
		result = FS_FILE_OK;

	gProgressProc(gPluginNr, oldlocal, newlocal, 100);
	g_free(oldlocal);
	g_free(newlocal);

	return result;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	if (strcmp(Verb, "open") == 0)
	{
		gchar *localpath = g_strdup_printf("%s%s", gStartPath, RemoteName);
		gchar *quoted = g_shell_quote(localpath);
		gchar *command = g_strdup_printf("xdg-open %s", quoted);
		g_free(quoted);
		g_free(localpath);
		g_spawn_command_line_async(command, NULL);
		g_free(command);
		return FS_FILE_OK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		if (strcmp(RemoteName, "/") == 0 || strcmp(RemoteName, "/..") == 0)
			OptionsDialog();
		else
			PropertiesDialog(RemoteName);

		return FS_EXEC_OK;
	}

	return FS_EXEC_ERROR;
}

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{
	if (InfoOperation == FS_STATUS_OP_GET_SINGLE || InfoOperation == FS_STATUS_OP_GET_MULTI)
	{
		if (InfoStartEnd == FS_STATUS_START)
			ConvertDialog();
	}
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);
	Units[0] = '\0';
	return gFields[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int value;
	char *string;
	const TagLib_AudioProperties *props;

	gchar *localpath = g_strdup_printf("%s%s", gStartPath, FileName);

	if (!gLastFile || g_strcmp0(localpath, gLastFile) != 0)
	{
		g_free(gLastFile);
		gLastFile = localpath;

		if (gLastTagFile != NULL)
		{
			taglib_tag_free_strings();
			taglib_file_free(gLastTagFile);
		}

		gLastTagFile = taglib_file_new(localpath);

		if (gLastTagFile != NULL && taglib_file_is_valid(gLastTagFile))
			gLastTag = taglib_file_tag(gLastTagFile);
	}

	if (!gLastTagFile)
		return ft_fileerror;

	if (!gLastTag || !taglib_file_is_valid(gLastTagFile))
		return ft_fieldempty;

	if (FieldIndex > 6)
		props = taglib_file_audioproperties(gLastTagFile);

	switch (FieldIndex)
	{
	case 0:
		string = taglib_tag_title(gLastTag);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 1:
		string = taglib_tag_artist(gLastTag);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 2:
		string = taglib_tag_album(gLastTag);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 3:
		string = taglib_tag_comment(gLastTag);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 4:
		string = taglib_tag_genre(gLastTag);

		if (string)
			g_strlcpy((char*)FieldValue, string, maxlen - 1);
		else
			return ft_fieldempty;

		break;

	case 5:
		value = (int)taglib_tag_year(gLastTag);

		if (value != 0)
			*(int*)FieldValue = value;
		else
			return ft_fieldempty;

		break;

	case 6:
		value = (int)taglib_tag_track(gLastTag);

		if (value != 0)
			*(int*)FieldValue = value;
		else
			return ft_fieldempty;

		break;

	case 7:
		if (props != NULL)
			*(int*)FieldValue = taglib_audioproperties_bitrate(props);
		else
			return ft_fieldempty;

		break;

	case 8:
		if (props != NULL)
			*(int*)FieldValue = taglib_audioproperties_samplerate(props);
		else
			return ft_fieldempty;

		break;

	case 9:
		if (props != NULL)
			*(int*)FieldValue = taglib_audioproperties_channels(props);
		else
			return ft_fieldempty;

		break;

	case 10:
		if (props != NULL)
		{
			int len = taglib_audioproperties_length(props);

			if (len > 0)
			{
				int sec = len  % 60;
				int min = (len - sec) / 60;
				gchar *length = g_strdup_printf("%i:%02i", min, sec);
				g_strlcpy((char*)FieldValue, length, maxlen - 1);
				g_free(length);
			}
		}
		else
			return ft_fieldempty;

		break;

	default:
		return ft_nosuchfield;
	}

	return gFields[FieldIndex].type;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	char buff[MAX_PATH];
	GString *headers = g_string_new(NULL);
	Translate("Artist", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	Translate("Title", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	Translate("Album", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	Translate("No.", buff, MAX_PATH);
	g_string_append(headers, buff);
	g_string_append(headers, "\\n");
	g_strlcpy(ViewHeaders, headers->str, maxlen - 1);
	g_string_free(headers, TRUE);
	g_strlcpy(ViewWidths, "100,0,55,55,40,-10", maxlen - 1);
	g_strlcpy(ViewContents, "[Plugin(FS).artist{}]\\n[Plugin(FS).title{}]\\n[Plugin(FS).album{}]\\n[Plugin(FS).track{}]", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);

	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	snprintf(DefRootName, maxlen - 1, ROOTNAME);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	if (gCfg == NULL)
	{
		gCfg = g_key_file_new();
		gchar *cfgdir = g_path_get_dirname(dps->DefaultIniName);
		gCfgPath = g_strdup_printf("%s/%s", cfgdir, ININAME);
		g_free(cfgdir);
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
	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;

	if (gCfg != NULL)
	{
		g_key_file_free(gCfg);
		g_free(gCfgPath);
	}

	gCfg = NULL;

	g_free(gCommand);
	gCommand = NULL;
	g_free(gTerm);
	gTerm = NULL;
	g_free(gLastFile);
	gLastFile = NULL;

	if (gLastTagFile != NULL)
	{
		taglib_tag_free_strings();
		taglib_file_free(gLastTagFile);
	}

	gLastTagFile = NULL;

}
