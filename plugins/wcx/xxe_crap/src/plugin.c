#include <glib.h>
#include <glib/gstdio.h>
#include <uchardet/uchardet.h>
#include <locale.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define MessageBox dc_extensions->MessageBox
#define PACKERCAPS PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT
#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))

typedef struct
{
	char *ext;
	char *chars;
} EncoderData;

typedef struct
{
	FILE *fp;
	int unixmode;
	size_t offset;
	int encoder_index;
	gboolean is_finished;
	guint16 checksum;
	tProcessDataProc show_progress;
} ArcData;

typedef void *HINSTANCE;

enum
{
	ENC_XXE = 0,
	ENC_UUE,
//	ENC_B64,
	ENC_COUNT
};

static const EncoderData encoders[ENC_COUNT] =
{
	[ENC_XXE] = {".xxe",	  "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"},
	[ENC_UUE] = {".uue",	"`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"},
//	[ENC_B64] = {".b64u",	  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"},
}; //swag

uchardet_t detector;
static tProcessDataProc show_progress = NULL;
static tExtensionStartupInfo* dc_extensions = NULL;

static void update_checksum(guint16 *checksum, unsigned char byte)
{
	guint16 current = *checksum;
	current = (current >> 1) | ((current & 1) << 15);
	current += byte;
	*checksum = current & 0xFFFF;
}

static gboolean process_byte(int mode, unsigned char byte, guint16 *checksum, FILE *fp)
{
	gboolean result = TRUE;

	if (mode == PK_TEST)
		update_checksum(checksum, byte);
	else
	{
		fputc(byte, fp);

		if (ferror(fp))
			result = FALSE;
	}

	return result;
}

static int get_chr_index(char chr, int encoder_index)
{
	if (chr == ' ' && encoder_index == ENC_UUE)
		return 0;

	EncoderData encoder = encoders[encoder_index];

	char *pos = strchr(encoder.chars, chr);

	if (!pos)
		return -1;

	return pos - encoder.chars;
}

static int get_encoder_index_by_ext(char *filename)
{
	char *ext = strrchr(filename, '.');

	if (!ext)
		return -1;

	for (int i = 0; i < ENC_COUNT; i++)
	{
		if (strcasecmp(ext, encoders[i].ext) == 0)
			return i;
	}

	return -1;
}

static int get_org_size(char *line, int encoder_index)
{
	int len = get_chr_index(line[0], encoder_index);

	if (len < 1)
		return 0;

	return len;
}


