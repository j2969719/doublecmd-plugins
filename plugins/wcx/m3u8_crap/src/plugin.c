#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include <tag_c.h>
#include <string.h>
#include <sys/stat.h>
#include "wcxplugin.h"

#define MAXFILESIZE 10000000

typedef struct sArcData
{
	FILE *playlist;
	char dirname[PATH_MAX];
	char lastfile[PATH_MAX];
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;

static gchar *StringReplace(gchar *Text, gchar *Str, gchar *Repl)
{
	gchar *result = NULL;

	gchar **split = g_strsplit(Text, Str, -1);
	result = g_strjoinv(Repl, split);
	g_strfreev(split);

	return result;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	struct stat buf;

	if (stat(ArchiveData->ArcName, &buf) != 0)
	{
		ArchiveData->OpenResult = E_EOPEN;
		return E_SUCCESS;
	}

	if (buf.st_size > MAXFILESIZE)
	{
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	ArcData data = malloc(sizeof(tArcData));

	if (data == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(data, 0, sizeof(tArcData));

	if ((data->playlist = fopen(ArchiveData->ArcName, "r")) == NULL)
	{
		free(data);
		ArchiveData->OpenResult = E_EOPEN;
		return E_SUCCESS;
	}

	gchar *adirname = g_path_get_dirname(ArchiveData->ArcName);
	g_strlcpy(data->dirname, adirname, PATH_MAX);
	g_free(adirname);

	return (HANDLE)data;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ssize_t lread;
	size_t len = 0;
	struct stat buf;
	char *line = NULL;
	ArcData data = (ArcData)hArcData;

	memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));

	while ((lread = getline(&line, &len, data->playlist)) != -1)
	{
		if (lread > 5 && line[0] != '#' && (line[1] != ':' && line[2] != '\\') && !g_str_has_prefix(line, "http"))
		{
			gchar *temp = StringReplace(line, "\r", "");
			free(line);
			line = NULL;
			gchar *newline = StringReplace(temp, "\n", "");
			g_free(temp);
			temp = StringReplace(newline, "\\", "/");
			g_free(newline);
			newline = temp;
			g_strstrip(newline);

			if (strlen(newline) < 5)
			{
				g_free(newline);
				continue;
			}

			if (newline[0] == '/')
			{
				g_strlcpy(data->lastfile, newline, sizeof(data->lastfile));
				g_strlcpy(HeaderDataEx->FileName, newline + 1, sizeof(HeaderDataEx->FileName) - 1);
			}
			else
			{
				g_strlcpy(HeaderDataEx->FileName, newline, sizeof(HeaderDataEx->FileName) - 1);
				gchar *path = g_strdup_printf("%s/%s", data->dirname, newline);
				g_strlcpy(data->lastfile, path, sizeof(data->lastfile));
			}

			g_free(newline);

			if (stat(data->lastfile, &buf) == 0)
			{
				HeaderDataEx->PackSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
				HeaderDataEx->PackSize = buf.st_size & 0x00000000FFFFFFFF;
				HeaderDataEx->UnpSizeHigh = (buf.st_size & 0xFFFFFFFF00000000) >> 32;
				HeaderDataEx->UnpSize = buf.st_size & 0x00000000FFFFFFFF;
				HeaderDataEx->FileTime = buf.st_mtime;
				HeaderDataEx->FileAttr = buf.st_mode;
			}

			return E_SUCCESS;
		}
	}

	free(line);

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	int result = E_SUCCESS;
	ArcData data = (ArcData)hArcData;

	if (Operation != PK_SKIP && !DestPath)
	{

		if (Operation == PK_EXTRACT)
		{
			if (!g_file_test(data->lastfile, G_FILE_TEST_EXISTS) && data->ChangeVolProc)
			{
				if (data->ChangeVolProc(data->dirname, PK_VOL_ASK) != 0)
				{
					gchar *fname = g_path_get_basename(data->lastfile);
					gchar *path = g_strdup_printf("%s/%s", data->dirname, fname);
					g_strlcpy(data->lastfile, path, sizeof(data->lastfile));
					g_free(fname);
					g_free(path);
				}
			}

			if (symlink(data->lastfile, DestName) != 0)
				return E_EWRITE;
		}
	}

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	fclose(data->playlist);
	free(data);

	return E_SUCCESS;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gProcessDataProc = pProcessDataProc;
	else
		data->ProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gChangeVolProc = pChangeVolProc1;
	else
		data->ChangeVolProc = pChangeVolProc1;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	if (g_str_has_suffix(FileName, "m3u") || g_str_has_suffix(FileName, "m3u8"))
		return TRUE;

	return FALSE;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_HIDE;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	FILE *fp;

	if (SubPath != NULL)
		return E_NOT_SUPPORTED;

	gboolean rel = (strncmp(PackedFile, SrcPath, strlen(SrcPath)) == 0);

	if (!g_file_test(PackedFile, G_FILE_TEST_EXISTS))
	{
		if ((fp = fopen(PackedFile, "w")) != NULL)
			fprintf(fp, "#EXTM3U\n");
	}
	else
		fp = fopen(PackedFile, "a");

	if (fp == NULL)
		return E_EWRITE;

	while (*AddList)
	{
		int len;
		char *artist, *title;

		if (AddList[strlen(AddList) - 1] != '/')
		{
			gchar *path = g_strdup_printf("%s%s", SrcPath, AddList);
			GFile *gfile = g_file_new_for_path(path);
			GFileInfo *fileinfo = g_file_query_info(gfile, "standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, NULL);
			const gchar *content_type = g_file_info_get_content_type(fileinfo);

			if (g_str_has_prefix(content_type, "audio/") && !g_str_has_suffix(content_type, "url"))
			{
				TagLib_File *tagfile = taglib_file_new(path);

				if (tagfile != NULL && taglib_file_is_valid(tagfile))
				{
					TagLib_Tag *tags = taglib_file_tag(tagfile);
					const TagLib_AudioProperties *props = taglib_file_audioproperties(tagfile);
					title = taglib_tag_title(tags);
					artist = taglib_tag_artist(tags);
					len = taglib_audioproperties_length(props);

					gchar *fpath = NULL;

					if (rel)
						fpath = g_strdup(path + strlen(SrcPath));
					else
						fpath = g_strdup(path);

					if ((artist != NULL && artist[0] != '\0') && (title != NULL && title[0] != '\0'))
						fprintf(fp, "#EXTINF:%d,%s - %s\n%s\n", len, artist, title, fpath);
					else if ((artist == NULL || artist[0] == '\0') && (title != NULL && title[0] != '\0'))
						fprintf(fp, "#EXTINF:%d,%s\n%s\n", len, title, fpath);
					else
					{
						gchar *filename = g_strdup(AddList);
						char *pos = strrchr(filename, '.');

						if (pos)
							*pos = '\0';

						fprintf(fp, "#EXTINF:%d,%s\n%s\n", len, filename, fpath);
						g_free(filename);
					}

					taglib_tag_free_strings();
					taglib_file_free(tagfile);
					g_free(fpath);
				}
			}

			g_free(path);
			g_object_unref(fileinfo);
			g_object_unref(gfile);

			if (gProcessDataProc(AddList, -1100) == 0)
			{
				fclose(fp);
				return E_EABORTED;
			}
		}

		while (*AddList++);
	}

	fclose(fp);

	return E_SUCCESS;
}
