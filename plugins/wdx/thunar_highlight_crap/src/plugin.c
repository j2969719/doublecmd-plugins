#include <glib.h>
#include <stdio.h>
#include <gio/gio.h>
#include <math.h>
#include "string.h"
#include "wdxplugin.h"

#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))

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
	{"foreground color",	ft_string, "metadata::thunar-highlight-color-foreground"},
	{"background color",	ft_string, "metadata::thunar-highlight-color-background"},
//	{"emblems",		ft_multiplechoice,		     "metadata::emblems"},
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
		long dr = r - rainbow[i].r;
		long dg = g - rainbow[i].g;
		long db = b - rainbow[i].b;
		long distance = sqrt((dr * dr) + (dg * dg) + (db * db));

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

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= ARRAY_SIZE(fields))
		return ft_nomorefields;

	Units[0] = '\0';

	if (fields[FieldIndex].type == ft_multiplechoice && ARRAY_SIZE(emblems) * 3 < maxlen)
	{
		for (int i = 0; i < ARRAY_SIZE(emblems); i++)
		{
			if (i != 0)
				strcat(Units, "|");

			strcat(Units, emblems[i].emoji);
		}
	}

	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int result = fields[FieldIndex].type;

	GFile *gfile = g_file_new_for_path(FileName);

	if (gfile)
	{
		GFileInfo *fileinfo = g_file_query_info(gfile, fields[FieldIndex].attr, G_FILE_QUERY_INFO_NONE, NULL, NULL);

		if (fileinfo)
		{
			char *shiet = NULL;
			gchar *string = g_file_info_get_attribute_as_string(fileinfo, fields[FieldIndex].attr);

			if (FieldIndex == 0)
				shiet = get_color_circle(string, NULL);
			else if (FieldIndex == 1)
				shiet = get_color_square(string, NULL);
			else if (FieldIndex == 2)
				shiet = get_emblem_emoji(string, NULL);

			if (!shiet)
				result = ft_fieldempty;
			else
				g_strlcpy((char*)FieldValue, shiet, maxlen - 1);

			g_object_unref(fileinfo);
		}
		else
			result = ft_fileerror;

			g_object_unref(gfile);
	}
	else
		return ft_fileerror;

	return result;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen - 1);
	return 0;
}
