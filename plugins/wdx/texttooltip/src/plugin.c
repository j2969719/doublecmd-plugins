#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <ctype.h>
#include <uchar.h>
#include <enca.h>
#include <iconv.h>
#include <magic.h>
#include "string.h"
#include "wdxplugin.h"

#define MAXLEN 2048
#define LONGCAT 61
#define LINES MAXLEN / LONGCAT
#define EMPTYLINE " \n"
#define EMPTYENC "???"
#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))
#define FIELDCOUNT ARRAY_SIZE(gFields)

char gLang[3] = "en";
char gUnits[MAXLEN] = "";
char gMultiChoice[MAXLEN] = "";

const char *gFields[] =
{
	"lines",
	"lines (skip blank)",
	"only line",
	"only line (skip blank)",
	"lines (detect encoding)",
	"lines (skip blank and detect encoding)",
	"only line (detect encoding)",
	"only line (skip blank and detect encoding)",
	"detected encoding",
};

size_t UTF16len(WCHAR* src)
{
	if (!src)
		return -1;

	size_t len = 0;
	char16_t *str = (char16_t*)src;

	while (*str != u'\0')
	{
		len++;
		str++;
	}

	return len;
}

size_t UTF16cpy(void *dst, char *src, size_t maxlen)
{
	if (!src || !dst)
		return -1;

	size_t ret;
	size_t pos = 0;
	size_t result = 0;
	mbstate_t state;
	size_t len = strlen(src);
	char16_t *out = (char16_t*)dst;
	memset(&state, 0, sizeof(state));

	for (size_t i = 0; i < maxlen - 1; i++)
	{
		char16_t chr;
		ret = mbrtoc16(&chr, &src[pos], len - pos, &state);

		if (ret == (size_t) -3)
		{
			pos += ret;
		}
		else if (ret == 0)
		{
			pos++;
		}
		else if (ret != (size_t) -1 && ret != (size_t) -2)
		{
			out[i] = chr;
			pos += ret;
			result++;
		}

		if (pos > len)
			break;
	}

	out[result + 1] = u'\0';

	return result;
}

size_t UTF16ncat(void *dst, char *src, size_t maxlen, size_t dstlen)
{
	if (!src || !dst)
		return -1;

	size_t result = 0;
	char16_t *out = (char16_t*)dst;
	size_t len = UTF16len(out);

	if (len < dstlen)
		result = UTF16cpy(out + len, src, maxlen != -1 ? maxlen : dstlen);

	return result;
}

size_t UTF16cat(void *dst, char *src, size_t dstlen)
{
	return UTF16ncat(dst, src, -1, dstlen);
}

size_t UTF16toUTF8(char *dst, WCHAR *src, size_t maxlen)
{
	if (!src || !dst)
		return -1;

	size_t ret;
	size_t result = 0;
	mbstate_t state;
	char16_t *in = (char16_t*)src;
	char buf[MB_CUR_MAX];
	memset(dst, 0, maxlen);
	memset(&state, 0, sizeof(state));

	for (size_t i = 0; in[i] != u'\0'; i++)
	{
		memset(buf, 0, MB_CUR_MAX);

		if ((ret = c16rtomb(buf, in[i], &state)) != (size_t) -1)
		{
			if (result + ret < maxlen)
			{
				result += ret;
				strcat(dst, buf);
			}
			else
				break;
		}
	}

	return result;
}

void strupr(char *str)
{
	char *p = str;

	while (*p)
	{
		*p = toupper(*p);
		p++;
	}
}

bool check_if_blank(char *line)
{
	char *p = line;

	while (*p)
	{
		if (*p != ' ' && *p != '\t')
			return false;

		p++;
	}

	return true;
}

void chomp_endl(char *line, size_t lread)
{
	if (lread >= 2 && (line[lread - 2] == '\n' || line[lread - 2] == '\r'))
		line[lread - 2] = '\0';
	else if (lread >= 1 && (line[lread - 1] == '\n' || line[lread - 1] == '\r'))
		line[lread - 1] = '\0';
}

