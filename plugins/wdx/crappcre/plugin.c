#define _GNU_SOURCE
#include <glib.h>
#include <dlfcn.h>
#include <string.h>
#include "wdxplugin.h"

#define _plgname "crappcre.wdx"
#define _detectstring ""
#define _units ""

typedef struct _field
{
	gchar *name;
	gint type;
	gchar *pattern;
	gchar *separator;
	gint match_num;
	gint match_count;
	gboolean filename;
	GRegexCompileFlags compile_options;
	GRegexMatchFlags match_options;
} FIELD;

#define fieldlimit 100
FIELD fields[] = {};
gsize fieldcount;

static gchar cfg_path[PATH_MAX];
const gchar* cfg_file = "crappcre.ini";


void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	GKeyFile *cfg;
	GError *err = NULL;
	gchar **sections;
	gchar *strval;
	gboolean bval;
	gsize index;

	cfg = g_key_file_new();

	strncpy(cfg_path, dps->DefaultIniName, PATH_MAX);
	char *pos = strrchr(cfg_path, '/');

	if (pos)
		strcpy(pos + 1, cfg_file);

	if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{

		memset(&dlinfo, 0, sizeof(dlinfo));



		if (dladdr(cfg_path, &dlinfo) != 0)
		{
			strncpy(cfg_path, dlinfo.dli_fname, PATH_MAX);
			char *pos = strrchr(cfg_path, '/');

			if (pos)
				strcpy(pos + 1, cfg_file);
		}

		if (!g_key_file_load_from_file(cfg, cfg_path, G_KEY_FILE_KEEP_COMMENTS, &err))
			g_print("%s (%s): %s\n", _plgname, cfg_path, (err)->message);
	}

	if (err)
		g_error_free(err);

	if (cfg)
	{
		sections = g_key_file_get_groups(cfg, &fieldcount);

		for (index = 0; index < fieldcount; index++)
		{
			fields[index].compile_options = 0;
			fields[index].match_options = 0;
			fields[index].type = ft_string;

			strval = g_key_file_get_string(cfg, sections[index], "Name", NULL);

			if (strval)
				fields[index].name = strval;
			else
				fields[index].name = "";

			strval = g_key_file_get_string(cfg, sections[index], "Type", NULL);

			if (strval)
				if (g_strcmp0(strval, "count") == 0)
					fields[index].type = ft_numeric_32;
				else if (g_strcmp0(strval, "exist") == 0)
					fields[index].type = ft_boolean;

			strval = g_key_file_get_string(cfg, sections[index], "Pattern", NULL);

			if (strval)
				fields[index].pattern = strval;
			else
				fields[index].pattern = "";

			strval = g_key_file_get_string(cfg, sections[index], "Separator", NULL);

			if (strval)
				fields[index].separator = strval;
			else
				fields[index].separator = " ";

			fields[index].match_num = g_key_file_get_integer(cfg, sections[index], "MatchNum", NULL);
			fields[index].match_count = g_key_file_get_integer(cfg, sections[index], "MatchCount", NULL);

			bval = g_key_file_get_boolean(cfg, sections[index], "Filename", NULL);

			if (bval)
				fields[index].filename = bval;


			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_CASELESS", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_CASELESS;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MULTILINE", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_MULTILINE;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_DOTALL", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_DOTALL;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_EXTENDED", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_EXTENDED;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_ANCHORED", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_ANCHORED;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_DOLLAR_ENDONLY", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_DOLLAR_ENDONLY;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_UNGREEDY", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_UNGREEDY;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_RAW", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_RAW;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_NO_AUTO_CAPTURE", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_NO_AUTO_CAPTURE;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_OPTIMIZE", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_OPTIMIZE;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_FIRSTLINE", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_FIRSTLINE;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_DUPNAMES", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_DUPNAMES;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_NEWLINE_CR", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_NEWLINE_CR;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_NEWLINE_LF", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_NEWLINE_LF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_NEWLINE_CRLF", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_NEWLINE_CRLF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_NEWLINE_ANYCRLF", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_NEWLINE_ANYCRLF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_BSR_ANYCRLF", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_BSR_ANYCRLF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_JAVASCRIPT_COMPAT", NULL);

			if (bval)
				fields[index].compile_options |= G_REGEX_JAVASCRIPT_COMPAT;


			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_ANCHORED", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_ANCHORED;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NOTBOL", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NOTBOL;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NOTEOL", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NOTEOL;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NOTEMPTY", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NOTEMPTY;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_PARTIAL", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_PARTIAL;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NEWLINE_CR", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NEWLINE_CR;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NEWLINE_LF", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NEWLINE_LF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NEWLINE_CRLF", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NEWLINE_CRLF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NEWLINE_ANY", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NEWLINE_ANY;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NEWLINE_ANYCRLF", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NEWLINE_ANYCRLF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_BSR_ANYCRLF", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_BSR_ANYCRLF;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_BSR_ANY", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_BSR_ANY;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_PARTIAL_SOFT", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_PARTIAL_SOFT;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_PARTIAL_HARD", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_PARTIAL_HARD;

			bval = g_key_file_get_boolean(cfg, sections[index], "G_REGEX_MATCH_NOTEMPTY_ATSTART", NULL);

			if (bval)
				fields[index].match_options |= G_REGEX_MATCH_NOTEMPTY_ATSTART;
		}

		g_key_file_free(cfg);
	}
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	g_strlcpy(Units, _units, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	gchar *target, *result = "";
	gint count = 0;
	GRegex *regex;
	GMatchInfo *match_info;
	GError *err = NULL;
	gboolean vempty = FALSE;

	if (!g_file_get_contents(FileName, &target, NULL, NULL) && (fields[FieldIndex].filename != TRUE))
		return ft_fileerror;
	else if (fields[FieldIndex].filename == TRUE)
		target = FileName;

	regex = g_regex_new(fields[FieldIndex].pattern, fields[FieldIndex].compile_options,
	                    fields[FieldIndex].match_options, &err);
	//g_print("%s\n", fields[FieldIndex].pattern);

	if (err)
	{
		g_print("%s (%s, %s): %s\n", _plgname, FileName, fields[FieldIndex].pattern, (err)->message);
		g_error_free(err);
	}

	if (regex)
	{
		g_regex_match(regex, target, fields[FieldIndex].match_options, &match_info);

		while (g_match_info_matches(match_info))
		{
			gchar *fetch = g_match_info_fetch(match_info, fields[FieldIndex].match_num);

			if (fields[FieldIndex].match_count < 1)
			{
				if (result == "")
					result = g_strdup(fetch);
				else
					result = g_strdup_printf("%s%s%s", result, fields[FieldIndex].separator, fetch);
			}

			count++;

			if (count == fields[FieldIndex].match_count)
			{
				result = g_strdup(fetch);
				break;
			}

			g_match_info_next(match_info, NULL);
			g_free(fetch);
		}

		switch (fields[FieldIndex].type)
		{
		case ft_numeric_32:
			*(int*)FieldValue = count;
			break;

		case ft_boolean:
			if (result == "")
				*(int*)FieldValue = 0;
			else
				*(int*)FieldValue = 1;

			break;

		case ft_string:

			if (result)
				g_strlcpy((char*)FieldValue, result, maxlen - 1);
			else
				vempty = TRUE;

			break;

		default:
			vempty = TRUE;
			break;
		}

		g_match_info_free(match_info);
	}
	else
		return ft_fileerror;


	g_regex_unref(regex);

	if (vempty)
		return ft_fieldempty;
	else
		return fields[FieldIndex].type;
}
