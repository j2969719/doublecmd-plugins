#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "wdxplugin.h"

#define TSV_FILE PLUGDIR ".tsv"
#define MAX_BYTES 20
#define MAX_FIELDS 20

typedef struct
{
	char *name;
	unsigned char bytes[MAX_BYTES];
	int byte_count;
} FieldItem;

int fields_count = 0;
FieldItem fields[MAX_FIELDS];

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	static char tsv_path[PATH_MAX] = "";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(tsv_path, &dlinfo) != 0)
	{
		snprintf(tsv_path, PATH_MAX, "%s", dlinfo.dli_fname);
		char *pos = strrchr(tsv_path, '/');

		if (pos)
			strcpy(pos + 1, TSV_FILE);

		FILE *fp = fopen(tsv_path, "r");

		if (fp)
		{
			ssize_t read;
			size_t len = 0;
			char *line = NULL;

			if (getline(&line, &len, fp) == -1)
			{
				free(line);
				fclose(fp);
				return;
			}

			while ((read = getline(&line, &len, fp)) != -1)
			{
				if (line[read - 1] == '\n')
					line[read - 1] = '\0';

				char *cell = strtok(line, "\t");

				if (cell)
				{
					fields[fields_count].name = strdup(cell);
					cell = strtok(NULL, "\t");

					if (cell)
					{
						char *end;
						unsigned char *bytes = fields[fields_count].bytes;
						int *pos = &fields[fields_count].byte_count;

						while (*cell != '\0' && fields[fields_count].byte_count < MAX_BYTES)
						{
							bytes[*pos] = (unsigned char)strtol(cell, &end, 16);

							if (cell == end)
								break;

							cell = end;
							(*pos)++;

							if (fields[fields_count].byte_count == MAX_BYTES)
								break;
						}

						fields_count++;
					}
					else
						free(fields[fields_count].name);
				}
			}

			free(line);
			fclose(fp);
		}
	}
}

void DCPCALL ContentPluginUnloading(void)
{
	for (int i = 0; i < fields_count; i++)
		free(fields[i].name);
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	Units[0] = '\0';

	if (FieldIndex < 0 || FieldIndex >= fields_count)
		return ft_nomorefields;

	if (fields[FieldIndex].name == NULL || fields[FieldIndex].name[0] == '\0')
		return ft_nomorefields;
	else
		snprintf(FieldName, maxlen - 1, "%s", fields[FieldIndex].name);

	return ft_boolean;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	struct stat buf;

	if (lstat(FileName, &buf) != 0 || !S_ISREG(buf.st_mode) || buf.st_size == 0)
		return ft_fileerror;

	int fd = open(FileName, O_RDONLY);

	if (fd == -1)
		return ft_fileerror;

	ssize_t len = 0;
	char *buff = malloc(buf.st_blksize * sizeof(char));
	*(int*)FieldValue = 1;

	for (blkcnt_t i = 0; i < buf.st_blocks; i++)
	{
		if ((len = read(fd, buff, buf.st_blksize)) > 0)
		{
			for (int c = 0; c < len; c++)
			{
				if (memchr(fields[FieldIndex].bytes, buff[c], fields[FieldIndex].byte_count) == NULL)
				{
					*(int*)FieldValue = 0;
					break;
				}
			}
		}
		else if (len < 0)
		{
			*(int*)FieldValue = 0;
			break;
		}

		if (*(int*)FieldValue == 0)
			break;
	}

	free(buff);
	close(fd);

	return ft_boolean;
}