void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (!dc_extensions)
	{
		dc_extensions = (tExtensionStartupInfo*)malloc(sizeof(tExtensionStartupInfo));
		memcpy(dc_extensions, StartupInfo, sizeof(tExtensionStartupInfo));
		detector = uchardet_new();
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (dc_extensions)
	{
		uchardet_delete(detector);
		free(dc_extensions);
		dc_extensions = NULL;
	}
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData *data = (ArcData*)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		show_progress = pProcessDataProc;
	else
		data->show_progress = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc)
{
	return;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	return (get_encoder_index_by_ext(FileName) != -1);
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	int encoder_index = get_encoder_index_by_ext(ArchiveData->ArcName);

	if (encoder_index != -1)
	{
		FILE *fp = fopen(ArchiveData->ArcName, "r");

		if (fp)
		{
			ArcData *data = (ArcData*)g_new0(ArcData, 1);
			data->fp = fp;
			data->encoder_index = encoder_index;
			return data;
		}
	}

	ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
	return E_SUCCESS;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ArcData *data = (ArcData*)hArcData;

	memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));

	if (!data->is_finished)
	{
		size_t len = 0;
		gint64 pk_size = 0;
		gint64 unpk_size = 0;
		char *line = NULL;
		gboolean is_data_begin = FALSE;

		while (getline(&line, &len, data->fp) != -1)
		{
			if (strncmp(line, "end", 3) == 0)
				is_data_begin = FALSE;

			g_strchomp(line);

			if (!line)
				continue;

			if (is_data_begin)
				pk_size += strlen(line);

			if (!is_data_begin)
			{
				if (strncmp(line, "begin", 5) == 0)
				{
					int pos = 0;

					if (sscanf(line, "begin %o %n", (unsigned int *)&HeaderDataEx->FileAttr, &pos) == 1 && pos > 0)
					{
						len = strlen(line + pos);
						int ret = uchardet_handle_data(detector, line + pos, len);
						uchardet_data_end(detector);

						if (ret != 0)
							g_strlcpy(HeaderDataEx->FileName, line + pos, sizeof(HeaderDataEx->FileName));
						else
						{
							gchar *converted = g_convert(line + pos, len, "UTF-8", uchardet_get_charset(detector), NULL, NULL, NULL);
							g_strlcpy(HeaderDataEx->FileName, converted, sizeof(HeaderDataEx->FileName));
							g_free(converted);
						}

						g_strchomp(HeaderDataEx->FileName);
						g_print("!!! %s, %d, %s\n", HeaderDataEx->FileName, pos, uchardet_get_charset(detector));
						data->offset = ftell(data->fp);
						is_data_begin = TRUE;
					}
				}

				if (strncmp(line, "sum -r/size ", 12) == 0)
					sscanf(line, "sum -r/size %hu/%ld", &data->checksum, &unpk_size);
			}
			else
				unpk_size += get_org_size(line, data->encoder_index);
		}

		HeaderDataEx->PackSizeHigh = (pk_size & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->PackSize = pk_size & 0x00000000FFFFFFFF;
		HeaderDataEx->UnpSizeHigh = (unpk_size & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->UnpSize = unpk_size & 0x00000000FFFFFFFF;
		data->unixmode = HeaderDataEx->FileAttr;
		HeaderDataEx->FileAttr |= 0100000;

		data->is_finished = TRUE;
		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	FILE *out = NULL;
	int result = E_SUCCESS;

	ArcData *data = (ArcData*)hArcData;

	if (Operation != PK_SKIP)
	{
		if (Operation == PK_TEST && data->checksum == 0)
			return E_NOT_SUPPORTED;

		size_t len = 0;
		char *line = NULL;
		guint16 checksum = 0;
		fseek(data->fp, data->offset, SEEK_SET);

		if (Operation == PK_EXTRACT)
		{
			out = fopen(DestName, "wb");

			if (!out)
				return E_EWRITE;
		}

		while (getline(&line, &len, data->fp) != -1)
		{
			if (strncmp(line, "end", 3) == 0)
				break;

			g_strchomp(line);

			if (!line || line[0] == '\0')
				break;

			int expected = get_org_size(line, data->encoder_index);
			char *pos = &line[1];
			int len_out = 0;

			while (*pos && len_out < expected)
			{
				int chr1 = 0, chr2 = 0, chr3 = 0, chr4 = 0;

				if (*pos)
					chr1 = get_chr_index(*pos++, data->encoder_index);

				if (*pos)
					chr2 = get_chr_index(*pos++, data->encoder_index);

				if (*pos)
					chr3 = get_chr_index(*pos++, data->encoder_index);

				if (*pos)
					chr4 = get_chr_index(*pos++, data->encoder_index);

				if (chr1 < 0 || chr2 < 0 || chr3 < 0 || chr4 < 0)
					break;

/*
	-- -- a8 a7 a6 a5 a4 a3  -- -- a2 a1 b8 b7 b6 b5  -- -- b4 b3 b2 b1 c8 c7  -- -- c6 c5 c4 c3 c2 c1
	          chr1                     chr2                     chr3                     chr4
	xx xx a8 a7 a6 a5 a4 a3
	         a >> 2
	                         a4 a3 a2 a1 xx xx xx xx
	                                 a << 4
	                         xx xx xx xx b8 b7 b6 b5
	                                 b >> 4
	                                                  b6 b5 b4 b3 b2 b1 xx xx
	                                                          b << 2
	                                                  xx xx xx xx xx xx c8 c7
	                                                          c >> 6
	                                                                           c8 c7 c6 c5 c4 c3 c2 c1
	                                                                                       c
	00111111 - 3F

	chr1 = (a >> 2) & 0x3F
	chr2 = ((b << 4) | (b >> 4)) & 0x3F
	chr3 = ((b << 2) | (c >> 6)) & 0x3F
	chr4 = c & 0x3F

	-- -- a8 a7 a6 a5 a4 a3  -- -- a2 a1 b8 b7 b6 b5  -- -- b4 b3 b2 b1 c8 c7  -- -- c6 c5 c4 c3 c2 c1
	          chr1                     chr2                     chr3                     chr4
	a8 a7 a6 a5 a4 a3 xx xx  xx xx xx xx xx xx a2 a1
	       chr1 << 2                chr2 >> 4
	                         b8 b7 b6 b5 xx xx xx xx  xx xx xx xx b4 b3 b2 b1
	                                chr2 << 4                chr3 >> 2
	                                                  c8 c7 xx xx xx xx xx xx  xx xx c6 c5 c4 c3 c2 c1
	                                                         chr3 << 6                   chr4

	a = chr1 << 2 | chr2 >> 4
	b = chr2 << 4 | chr3 >> 2
	c = chr3 << 6 | chr4

	hangover_math.jpg
*/

				if (len_out < expected)
				{
					if (!process_byte(Operation, (unsigned char)((chr1 << 2) | (chr2 >> 4)), &checksum, out))
					{
						result = E_EWRITE;
						break;
					}

					len_out++;
				}

				if (len_out < expected)
				{
					if (!process_byte(Operation, (unsigned char)((chr2 << 4) | (chr3 >> 2)), &checksum, out))
					{
						result = E_EWRITE;
						break;
					}

					len_out++;
				}

				if (len_out < expected)
				{
					if (!process_byte(Operation, (unsigned char)((chr3 << 6) | chr4), &checksum, out))
					{
						result = E_EWRITE;
						break;
					}

					len_out++;
				}
			}

			if (data->show_progress(DestName, len_out) == 0)
			{
				result = E_EABORTED;
				break;
			}
		}

		if (result == E_SUCCESS && Operation == PK_TEST && checksum != data->checksum)
			result = E_BAD_DATA;
		else if (result == E_SUCCESS && Operation == PK_EXTRACT)
			g_chmod(DestName, data->unixmode);
	}

	if (out)
		fclose(out);

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData *data = (ArcData*)hArcData;

	if (data)
	{
		fclose(data->fp);
		g_free(data);
	}

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	size_t read;
	struct stat bif;
	gint64 size = 0;
	guint16 checksum = 0;
	unsigned char buf[45];
	int result = E_SUCCESS;
	int encoder_index = get_encoder_index_by_ext(PackedFile);

	if (encoder_index == -1)
		return E_NOT_SUPPORTED;

	EncoderData encoder = encoders[encoder_index];
	gchar *src = g_strdup_printf("%s/%s", SrcPath, AddList);
	FILE *in = fopen(src, "rb");
	stat(src, &bif);
	g_free(src);

	if (!in)
		return E_EREAD;

	FILE *out = fopen(PackedFile, "w");

	if (!out)
	{
		fclose(in);
		return E_EWRITE;
	}

	gchar *name = g_path_get_basename(AddList);
	fprintf(out, "begin %o %s\n", bif.st_mode & 07777, name);
	g_free(name);

	while ((read = fread(buf, 1, 45, in)) > 0)
	{
		fputc(encoder.chars[read], out);
		size += read;

		for (size_t i = 0; i < read; i += 3)
		{
			unsigned int a = buf[i];
			unsigned int b = 0;
			unsigned int c = 0;
			update_checksum(&checksum, buf[i]);

			if (i + 1 < read)
			{
				b = buf[i + 1];
				update_checksum(&checksum, buf[i + 1]);
			}

			if (i + 2 < read)
			{
				c = buf[i + 2];
				update_checksum(&checksum, buf[i + 2]);
			}

			fputc(encoder.chars[(a >> 2) & 0x3F], out);
			fputc(encoder.chars[((a << 4) | (b >> 4)) & 0x3F], out);
			fputc(encoder.chars[((b << 2) | (c >> 6)) & 0x3F], out);
			fputc(encoder.chars[c & 0x3F], out);
		}

		fputc('\n', out);
	}

	if (result == E_SUCCESS)
		fprintf(out, "%c\nend\nsum -r/size %hu/%ld\n", encoder.chars[0], checksum, size);

	fclose(out);
	fclose(in);

	return result;
}

int DCPCALL GetPackerCaps(void)
{
	return PACKERCAPS;
}
