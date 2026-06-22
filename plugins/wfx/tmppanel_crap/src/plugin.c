#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <errno.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <json.h>
#include <json_pointer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "wfxplugin.h"
#include "extension.h"

#define ROOTNAME "TmpPanel"
#define BUFSIZE 8192
#define DIRFAKESTAT 16877
#define IS_VFSROOT(X) (X[1] == '\0')
#define EBMLEM_UNKNWN 16
#define EBMLEM_EMPTY 17
#define COLOR_EMPTY 9

#define MessageBox dc_extensions->MessageBox
#define SendDlgMsg dc_extensions->SendDlgMsg
#define CreateComponent dc_extensions->CreateComponent
#define GetProperty dc_extensions->GetProperty
#define SetProperty dc_extensions->SetProperty
#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))

typedef struct
{
	struct json_object *parent;
	struct json_object_iterator iter;
	struct json_object_iterator end;
} VFSDirData;

typedef struct
{
	char *circle;
	char *square;
	int r, g, b;
} RainbowColor;

typedef struct
{
	char *name;
	int type;
	char *attr;
} ContentField;

typedef struct
{
	char *name;
	char *emoji;
} EmblemItem;

static const RainbowColor rainbow[] =
{
	{"🔴", "🟥", 221,  46,  68},
	{"🟠", "🟧", 244, 144,  12},
	{"🟡", "🟨", 253, 203,  88},
	{"🟢", "🟩", 120, 177,  89},
	{"🔵", "🟦",  85, 172, 238},
	{"🟣", "🟪", 170, 142, 214},
	{"🟤", "🟫", 193, 105,  79},
	{"⚫", "⬛",  49,  55,  61},
	{"⚪", "⬜", 230, 231, 232},
};

static const ContentField fields[] =
{
	{"link created",		ft_datetime,					    NULL},
	{"localname",			ft_string,					    NULL},
	{"notes",			ft_string,					    NULL},
	{"descript.ion",		ft_string,					    NULL},
	{"description",			ft_string,			"standard::content-type"},
	{"mode",			ft_numeric_32,				    "unix::mode"},
	{"user",			ft_string,				   "owner::user"},
	{"group",			ft_string,				  "owner::group"},
	{"access",			ft_string,				     "access::*"},
	{"thunar foreground color",	ft_string, "metadata::thunar-highlight-color-foreground"},
	{"thunar background color",	ft_string, "metadata::thunar-highlight-color-background"},
	{"emblems",			ft_string,			     "metadata::emblems"},
};

static const EmblemItem emblems[] =
{
	{"[emblem-checkmark]",	   "✔️"},
	{"[emblem-documents]",	   "📄"},
	{"[emblem-downloads]",	   "📥"},
	{"[emblem-favorite]",	   "⭐"},
	{"[emblem-important]",	   "⚠️"},
	{"[emblem-mail]",	   "📧"},
	{"[emblem-new]",	   "✨"},
	{"[emblem-package]",	   "📦"},
	{"[emblem-photos]",	   "📸"},
	{"[emblem-readonly]",	   "🔒"},
	{"[emblem-shared]",	   "🔗"},
	{"[emblem-synchronizing]", "🔂"},
	{"[emblem-system]",	   "⚙️"},
	{"[emblem-unreadable]",    "🚫"},
	{"[emblem-urgent]",	   "🚨"},
	{"[emblem-web]",	   "🌐"},
};

enum
{
	WFX_JSON_INFO_PATH,
	WFX_JSON_INFO_TIME,
	WFX_JSON_INFO_NOTE,
};

static int plug_id;
static gchar *plug_conf = NULL;
static tLogProc log_msg = NULL;
static gchar *descr_dir = NULL;
static gchar **descr_data = NULL;
static gboolean is_copyin = FALSE;
static gboolean is_rmfile = FALSE;
struct json_object *json_root = NULL;
static tRequestProc show_request = NULL;
static tProgressProc show_progress = NULL;
static tExtensionStartupInfo* dc_extensions = NULL;

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (dc_extensions == NULL)
	{
		dc_extensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(dc_extensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (dc_extensions != NULL)
	{
		free(dc_extensions);
		dc_extensions = NULL;
		free(descr_dir);
		descr_dir = NULL;
		g_strfreev(descr_data);
	}

	if (json_root != NULL)
	{
		json_object_to_file_ext(plug_conf, json_root, JSON_C_TO_STRING_PRETTY);
		json_object_put(json_root);
		json_root = NULL;
	}

	g_free(plug_conf);
}

static void clear_filetime(LPFILETIME ft)
{
	ft->dwHighDateTime = 0xFFFFFFFF;
	ft->dwLowDateTime = 0xFFFFFFFE;
}

static void set_unixtime(LPFILETIME ft, int64_t unixtime)
{
	if (unixtime < 0)
		return;

	int64_t value = unixtime * 10000000 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)value;
	ft->dwHighDateTime = value >> 32;
}

