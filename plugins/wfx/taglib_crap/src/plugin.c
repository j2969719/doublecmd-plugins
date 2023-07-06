#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <tag_c.h>
#include <utime.h>
#include <errno.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((int64_t)(a)*(int64_t)(b))
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox

#define ROOTNAME "Taglib"
#define ININAME "j2969719_wfx.ini"
#define MAXINIITEMS 256
#define BUFSIZE 8192
#define TEMPLATEDIR "wfx_"ROOTNAME "_XXXXXX"
#define TMPLSTNAME "filelist.lst"


#define RE_FLAGS_COMPILE G_REGEX_DUPNAMES
#define RE_FLAGS_MATCH   0
#define CHECK_NAMED(N) (strstr(expr, "(?'" N "'") != NULL || strstr(expr, "(?<" N ">") || strstr(expr, "(?P<" N ">"))

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

enum
{
	WFX_ACT_SET,
	WFX_ACT_FILL,
	WFX_ACT_CONV,
	WFX_ACT_COPY,
	WFX_ACTS
};

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
gchar *gTempDir = NULL;
FILE *gTempFile = NULL;
int gAction = -1;


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

static gchar *ReplaceString(gchar *text, gchar *str, gchar *repl, gboolean quote)
{
	gchar *result = NULL;

	if (!str || !repl || !text)
		return result;

	gchar **split = g_strsplit(text, str, -1);
	g_free(text);

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

static int AddToFileList(char *FileName)
{
	if (gTempFile)
	{
		fprintf(gTempFile, "%s\n", FileName);
		return FS_FILE_OK;
	}

	return FS_FILE_WRITEERROR;
}

static gchar *BuildCommand(gchar *Template, char *InputFileName, char *OutputFileName, gchar *Term)
{
	if (!Template)
		return NULL;

	gchar *result = ReplaceString(g_strdup(Template), "{infile}", InputFileName, TRUE);
	result = ReplaceString(result, "{outfilenoext.ext}", OutputFileName, TRUE);

	if (Term && g_strrstr(Term, "\"{command}\""))
	{
		gchar *command = ReplaceString(result, "\"", "\\\"", FALSE);
		result = ReplaceString(g_strdup(Term), "{command}", command, FALSE);
		g_free(command);
	}

	return result;
}

static int ConvertFile(char *InputFileName, char *TargetPath, gboolean OverwriteCheck)
{
	int status = 0;
	GPid pid;
	gchar **argv;
	GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
	GError *err = NULL;
	char str[MAX_PATH];
	int result = FS_FILE_OK;

	if (gProgressProc(gPluginNr, InputFileName, TargetPath, 0))
		return FS_FILE_USERABORT;

	if (!gCommand || gCommand[0] == '\0' || gOutExt[0] == '\0')
		return FS_FILE_USERABORT;

	gchar *outdir = g_path_get_dirname(TargetPath);
	gchar *file = g_path_get_basename(TargetPath);
	char *pos = strrchr(file, '.');

	if (pos)
		*pos = '\0';

	gchar *outfile = g_strdup_printf("%s/%s.%s", outdir, file, gOutExt);
	g_free(outdir);
	g_free(file);

	if (g_strcmp0(InputFileName, outfile) == 0)
	{
		g_free(outfile);
		return FS_FILE_NOTSUPPORTED;
	}

	gProgressProc(gPluginNr, InputFileName, outfile, 0);

	if (OverwriteCheck && g_file_test(outfile, G_FILE_TEST_EXISTS))
	{
		Translate("File %s already exists, overwrite?", str, MAX_PATH);
		gchar *msg = g_strdup_printf(str, outfile);
		int ret = MessageBox((char*)msg, ROOTNAME, MB_YESNOCANCEL | MB_ICONQUESTION);
		g_free(msg);

		if (ret != ID_YES)
		{
			g_free(outfile);

			if (ret == ID_NO)
				return FS_FILE_OK;
			else
				return FS_FILE_USERABORT;
		}
	}

	gchar *command = BuildCommand(gCommand, InputFileName, outfile, gTerm);

	if (!g_shell_parse_argv(command, NULL, &argv, &err))
	{
		MessageBox((char*)err->message, ROOTNAME, MB_OK | MB_ICONERROR);
		g_error_free(err);
		g_free(command);
		g_free(outfile);
		return FS_FILE_USERABORT;
	}

	g_free(command);

	if (!g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, &err))
	{
		MessageBox((char*)err->message, ROOTNAME, MB_OK | MB_ICONERROR);
		g_error_free(err);
		g_free(outfile);
		return FS_FILE_USERABORT;
	}
	else
	{
		while (gProgressProc(gPluginNr, InputFileName, outfile, 50) == 0 && kill(pid, 0) == 0)
			sleep(0.5);

		if (gProgressProc(gPluginNr, InputFileName, outfile, 50) != 0 && kill(pid, 0) == 0)
		{
			kill(pid, SIGTERM);
			remove(outfile);
			result = FS_FILE_USERABORT;
		}

		waitpid(pid, &status, 0);
		g_spawn_close_pid(pid);
	}

	gProgressProc(gPluginNr, InputFileName, outfile, 100);
	g_free(outfile);

	return result;
}

static int CopyLocalFile(char* InFileName, char* OutFileName, gboolean OverwriteCheck)
{
	int ifd, ofd, done;
	ssize_t len, total = 0;
	char buff[BUFSIZE];
	struct stat buf;
	int result = FS_FILE_OK;

	if (strcmp(InFileName, OutFileName) == 0)
		return FS_FILE_NOTSUPPORTED;

	if (OverwriteCheck && access(OutFileName, F_OK) == 0)
		return FS_FILE_EXISTS;

	if (stat(InFileName, &buf) != 0)
		return FS_FILE_READERROR;

	ifd = open(InFileName, O_RDONLY);

	if (ifd == -1)
		return FS_FILE_READERROR;

	ofd = open(OutFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

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
				done = total * 100 / buf.st_size;
			else
				done = 0;

			if (done > 100)
				done = 100;

			if (gProgressProc(gPluginNr, InFileName, OutFileName, done) == 1)
			{
				result = FS_FILE_USERABORT;
				remove(OutFileName);
				break;
			}
		}

		close(ofd);
		chmod(OutFileName, buf.st_mode);
	}
	else
		result = FS_FILE_WRITEERROR;

	close(ifd);

	return result;
}

