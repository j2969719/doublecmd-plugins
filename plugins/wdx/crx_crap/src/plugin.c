#include <glib.h>
#include <limits.h>
#include <archive.h>
#include <archive_entry.h>
#include <json-glib/json-glib.h>
#include <string.h>
#include "wdxplugin.h"

typedef struct sfield
{
	char *name;
	char *path;
	char *value;
} tfield;

#define FIELDCOUNT (sizeof(gFields)/sizeof(tfield))

tfield gFields[] =
{
	{"author",					  "$.author", NULL},
	{"name",					    "$.name", NULL},
	{"short_name",				      "$.short_name", NULL},
	{"gecko.id",				      "$.*.gecko.id", NULL},
	{"description",				     "$.description", NULL},
	{"permissions",				     "$.permissions", NULL},
	{"version_name",			    "$.version_name", NULL},
	{"homepage_url",			    "$.homepage_url", NULL},
	{"minimum_chrome_version",	  "$.minimum_chrome_version", NULL},
	{"gecko.strict_min_version",  "$.*.gecko.strict_min_version", NULL},
	{"web_accessible_resources",	"$.web_accessible_resources", NULL},
	{"update_url",				      "$.update_url", NULL},
	{"incognito",				       "$.incognito", NULL},

};

char gLastFile[PATH_MAX];
gboolean gLastFileIsOK = FALSE;

static gchar* ArrayToString(JsonNode *ret)
{
	gchar *tmp = NULL;
	gchar *result = NULL;

	JsonArray *arr = json_node_get_array(ret);
	GList *list = json_array_get_elements(arr);

	for (GList *l = list; l != NULL; l = l->next)
	{

		JsonNode *node = l->data;
		JsonNodeType type = json_node_get_node_type(node);

		if (type == JSON_NODE_VALUE)
		{
			if (json_node_get_value_type(node) == G_TYPE_STRING)
				tmp = json_node_dup_string(node);
			else if (json_node_get_value_type(node) == G_TYPE_INT64)
			{
				tmp = g_strdup_printf("%ld", (long)json_node_get_int(node));
			}
			else if (json_node_get_value_type(node) == G_TYPE_DOUBLE)
			{
				tmp = g_strdup_printf("%f", json_node_get_double(node));
			}
			else if (json_node_get_value_type(node) == G_TYPE_BOOLEAN)
			{
				tmp = g_strdup_printf("%s", json_node_get_boolean(node) ? "True" : "False");
			}
		}
		else if (type == JSON_NODE_ARRAY)
			tmp = ArrayToString(node);

		if (!result)
			result = tmp;
		else
		{
			gchar *prev = result;
			result = g_strdup_printf("%s\n%s", prev, tmp);
			g_free(prev);
			g_free(tmp);
		}
	}

	g_list_free(list);

	return result;
}

static gchar* GetValueStringFromPath(JsonNode *root, const char *JsonPath)
{
	gchar *result = NULL;
	GError *err = NULL;

	JsonNode *node = json_path_query(JsonPath, root, &err);

	if (err)
		g_print("%s: %s\n", PLUGNAME, err->message);
	else
	{
		result = ArrayToString(node);
		json_node_unref(node);
	}

	if (err)
		g_error_free(err);

	return result;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= FIELDCOUNT)
		return ft_nomorefields;

	g_strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);
	Units[0] = '\0';

	return ft_string;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	int r;
	struct stat buf;
	struct archive *a;
	GError *err = NULL;
	struct archive_entry *entry;

	if (stat(FileName, &buf) != 0 || !S_ISREG(buf.st_mode))
		return ft_fileerror;

	if (g_strcmp0(FileName, gLastFile) != 0)
	{
		for (int i = 0; i < FIELDCOUNT; i++)
		{
			g_free(gFields[i].value);
			gFields[i].value = NULL;
		}

		gLastFileIsOK = FALSE;

		g_strlcpy(gLastFile, FileName, PATH_MAX);
		a = archive_read_new();
		archive_read_support_format_zip(a);
		r = archive_read_open_filename(a, gLastFile, 10240);

		if (r == ARCHIVE_OK)
		{
			while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK || r == ARCHIVE_WARN)
			{
				if (strcmp(archive_entry_pathname(entry), "manifest.json") == 0)
				{
					size_t size = (size_t)archive_entry_size(entry);
					gchar *json_data = g_malloc((gsize)size);
					la_ssize_t ret = archive_read_data(a, json_data, size);

					if (ret > 0)
					{
						json_data[size] = '\0';
						JsonNode *root = json_from_string(json_data, &err);

						if (err)
						{
							g_print("%s: %s\n", PLUGNAME, err->message);
							g_error_free(err);
						}
						else
						{
							for (int i = 0; i < FIELDCOUNT; i++)
								gFields[i].value = GetValueStringFromPath(root, gFields[i].path);

							gLastFileIsOK = TRUE;
							json_node_free(root);
						}

						g_free(json_data);
					}

					break;
				}
			}
		}

		archive_read_close(a);
		archive_read_free(a);

	}

	if (!gLastFileIsOK)
		return ft_fileerror;

	if (!gFields[FieldIndex].value)
		return ft_fieldempty;

	g_strlcpy((char*)FieldValue, gFields[FieldIndex].value, maxlen - 1);

	return ft_string;
}

void DCPCALL ContentPluginUnloading(void)
{
	for (int i = 0; i < FIELDCOUNT; i++)
		g_free(gFields[i].value);
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, DETECT_STRING, maxlen - 1);
}