static time_t get_unixtime(LPFILETIME ft)
{
	int64_t result = ft->dwHighDateTime;
	result = (result << 32) | ft->dwLowDateTime;
	result = (result - 116444736000000000) / 10000000;
	return result;
}

static int copy_file(char* src, char* dst)
{
	int ifd, ofd, done = 0;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;

	if (strcmp(src, dst) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (stat(src, &buf) != 0)
		return FS_FILE_READERROR;

	ifd = open(src, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{
		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				result = FS_FILE_WRITEERROR;
				break;
			}

			total += len;

			if (buf.st_size > 0)
			{
				done = total * 100 / buf.st_size;

				if (done > 100)
					done = 100;
			}

			if (show_progress(plug_id, src, dst, done) == 1)
			{
				result = FS_FILE_USERABORT;
				remove(dst);
				break;
			}
		}

		close(ofd);
		chmod(dst, buf.st_mode);
	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);

	return result;
}

static char *get_node_name(char *path)
{
	if (!path)
		return NULL;

	char *pos = strrchr(path, '/');

	if (pos)
		return pos + 1;

	return NULL;
}

static struct json_object *get_json_node_from_path(char *path)
{
	struct json_object *result = NULL;

	if (IS_VFSROOT(path))
		result = json_root;
	else
		json_pointer_get(json_root, path, &result);

	return result;
}

static struct json_object *get_parent_node(char *path)
{
	gchar *root_dir = g_path_get_dirname(path);
	struct json_object *result = get_json_node_from_path(root_dir);
	g_free(root_dir);

	return result;
}

static gboolean json_node_exists(struct json_object *parent, gchar *name)
{
	if (!parent || !name)
		return FALSE;

	struct json_object *found = NULL;
	json_object_object_get_ex(parent, name, &found);
	return (found != NULL);
}

static gboolean json_node_remove(gchar *path)
{
	gboolean result = TRUE;
	struct json_object *parent = get_parent_node(path);
	char *name = get_node_name(path);

	if (json_node_exists(parent, name))
		json_object_object_del(parent, name);

	return result;
}

static void add_file_node(struct json_object *parent, gchar *name, char *path)
{
	struct json_object *array = json_object_new_array();
	json_object_array_put_idx(array, WFX_JSON_INFO_PATH, path ? json_object_new_string(path) : NULL);
	json_object_array_put_idx(array, WFX_JSON_INFO_TIME, json_object_new_int64((int64_t)time(NULL)));
	json_object_object_add(parent, name ? name : ".", array);
}

static void add_dir_node(char *path)
{
	struct json_object *parent = get_parent_node(path);

	if (parent != NULL)
	{
		char *name = get_node_name(path);

		if (!json_node_exists(parent, name))
		{
			json_object_object_add(parent, name, json_object_new_object());
			add_file_node(parent, NULL, NULL);
		}
	}
}

static void build_tree(char *path)
{
	char *pos = strchr(path + 1, '/');

	while (pos)
	{
		*pos = '\0';
		add_dir_node(path);
		*pos = '/';
		pos = strchr(pos + 1, '/');
	}

	add_dir_node(path);
}

static const char *get_localname(char *path)
{
	const char *result = NULL;
	struct json_object *node = get_json_node_from_path(path);

	if (json_object_is_type(node, json_type_array))
		result = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_PATH));

	return result;
}