static void ClearTagLabels(uintptr_t pDlg)
{
	SendDlgMsg(pDlg, "lblArtist", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblTitle", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblAlbum", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblTrack", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblYear", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblGenre", DM_SETTEXT, 0, 0);

	SendDlgMsg(pDlg, "lblArtistOld", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblTitleOld", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblAlbumOld", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblTrackOld", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblYearOld", DM_SETTEXT, 0, 0);
	SendDlgMsg(pDlg, "lblGenreOld", DM_SETTEXT, 0, 0);
}

static gboolean FillTags(uintptr_t pDlg, gchar *FileName, GRegex *re, gboolean Save)
{
	gboolean result = FALSE;

	TagLib_Tag *tag;
	GMatchInfo *match_info;

	ClearTagLabels(pDlg);
	TagLib_File *tagfile = taglib_file_new(FileName);

	if (tagfile != NULL && taglib_file_is_valid(tagfile))
	{
		tag = taglib_file_tag(tagfile);
		char *string = taglib_tag_title(tag);

		if (string)
			SendDlgMsg(pDlg, "lblTitleOld", DM_SETTEXT, (intptr_t)string, 0);

		string = taglib_tag_artist(tag);

		if (string)
			SendDlgMsg(pDlg, "lblArtistOld", DM_SETTEXT, (intptr_t)string, 0);

		string = taglib_tag_album(tag);

		if (string)
			SendDlgMsg(pDlg, "lblAlbumOld", DM_SETTEXT, (intptr_t)string, 0);

		string = taglib_tag_genre(tag);

		if (string)
			SendDlgMsg(pDlg, "lblGenreOld", DM_SETTEXT, (intptr_t)string, 0);

		unsigned int value = taglib_tag_year(tag);

		if (value != 0)
		{
			char year[5];
			snprintf(year, sizeof(year), "%d", value);
			SendDlgMsg(pDlg, "lblYearOld", DM_SETTEXT, (intptr_t)year, 0);
		}

		value = taglib_tag_track(tag);

		if (value != 0)
		{
			char track[5];
			snprintf(track, sizeof(track), "%d", value);
			SendDlgMsg(pDlg, "lblTrackOld", DM_SETTEXT, (intptr_t)track, 0);
		}
	}
	else
	{
		char string[MAX_PATH];
		Translate("not a valid file.", string, MAX_PATH);
		gchar *msg = g_strdup_printf("(!!!) %s: %s", FileName, string);
		SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)msg, 0);
		g_free(msg);
		return result;
	}

	gboolean is_artist = (gboolean)SendDlgMsg(pDlg, "chArtist", DM_GETCHECK, 0, 0);
	gboolean is_title = (gboolean)SendDlgMsg(pDlg, "chTitle", DM_GETCHECK, 0, 0);
	gboolean is_album = (gboolean)SendDlgMsg(pDlg, "chAlbum", DM_GETCHECK, 0, 0);
	gboolean is_track = (gboolean)SendDlgMsg(pDlg, "chTrack", DM_GETCHECK, 0, 0);
	gboolean is_year = (gboolean)SendDlgMsg(pDlg, "chYear", DM_GETCHECK, 0, 0);
	gboolean is_genre = (gboolean)SendDlgMsg(pDlg, "chGenre", DM_GETCHECK, 0, 0);

	if (is_artist || is_title || is_album || is_track || is_year || is_genre)
		SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)FileName, 0);

	if (g_regex_match(re, FileName, 0, &match_info))
	{
		while (g_match_info_matches(match_info))
		{
			gchar *string = g_match_info_fetch_named(match_info, "artist");

			if (string)
			{
				SendDlgMsg(pDlg, "lblArtist", DM_SETTEXT, (intptr_t)string, 0);

				if (is_artist)
				{
					result = TRUE;
					gchar *caption = g_strdup((char*)SendDlgMsg(pDlg, "chArtist", DM_GETTEXT, 0, 0));
					gchar *old = g_strdup((char*)SendDlgMsg(pDlg, "lblArtistOld", DM_GETTEXT, 0, 0));
					gchar *line = g_strdup_printf("\t%s '%s' -> '%s'", caption, old, string);
					SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
					g_free(caption);
					g_free(old);
					g_free(line);

					if (Save)
						taglib_tag_set_artist(tag, string);
				}

				g_free(string);
			}

			string = g_match_info_fetch_named(match_info, "title");

			if (string)
			{
				SendDlgMsg(pDlg, "lblTitle", DM_SETTEXT, (intptr_t)string, 0);

				if (is_title)
				{
					result = TRUE;
					gchar *caption = g_strdup((char*)SendDlgMsg(pDlg, "chTitle", DM_GETTEXT, 0, 0));
					gchar *old = g_strdup((char*)SendDlgMsg(pDlg, "lblTitleOld", DM_GETTEXT, 0, 0));
					gchar *line = g_strdup_printf("\t%s '%s' -> '%s'", caption, old, string);
					SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
					g_free(caption);
					g_free(old);
					g_free(line);

					if (Save)
						taglib_tag_set_title(tag, string);

				}

				g_free(string);
			}

			string = g_match_info_fetch_named(match_info, "album");

			if (string)
			{
				SendDlgMsg(pDlg, "lblAlbum", DM_SETTEXT, (intptr_t)string, 0);

				if (is_album)
				{
					result = TRUE;
					gchar *caption = g_strdup((char*)SendDlgMsg(pDlg, "chAlbum", DM_GETTEXT, 0, 0));
					gchar *old = g_strdup((char*)SendDlgMsg(pDlg, "lblAlbumOld", DM_GETTEXT, 0, 0));
					gchar *line = g_strdup_printf("\t%s '%s' -> '%s'", caption, old, string);
					SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
					g_free(caption);
					g_free(old);
					g_free(line);

					if (Save)
						taglib_tag_set_album(tag, string);

				}

				g_free(string);
			}

			string = g_match_info_fetch_named(match_info, "track");

			if (string)
			{
				SendDlgMsg(pDlg, "lblTrack", DM_SETTEXT, (intptr_t)string, 0);

				if (is_track)
				{
					result = TRUE;
					gchar *caption = g_strdup((char*)SendDlgMsg(pDlg, "chTrack", DM_GETTEXT, 0, 0));
					gchar *old = g_strdup((char*)SendDlgMsg(pDlg, "lblTrackOld", DM_GETTEXT, 0, 0));
					gchar *line = g_strdup_printf("\t%s '%s' -> '%s'", caption, old, string);
					SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
					g_free(caption);
					g_free(old);
					g_free(line);

					if (Save)
						taglib_tag_set_track(tag, (unsigned int)atoi(string));

				}

				g_free(string);
			}

			string = g_match_info_fetch_named(match_info, "year");

			if (string)
			{
				SendDlgMsg(pDlg, "lblYear", DM_SETTEXT, (intptr_t)string, 0);

				if (is_year)
				{
					result = TRUE;
					gchar *caption = g_strdup((char*)SendDlgMsg(pDlg, "chYear", DM_GETTEXT, 0, 0));
					gchar *old = g_strdup((char*)SendDlgMsg(pDlg, "lblYearOld", DM_GETTEXT, 0, 0));
					gchar *line = g_strdup_printf("\t%s '%s' -> '%s'", caption, old, string);
					SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
					g_free(caption);
					g_free(old);
					g_free(line);

					if (Save)
						taglib_tag_set_year(tag, (unsigned int)atoi(string));

				}

				g_free(string);
			}

			string = g_match_info_fetch_named(match_info, "genre");

			if (string)
			{
				SendDlgMsg(pDlg, "lblGenre", DM_SETTEXT, (intptr_t)string, 0);

				if (is_genre)
				{
					result = TRUE;
					gchar *caption = g_strdup((char*)SendDlgMsg(pDlg, "chGenre", DM_GETTEXT, 0, 0));
					gchar *old = g_strdup((char*)SendDlgMsg(pDlg, "lblGenreOld", DM_GETTEXT, 0, 0));
					gchar *line = g_strdup_printf("\t%s '%s' -> '%s'", caption, old, string);
					SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
					g_free(caption);
					g_free(old);
					g_free(line);

					if (Save)
						taglib_tag_set_genre(tag, string);

				}

				g_free(string);
			}

			g_match_info_next(match_info, NULL);
		}
	}

	if (match_info)
		g_match_info_free(match_info);

	if (Save && !taglib_file_save(tagfile))
	{
		char msg[MAX_PATH];
		Translate("Failed to write tag.", msg, MAX_PATH);
		gchar *message = g_strdup_printf("%s: %s", FileName, msg);
		MessageBox(msg, ROOTNAME, MB_OK | MB_ICONERROR);
		g_free(message);
		result = FALSE;
	}

	taglib_tag_free_strings();
	taglib_file_free(tagfile);

	return result;
}

static void UpdateTagPreview(uintptr_t pDlg)
{
	SendDlgMsg(pDlg, "lblError", DM_SHOWITEM, 0, 0);

	int i = (int)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETITEMINDEX, 0, 0);

	if (i == -1)
	{
		ClearTagLabels(pDlg);
		return;
	}

	gchar *filename = g_strdup((char*)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETITEM, i, 0));

	if (!filename || filename[0] == '\0')
	{
		g_free(filename);
		return;
	}

	SendDlgMsg(pDlg, "chError", DM_SETCHECK, 0, 0);

	GError *err = NULL;
	gchar *expr = g_strdup((char*)SendDlgMsg(pDlg, "cbRegEx", DM_GETTEXT, 0, 0));
	GRegex *regex = g_regex_new(expr, RE_FLAGS_COMPILE, RE_FLAGS_MATCH, &err);

	if (err)
	{
		SendDlgMsg(pDlg, "lblError", DM_SHOWITEM, 1, 0);
		SendDlgMsg(pDlg, "chError", DM_SETCHECK, 1, 0);
		SendDlgMsg(pDlg, "lblError", DM_SETTEXT, (intptr_t)err->message, 0);
		g_error_free(err);
	}
	else
		FillTags(pDlg, filename, regex, FALSE);

	g_free(expr);

	if (regex)
		g_regex_unref(regex);

	g_free(filename);
}

static void FillRegExMemo(uintptr_t pDlg)
{
	gchar *contents = NULL;
	gchar *readme = g_strdup_printf("%s/regex_readme.txt", gExtensions->PluginDir);

	if (g_file_get_contents(readme, &contents, NULL, NULL))
		SendDlgMsg(pDlg, "mRegEx", DM_SETTEXT, (intptr_t)contents, 0);
	else
		SendDlgMsg(pDlg, "mRegEx", DM_SHOWITEM, 0, 0);

	g_free(contents);
	g_free(readme);
}

static void FillFileList(uintptr_t pDlg)
{
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;

	SendDlgMsg(pDlg, "lbFileList", DM_LISTCLEAR, 0, 0);

	if (gTempFile)
	{
		gchar *filename = g_strdup_printf("%s/%s", gTempDir, TMPLSTNAME);
		fseek(gTempFile, 0, SEEK_SET);

		while ((read = getline(&line, &len, gTempFile)) != -1)
		{
			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			SendDlgMsg(pDlg, "lbFileList", DM_LISTADDSTR, (intptr_t)line, 0);

		}

		fclose(gTempFile);
		remove(filename);
		g_free(filename);
		gTempFile = NULL;
	}
	else
		SendDlgMsg(pDlg, "lbFileList", DM_LISTADDSTR, (intptr_t)gLastFile, 0);
}