void chomp_line(char *line, size_t len, char *buf, size_t outlen)
{
	snprintf(buf, outlen, "%s", line);
	size_t lbuf = strlen(buf);

	if (lbuf > 3 && len > lbuf)
	{
		if ((buf[lbuf - 1] & 0xE0) == 0xC0)
			buf[lbuf - 1] = '\0';
		else if ((buf[lbuf - 2] & 0xF0) == 0xE0)
			buf[lbuf - 2] = '\0';
		else if ((buf[lbuf - 3] & 0xF8) == 0xF0)
			buf[lbuf - 3] = '\0';
	}
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= FIELDCOUNT)
		return ft_nomorefields;

	if (gMultiChoice[0] == '\0' && FieldIndex > 1)
		return ft_nomorefields;

	snprintf(FieldName, maxlen - 1, gFields[FieldIndex]);

	if (FieldIndex < 8)
		snprintf(Units, maxlen - 1, gUnits);
	else
	{
		snprintf(Units, maxlen - 1, "%s", gMultiChoice);
		return ft_multiplechoice;
	}

	return ft_stringw;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (FieldIndex >= 0 && FieldIndex < FIELDCOUNT)
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

				FILE *fp;
				int count = 0;
				char *line = NULL;
				size_t len = 0;
				ssize_t lread;
				int count_comma = 0;
				int count_semicol = 0;
				char buf[LONGCAT * 3];
				char out_buf[MAXLEN * 2];
				char enc[LONGCAT] = "";
				char *text = (char*)FieldValue;
				memset(text, u'\0', maxlen);
				char *ext = strrchr(FileName, '.');
				char sep = ',';
				bool is_csv = (ext && strcasecmp(ext, ".csv") == 0);
				bool is_oneline = (strncasecmp(gFields[FieldIndex], "only", 4) == 0);
				bool is_detect = (FieldIndex > 3);
				bool is_onlyenc = (FieldIndex == 8);
				bool is_skipblank = (FieldIndex % 2 != 0);

				if (is_detect)
				{
					magic_cookie = magic_open(MAGIC_MIME_ENCODING | MAGIC_SYMLINK);

					if (magic_cookie && magic_load(magic_cookie, NULL) == 0)
					{
						strcpy(enc, magic_file(magic_cookie, FileName));
						magic_close(magic_cookie);

						if (strncmp(enc, "utf", 3) == 0)
						{
							strupr(enc);

							if (is_onlyenc)
							{
								strcpy((char*)FieldValue, enc);
								return ft_multiplechoice;
							}
							else
								is_detect = false;
						}
						else if (strcmp(enc, "us-ascii") == 0)
						{
							if (is_onlyenc)
							{
								strcpy((char*)FieldValue, "ASCII");
								return ft_multiplechoice;
							}
							else
							{
								is_detect = false;
								enc[0] = '\0';
							}
						}
					}
				}

				if ((fp = fopen(FileName, "r")) != NULL)
				{
					if (is_detect)
					{
						EncaAnalyser analyser = enca_analyser_alloc(gLang);

						if (analyser)
						{
							if ((len = fread(out_buf, sizeof(out_buf), 1, fp)) > 0)
							{
								enca_set_threshold(analyser, 1.38);
								enca_set_multibyte(analyser, 1);
								enca_set_ambiguity(analyser, 1);
								enca_set_garbage_test(analyser, 1);
								enca_set_filtering(analyser, 0);

								EncaEncoding encoding = enca_analyse(analyser,
								                                     (unsigned char*)out_buf,
								                                     (size_t)sizeof(enc));
								snprintf(enc, sizeof(enc), "%s",
								         enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV));
								enca_analyser_free(analyser);
							}

							if (is_onlyenc)
							{
								fclose(fp);

								if (enc[0] == '\0')
									strcpy((char *)FieldValue, EMPTYENC);
								else
									strcpy((char*)FieldValue, enc);

								return ft_multiplechoice;
							}

							fseek(fp, 0, SEEK_SET);
						}
					}

					iconv_t conv = (iconv_t) -1;

					if (enc[0] != '\0' && strcmp(enc, "ASCII") != 0 && strcmp(enc, "UTF-8") != 0)
						conv = iconv_open("UTF-8", enc);

					if (conv != (iconv_t) -1)
						printf("%s (%s): charset %s (lang %s)\n", PLUGNAME, FileName, enc, gLang);

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

						chomp_endl(line, lread);
						bool is_blank = check_if_blank(line);

						if (is_oneline)
						{
							if (count == UnitIndex + 1)
								break;

							if (is_skipblank && !is_blank)
								count++;
							else if (!is_skipblank)
								count++;

							if (count < UnitIndex + 1)
								continue;
						}
						else if (count++ == UnitIndex + 1 || count > LINES)
							break;

						if (is_blank)
						{
							if (!is_oneline && is_skipblank)
							{
								count--;
								continue;
							}
							else
								UTF16cat(text, EMPTYLINE, maxlen);
						}
						else
						{

							chomp_line(line, strlen(line), buf, sizeof(buf));

							if (is_csv)
								for (size_t i = 0; i < LONGCAT; i++)
								{
									if (buf[i] == sep)
										buf[i] = '\t';
								}

							if (conv != (iconv_t) -1)
							{
								char *in = buf;
								char *out = out_buf;
								size_t inlen = strlen(buf);
								size_t outlen = sizeof(out_buf);

								if (iconv(conv, &in, &inlen, &out, &outlen) != -1)
								{
									*out = '\0';
									UTF16ncat(text, out_buf, LONGCAT, maxlen);
								}
								else
									perror(PLUGNAME " (iconv)");
							}
							else
								UTF16ncat(text, buf, LONGCAT, maxlen);

							UTF16cat(text, "\n", maxlen);
						}
					}

					if (conv != (iconv_t) -1)
						iconv_close(conv);

					free(line);
					fclose(fp);

					return ft_stringw;
				}
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

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	char buf[24];
	snprintf(gLang, sizeof(gLang), "%s", setlocale(LC_ALL, ""));
	sprintf(gUnits, "1");

	for (int i = 2; i <= LINES; i++)
	{
		snprintf(buf, sizeof(buf), "|%d", i);
		strcat(gUnits, buf);
	}

	size_t len;
	int *ecns = enca_get_language_charsets(gLang, &len);

	if (len > 0)
	{
		strcpy(gMultiChoice, "UTF-8");

		for (int i = 0; i <= len; i++)
		{
			snprintf(buf, sizeof(buf), "|%s", enca_charset_name(ecns[i], ENCA_NAME_STYLE_ICONV));
			strcat(gMultiChoice, buf);
		}
	}
}