static gboolean fill_entry(VFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	gboolean result = FALSE;

	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));
	clear_filetime(&FindData->ftCreationTime);
	clear_filetime(&FindData->ftLastAccessTime);
	clear_filetime(&FindData->ftLastWriteTime);
	FindData->dwFileAttributes = FILE_ATTRIBUTE_UNIX_MODE;

	while (!result && !json_object_iter_equal(&dirdata->iter, &dirdata->end))
	{
		const char *name = json_object_iter_peek_name(&dirdata->iter);

		if (strcmp(name, ".") != 0)
		{
			g_strlcpy(FindData->cFileName, name, MAX_PATH);
			struct json_object *value = json_object_iter_peek_value(&dirdata->iter);
			json_type type = json_object_get_type(value);

			if (type == json_type_object)
			{
				FindData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
				FindData->dwReserved0 = DIRFAKESTAT;

				struct json_object *info = NULL;

				if (json_object_object_get_ex(dirdata->parent, ".", &info) && json_object_is_type(info, json_type_array))
				{
					int64_t unixtime = json_object_get_int64(json_object_array_get_idx(info, WFX_JSON_INFO_TIME));
					set_unixtime(&FindData->ftLastWriteTime, unixtime);
				}

				result = TRUE;
			}
			else if (type == json_type_array)
			{
				struct stat buf;
				const char *localname = json_object_get_string(json_object_array_get_idx(value, 0));

				if (lstat(localname, &buf) == 0)
				{
					FindData->nFileSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
					FindData->nFileSizeLow = buf.st_size & 0x00000000FFFFFFFF;

					FindData->dwReserved0 = buf.st_mode;
					set_unixtime(&FindData->ftLastWriteTime, buf.st_mtime);
					set_unixtime(&FindData->ftLastAccessTime, buf.st_atime);
				}
				else
				{
					FindData->nFileSizeHigh = 0xFFFFFFFF;
					FindData->nFileSizeLow = 0xFFFFFFFE;
				}

				result = TRUE;
			}
		}

		json_object_iter_next(&dirdata->iter);
	}

	return result;
}

static char* get_emblem_emoji(gchar *value, int *index)
{
	char* result = "";

	if (!value)
		return result;

	for (gsize i = 0; i < ARRAY_SIZE(emblems); i++)
	{
		if (g_strcmp0(value, emblems[i].name) == 0)
		{
			result = emblems[i].emoji;

			if (index)
				*index = i;
		}
	}

	return result;
}

static int get_pride_color(gchar *value)
{
	int result = -1;

	if (!value)
		return result;

	int r = 0, g = 0, b = 0;
	long min_distance = LONG_MAX;
	sscanf(value, "rgb(%d,%d,%d)", &r, &g, &b);
	g_free(value);

	for (int i = 0; i < ARRAY_SIZE(rainbow); i++)
	{
		long r_distance = r - rainbow[i].r;
		long g_distance = g - rainbow[i].g;
		long b_distance = b - rainbow[i].b;

		// hes fighting and biting and riding on his horse
		long distance = sqrt((r_distance * r_distance) + (g_distance * g_distance) + (b_distance * b_distance));

		if (distance < min_distance)
		{
			min_distance = distance;
			result = i;
		}
	}

	return result;
}

static char* get_color_circle(gchar *value, int *index)
{
	int pos = get_pride_color(value);

	if (index)
		*index = pos;

	if (pos == -1)
		return "";

	return rainbow[pos].circle;
}

static char* get_color_square(gchar *value, int *index)
{
	int pos = get_pride_color(value);

	if (index)
		*index = pos;

	if (pos == -1)
		return "";

	return rainbow[pos].square;
}

static gchar *get_4dos_descr(const char *localname)
{
	if (!localname)
		return NULL;

	gchar *result = NULL;
	gchar *dir = g_path_get_dirname(localname);

	if (!descr_dir || strcmp(descr_dir, dir) != 0)
	{
		gchar *contents = NULL;
		gchar *descr_file = g_strdup_printf("%s/descript.ion", dir);

		if (g_file_get_contents(descr_file, &contents, NULL, NULL))
		{
			g_strfreev(descr_data);
			descr_data = g_strsplit((strncmp(contents, "\xEF\xBB\xBF", 3) == 0) ? contents + 3: contents, "\n", -1);
			g_free(contents);
			g_free(descr_dir);
			descr_dir = dir;
			dir = NULL;
		}
	}

	if (!dir || (descr_dir && strcmp(descr_dir, dir) == 0))
	{
		gchar *name = g_path_get_basename(localname);
		size_t len = strlen(name);

		for (gsize i = 0; descr_data[i] != NULL; i++)
		{
			gchar *line = descr_data[i];

			if (strlen(line) > len + 2 && g_str_has_prefix(line, name) && line[len] == ' ')
			{
				gchar **split = g_strsplit(line + len + 1, "\xC2\xA0", -1);
				result = g_strjoinv("\n", split);
				g_strfreev(split);
				break;
			}
		}

		g_free(name);
	}

	g_free(dir);

	return result;
}