static void ProcessFilelist(uintptr_t pDlg, char *RegEx, gboolean Save)
{
	GError *err = NULL;

	gsize count = (gsize)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETCOUNT, 0, 0);
	GRegex *regex = g_regex_new(RegEx, RE_FLAGS_COMPILE, RE_FLAGS_MATCH, &err);

	if (err)
	{
		MessageBox(err->message, ROOTNAME, MB_OK | MB_ICONERROR);
		g_error_free(err);
	}
	else
	{
		SendDlgMsg(pDlg, "mLog", DM_SETTEXT, 0, 0);
		SendDlgMsg(pDlg, "ProgressBar", DM_SHOWITEM, 1, 0);
		gsize replaces = 0;

		for (gsize i = 0; i < count; i++)
		{
			gchar *filename = g_strdup((char*)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETITEM, i, 0));
			SendDlgMsg(pDlg, "ProgressBar", DM_SETPROGRESSVALUE, (int)i * 100 / count, 0);

			if (FillTags(pDlg, filename, regex, Save))
				replaces++;

			g_free(filename);
		}

		gchar *line = g_strdup_printf("(%ld/%ld)", replaces, count);
		SendDlgMsg(pDlg, "mLog", DM_LISTADDSTR, (intptr_t)line, 0);
		g_free(line);
	}

	if (regex)
		g_regex_unref(regex);

	SendDlgMsg(pDlg, "ProgressBar", DM_SHOWITEM, 0, 0);
}

static void DeleteFromList(uintptr_t pDlg, char* DlgItemName)
{
	int i = (int)SendDlgMsg(pDlg, DlgItemName, DM_LISTGETITEMINDEX, 0, 0);
	g_print("1!! %d\n", i);

	if (i != -1)
		SendDlgMsg(pDlg, DlgItemName, DM_LISTDELETE, i, 0);
}

static void GetCommandsForExt(uintptr_t pDlg)
{
	int num = 0;
	gchar *key = NULL;
	const char *default_cmd = "ffmpeg -y -i {infile} {outfilenoext.ext}";
	gchar *ext = g_strdup((char*)SendDlgMsg(pDlg, "cbExt", DM_GETTEXT, 0, 0));

	SendDlgMsg(pDlg, "cbCommand", DM_LISTCLEAR, 0, 0);
	SendDlgMsg(pDlg, "cbCommand", DM_SETTEXT, (intptr_t)default_cmd, 0);

	do
	{
		g_free(key);
		key = g_strdup_printf("Command_%s_%d", ext, num++);
		gchar *cmd = g_key_file_get_string(gCfg, ROOTNAME, key, NULL);

		if (cmd)
			SendDlgMsg(pDlg, "cbCommand", DM_LISTADDSTR, (intptr_t)cmd, 0);

		g_free(cmd);
	}
	while (g_key_file_has_key(gCfg, ROOTNAME, key, NULL));

	SendDlgMsg(pDlg, "cbCommand", DM_LISTSETITEMINDEX, 0, 0);
	g_free(key);
	g_free(ext);
}

intptr_t DCPCALL RegExDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		FillRegExMemo(pDlg);
		FillFileList(pDlg);
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);

		if (g_key_file_has_key(gCfg, ROOTNAME, "RegEx_0", NULL))
			SendDlgMsg(pDlg, "cbRegEx", DM_LISTCLEAR, 0, 0);

		gchar **items = g_key_file_get_keys(gCfg, ROOTNAME, NULL, NULL);

		if (items)
		{
			for (gsize i = 0; items[i] != NULL; i++)
			{
				if (strncmp(items[i], "RegEx_", 6) == 0)
				{
					gchar *item = g_key_file_get_string(gCfg, ROOTNAME, items[i], NULL);

					if (item && strlen(item) > 0)
						SendDlgMsg(pDlg, "cbRegEx", DM_LISTADDSTR, (intptr_t)item, 0);

					g_free(item);
				}
			}
		}

		g_strfreev(items);
		break;


	case DN_CLICK:
		if (strcmp(DlgItemName, "btnDel") == 0)
		{
			DeleteFromList(pDlg, "lbFileList");
		}
		else if (strcmp(DlgItemName, "lbFileList") == 0)
		{
			UpdateTagPreview(pDlg);

		}
		else if (strcmp(DlgItemName, "btnTest") == 0)
		{
			gchar *expr = g_strdup((char*)SendDlgMsg(pDlg, "cbRegEx", DM_GETTEXT, 0, 0));
			ProcessFilelist(pDlg, expr, FALSE);
			g_free(expr);
		}
		else if (strcmp(DlgItemName, "btnOK") == 0)
		{
			gchar *expr = g_strdup((char*)SendDlgMsg(pDlg, "cbRegEx", DM_GETTEXT, 0, 0));
			g_key_file_set_string(gCfg, ROOTNAME, "RegEx_0", expr);
			gsize count = (gsize)SendDlgMsg(pDlg, "cbRegEx", DM_LISTGETCOUNT, 0, 0);
			int num = 1;

			for (gsize i = 0; i < count; i++)
			{
				char *line = (char*)SendDlgMsg(pDlg, "cbRegEx", DM_LISTGETITEM, i, 0);

				if (g_strcmp0(line, expr) != 0)
				{
					gchar *key = g_strdup_printf("RegEx_%d", num++);
					g_key_file_set_string(gCfg, ROOTNAME, key, line);
					g_free(key);

					if (num == MAXINIITEMS)
						break;
				}
			}

			g_key_file_save_to_file(gCfg, gCfgPath, NULL);
			ProcessFilelist(pDlg, expr, TRUE);
			g_free(expr);
		}

		break;

	case DN_CHANGE:
		if (strncmp(DlgItemName, "ch", 2) == 0)
		{
			gchar *item = g_strdup_printf("lbl%s", DlgItemName + 2);
			SendDlgMsg(pDlg, item, DM_ENABLE, (int)wParam, 0);
			g_free(item);
			item = g_strdup_printf("lbl%sOld", DlgItemName + 2);
			SendDlgMsg(pDlg, item, DM_ENABLE, (int)wParam, 0);
			g_free(item);

			if (wParam)
			{
				gboolean is_error = (gboolean)SendDlgMsg(pDlg, "chError", DM_GETCHECK, 0, 0);
				SendDlgMsg(pDlg, "btnOK", DM_ENABLE, (int)(wParam && !is_error), 0);
				SendDlgMsg(pDlg, "btnTest", DM_ENABLE, (int)(wParam && !is_error), 0);
			}
		}
		else if (strcmp(DlgItemName, "cbRegEx") == 0)
		{
			gchar *expr = g_strdup((char*)SendDlgMsg(pDlg, "cbRegEx", DM_GETTEXT, 0, 0));
			gboolean is_artist = CHECK_NAMED("artist");
			SendDlgMsg(pDlg, "chArtist", DM_ENABLE, (int)is_artist, 0);

			if (!is_artist)
				SendDlgMsg(pDlg, "chArtist", DM_SETCHECK, (int)is_artist, 0);

			gboolean is_title = CHECK_NAMED("title");
			SendDlgMsg(pDlg, "chTitle", DM_ENABLE, (int)is_title, 0);

			if (!is_title)
				SendDlgMsg(pDlg, "chTitle", DM_SETCHECK, (int)is_title, 0);

			gboolean is_album = CHECK_NAMED("album");
			SendDlgMsg(pDlg, "chAlbum", DM_ENABLE, (int)is_album, 0);

			if (!is_album)
				SendDlgMsg(pDlg, "chAlbum", DM_SETCHECK, (int)is_album, 0);

			gboolean is_track = CHECK_NAMED("track");
			SendDlgMsg(pDlg, "chTrack", DM_ENABLE, (int)is_track, 0);

			if (!is_track)
				SendDlgMsg(pDlg, "chTrack", DM_SETCHECK, (int)is_track, 0);

			gboolean is_year = CHECK_NAMED("year");
			SendDlgMsg(pDlg, "chYear", DM_ENABLE, (int)is_year, 0);

			if (!is_year)
				SendDlgMsg(pDlg, "chYear", DM_SETCHECK, (int)is_year, 0);

			gboolean is_genre = CHECK_NAMED("genre");
			SendDlgMsg(pDlg, "chGenre", DM_ENABLE, (int)is_genre, 0);

			if (!is_genre)
				SendDlgMsg(pDlg, "chGenre", DM_SETCHECK, (int)is_genre, 0);

			g_free(expr);

			UpdateTagPreview(pDlg);

			is_artist = (gboolean)SendDlgMsg(pDlg, "chArtist", DM_GETCHECK, 0, 0);
			is_title = (gboolean)SendDlgMsg(pDlg, "chTitle", DM_GETCHECK, 0, 0);
			is_album = (gboolean)SendDlgMsg(pDlg, "chAlbum", DM_GETCHECK, 0, 0);
			is_track = (gboolean)SendDlgMsg(pDlg, "chTrack", DM_GETCHECK, 0, 0);
			is_year = (gboolean)SendDlgMsg(pDlg, "chYear", DM_GETCHECK, 0, 0);
			is_genre = (gboolean)SendDlgMsg(pDlg, "chGenre", DM_GETCHECK, 0, 0);
			gboolean is_error = (gboolean)SendDlgMsg(pDlg, "chError", DM_GETCHECK, 0, 0);

			SendDlgMsg(pDlg, "btnOK", DM_ENABLE, (int)((is_artist || is_title || is_album || is_track || is_year || is_genre) && !is_error), 0);
			SendDlgMsg(pDlg, "btnTest", DM_ENABLE, (int)((is_artist || is_title || is_album || is_track || is_year || is_genre) && !is_error), 0);
		}

		break;

	case DN_KEYUP:
		if (lParam == 1 && *(int16_t*)wParam == 46)
		{
			if (strcmp(DlgItemName, "lbFileList") == 0)
				DeleteFromList(pDlg, "lbFileList");
			else if (strcmp(DlgItemName, "cbRegEx") == 0)
				DeleteFromList(pDlg, "cbRegEx");
		}

		break;
	}

	return 0;
}

