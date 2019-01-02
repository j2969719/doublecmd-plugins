#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <magic.h>
#include <string.h>
#include "wdxplugin.h"

static gchar *buf1;
static gsize pos;
static gchar cfg_path[PATH_MAX];
static gchar plug_path[PATH_MAX];
const  gchar* cfg_file = "textsearch.ini";

gchar *get_file_ext(const gchar *Filename)
{
	if (g_file_test(Filename, G_FILE_TEST_IS_DIR))
		return NULL;

	gchar *basename, *result, *tmpval;

	basename = g_path_get_basename(Filename);
	result = g_strrstr(basename, ".");

	if (result)
	{
		if (g_strcmp0(result, basename) != 0)
		{
			tmpval = g_strdup_printf("%s", result + 1);
			result = g_ascii_strdown(tmpval, -1);
			g_free(tmpval);
		}
		else
			result = NULL;
	}

	g_free(basename);

	return result;
}

gchar *get_mime_type(const gchar *Filename)
{
	magic_t magic_cookie;
	gchar *result;

	magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

	if (magic_load(magic_cookie, NULL) != 0)
	{
		magic_close(magic_cookie);
		return NULL;
	}

	result = g_strdup(magic_file(magic_cookie, Filename));
	magic_close(magic_cookie);
	return result;
}

static gchar *cfg_get_frmt_str(GKeyFile *Cfg, const gchar *Group)
{
	gchar *result, *cfg_value, *tmp;
	cfg_value = g_key_file_get_string(Cfg, Group, "script", NULL);

	if (!cfg_value)
	{
		result = g_key_file_get_string(Cfg, Group, "command", NULL);

		if ((!result) || (!g_strrstr(result, "%s")))
			result = NULL;
	}
	else
	{
		tmp = g_strdup_printf("%s/%s", plug_path, cfg_value);

		if (g_file_test(tmp, G_FILE_TEST_EXISTS))
			result = g_strconcat(g_shell_quote(tmp), " %s", NULL);
		else if (g_file_test(cfg_value, G_FILE_TEST_EXISTS))
			result = g_strconcat(cfg_value, " %s", NULL);
		else
			result = NULL;
	}

	return result;
}

gchar *cfg_find_value(GKeyFile *Cfg, const gchar *Group)
{
	gchar *result, *redirect;
	redirect = g_key_file_get_string(Cfg, Group, "redirect", NULL);

	if (redirect)
		result = cfg_get_frmt_str(Cfg, redirect);
	else
		result = cfg_get_frmt_str(Cfg, Group);

	g_free(redirect);
	return result;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		g_strlcpy(FieldName, "Text", maxlen - 1);
		return ft_fulltext;
	}
	else
		return ft_nomorefields;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (FieldIndex == 0)
	{

		if (UnitIndex == 0)
		{

			GKeyFile *cfg;
			GError *err = NULL;
			gchar *file_ext, *mime_type;
			gchar *command;
			gchar *frmt_str = NULL;

			mime_type = get_mime_type(FileName);
			file_ext = get_file_ext(FileName);

			cfg = g_key_file_new();

			if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
			{
				g_print("%s: %s\n", cfg_path, (err)->message);
				return ft_fieldempty;
			}
			else
			{
				if (file_ext)
					frmt_str = cfg_find_value(cfg, file_ext);

				if (mime_type && (!file_ext || !frmt_str))
					frmt_str = cfg_find_value(cfg, mime_type);
			}

			g_key_file_free(cfg);

			if (err)
				g_error_free(err);

			command =  g_strdup_printf(frmt_str, g_shell_quote(FileName));

			if (!g_spawn_command_line_sync(command, &buf1, NULL, NULL, NULL))
				return ft_fieldempty;

			if (buf1 != NULL && buf1 != "")
			{
				buf1[strlen(buf1) + 1] += '\0';
				g_strlcpy(FieldValue, buf1, maxlen - 1);
				pos = maxlen - 2;
			}
			else
				return ft_fieldempty;
		}
		else if (UnitIndex == -1)
		{
			buf1 = NULL;
			pos = 0;
			return ft_fieldempty;
		}
		else
		{
			if (strlen(buf1 + pos) > 0)
			{
				g_strlcpy((char*)FieldValue, buf1 + pos, maxlen - 1);
				pos += maxlen - 2;
			}
			else
				return ft_fieldempty;
		}

		return ft_fulltext;
	}

	return ft_nosuchfield;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(cfg_path, &dlinfo) != 0)
	{
		strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
		strncpy(plug_path, g_path_get_dirname(cfg_path), PATH_MAX);
		char *pos = strrchr(cfg_path, '/');

		if (pos)
			strcpy(pos + 1, cfg_file);
	}
}