static void add_descr_label(uintptr_t pDlg, const char *localname)
{
	gchar *text = get_4dos_descr(localname);

	if (text)
	{
		CreateComponent(pDlg, "sbProps", "lbl4dosDescr", "TLabel", NULL);
		SetProperty(pDlg, "lbl4dosDescr", "Caption", text, TK_STRING);
		SetProperty(pDlg, "lbl4dosDescr", "Alignment", "taCenter", TK_STRING);
		SetProperty(pDlg, "lbl4dosDescr", "Align", "alTop", TK_STRING);
		SetProperty(pDlg, "lbl4dosDescr", "WordWrap", "False", TK_STRING);
		g_free(text);
	}
}

static void add_prop_label(uintptr_t pDlg, guint index, char *caption, gboolean is_value)
{
	char control[MAX_PATH];
	snprintf(control, sizeof(control), "%s%d", is_value ? "lblValue" : "lblProp", index);
	CreateComponent(pDlg, "pnlProps", control, "TLabel", NULL);
	SetProperty(pDlg, control, "Caption", caption, TK_STRING);

	if (!is_value)
	{
		SetProperty(pDlg, control, "Font.Style", "[fsBold]", TK_STRING);
		SetProperty(pDlg, control, "Font.Style", "[fsBold]", TK_STRING);
	}
}