intptr_t DCPCALL PropertiesDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	char *string;
	unsigned int value;

	switch (Msg)
	{
	case DN_INITDIALOG:
		if (gAction != WFX_ACT_SET && gLastTag)
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
		else if (gAction == WFX_ACT_SET)
		{
			FillFileList(pDlg);
			gsize count = (gsize)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETCOUNT, 0, 0);
			char buff[MAX_PATH];
			Translate("file(s)", buff, MAX_PATH);
			gchar *string = g_strdup_printf("%ld %s", count, buff);
			SendDlgMsg(pDlg, "edFileName", DM_SETTEXT, (intptr_t)string, 0);
			g_free(string);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			if (gAction != WFX_ACT_SET)
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
			else
			{
				gsize count = (gsize)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETCOUNT, 0, 0);
				SendDlgMsg(pDlg, "ProgressBar", DM_SHOWITEM, 1, 0);

				for (gsize i = 0; i < count; i++)
				{
					gchar *filename = g_strdup((char*)SendDlgMsg(pDlg, "lbFileList", DM_LISTGETITEM, i, 0));
					SendDlgMsg(pDlg, "ProgressBar", DM_SETPROGRESSVALUE, (int)i * 100 / count, 0);

					TagLib_Tag *tag;
					TagLib_File *tagfile = taglib_file_new(filename);

					if (tagfile != NULL && taglib_file_is_valid(tagfile))
					{
						tag = taglib_file_tag(tagfile);

						if (SendDlgMsg(pDlg, "chArtist", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edArtist", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_artist(tag, string);
						}

						if (SendDlgMsg(pDlg, "chTitle", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edTitle", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_title(tag, string);
						}

						if (SendDlgMsg(pDlg, "chAlbum", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edAlbum", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_album(tag, string);
						}

						if (SendDlgMsg(pDlg, "chTrack", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edTrack", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_track(tag, (unsigned int)atoi(string));
						}

						if (SendDlgMsg(pDlg, "chYear", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edYear", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_year(tag, (unsigned int)atoi(string));
						}

						if (SendDlgMsg(pDlg, "chGenre", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edGenre", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_genre(tag, string);
						}

						if (SendDlgMsg(pDlg, "chComment", DM_GETCHECK, 0, 0))
						{
							string = (char*)SendDlgMsg(pDlg, "edComment", DM_GETTEXT, 0, 0);

							if (string)
								taglib_tag_set_comment(tag, string);
						}

						if (!taglib_file_save(tagfile))
						{
							char buff[MAX_PATH];
							Translate("Failed to write tag.", buff, MAX_PATH);
							gchar *msg = g_strdup_printf("%s (%s)", buff, filename);
							MessageBox(msg, ROOTNAME, MB_OK | MB_ICONERROR);
							g_free(msg);
						}

						taglib_tag_free_strings();
						taglib_file_free(tagfile);
					}
					else
					{
						char buff[MAX_PATH];
						Translate("not a valid file.", buff, MAX_PATH);
						gchar *msg = g_strdup_printf("%s: %s", filename, buff);
						MessageBox(msg, ROOTNAME, MB_OK | MB_ICONERROR);
						g_free(msg);
					}

					g_free(filename);
				}
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

			int ret = MessageBox((char*)message, ROOTNAME, MB_YESNO | MB_ICONQUESTION);
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

	case DN_CHANGE:
		if (strncmp(DlgItemName, "ch", 2) == 0)
		{
			gchar *item = g_strdup_printf("lbl%s", DlgItemName + 2);
			SendDlgMsg(pDlg, item, DM_ENABLE, (int)wParam, 0);
			g_free(item);
			item = g_strdup_printf("ed%s", DlgItemName + 2);
			SendDlgMsg(pDlg, item, DM_ENABLE, (int)wParam, 0);
			g_free(item);
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
		gboolean history = g_key_file_has_key(gCfg, ROOTNAME, "HistoryPath0", NULL);
		SendDlgMsg(pDlg, "lbHistory", DM_SHOWITEM, (int)history, 0);

		if (path)
		{
			SendDlgMsg(pDlg, "fnSelectPath", DM_SETTEXT, (intptr_t)path, 0);
			SendDlgMsg(pDlg, "lbHistory", DM_LISTADDSTR, (intptr_t)path, 0);
			g_free(path);
		}

		if (history)
		{
			int num = 0;
			gchar *key = NULL;

			do
			{
				g_free(key);
				key = g_strdup_printf("HistoryPath%d", num++);
				gchar *path = g_key_file_get_string(gCfg, ROOTNAME, key, NULL);

				if (path)
					SendDlgMsg(pDlg, "lbHistory", DM_LISTADDSTR, (intptr_t)path, 0);

				g_free(path);
			}
			while (g_key_file_has_key(gCfg, ROOTNAME, key, NULL));

			g_free(key);
		}

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			char *path = (char*)SendDlgMsg(pDlg, "fnSelectPath", DM_GETTEXT, 0, 0);

			if (path && strlen(path) > 0)
			{
				if (strcmp(gStartPath, path) != 0)
				{
					g_strlcpy(gStartPath, path, sizeof(gStartPath));
					g_key_file_set_string(gCfg, ROOTNAME, "StartPath", path);
				}

				int num = 0;
				gchar *key = NULL;

				gsize count = (gsize)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETCOUNT, 0, 0);

				for (gsize i = 0; i < count; i++)
				{
					if (num >= MAX_PATH)
						break;

					path = (char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0);

					if (strcmp(gStartPath, path) != 0)
					{
						gchar *key = g_strdup_printf("HistoryPath%d", num++);
						g_key_file_set_string(gCfg, ROOTNAME, key, path);
						g_free(key);
					}
				}

				key = g_strdup_printf("HistoryPath%d", num);

				if (g_key_file_has_key(gCfg, ROOTNAME, key, NULL))
					g_key_file_remove_key(gCfg, ROOTNAME, key, NULL);

				g_free(key);

				g_key_file_save_to_file(gCfg, gCfgPath, NULL);
			}
		}
		else if (strcmp(DlgItemName, "lbHistory") == 0)
		{
			int i = (int)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				char *path = (char*)SendDlgMsg(pDlg, "lbHistory", DM_LISTGETITEM, i, 0);
				SendDlgMsg(pDlg, "fnSelectPath", DM_SETTEXT, (intptr_t)path, 0);
			}
		}

		break;

	case DN_KEYUP:
		if (strcmp(DlgItemName, "lbHistory") == 0)
		{
			int16_t *key = (int16_t*)wParam;

			if (lParam == 1 && *key == 46)
				DeleteFromList(pDlg, "lbHistory");
		}

		break;
	}

	return 0;
}

intptr_t DCPCALL ConvertDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	gboolean term_enabled;

	switch (Msg)
	{
	case DN_INITDIALOG:
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);

		if (g_key_file_has_key(gCfg, ROOTNAME, "Ext0", NULL))
			SendDlgMsg(pDlg, "cbExt", DM_LISTCLEAR, 0, 0);

		gchar **items = g_key_file_get_keys(gCfg, ROOTNAME, NULL, NULL);

		if (items)
		{
			for (gsize i = 0; items[i] != NULL; i++)
			{
				if (strncmp(items[i], "Ext", 3) == 0)
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

		term_enabled = g_key_file_get_boolean(gCfg, ROOTNAME, "TermminalEnabled", NULL);
		SendDlgMsg(pDlg, "ckTerm", DM_SETCHECK, (int)term_enabled, 0);
		SendDlgMsg(pDlg, "edTerm", DM_ENABLE, (int)term_enabled, 0);
		SendDlgMsg(pDlg, "cbExt", DM_LISTSETITEMINDEX, 0, 0);
		GetCommandsForExt(pDlg);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			g_strlcpy(gOutExt, (char*)SendDlgMsg(pDlg, "cbExt", DM_GETTEXT, 0, 0), sizeof(gOutExt));
			gCommand = g_strdup((char*)SendDlgMsg(pDlg, "cbCommand", DM_GETTEXT, 0, 0));

			gboolean term_enabled = (gboolean)SendDlgMsg(pDlg, "ckTerm", DM_GETCHECK, 0, 0);
			g_key_file_set_boolean(gCfg, ROOTNAME, "TermminalEnabled", term_enabled);
			char *term = (char*)SendDlgMsg(pDlg, "edTerm", DM_GETTEXT, 0, 0);
			g_key_file_set_string(gCfg, ROOTNAME, "Termminal", term);

			if (term_enabled)
				gTerm = g_strdup(term);

			g_key_file_set_string(gCfg, ROOTNAME, "Ext0", gOutExt);
			gchar *key = g_strdup_printf("Command_%s_0", gOutExt);
			g_key_file_set_string(gCfg, ROOTNAME, key, gCommand);
			g_free(key);

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
					gchar *key = g_strdup_printf("Command_%s_%d", gOutExt, num++);
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
		if (strcmp(DlgItemName, "ckTerm") == 0)
			SendDlgMsg(pDlg, "edTerm", DM_ENABLE, wParam, 0);

		if (strcmp(DlgItemName, "cbExt") == 0)
			GetCommandsForExt(pDlg);

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

		gchar *term = NULL;
		term_enabled = (gboolean)SendDlgMsg(pDlg, "ckTerm", DM_GETCHECK, 0, 0);

		if (term_enabled)
		{
			term = g_strdup((char*)SendDlgMsg(pDlg, "edTerm", DM_GETTEXT, 0, 0));

			if (!g_strrstr(term, "{command}"))
			{
				g_free(term);
				g_free(cmd);
				SendDlgMsg(pDlg, "lblErr", DM_SHOWITEM, 1, 0);
				SendDlgMsg(pDlg, "lblTemplate", DM_SHOWITEM, 1, 0);
				SendDlgMsg(pDlg, "lblTemplate", DM_SETTEXT, (intptr_t)"{command}", 0);
				return 0;
			}
		}

		gchar *ext = g_strdup((char*)SendDlgMsg(pDlg, "cbExt", DM_GETTEXT, 0, 0));
		gchar *outfile = g_strdup_printf("/home/user/test.%s", ext);
		gchar *temp = BuildCommand(cmd, "/home/user/test.mp3", outfile, term);
		SendDlgMsg(pDlg, "lblPreview", DM_SETTEXT, (intptr_t)temp, 0);
		g_free(temp);
		g_free(outfile);
		g_free(ext);
		g_free(term);
		g_free(cmd);

		break;
	}

	return 0;
}

intptr_t DCPCALL ActionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	if (Msg == DN_INITDIALOG)
	{
		g_key_file_load_from_file(gCfg, gCfgPath, 0, NULL);
		gint i = g_key_file_get_integer(gCfg, ROOTNAME, "LastAction", NULL);
		SendDlgMsg(pDlg, "rgActions", DM_LISTSETITEMINDEX, i, 0);
	}
	else if (Msg == DN_CLICK && strcmp(DlgItemName, "btnOK") == 0)
	{
		gAction = (int)SendDlgMsg(pDlg, "rgActions", DM_LISTGETITEMINDEX, 0, 0);
		g_key_file_set_integer(gCfg, ROOTNAME, "LastAction", gAction);
		g_key_file_save_to_file(gCfg, gCfgPath, NULL);
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
	                       "      Layout = tlCenter\n"
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
	                       "      Layout = tlCenter\n"
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
	                       "      Layout = tlCenter\n"
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
	                       "      Layout = tlCenter\n"
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
	                       "      Layout = tlCenter\n"
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
	                       "      Layout = tlCenter\n"
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
	                       "      Layout = tlCenter\n"
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


static BOOL SetTagDialog(void)
{
	const char lfmdata[] = ""
	                       "object PropDialogBox: TPropDialogBox\n"
	                       "  Left = 412\n"
	                       "  Height = 466\n"
	                       "  Top = 125\n"
	                       "  Width = 450\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Properties'\n"
	                       "  ChildSizing.LeftRightSpacing = 15\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ChildSizing.VerticalSpacing = 10\n"
	                       "  ClientHeight = 466\n"
	                       "  ClientWidth = 450\n"
	                       "  DesignTimePPI = 100\n"
	                       "  OnShow = DialogBoxShow\n"
	                       "  Position = poOwnerFormCenter\n"
	                       "  LCLVersion = '2.2.4.0'\n"
	                       "  object edFileName: TEdit\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = Owner\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 36\n"
	                       "    Top = 15\n"
	                       "    Width = 412\n"
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
	                       "  object gbTag: TGroupBox\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = ProgressBar\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 302\n"
	                       "    Top = 96\n"
	                       "    Width = 412\n"
	                       "    AutoSize = True\n"
	                       "    ChildSizing.LeftRightSpacing = 10\n"
	                       "    ChildSizing.TopBottomSpacing = 10\n"
	                       "    ChildSizing.HorizontalSpacing = 10\n"
	                       "    ChildSizing.VerticalSpacing = 5\n"
	                       "    ChildSizing.EnlargeHorizontal = crsHomogenousChildResize\n"
	                       "    ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
	                       "    ChildSizing.ControlsPerLine = 3\n"
	                       "    ClientHeight = 302\n"
	                       "    ClientWidth = 396\n"
	                       "    TabOrder = 2\n"
	                       "    object chArtist: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 10\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 0\n"
	                       "    end\n"
	                       "    object lblArtist: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 10\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Artist'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edArtist: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 10\n"
	                       "      Width = 256\n"
	                       "      Enabled = False\n"
	                       "      TabOrder = 1\n"
	                       "    end\n"
	                       "    object chTitle: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 51\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 2\n"
	                       "    end\n"
	                       "    object lblTitle: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 51\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Title'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edTitle: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 51\n"
	                       "      Width = 256\n"
	                       "      Constraints.MinWidth = 256\n"
	                       "      Enabled = False\n"
	                       "      TabOrder = 3\n"
	                       "    end\n"
	                       "    object chAlbum: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 92\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 4\n"
	                       "    end\n"
	                       "    object lblAlbum: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 92\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Album'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edAlbum: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 92\n"
	                       "      Width = 256\n"
	                       "      Enabled = False\n"
	                       "      TabOrder = 5\n"
	                       "    end\n"
	                       "    object chTrack: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 133\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 6\n"
	                       "    end\n"
	                       "    object lblTrack: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 133\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Track'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edTrack: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 133\n"
	                       "      Width = 256\n"
	                       "      Enabled = False\n"
	                       "      MaxLength = 4\n"
	                       "      NumbersOnly = True\n"
	                       "      OnChange = EditChange\n"
	                       "      TabOrder = 7\n"
	                       "    end\n"
	                       "    object chYear: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 174\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 8\n"
	                       "    end\n"
	                       "    object lblYear: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 174\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Year'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edYear: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 174\n"
	                       "      Width = 256\n"
	                       "      Enabled = False\n"
	                       "      MaxLength = 4\n"
	                       "      NumbersOnly = True\n"
	                       "      OnChange = EditChange\n"
	                       "      TabOrder = 9\n"
	                       "    end\n"
	                       "    object chGenre: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 215\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 10\n"
	                       "    end\n"
	                       "    object lblGenre: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 215\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Genre'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edGenre: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 215\n"
	                       "      Width = 256\n"
	                       "      Enabled = False\n"
	                       "      TabOrder = 11\n"
	                       "    end\n"
	                       "    object chComment: TCheckBox\n"
	                       "      Left = 10\n"
	                       "      Height = 36\n"
	                       "      Top = 256\n"
	                       "      Width = 30\n"
	                       "      OnChange = CheckBoxChange\n"
	                       "      TabOrder = 12\n"
	                       "    end\n"
	                       "    object lblComment: TLabel\n"
	                       "      Left = 50\n"
	                       "      Height = 36\n"
	                       "      Top = 256\n"
	                       "      Width = 70\n"
	                       "      Caption = 'Comment'\n"
	                       "      Enabled = False\n"
	                       "      Layout = tlCenter\n"
	                       "      ParentColor = False\n"
	                       "    end\n"
	                       "    object edComment: TEdit\n"
	                       "      Left = 130\n"
	                       "      Height = 36\n"
	                       "      Top = 256\n"
	                       "      Width = 256\n"
	                       "      Enabled = False\n"
	                       "      TabOrder = 13\n"
	                       "    end\n"
	                       "  end\n"
	                       "  object lbFileList: TListBox\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideBottom.Control = Owner\n"
	                       "    AnchorSideBottom.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 83\n"
	                       "    Top = 368\n"
	                       "    Width = 104\n"
	                       "    Anchors = [akLeft, akBottom]\n"
	                       "    ItemHeight = 0\n"
	                       "    TabOrder = 3\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object ProgressBar: TProgressBar\n"
	                       "    AnchorSideLeft.Control = gbTag\n"
	                       "    AnchorSideTop.Control = edFileName\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = gbTag\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 25\n"
	                       "    Top = 61\n"
	                       "    Width = 412\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    TabOrder = 1\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 220\n"
	                       "    Height = 31\n"
	                       "    Top = 418\n"
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
	                       "    TabOrder = 5\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = gbTag\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = gbTag\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 326\n"
	                       "    Height = 31\n"
	                       "    Top = 418\n"
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
	                       "    TabOrder = 4\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), PropertiesDlgProc);
}

static BOOL OptionsDialog(void)
{
	const char lfmdata[] = ""
	                       "object OptDialogBox: TOptDialogBox\n"
	                       "  Left = 458\n"
	                       "  Height = 262\n"
	                       "  Top = 307\n"
	                       "  Width = 672\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Options'\n"
	                       "  ChildSizing.LeftRightSpacing = 15\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ChildSizing.VerticalSpacing = 10\n"
	                       "  ClientHeight = 262\n"
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
	                       "  object lbHistory: TListBox\n"
	                       "    AnchorSideLeft.Control = fnSelectPath\n"
	                       "    AnchorSideTop.Control = lblStartPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lblStartPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 83\n"
	                       "    Top = 74\n"
	                       "    Width = 638\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    ItemHeight = 0\n"
	                       "    OnClick = ListBoxClick\n"
	                       "    OnKeyUp = ListBoxKeyUp\n"
	                       "    TabOrder = 0\n"
	                       "    Visible = False\n"
	                       "  end\n"
	                       "  object fnSelectPath: TDirectoryEdit\n"
	                       "    AnchorSideLeft.Control = lblStartPath\n"
	                       "    AnchorSideTop.Control = lbHistory\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = lblStartPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 15\n"
	                       "    Height = 36\n"
	                       "    Top = 167\n"
	                       "    Width = 638\n"
	                       "    Directory = '/'\n"
	                       "    DialogTitle = 'Select folder'\n"
	                       "    ShowHidden = False\n"
	                       "    ButtonWidth = 24\n"
	                       "    NumGlyphs = 1\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    BorderSpacing.Bottom = 1\n"
	                       "    MaxLength = 0\n"
	                       "    TabOrder = 1\n"
	                       "    Text = '/'\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 446\n"
	                       "    Height = 31\n"
	                       "    Top = 218\n"
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
	                       "    TabOrder = 3\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = fnSelectPath\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = fnSelectPath\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 552\n"
	                       "    Height = 31\n"
	                       "    Top = 218\n"
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
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), OptionsDlgProc);
}

static BOOL ConvertDialog(void)
{
	const char lfmdata[] = ""
	                       "object ConvDialogBox: TConvDialogBox\n"
	                       "  Left = 444\n"
	                       "  Height = 210\n"
	                       "  Top = 143\n"
	                       "  Width = 652\n"
	                       "  AutoSize = True\n"
	                       "  BorderStyle = bsDialog\n"
	                       "  Caption = 'Convert'\n"
	                       "  ChildSizing.LeftRightSpacing = 10\n"
	                       "  ChildSizing.TopBottomSpacing = 15\n"
	                       "  ClientHeight = 210\n"
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
	                       "    Width = 69\n"
	                       "    Caption = 'Extension'\n"
	                       "    ParentColor = False\n"
	                       "  end\n"
	                       "  object cbExt: TComboBox\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = lblExt\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 36\n"
	                       "    Top = 32\n"
	                       "    Width = 129\n"
	                       "    ItemHeight = 23\n"
	                       "    ItemIndex = 0\n"
	                       "    Items.Strings = (\n"
	                       "      'ogg'\n"
	                       "      'flac'\n"
	                       "      'mp3'\n"
	                       "    )\n"
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
	                       "    Height = 36\n"
	                       "    Top = 32\n"
	                       "    Width = 491\n"
	                       "    BorderSpacing.Left = 10\n"
	                       "    ItemHeight = 23\n"
	                       "    OnChange = ComboBoxChange\n"
	                       "    TabOrder = 1\n"
	                       "    Text = 'ffmpeg -y -i {infile} {outfilenoext.ext}'\n"
	                       "  end\n"
	                       "  object ckTerm: TCheckBox\n"
	                       "    AnchorSideLeft.Control = edTerm\n"
	                       "    AnchorSideTop.Control = cbExt\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 23\n"
	                       "    Top = 78\n"
	                       "    Width = 162\n"
	                       "    BorderSpacing.Top = 10\n"
	                       "    Caption = 'Launch in Terminal'\n"
	                       "    OnChange = CheckBoxChange\n"
	                       "    TabOrder = 2\n"
	                       "  end\n"
	                       "  object edTerm: TEdit\n"
	                       "    AnchorSideLeft.Control = Owner\n"
	                       "    AnchorSideTop.Control = ckTerm\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = cbCommand\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 10\n"
	                       "    Height = 36\n"
	                       "    Top = 101\n"
	                       "    Width = 630\n"
	                       "    Anchors = [akTop, akLeft, akRight]\n"
	                       "    Enabled = False\n"
	                       "    OnChange = EditChange\n"
	                       "    TabOrder = 3\n"
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
	                       "    Top = 147\n"
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
	                       "    Top = 148\n"
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
	                       "    Top = 148\n"
	                       "    Width = 1\n"
	                       "    BorderSpacing.Left = 5\n"
	                       "    Font.Style = [fsBold]\n"
	                       "    ParentColor = False\n"
	                       "    ParentFont = False\n"
	                       "    Visible = False\n"
	                       "    WordWrap = True\n"
	                       "  end\n"
	                       "  object btnOK: TBitBtn\n"
	                       "    AnchorSideTop.Control = lblPreview\n"
	                       "    AnchorSideTop.Side = asrBottom\n"
	                       "    AnchorSideRight.Control = cbCommand\n"
	                       "    AnchorSideRight.Side = asrBottom\n"
	                       "    Left = 539\n"
	                       "    Height = 31\n"
	                       "    Top = 168\n"
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
	                       "    TabOrder = 4\n"
	                       "  end\n"
	                       "  object btnCancel: TBitBtn\n"
	                       "    AnchorSideTop.Control = btnOK\n"
	                       "    AnchorSideTop.Side = asrCenter\n"
	                       "    AnchorSideRight.Control = btnOK\n"
	                       "    Left = 433\n"
	                       "    Height = 31\n"
	                       "    Top = 168\n"
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
	                       "    TabOrder = 5\n"
	                       "  end\n"
	                       "end\n";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), ConvertDlgProc);
}


