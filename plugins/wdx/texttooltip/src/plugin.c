#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <magic.h>
#include "string.h"
#include "wdxplugin.h"

#define MAXLEN 2048
#define LONGCAT 60
#define LINES MAXLEN / LONGCAT
#define EMPTYLINE " \n"

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex == 0)
	{
		snprintf(FieldName, maxlen - 1, "lines");
		snprintf(Units, maxlen - 1, "1");
		char buf[4];

		for (int i = 2; i <= LINES; i++)
		{
			snprintf(buf, 4, "|%d", i);
			strcat(Units, buf);
		}

		return ft_string;
	}

	return ft_nomorefields;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (FieldIndex == 0)
	{
		magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK);

		if (magic_cookie == NULL)
		{
			fprintf(stderr, "%s: unable to initialize magic library\n", PLUGNAME);
			return ft_fileerror;
		}

		if (magic_load(magic_cookie, NULL) == 0)
		{
			const char *mime = magic_file(magic_cookie, FileName);

			if (strncmp(mime, "text/", 5) == 0)
			{
				magic_close(magic_cookie);
				char buf[LONGCAT];

				FILE *fp;
				int count = 0;
				char *line = NULL;
				size_t len = 0;
				ssize_t lread;
				int count_comma = 0;
				int count_semicol = 0;

				char *text = (char*)FieldValue;
				text[0] = '\0';
				char *ext = strrchr(FileName, '.');
				char sep = ',';
				bool is_csv = (ext && strcasecmp(ext, ".csv") == 0);

				if ((fp = fopen(FileName, "r")) != NULL)
				{
					while ((lread = getline(&line, &len, fp)) != -1)
					{
						if (count == 0)
						{
							if (is_csv)
							{
								for (size_t i = 0; i < lread; i++)
								{
									if (line[i] == ',')
										count_comma++;
									else if (line[i] == ';')
										count_semicol++;
								}

								if (count_semicol > count_comma)
									sep = ';';
							}
						}

						if (count++ == UnitIndex + 1 || count > LINES)
							break;

						if (lread == 1)
						{
							//continue;
							strcat(text, EMPTYLINE);
						}
						else
						{
							if (line[lread - 1] == '\n')
								line[lread - 1] = '\0';

							snprintf(buf, LONGCAT, "%s", line);

							if (is_csv)
								for (size_t i = 0; i < LONGCAT; i++)
								{
									if (buf[i] == sep)
									{
										buf[i] = '\t';
									}
								}

							size_t lbuf = strlen(buf);

							if (lbuf > 3 && lread - 1 > lbuf)
							{
								//printf(">%s<, (len = %d) tail: 0x%2X 0x%2X 0x%2X\n", buf, lbuf, buf[lbuf - 3] & 0xF8, buf[lbuf - 2] & 0xF0, buf[lbuf - 1] & 0xE0);

								if ((buf[lbuf - 1] & 0xE0) == 0xC0)
									buf[lbuf - 1] = '\0';
								else if ((buf[lbuf - 2] & 0xF0) == 0xE0)
									buf[lbuf - 2] = '\0';
								else if ((buf[lbuf - 3] & 0xF8) == 0xF0)
									buf[lbuf - 3] = '\0';
							}

							strcat(text, buf);
							strcat(text, "\n");
						}
					}

					free(line);
					fclose(fp);
				}

				return ft_string;
			}
			else
			{
				magic_close(magic_cookie);
				return ft_fileerror;
			}
		}
		else
		{
			fprintf(stderr, "%s: cannot load magic database - %s\n", PLUGNAME, magic_error(magic_cookie));
			magic_close(magic_cookie);
			return ft_fileerror;
		}
	}

	return ft_fieldempty;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	snprintf(DetectString, maxlen - 1, "%s", DETECT_STRING);
	return 0;
}