intptr_t DCPCALL prop_dialog_cb(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		const char *localname = NULL;
		char *filename = (char*)SendDlgMsg(pDlg, NULL, DM_GETDLGDATA, 0, 0);
		char *pos = strrchr(filename, '/');
		SendDlgMsg(pDlg, "edFleName", DM_SETTEXT, (intptr_t)pos + 1, 0);
		struct json_object *node = get_json_node_from_path(filename);

		if (json_object_is_type(node, json_type_array))
		{
			guint64 unixtime = (guint64)json_object_get_int64(json_object_array_get_idx(node, WFX_JSON_INFO_TIME));
			gchar *value = g_date_time_format(g_date_time_new_from_unix_local(unixtime), "%Y-%m-%dT%H:%M:%S");
			add_prop_label(pDlg, 0, "link created", FALSE);
			add_prop_label(pDlg, 0, value, TRUE);
			g_free(value);
			localname = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_PATH));
			SendDlgMsg(pDlg, "edLocalName", DM_SETTEXT, (intptr_t)localname, 0);
			const char *note = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_NOTE));
			SendDlgMsg(pDlg, "Memo", DM_SETTEXT, (intptr_t)note, 0);
		}

		if (!localname)
			return 0;

		add_descr_label(pDlg, localname);
		GFile *gfile = g_file_new_for_path(localname);

		if (gfile)
		{
			GFileInfo *fileinfo = g_file_query_info(gfile, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

			if (fileinfo)
			{
				CreateComponent(pDlg, "sbProps", "lblDescr", "TLabel", NULL);
				const char *content = g_file_info_get_content_type(fileinfo);
				gchar *descr = g_content_type_get_description(content);
				SetProperty(pDlg, "lblDescr", "Caption", descr, TK_STRING);
				SetProperty(pDlg, "lblDescr", "Alignment", "taCenter", TK_STRING);
				SetProperty(pDlg, "lblDescr", "Align", "alTop", TK_STRING);
				g_free(descr);
				gchar **attrs = g_file_info_list_attributes(fileinfo, NULL);
				guint len = g_strv_length(attrs);

				for (guint i = 0; i < len; i++)
				{
					if (g_strcmp0(attrs[i], "standard::type") == 0)
						continue;

					GFileAttributeType type = g_file_info_get_attribute_type(fileinfo, attrs[i]);

					if (type == G_FILE_ATTRIBUTE_TYPE_OBJECT || type == G_FILE_ATTRIBUTE_TYPE_BYTE_STRING)
						continue;

					add_prop_label(pDlg, i + 1, attrs[i], FALSE);
					gchar *value = NULL;

					if (type == G_FILE_ATTRIBUTE_TYPE_UINT64) 
					{
						if (g_str_has_prefix(attrs[i], "time::") && !g_str_has_suffix(attrs[i], "sec"))
						{
							guint64 unixtime = g_file_info_get_attribute_uint64(fileinfo, attrs[i]);
							value = g_date_time_format(g_date_time_new_from_unix_local(unixtime), "%Y-%m-%dT%H:%M:%S");
						}
						else if (strstr(attrs[i], "size"))
							value = g_format_size(g_file_info_get_attribute_uint64(fileinfo, attrs[i]));
						else
							value = g_file_info_get_attribute_as_string(fileinfo, attrs[i]);
					}
					else if (type == G_FILE_ATTRIBUTE_TYPE_UINT32 && g_strcmp0(attrs[i], "unix::mode") == 0)
						value = g_strdup_printf("%o", g_file_info_get_attribute_uint32(fileinfo, attrs[i]));
					else if (g_strcmp0(attrs[i], "metadata::emblems") == 0)
					{
						int pos = -1;
						value = g_file_info_get_attribute_as_string(fileinfo, attrs[i]);
						char *emoji = get_emblem_emoji(value, &pos);

						if (pos != -1)
						{
							g_free(value);
							value = g_strdup(emoji);
							SendDlgMsg(pDlg, "rgEmblems", DM_LISTSETITEMINDEX, (intptr_t)pos, 0);
						}
						else if (value)
							SendDlgMsg(pDlg, "rgEmblems", DM_LISTSETITEMINDEX, EBMLEM_UNKNWN, 0);

					}
					else if (g_strcmp0(attrs[i], "metadata::thunar-highlight-color-foreground") == 0 || g_strcmp0(attrs[i], "metadata::thunar-highlight-color-background") == 0)
					{
						int pos = 9;
						value = g_file_info_get_attribute_as_string(fileinfo, attrs[i]);

						if (strstr(attrs[i], "foreground"))
						{
							add_prop_label(pDlg, i + 1,  get_color_circle(value, &pos), TRUE);
							SendDlgMsg(pDlg, "rgFgColor", DM_LISTSETITEMINDEX, (intptr_t)pos, 0);
						}
						else
						{
							add_prop_label(pDlg, i + 1,  get_color_square(value, &pos), TRUE);
							SendDlgMsg(pDlg, "rgBgColor", DM_LISTSETITEMINDEX, (intptr_t)pos, 0);
						}
						continue;
					}
					else
						value = g_file_info_get_attribute_as_string(fileinfo, attrs[i]);

					add_prop_label(pDlg, i + 1, value, TRUE);
					g_free(value);
				}

				g_strfreev(attrs);
				g_object_unref(fileinfo);
			}

			g_object_unref(gfile);
		}
	}
	else if (Msg == DN_CLICK && strcmp(DlgItemName, "btnApply") == 0)
	{
		
	}
	else if (Msg == DN_CLOSE)
	{
		char *filename = (char*)SendDlgMsg(pDlg, NULL, DM_GETDLGDATA, 0, 0);
		struct json_object *node = get_json_node_from_path(filename);

		if (json_object_is_type(node, json_type_array))
		{
			char *path = (char*)SendDlgMsg(pDlg, "edLocalName", DM_GETTEXT, 0, 0);
			const char *localname = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_PATH));

			if (!localname || strcmp(path, localname) != 0)
				json_object_array_put_idx(node, WFX_JSON_INFO_PATH, path ? json_object_new_string(path) : NULL);

			if (localname)
			{
				GFile *gfile = g_file_new_for_path(localname);
				if (gfile)
				{
					gchar *value = NULL;
					GFileInfo *fileinfo = g_file_query_info(gfile, "metadata::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);

					if (fileinfo)
					{
						int index = (int)SendDlgMsg(pDlg, "rgFgColor", DM_LISTGETITEMINDEX, 0, 0);

						if (index == COLOR_EMPTY)
						{
							value = g_file_info_get_attribute_as_string(fileinfo, "metadata::thunar-highlight-color-foreground");

							if (value)
							{
								g_free(value);
								g_file_set_attribute(gfile, "metadata::thunar-highlight-color-foreground", G_FILE_ATTRIBUTE_TYPE_INVALID, NULL, G_FILE_QUERY_INFO_NONE, NULL, NULL);
							}
						}
						else
						{
							gchar *color = g_strdup_printf("rgb(%d,%d,%d)", rainbow[index].r, rainbow[index].g, rainbow[index].b);
							g_file_set_attribute_string(gfile, "metadata::thunar-highlight-color-foreground", color, G_FILE_QUERY_INFO_NONE, NULL, NULL);
							g_free(color);
						}

						index = (int)SendDlgMsg(pDlg, "rgBgColor", DM_LISTGETITEMINDEX, 0, 0);

						if (index == COLOR_EMPTY)
						{
							value = g_file_info_get_attribute_as_string(fileinfo, "metadata::thunar-highlight-color-background");

							if (value)
							{
								g_free(value);
								g_file_set_attribute(gfile, "metadata::thunar-highlight-color-background", G_FILE_ATTRIBUTE_TYPE_INVALID, NULL, G_FILE_QUERY_INFO_NONE, NULL, NULL);
							}
						}
						else
						{
							gchar *color = g_strdup_printf("rgb(%d,%d,%d)", rainbow[index].r, rainbow[index].g, rainbow[index].b);
							g_file_set_attribute_string(gfile, "metadata::thunar-highlight-color-background", color, G_FILE_QUERY_INFO_NONE, NULL, NULL);
							g_free(color);
						}

						index = (int)SendDlgMsg(pDlg, "rgEmblems", DM_LISTGETITEMINDEX, 0, 0);

						if (index != EBMLEM_UNKNWN)
						{
							if (index == EBMLEM_EMPTY)
							{
								value = g_file_info_get_attribute_as_string(fileinfo, "metadata::emblems");

								if (value)
								{
									g_free(value);
									g_file_set_attribute(gfile, "metadata::emblems", G_FILE_ATTRIBUTE_TYPE_INVALID, NULL, G_FILE_QUERY_INFO_NONE, NULL, NULL);
								}
							}
							else
								g_file_set_attribute_string(gfile, "metadata::emblems", emblems[index].name, G_FILE_QUERY_INFO_NONE, NULL, NULL);
						}

						g_object_unref(fileinfo);
					}

					g_object_unref(gfile);
				}
			}

			char *note = (char*)SendDlgMsg(pDlg, "Memo", DM_GETTEXT, 0, 0);
			json_object_array_put_idx(node, WFX_JSON_INFO_NOTE, note ? json_object_new_string(note) : NULL);
		}
	}

	return 0;
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	if (!plug_conf)
	{
		gchar *cfg_dir = g_path_get_dirname(dps->DefaultIniName);
		plug_conf = g_strdup_printf("%s/tmppanel_crap.ini", cfg_dir);

		if (g_file_test(plug_conf, G_FILE_TEST_EXISTS))
		{
			json_root = json_object_new_object();
			GKeyFile *cfg = g_key_file_new();
			struct json_object *parent = json_root;

			if (g_key_file_load_from_file(cfg, plug_conf, G_KEY_FILE_KEEP_COMMENTS, NULL))
			{
				gchar **groups = g_key_file_get_groups(cfg, NULL);

				for (char **dir = groups; *dir != NULL; dir++)
				{
					if (strcmp(*dir, "/") != 0)
					{
						build_tree(*dir);
						parent = get_json_node_from_path(*dir);
					}

					gchar **items = g_key_file_get_keys(cfg, *dir, NULL, NULL);

					for (gsize i = 0; items[i] != NULL; i++)
					{
						gchar *localname = g_key_file_get_string(cfg, *dir, items[i], NULL);

						if (strcmp(localname, "folder") != 0)
							add_file_node(parent, items[i], localname);

						g_free(localname);
					}

					g_strfreev(items);
				}

				g_strfreev(groups);
			}

			g_key_file_free(cfg);
			gchar *bakup = g_strdup_printf("%s/tmppanel_crap.ini.bak", cfg_dir);
			rename(plug_conf, bakup);
			g_free(bakup);
		}

		g_free(plug_conf);
		plug_conf = g_strdup_printf("%s/" PLUGDIR ".json", cfg_dir);
		g_free(cfg_dir);

		if (!json_root)
			json_root = json_object_from_file(plug_conf);

		if (!json_root)
			json_root = json_object_new_object();
	}
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	plug_id = PluginNr;
	show_progress = pProgressProc;
	log_msg = pLogProc;
	show_request = pRequestProc;

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	struct json_object *node = get_json_node_from_path(Path);

	if (node != NULL)
	{
		VFSDirData *dirdata = g_new0(VFSDirData, 1);
		dirdata->iter = json_object_iter_begin(node);
		dirdata->end = json_object_iter_end(node);
		dirdata->parent = node;

		if (fill_entry(dirdata, FindData))
			return (HANDLE)dirdata;
		else
			g_free(dirdata);
	}

	return (HANDLE)(-1);
}


BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	VFSDirData *dirdata = (VFSDirData*)Hdl;
	return fill_entry(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	VFSDirData *dirdata = (VFSDirData*)Hdl;
	g_free(dirdata);
	return 0;
}

BOOL DCPCALL FsLinksToLocalFiles(void)
{
	return TRUE;
}

BOOL DCPCALL FsGetLocalName(char* RemoteName, int maxlen)
{
	const char *localname = get_localname(RemoteName);

	if (localname)
	{
		g_strlcpy(RemoteName, localname, maxlen);
		return TRUE;
	}

	return FALSE;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	if (show_progress(plug_id, RemoteName, LocalName, 0) != 0)
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	return copy_file((char*)get_localname(RemoteName), LocalName);
}

BOOL DCPCALL FsMkDir(char* Path)
{
	build_tree(Path);
	return TRUE;
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	if (show_progress(plug_id, LocalName, RemoteName, 0) != 0)
		return FS_FILE_USERABORT;

	int result = FS_FILE_OK;

	struct json_object *parent = get_parent_node(RemoteName);

	if (parent != NULL)
	{
		char *name = get_node_name(RemoteName);

		if (CopyFlags == 0 && json_node_exists(parent, name))
			return FS_FILE_EXISTS;

		add_file_node(parent, name, LocalName);
	}

	show_progress(plug_id, LocalName, RemoteName, 100);
	return result;
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct* ri)
{
	if (show_progress(plug_id, OldName, NewName, 0) != 0)
		return FS_FILE_USERABORT;

	int result = FS_FILE_OK;

	struct json_object *parent = get_parent_node(NewName);
	char *name = get_node_name(NewName);

	if (strcmp(name, ".") == 0)
		result = FS_FILE_NOTSUPPORTED;

	if (!OverWrite && json_node_exists(parent, name))
		result = FS_FILE_EXISTS;

	if (result == FS_FILE_OK)
	{
		struct json_object *node = get_json_node_from_path(OldName);
		json_object_get(node);
		json_node_remove(OldName);
		json_object_object_add(parent, name, node);
		show_progress(plug_id, OldName, NewName, 100);
	}

	return result;
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	return json_node_remove(RemoteName);
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	if (is_rmfile)
	{
		const char *localname = get_localname(RemoteName);

		if (remove(localname) != 0)
			return FALSE;
	}

	return json_node_remove(RemoteName);
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;
	const char *localname = get_localname(RemoteName);

	if (!localname || IS_VFSROOT(RemoteName) || strcmp(RemoteName, "/..") == 0)
		return result;

	if (strcmp(Verb, "open") == 0)
	{
		struct stat buf;
		gchar *command = NULL;

		if (stat(localname, &buf) == 0)
		{
			if (buf.st_mode & S_IXUSR)
				command = g_shell_quote(localname);
			else
			{
				gchar *quoted = g_shell_quote(localname);
				command = g_strdup_printf("xdg-open %s", quoted);
				g_free(quoted);
			}

			g_spawn_command_line_async(command, NULL);
			g_free(command);
			result = FS_FILE_OK;
		}
	}
	else if (strcmp(Verb, "chmod") == 0)
	{
		gint mode = g_ascii_strtoll(Verb + 6, 0, 8);

		if (chmod(localname, mode) == -1)
		{
			int errsv = errno;
			show_request(plug_id, RT_MsgOK, NULL, strerror(errsv), NULL, 0);
		}
		else
			result = FS_FILE_OK;
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		gchar *lfm = g_strdup_printf("%s/dialog.lfm", dc_extensions->PluginDir);
		dc_extensions->DialogBoxParam((void*)lfm, (unsigned long)strlen(lfm), prop_dialog_cb, DB_FILENAME, (void*)RemoteName, NULL);
		g_free(lfm);
		result = FS_FILE_OK;
	}

	return result;
}