static BOOL RegExDialog()
{
	const char *lfmdata = R"(
object RegExDialog: TRegExDialog
  Left = 591
  Height = 726
  Top = 223
  Width = 566
  AutoSize = True
  BorderStyle = bsDialog
  Caption = 'RegEx'
  ChildSizing.LeftRightSpacing = 10
  ChildSizing.TopBottomSpacing = 15
  ClientHeight = 726
  ClientWidth = 566
  DesignTimePPI = 100
  OnShow = DialogBoxShow
  Position = poOwnerFormCenter
  LCLVersion = '2.2.4.0'
  object mRegEx: TMemo
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = Owner
    AnchorSideRight.Control = cbRegEx
    AnchorSideRight.Side = asrBottom
    Left = 10
    Height = 176
    Top = 15
    Width = 546
    Anchors = [akTop, akLeft, akRight]
    Font.Name = 'Monospace'
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssAutoBoth
    TabOrder = 0
    WordWrap = False
  end
  object cbRegEx: TComboBox
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = mRegEx
    AnchorSideTop.Side = asrBottom
    Left = 10
    Height = 36
    Top = 191
    Width = 546
    ItemHeight = 23
    Items.Strings = (
      '(?''artist''[^/]+)\s+-\s+(?''title''[^/]+)\..+$|(?''title''[^/]+)\..+$'
      '(?''artist''[^/]+)/(?''album''[^/]+)/(?''track''\d{0,2})[\.:]\s+(?''title''[^/]+)\..+$'
    )
    OnChange = ComboBoxChange
    OnKeyUp = ComboBoxKeyUp
    TabOrder = 1
  end
  object lblError: TLabel
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = cbRegEx
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Control = Owner
    AnchorSideRight.Side = asrBottom
    Left = 10
    Height = 1
    Top = 232
    Width = 546
    Anchors = [akTop, akLeft, akRight]
    BorderSpacing.Top = 5
    Font.Style = [fsBold, fsItalic]
    ParentColor = False
    ParentFont = False
    Visible = False
    WordWrap = True
  end
  object gbPreview: TGroupBox
    AnchorSideLeft.Control = Owner
    AnchorSideTop.Control = lblError
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Control = cbRegEx
    AnchorSideRight.Side = asrBottom
    Left = 10
    Height = 429
    Top = 243
    Width = 546
    Anchors = [akTop, akLeft, akRight]
    AutoSize = True
    BorderSpacing.Top = 10
    ChildSizing.VerticalSpacing = 5
    ClientHeight = 429
    ClientWidth = 530
    TabOrder = 2
    object Panel: TPanel
      AnchorSideLeft.Control = gbPreview
      AnchorSideTop.Control = gbPreview
      AnchorSideRight.Control = gbPreview
      AnchorSideRight.Side = asrBottom
      Left = 0
      Height = 155
      Top = 0
      Width = 530
      Anchors = [akTop, akLeft, akRight]
      AutoSize = True
      BevelOuter = bvNone
      ChildSizing.EnlargeHorizontal = crsHomogenousChildResize
      ChildSizing.Layout = cclLeftToRightThenTopToBottom
      ChildSizing.ControlsPerLine = 3
      ClientHeight = 155
      ClientWidth = 530
      TabOrder = 0
      object lblReplace: TLabel
        Left = 0
        Height = 17
        Top = 0
        Width = 190
        Alignment = taCenter
        Caption = 'Replace'
        Font.Style = [fsBold]
        Layout = tlCenter
        ParentColor = False
        ParentFont = False
      end
      object lblCurrent: TLabel
        Left = 190
        Height = 17
        Top = 0
        Width = 176
        Alignment = taCenter
        Caption = 'Current'
        Font.Style = [fsBold]
        Layout = tlCenter
        ParentColor = False
        ParentFont = False
      end
      object lblMatch: TLabel
        Left = 366
        Height = 17
        Top = 0
        Width = 164
        Alignment = taCenter
        Caption = 'Result'
        Font.Style = [fsBold]
        Layout = tlCenter
        ParentColor = False
        ParentFont = False
      end
      object chArtist: TCheckBox
        Left = 0
        Height = 23
        Top = 17
        Width = 190
        Caption = 'Artist'
        Enabled = False
        OnChange = CheckBoxChange
        TabOrder = 1
      end
      object lblArtistOld: TLabel
        Left = 190
        Height = 23
        Top = 17
        Width = 176
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object lblArtist: TLabel
        Left = 366
        Height = 23
        Top = 17
        Width = 164
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object chTitle: TCheckBox
        Left = 0
        Height = 23
        Top = 40
        Width = 190
        Caption = 'Title'
        Enabled = False
        OnChange = CheckBoxChange
        TabOrder = 2
      end
      object lblTitleOld: TLabel
        Left = 190
        Height = 23
        Top = 40
        Width = 176
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object lblTitle: TLabel
        Left = 366
        Height = 23
        Top = 40
        Width = 164
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object chAlbum: TCheckBox
        Left = 0
        Height = 23
        Top = 63
        Width = 190
        Caption = 'Album'
        Enabled = False
        OnChange = CheckBoxChange
        TabOrder = 3
      end
      object lblAlbumOld: TLabel
        Left = 190
        Height = 23
        Top = 63
        Width = 176
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object lblAlbum: TLabel
        Left = 366
        Height = 23
        Top = 63
        Width = 164
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object chTrack: TCheckBox
        Left = 0
        Height = 23
        Top = 86
        Width = 190
        Caption = 'Track'
        Enabled = False
        OnChange = CheckBoxChange
        TabOrder = 4
      end
      object lblTrackOld: TLabel
        Left = 190
        Height = 23
        Top = 86
        Width = 176
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object lblTrack: TLabel
        Left = 366
        Height = 23
        Top = 86
        Width = 164
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object chYear: TCheckBox
        Left = 0
        Height = 23
        Top = 109
        Width = 190
        Caption = 'Year'
        Enabled = False
        OnChange = CheckBoxChange
        TabOrder = 5
      end
      object lblYearOld: TLabel
        Left = 190
        Height = 23
        Top = 109
        Width = 176
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object lblYear: TLabel
        Left = 366
        Height = 23
        Top = 109
        Width = 164
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object chGenre: TCheckBox
        Left = 0
        Height = 23
        Top = 132
        Width = 190
        Caption = 'Genre'
        Enabled = False
        OnChange = CheckBoxChange
        TabOrder = 6
      end
      object lblGenreOld: TLabel
        Left = 190
        Height = 23
        Top = 132
        Width = 176
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object lblGenre: TLabel
        Left = 366
        Height = 23
        Top = 132
        Width = 164
        Enabled = False
        Layout = tlCenter
        ParentColor = False
        WordWrap = True
      end
      object chError: TCheckBox
        AnchorSideLeft.Side = asrBottom
        AnchorSideTop.Control = Panel
        AnchorSideRight.Control = Panel
        AnchorSideRight.Side = asrBottom
        Left = 449
        Height = 23
        Top = 0
        Width = 81
        Anchors = [akTop, akRight]
        Caption = 'chError'
        TabOrder = 0
        Visible = False
      end
    end
    object ProgressBar: TProgressBar
      AnchorSideLeft.Control = Panel
      AnchorSideTop.Control = mLog
      AnchorSideTop.Side = asrBottom
      AnchorSideRight.Control = Panel
      AnchorSideRight.Side = asrBottom
      Left = 0
      Height = 25
      Top = 404
      Width = 530
      Anchors = [akTop, akLeft, akRight]
      BorderSpacing.Top = 5
      TabOrder = 5
      Visible = False
    end
    object lbFileList: TListBox
      AnchorSideLeft.Control = Panel
      AnchorSideTop.Control = Panel
      AnchorSideTop.Side = asrBottom
      AnchorSideRight.Control = Panel
      AnchorSideRight.Side = asrBottom
      Left = 0
      Height = 106
      Top = 160
      Width = 530
      Anchors = [akTop, akLeft, akRight]
      BorderSpacing.Top = 5
      ItemHeight = 0
      OnClick = ListBoxClick
      OnKeyUp = ListBoxKeyUp
      TabOrder = 1
    end
    object btnDel: TButton
      AnchorSideLeft.Control = Panel
      AnchorSideTop.Control = lbFileList
      AnchorSideTop.Side = asrBottom
      Left = 0
      Height = 29
      Top = 271
      Width = 217
      AutoSize = True
      Caption = '&Remove filename from list'
      OnClick = ButtonClick
      TabOrder = 2
    end
    object mLog: TMemo
      AnchorSideLeft.Control = Panel
      AnchorSideTop.Control = btnDel
      AnchorSideTop.Side = asrBottom
      AnchorSideRight.Control = Panel
      AnchorSideRight.Side = asrBottom
      Left = 0
      Height = 94
      Top = 305
      Width = 530
      Anchors = [akTop, akLeft, akRight]
      ReadOnly = True
      ScrollBars = ssAutoBoth
      TabOrder = 4
      WantTabs = True
      WordWrap = False
    end
    object btnTest: TButton
      AnchorSideTop.Control = btnDel
      AnchorSideRight.Control = Panel
      AnchorSideRight.Side = asrBottom
      Left = 379
      Height = 29
      Top = 271
      Width = 151
      Anchors = [akTop, akRight]
      AutoSize = True
      Caption = '&Test all filenames'
      Enabled = False
      OnClick = ButtonClick
      TabOrder = 3
    end
  end
  object btnCancel: TBitBtn
    AnchorSideTop.Control = btnOK
    AnchorSideTop.Side = asrCenter
    AnchorSideRight.Control = btnOK
    Left = 349
    Height = 31
    Top = 692
    Width = 101
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Right = 5
    Cancel = True
    Constraints.MinHeight = 31
    Constraints.MinWidth = 101
    DefaultCaption = True
    Kind = bkCancel
    ModalResult = 2
    OnClick = ButtonClick
    TabOrder = 4
  end
  object btnOK: TBitBtn
    AnchorSideTop.Control = gbPreview
    AnchorSideTop.Side = asrBottom
    AnchorSideRight.Control = cbRegEx
    AnchorSideRight.Side = asrBottom
    Left = 455
    Height = 31
    Top = 692
    Width = 101
    Anchors = [akTop, akRight]
    AutoSize = True
    BorderSpacing.Top = 20
    Constraints.MinHeight = 31
    Constraints.MinWidth = 101
    Default = True
    DefaultCaption = True
    Enabled = False
    Kind = bkOK
    ModalResult = 1
    OnClick = ButtonClick
    TabOrder = 3
  end
end
)";

	return gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), RegExDlgProc);
}

static void ActionsDialog(void)
{
	const char lfmdata_tmpl[] = ""
	                            "object ActionDialog: TActionDialog\n"
	                            "  Left = 591\n"
	                            "  Height = 171\n"
	                            "  Top = 223\n"
	                            "  Width = 423\n"
	                            "  AutoSize = True\n"
	                            "  BorderStyle = bsDialog\n"
	                            "  Caption = 'Select Action'\n"
	                            "  ChildSizing.LeftRightSpacing = 10\n"
	                            "  ChildSizing.TopBottomSpacing = 15\n"
	                            "  ClientHeight = 171\n"
	                            "  ClientWidth = 423\n"
	                            "  DesignTimePPI = 100\n"
	                            "  OnShow = DialogBoxShow\n"
	                            "  Position = poOwnerFormCenter\n"
	                            "  LCLVersion = '2.2.4.0'\n"
	                            "  object rgActions: TRadioGroup\n"
	                            "    AnchorSideLeft.Control = Owner\n"
	                            "    AnchorSideTop.Control = Owner\n"
	                            "    Left = 10\n"
	                            "    Height = 94\n"
	                            "    Top = 15\n"
	                            "    Width = 400\n"
	                            "    AutoFill = True\n"
	                            "    AutoSize = True\n"
	                            "    Caption = 'Select Action'\n"
	                            "    ChildSizing.LeftRightSpacing = 10\n"
	                            "    ChildSizing.EnlargeHorizontal = crsHomogenousChildResize\n"
	                            "    ChildSizing.EnlargeVertical = crsHomogenousChildResize\n"
	                            "    ChildSizing.ShrinkHorizontal = crsScaleChilds\n"
	                            "    ChildSizing.ShrinkVertical = crsScaleChilds\n"
	                            "    ChildSizing.Layout = cclLeftToRightThenTopToBottom\n"
	                            "    ChildSizing.ControlsPerLine = 1\n"
	                            "    ClientHeight = 69\n"
	                            "    ClientWidth = 384\n"
	                            "    Constraints.MinWidth = 400\n"
	                            "    ItemIndex = 0\n"
	                            "    Items.Strings = (\n%s"
	                            "    )\n"
	                            "    TabOrder = 0\n"
	                            "  end\n"
	                            "  object btnOK: TBitBtn\n"
	                            "    AnchorSideTop.Control = rgActions\n"
	                            "    AnchorSideTop.Side = asrBottom\n"
	                            "    AnchorSideRight.Control = rgActions\n"
	                            "    AnchorSideRight.Side = asrBottom\n"
	                            "    Left = 309\n"
	                            "    Height = 31\n"
	                            "    Top = 129\n"
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
	                            "    TabOrder = 1\n"
	                            "  end\n"
	                            "  object btnCancel: TBitBtn\n"
	                            "    AnchorSideTop.Control = btnOK\n"
	                            "    AnchorSideTop.Side = asrCenter\n"
	                            "    AnchorSideRight.Control = btnOK\n"
	                            "    Left = 203\n"
	                            "    Height = 31\n"
	                            "    Top = 129\n"
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
	                            "end\n"
	                            ;

	GString *items = g_string_new(NULL);

	for (int i = 0; i < WFX_ACTS; i++)
	{
		char buff[MAX_PATH] = "";

		switch (i)
		{
		case WFX_ACT_CONV:
			Translate("Run an external program to convert files", buff, MAX_PATH);
			break;
		case WFX_ACT_FILL:
			Translate("Fill tags with data from filenames", buff, MAX_PATH);
			break;
		case WFX_ACT_COPY:
			Translate("Copy audio files", buff, MAX_PATH);
			break;
		case WFX_ACT_SET:
			Translate("Set tag values for selected files", buff, MAX_PATH);
			break;
		}

		if (buff[0] != '\0')
		{
			g_string_append(items, "      '");
			g_string_append(items, buff);
			g_string_append(items, "'\n");
		}
	}

	gchar *lfmdata = g_strdup_printf(lfmdata_tmpl, items->str);
	gExtensions->DialogBoxLFM((intptr_t)lfmdata, (unsigned long)strlen(lfmdata), ActionsDlgProc);
	g_free(lfmdata);
	g_string_free(items, TRUE);
}