BOOL DCPCALL FsSetTime(char* RemoteName, FILETIME *CreationTime, FILETIME *LastAccessTime, FILETIME *LastWriteTime)
{
	if (is_copyin)
		return TRUE;

	struct stat buf;
	struct utimbuf ubuf;
	gboolean result = FALSE;

	if (LastAccessTime != NULL || LastWriteTime != NULL)
	{
		const char *localname = get_localname(RemoteName);

		if (lstat(localname, &buf) == 0)
		{
			if (LastAccessTime != NULL)
				ubuf.actime = get_unixtime(LastAccessTime);
			else
				ubuf.actime = buf.st_atime;

			if (LastWriteTime != NULL)
				ubuf.modtime = get_unixtime(LastWriteTime);
			else
				ubuf.modtime = buf.st_mtime;

			result = (utime(localname, &ubuf) == 0);
		}
	}

	return result;
}

void DCPCALL FsStatusInfo(char* RemoteDir, int InfoStartEnd, int InfoOperation)
{

	if (InfoOperation == FS_STATUS_OP_PUT_SINGLE)
		is_copyin = (InfoStartEnd == FS_STATUS_START);
	else if (InfoOperation == FS_STATUS_OP_DELETE && InfoStartEnd == FS_STATUS_START)
		is_rmfile = (MessageBox("delete the local file itself as well?", ROOTNAME, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == ID_YES);
}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= ARRAY_SIZE(fields))
		return ft_nomorefields;

	Units[0] = '\0';
	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = fields[FieldIndex].type;

	struct json_object *node = get_json_node_from_path(FileName);

	if (!json_object_is_type(node, json_type_array))
		return ft_fileerror;

	if (fields[FieldIndex].attr == NULL)
	{
		const char *text = NULL;

		switch (FieldIndex)
		{
		case 0:
			int64_t unixtime = json_object_get_int64(json_object_array_get_idx(node, WFX_JSON_INFO_TIME));
			unixtime = (unixtime * 10000000) + 0x019DB1DED53E8000;
			((LPFILETIME)FieldValue)->dwLowDateTime = (DWORD)unixtime;
			((LPFILETIME)FieldValue)->dwHighDateTime = unixtime >> 32;
			break;

		case 1:
			text = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_PATH));

			if (!text)
				result = ft_fieldempty;
			else
				g_strlcpy((char*)FieldValue, text, maxlen - 1);

			break;

		case 2:
			text = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_NOTE));

			if (!text)
				result = ft_fieldempty;
			else
				g_strlcpy((char*)FieldValue, text, maxlen - 1);

			break;

		case 3:
			text = get_4dos_descr(json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_PATH)));

			if (!text)
				result = ft_fieldempty;
			else
				g_strlcpy((char*)FieldValue, text, maxlen - 1);

			break;

		default:
			return ft_fieldempty;
		}
	}
	else
	{
		const char *localname = json_object_get_string(json_object_array_get_idx(node, WFX_JSON_INFO_PATH));

		if (!localname)
			return ft_fileerror;

		GFile *gfile = g_file_new_for_path(localname);

		if (gfile)
		{
			gchar *string = NULL;
			GFileInfo *fileinfo = g_file_query_info(gfile, fields[FieldIndex].attr, G_FILE_QUERY_INFO_NONE, NULL, NULL);

			if (fileinfo)
			{
				if (FieldIndex == 4)
				{
					const char *content = g_file_info_get_content_type(fileinfo);
					string = g_content_type_get_description(content);

					if (!string)
						result = ft_fieldempty;
					else
						g_strlcpy((char*)FieldValue, string, maxlen - 1);

					g_free(string);
				}
				else if (FieldIndex == 5)
				{
					string = g_strdup_printf("%o", g_file_info_get_attribute_uint32(fileinfo, fields[FieldIndex].attr) & 0777);
					sscanf(string, "%d", (int*)FieldValue);
					g_free(string);
				}
				else if (FieldIndex == 8)
				{
					char access_str[3] = "---";

					if (g_file_info_get_attribute_boolean(fileinfo, "access::can-read"))
						access_str[0] = 'r';

					if (g_file_info_get_attribute_boolean(fileinfo, "access::can-write"))
						access_str[1] = 'w';

					if (g_file_info_get_attribute_boolean(fileinfo, "access::can-execute"))
						access_str[2] = 'x';

					g_strlcpy((char*)FieldValue, access_str, maxlen - 1);
				}
				else if (FieldIndex == 11)
				{
					string = g_file_info_get_attribute_as_string(fileinfo, fields[FieldIndex].attr);

					if (!string)
						result = ft_fieldempty;
					else
					{
						g_strlcpy((char*)FieldValue, get_emblem_emoji(string, NULL), maxlen - 1);
						g_free(string);
					}
				}
				else if (FieldIndex > 8)
				{
					string = g_file_info_get_attribute_as_string(fileinfo, fields[FieldIndex].attr);
					g_strlcpy((char*)FieldValue, (FieldIndex == 9) ? get_color_circle(string, NULL) : get_color_square(string, NULL), maxlen - 1);
				}
				else
				{
					string = g_file_info_get_attribute_as_string(fileinfo, fields[FieldIndex].attr);

					if (!string)
						result = ft_fieldempty;
					else
						g_strlcpy((char*)FieldValue, string, maxlen - 1);

					g_free(string);
				}

				g_object_unref(fileinfo);
			}
			else
				result = ft_fileerror;

			g_object_unref(gfile);
		}
		else
			return ft_fileerror;
	}

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	g_strlcpy(ViewContents, "[DC().GETFILESIZE{}]\\n[DC().GETFILETIME{}]\\n[Plugin(FS).access{}] [Plugin(FS).emblems{}] [Plugin(FS).thunar background color{}] [Plugin(FS).thunar foreground color{}]", maxlen - 1);
	g_strlcpy(ViewHeaders, "Size\\nDate\\nInfo", maxlen - 1);
	g_strlcpy(ViewWidths, "100,15,-25,30,30", maxlen - 1);
	g_strlcpy(ViewOptions, "-1|0", maxlen - 1);
	return TRUE;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, ROOTNAME, maxlen - 1);
}