static int ExecuteAction(char *InputFileName, char *TargetPath, gboolean OverwriteCheck)
{
	if (gAction == WFX_ACT_FILL || gAction == WFX_ACT_SET)
		return AddToFileList(InputFileName);
	else  if (gAction == WFX_ACT_CONV)
		return ConvertFile(InputFileName, TargetPath, OverwriteCheck);
	else  if (gAction == WFX_ACT_COPY)
		return CopyLocalFile(InputFileName, TargetPath, OverwriteCheck);

	return FS_FILE_USERABORT;
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
		FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;
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
	if (gAction == -1)
		return FS_FILE_USERABORT;

	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "%s%s", gStartPath, RemoteName);

	return ExecuteAction(filename, LocalName, CopyFlags == 0);
}

int DCPCALL FsPutFile(char* LocalName, char* RemoteName, int CopyFlags)
{
	if (gAction == -1)
		return FS_FILE_USERABORT;

	TagLib_File *tagfile = taglib_file_new(LocalName);

	if (tagfile != NULL && taglib_file_is_valid(tagfile))
	{
		taglib_tag_free_strings();
		taglib_file_free(tagfile);
	}
	else
		return FS_FILE_OK;

	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "%s%s", gStartPath, RemoteName);

	return ExecuteAction(LocalName, filename, CopyFlags == 0);
}

int DCPCALL FsRenMovFile(char* OldName, char* NewName, BOOL Move, BOOL OverWrite, RemoteInfoStruct * ri)
{
	int result = FS_FILE_WRITEERROR;
	char infile[PATH_MAX], outfile[PATH_MAX];

	snprintf(infile, sizeof(infile), "%s%s", gStartPath, OldName);
	snprintf(outfile, sizeof(outfile), "%s%s", gStartPath, NewName);

	if (gProgressProc(gPluginNr, infile, outfile, 0))
		return FS_FILE_USERABORT;

	if (!Move)
		return CopyLocalFile(infile, outfile, OverWrite == FALSE);

	if (OverWrite == FALSE && g_file_test(outfile, G_FILE_TEST_EXISTS))
	{
		return FS_FILE_EXISTS;
	}

	if (rename(infile, outfile) == 0)
		result = FS_FILE_OK;

	gProgressProc(gPluginNr, infile, outfile, 100);

	return result;
}

BOOL DCPCALL FsMkDir(char* Path)
{
	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "%s%s", gStartPath, Path);

	return (mkdir(filename, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0);
}

BOOL DCPCALL FsDeleteFile(char* RemoteName)
{
	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), "%s%s", gStartPath, RemoteName);

	return (remove(filename) == 0);
}

BOOL DCPCALL FsRemoveDir(char* RemoteName)
{
	return FsDeleteFile(RemoteName);
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
	if (InfoOperation > FS_STATUS_OP_LIST && InfoOperation < FS_STATUS_OP_RENMOV_SINGLE)
	{
		if (InfoStartEnd == FS_STATUS_START)
		{
			gAction = -1;
			ActionsDialog();

			if (gAction == WFX_ACT_CONV)
				ConvertDialog();
			else if (gAction == WFX_ACT_FILL || gAction == WFX_ACT_SET)
			{
				if (!gTempDir)
					gTempDir = g_dir_make_tmp(TEMPLATEDIR, NULL);

				gchar *filename = g_strdup_printf("%s/%s", gTempDir, TMPLSTNAME);
				gTempFile = fopen(filename, "w+");
				g_free(filename);
			}
		}
		else
		{
			if (gTempFile)
			{
				if (gAction == WFX_ACT_FILL)
					RegExDialog();
				else if (gAction == WFX_ACT_SET)
					SetTagDialog();
			}

			gAction = -1;
		}
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

	if (gTempDir)
	{
		remove(gTempDir);
		g_free(gTempDir);
	}

}
