#define _GNU_SOURCE
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <ctype.h>
#include <fcntl.h>
#include <locale.h>
#include <dirent.h>
#include <dlfcn.h>
#include <ftw.h>
#include <time.h>
#include <utime.h>
#include <math.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

#define MAX_MULTIKEYS 50
#define MAX_FILE_RANGE 2 * 1024 * 1024
#define HUP_TIMEOUT 1
#define BUFSIZE 256
#define FILEBUFSIZE 8192
#define MAX_LINEBUF PATH_MAX + MAX_PATH
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox
#define E_HANDLED -32769
#define APPEND_DST(X) X, sizeof(X) - 1
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ADDON_GET_STRING(X, Y) unquote_value(g_key_file_get_value(gCfg, X, Y, NULL), '"')
#define EXEC_SEP "< < < < < < < < < < < < < < < < < < < < < < < < < > > > > > > > > > > > > > > > > > > > > > > > > >"
#define LFM_CFG "/lfms/config.lfm"
#define LFM_ASK "/lfms/ask_opts.lfm"
#define LFM_IMPORT "/lfms/import.lfm"
#define MSG_PASSWD "Enter password"
#define MSG_TIMEOUT "Timeout expired, kill process?"
#define MSG_LFM_MISSIND "LFM file is missing."

enum
{
	MAF_UNIX_PATH = 1 << 0,
	MAF_WIN_PATH  = 1 << 1,
	MAF_UNIX_ATTR = 1 << 2,
	MAF_WIN_ATTR  = 1 << 3,
};

enum
{
	MAF_TARBALL   = 1 << 0,
	MAF_HIDE      = 1 << 1,
};

enum
{
	ASK_ONCE      = 1 << 0,
	ASK_PACK      = 1 << 1,
	ASK_UNPACK    = 1 << 2,
	ASK_LIST      = 1 << 3,
	ASK_DELETE    = 1 << 4,
	ASK_MOVE      = 1 << 5,
	ASK_TEST      = 1 << 6,
	ASK_MULTIVOL  = 1 << 7,
	ASK_LISTPASS  = 1 << 8,
	ASK_ASKED     = 1 << 9,
};

enum
{
	ARG_QUOTESPC = 1 << 0,
	ARG_QUOTEALL = 1 << 1,
	ARG_UNIXSEP  = 1 << 2,
	ARG_ALLWIN   = 1 << 3,
	ARG_ALLNONE  = 1 << 4,
	ARG_ALLUNIX  = 1 << 5,
	ARG_BASENAME = 1 << 6,
	ARG_DIRNAME  = 1 << 7,
	ARG_ANSIENC  = 1 << 8,
	ARG_OEMENC   = 1 << 9,
	ARG_UTFENC   = 1 << 10,
	ARG_FILEONLY = 1 << 11,
};

enum
{
	FORM_SIZE     = 1 << 0,
	FORM_PKSIZE   = 1 << 1,
	FORM_DAY      = 1 << 2,
	FORM_MONTH    = 1 << 3,
	FORM_YEAR     = 1 << 4,
	FORM_HOUR     = 1 << 5,
	FORM_MIN      = 1 << 6,
	FORM_SEC      = 1 << 7,
	FORM_YEARTIME = 1 << 8,
	FORM_FILLDATE = 1 << 9,
	FORM_LAZY     = 1 << 10,
	FORM_WINSEP   = 1 << 11,
	FORM_ATTR     = 1 << 12,
	FORM_UNIXATTR = 1 << 13,
	FORM_HUMAN    = 1 << 14,
};

enum
{
	ENC_DEF,
	ENC_ANSI,
	ENC_OEM,
};

enum
{
	OP_LIST,
	OP_UNPACK,
	OP_UNPACK_PATH,
	OP_PACK,
	OP_SFX,
	OP_MOVE,
	OP_DELETE,
	OP_TEST,
	OP_LAST
};

typedef struct sListPattern
{
	char *pattern;
	GRegex *re;
} tListPattern;

typedef struct sAskData
{
	char *addon;
	char *key_prefix;
	int mode;
	gboolean show_cheatsheet;
	char opts[MAX_PATH];
} tAskData;

typedef struct sArcData
{
	gchar *addon;
	gchar *archive;
	gchar *archiver;

	guint cur;
	guint count;
	guint form_index;

	gchar **months;
	gchar **units;
	gchar *units_cache;
	gchar *pass_str;
	gchar *progress_mask;
	gboolean tarball;

	gchar *itemlist;
	gchar *filelist;
	gchar *command;
	gchar *tempdir;
	gchar *dirlst;
	gchar **dirs;

	int op_type;
	int enc;
	int error_level;
	int form_caps;
	gboolean batch;
	gboolean wine;
	gboolean debug;
	gboolean ignore_err;
	char pass[MAX_PATH];
	char strip_chars[20];

	GPtrArray *lines;

	GPtrArray *list_patterns;
	GPtrArray *ignore_stings;
	gchar *list_start;
	gchar *list_end;
	GRegex *list_start_re;
	GRegex *list_end_re;
	GRegex *basedir_re;
	gboolean process_lines;

	char cur_name[1024];
	char cur_ext[1024];
	char cur_pksize[20];
	char cur_size[20];
	char cur_attr[20];
	char cur_day[3];
	char cur_month[20];
	char cur_year[6];
	char cur_hour[3];
	char cur_min[3];
	char cur_sec[3];
	gboolean cur_pm;
	gboolean cur_dot;
	gint64 pksize_mult;
	gint64 size_mult;

	char basedir[1024];
	char lastitem[1024];
	char destpath[1024];

	struct stat st;
	clock_t clock_start;

	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

tChangeVolProc gChangeVolProc  = NULL;
tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;
static gchar gLFMCfg[PATH_MAX];
static gchar gLFMAsk[PATH_MAX];
static gchar gLFMImport[PATH_MAX];
gchar *gConfName = NULL;
gchar *gEncodingOEM = NULL;
gchar *gEncodingANSI = NULL;
GKeyFile *gCfg = NULL;
GKeyFile *gCache = NULL;

int gTimeout = 10;
int gDebugFd = -1;
gchar *gDebugFileName = NULL;
const char *gHome = NULL;
static gchar gWineDrive[3] = "Z:";
static gchar *gWinePrefix = NULL;
static gchar *gWineBin = NULL;
gboolean gWineArchWin32 = FALSE;
gboolean gWineServerCalled = FALSE;


char *gUnits[] =
{
	"",
	"K",
	"M",
	"G",
	"T"
};

char *gMonths[] =
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

static void append_char(char c, char *dest, int dest_size)
{
	int len = (int)strlen(dest);

	if (len < dest_size)
	{
		dest[len] = c;
		dest[len + 1] = '\0';
	}
}

static void append_str(char *src, char *dest, int dest_size)
{
	int len = dest_size - (int)strlen(dest);
	strncat(dest, src, len);
}

static size_t append_not_blank(gchar *line, size_t line_pos, size_t line_len, char *dest, int dest_size)
{
	while (line_pos < line_len)
	{
		if (isblank(line[line_pos]))
			break;

		append_char(line[line_pos], dest, dest_size);

		line_pos++;
	}

	return line_pos;
}

static size_t append_digit(gchar c, size_t line_pos, gboolean debug, char *dest, int dest_size)
{
	if (isdigit(c))
		append_char(c, dest, dest_size);
	else if (!isspace(c))
	{
		if (debug)
			dprintf(gDebugFd, "\nRejected! (position %ld: '%c' received)", line_pos, c);

		line_pos = -1;
	}

	return line_pos;
}

static size_t append_digits(gchar *line, size_t line_pos, size_t line_len, gchar *strip_chars, int form_dest, int form_caps, char *dest, int dest_size)
{
	gboolean is_break = FALSE;

	while (line_pos < line_len)
	{
		if (isblank(line[line_pos]))
			break;

		if (isdigit(line[line_pos]))
			append_char(line[line_pos], dest, dest_size);
		else if (form_dest == FORM_YEAR && form_caps & FORM_YEARTIME && line[line_pos] == ':')
			append_char(line[line_pos], dest, dest_size);
		else if (form_dest == FORM_SIZE)
		{
			if (form_caps & FORM_HUMAN && (line[line_pos] == '.' || line[line_pos] == ','))
				append_char('.', dest, dest_size);
			else if (strchr(strip_chars, line[line_pos]) == NULL && line[line_pos] != '-')
				is_break = TRUE;
		}
		else
			is_break = TRUE;

		line_pos++;

		if (is_break)
			break;
	}

	return line_pos;
}

static void replace_char(gchar *string, char wut, char with)
{
	size_t len = strlen(string);

	for (size_t i = 0; i < len; i++)
	{
		if (string[i] == wut)
			string[i] = with;
	}
}

static void replace_win_sep(gchar *filename)
{
	replace_char(filename, '\\', '/');
}

static void replace_nix_sep(gchar *filename)
{
	replace_char(filename, '/', '\\');
}

static unsigned char* id_to_uchar(char* str, size_t *res_size)
{
	char *p;
	size_t i = 0;
	unsigned int chr;

	size_t len = strlen(str);

	if (len < 2)
		return NULL;

	*res_size = (len + 1) / 3;
	unsigned char *result = (unsigned char*)malloc(*res_size);

	for (p = str; *p; p += 3, i++)
	{
		if (sscanf(p, "%02X", &chr) != 1)
			break;

		result[i] = (unsigned char)chr;
	}

	return result;
}

static void debug_dump_char(char c, size_t bak_pos, size_t line_pos)
{
	size_t len = line_pos - bak_pos;

	if (len > 0)

		for (size_t i = 0; i < len; i++)
			dprintf(gDebugFd, "%c", c);
}

static int copy_local_file(char *src, char *dst, tProcessDataProc ProcessDataProc)
{
	ssize_t len;
	int ifd, ofd;
	char buff[FILEBUFSIZE];
	struct utimbuf ubuf;
	struct stat st;
	int result = 0;

	if (strcmp(src, dst) == 0)
	{
		dprintf(gDebugFd, "ERROR copy_local_file(%s, %s): src = dst\n", src, dst);
		return E_NOT_SUPPORTED;
	}

	if (stat(src, &st) != 0)
	{
		int errsv = errno;
		dprintf(gDebugFd, "ERROR copy_local_file(%s, %s): stat src %s\n", src, dst, strerror(errsv));
		return E_EREAD;
	}

	ifd = open(src, O_RDONLY);

	if (ifd == -1)
	{
		int errsv = errno;
		dprintf(gDebugFd, "ERROR copy_local_file(%s, %s): open src %s\n", src, dst, strerror(errsv));
		return E_EREAD;
	}

	ofd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (ofd > -1)
	{

		while ((len = read(ifd, buff, sizeof(buff))) > 0)
		{
			if (write(ofd, buff, len) == -1)
			{
				int errsv = errno;
				dprintf(gDebugFd, "ERROR copy_local_file(%s, %s): write buffer %s\n", src, dst, strerror(errsv));
				result = E_EWRITE;
				remove(dst);
				break;
			}

			if (ProcessDataProc && ProcessDataProc(dst, 0) == 0)
			{
				result = E_EABORTED;
				remove(dst);
				break;
			}

		}

		close(ofd);
		chmod(dst, st.st_mode);
		ubuf.actime = st.st_atime;
		ubuf.modtime = st.st_mtime;
		utime(dst, &ubuf);
	}
	else
	{
		int errsv = errno;
		dprintf(gDebugFd, "ERROR copy_local_file(%s, %s): open dst %s\n", src, dst, strerror(errsv));
		result = E_EWRITE;
	}

	close(ifd);

	return result;
}


static void remove_file(const char *file)
{
	if (remove(file) == -1)
	{
		int errsv = errno;
		dprintf(gDebugFd, "ERROR remove_file(%s): %s\n", file, strerror(errsv));
	}
}

static int nftw_remove_cb(const char *file, const struct stat *st, int tflag, struct FTW *ftwbuf)
{
	remove_file(file);
	return 0;
}

static void remove_target(const char *filename)
{
	struct stat st;

	if (lstat(filename, &st) == 0)
	{
		if S_ISDIR(st.st_mode)
			nftw(filename, nftw_remove_cb, 13, FTW_DEPTH | FTW_PHYS);
		else
			remove_file(filename);
	}
}

static gchar* get_dirname_from_list(gchar **list)
{
	if (!list)
		return NULL;

	guint len = g_strv_length(list);

	if (len == 1)
		return g_path_get_dirname(list[0]);

	gchar *result = NULL;
	gchar *first = g_strdup(list[0]);
	size_t first_len = strlen(first);

	for (guint i = 1; i < len; i++)
	{
		size_t pos = 0;
		gchar *path = list[i];

		while (*path != '\0')
		{
			if (first_len == pos)
				break;

			if (*path != first[pos])
				first[pos] = '\0';

			pos++;
			path++;
		}
	}

	first_len = strlen(first);

	if (first[first_len - 1] == '/')
	{
		first[first_len - 1] = '\0';
		result = first;
	}
	else
	{
		result = g_path_get_dirname(first);
		g_free(first);
	}

	return result;
}

static gboolean clone_file(gchar *filename, gchar *srcdir, gchar *tempdir, gchar *subdir)
{
	gboolean result = TRUE;

	gchar *src = g_strdup_printf("%s%s", srcdir, filename);
	gchar *dst = NULL;

	if (subdir)
		dst = g_strdup_printf("%s%s/%s", tempdir, subdir, filename);
	else
		dst = g_strdup_printf("%s/%s", tempdir, filename);

	if (link(src, dst) == -1)
	{
		int errsv = errno;
		dprintf(gDebugFd, "ERROR link (%s, %s): %s\n", src, dst, strerror(errsv));

		if ((errsv = copy_local_file(src, dst, gProcessDataProc)) != 0)
		{
			if (errsv != 0)
				result = FALSE;
		}
	}

	g_free(src);
	g_free(dst);

	return result;
}

static gboolean clone_tree(gchar *filename, gchar *srcdir, gchar *tempdir, gchar *subdir)
{
	DIR *dir;
	gboolean result = TRUE;
	struct dirent *ent;

	gchar *src = g_strdup_printf("%s%s", tempdir, filename);

	if ((dir = opendir(src)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
			{
				gchar *name = NULL;

				if (ent->d_type == DT_REG)
				{
					name = g_strdup_printf("%s%s", filename, ent->d_name);
					result = clone_file(name, srcdir, tempdir, subdir);
				}
				else if (ent->d_type == DT_DIR)
				{
					name = g_strdup_printf("%s%s/", filename, ent->d_name);

					gchar *dstdir = NULL;

					if (subdir)
						dstdir = g_strdup_printf("%s%s/%s", tempdir, subdir, name);
					else
						dstdir = g_strdup_printf("%s/%s", tempdir, name);

					result = (g_mkdir_with_parents(dstdir, 0755) != -1);
					g_free(dstdir);

					if (result)
						result = clone_tree(name, srcdir, tempdir, subdir);
				}

				g_free(name);

				if (result == FALSE)
					break;
			}
		}

		closedir(dir);
	}

	g_free(src);

	return result;
}


static gboolean build_temp_tree(gchar *tempdir, gchar **list, gchar *srcdir, gchar *subdir)
{
	gboolean result = TRUE;

	while (*list != NULL)
	{
		gchar *dir = g_path_get_dirname(*list);

		if (dir)
		{
			gboolean is_root = (strcmp(dir, ".") == 0);

			if (!subdir && is_root)
				g_free(dir);
			else
			{

				gchar *pathname = NULL;

				if (!subdir)
					pathname = g_strdup_printf("%s/%s", tempdir, dir);
				else if (is_root)
					pathname = g_strdup_printf("%s/%s", tempdir, subdir);
				else
					pathname = g_strdup_printf("%s/%s/%s", tempdir, subdir, dir);

				if (g_mkdir_with_parents(pathname, 0755) == -1)
				{
					int errsv = errno;
					dprintf(gDebugFd, "ERROR g_mkdir_with_parents (%s): %s\n", pathname, strerror(errsv));
					result = FALSE;
					g_free(pathname);
					g_free(dir);
					break;
				}

				g_free(pathname);
				g_free(dir);
			}
		}

		if (srcdir)
		{
			gchar *name = *list;

			if (name[strlen(name) - 1] == '/')
				result = clone_tree(name, srcdir, tempdir, subdir);
			else
				result = clone_file(name, srcdir, tempdir, subdir);

			if (result == FALSE)
				break;
		}

		list++;
	}

	return result;
}

static void set_list_subdir(gchar **list, gchar *subdir)
{
	while (*list != NULL)
	{
		gchar *oldpath = *list;
		*list = g_strdup_printf("%s/%s", subdir, oldpath);
		g_free(oldpath);
		list++;
	}
}

static gchar* make_tempdir(gchar *basedir, gchar *archive)
{
	gchar *templ = g_strdup_printf("%s/%.30s_XXXXXX", basedir, archive);
	return g_mkdtemp(templ);
}

static gchar* make_tree_temp_copy(char *srcdir, char *subdir, gchar **src_list, gchar *archive)
{
	gchar *result = NULL;

	result = make_tempdir(srcdir, archive);

	if (result)
		build_temp_tree(result, src_list, srcdir, subdir);

	return result;
}


static gchar* get_output(gchar *command)
{
	gchar *result;
	GError *err = NULL;

	if (!g_spawn_command_line_sync(command, &result, NULL, NULL, &err))
	{
		dprintf(gDebugFd, "ERROR (%s): %s\n", command, err->message);
		g_error_free(err);
		return NULL;
	}

	return result;
}

static void build_units_cache(ArcData data)
{
	gchar *string = NULL;

	if (data->units)
		string = g_strjoinv("", data->units);
	else
		string = g_strjoinv("", gUnits);

	if (string)
	{
		data->units_cache = g_ascii_strdown(string, -1);
		g_free(string);
	}
}

static gchar** addon_get_custom_crap(gchar *addon, const char *key, int expected_len)
{
	gchar **result = NULL;

	gchar *string = g_key_file_get_value(gCfg, addon, key, NULL);

	if (string)
	{
		result = g_strsplit(string, ",", -1);
		g_free(string);

		if (g_strv_length(result) != expected_len)
		{
			g_strfreev(result);
			result = NULL;
		}
	}

	return result;
}

static gdouble get_filesize(gchar *string, gint64 mult, gchar **custom_units)
{
	gchar *end = NULL;
	gdouble result = g_ascii_strtod(string, &end);

	if (end)
	{
		g_strstrip(end);

		if (end[0] != '\0')
		{
			gchar **units = gUnits;

			if (custom_units)
				units = custom_units;

			for (int i = 0; i < ARRAY_SIZE(gUnits); i++)
			{
				if (strcasecmp(end, units[i]) == 0)
				{
					mult = (gint64)pow(1024, i);
					break;
				}
			}
		}
	}

	if (mult > 0)
		result = result * mult;

	return result;
}

static int month_to_num(ArcData data)
{
	if (isdigit(data->cur_month[0]))
		return atoi(data->cur_month) - 1;

	int month = -1;

	gchar **months = gMonths;

	if (data->months)
		months = data->months;

	for (int i = 0; i < 12; i++)
	{
		if (strcmp(data->cur_month, months[i]) == 0)
		{
			month = i;
			break;
		}
	}

	return month;
}

static gboolean value_is_quoted(gchar *string, char chr, size_t last)
{
	if (last < 2)
		return FALSE;

	return (string[0] == chr && string[last] == chr);
}

static gchar* quote_and_free(gchar *string)
{
	gchar *result = g_strdup_printf("\"%s\"", string);
	g_free(string);
	return result;
}

static gchar* unquote_value(gchar *string, char chr)
{
	gchar *result = NULL;

	if (!string)
		return NULL;

	g_strchomp(string);

	size_t last = strlen(string) - 1;

	if (value_is_quoted(string, chr, last))
	{
		string[last] = '\0';
		result = g_strdup(string + 1);
		g_free(string);
	}
	else
		result = string;

	return result;
}

static gchar* string_convert_encoding(gchar *string, int encoding)
{
	gchar *result = NULL;

	if (encoding != ENC_DEF)
	{
		gchar *enc_string = NULL;

		if (encoding == ENC_ANSI)
			enc_string = gEncodingANSI;
		else
			enc_string = gEncodingOEM;

		result = g_convert_with_fallback(string, -1, enc_string, "UTF-8", NULL, NULL, NULL, NULL);
	}

	if (result == NULL)
		result = string;
	else
		g_free(string);

	return result;
}

static gchar* string_add_line(gchar *string, gchar *line, gchar *sep)
{
	gchar *result = NULL;

	if (!string)
		result = line;
	else
	{
		result = g_strdup_printf("%s%s%s", string, sep, line);
		g_free(string);
		g_free(line);
	}

	return result;
}

static void destroy_pattern_item(gpointer data)
{
	tListPattern *item = (tListPattern*)data;

	if (!item)
		return;

	g_free(item->pattern);

	if (item->re)
		g_regex_unref(item->re);

	g_free(item);
}

static gboolean set_regex_from_string(gchar *string, tListPattern *item, ArcData data)
{
	GError *err = NULL;
	gboolean result = FALSE;

	GRegexCompileFlags compile_flags = G_REGEX_DEFAULT;
	GRegexCompileFlags match_flags = G_REGEX_MATCH_DEFAULT;
	gchar *pattern = g_strdup(string + 1);
	char *end = strchr(pattern, '/');

	if (end != NULL)
	{
		*end = '\0';

		while (*++end)
		{
			switch (*end)
			{
			case 'i':
				compile_flags |= G_REGEX_CASELESS;
				break;

			case 'e':
				compile_flags |= G_REGEX_EXTENDED;
				break;

			case 'r':
				compile_flags |= G_REGEX_RAW;
				break;

			default:
				dprintf(gDebugFd, "ERROR (%s): flag %c is not supported\n", string, *end);
			}
		}
	}

	if (data->debug && compile_flags != G_REGEX_DEFAULT)
	{
		dprintf(gDebugFd, "regex compile_flags (%s): ", pattern);

		if (compile_flags & G_REGEX_CASELESS)
			dprintf(gDebugFd, "G_REGEX_CASELESS ");

		if (compile_flags & G_REGEX_EXTENDED)
			dprintf(gDebugFd, "G_REGEX_EXTENDED ");

		if (compile_flags & G_REGEX_RAW)
			dprintf(gDebugFd, "G_REGEX_RAW ");

		dprintf(gDebugFd, "\n");
	}

	GRegex *re = g_regex_new(pattern, compile_flags, match_flags, &err);

	if (err)
	{
		dprintf(gDebugFd, "ERROR (%s): %s\n", string, err->message);
		g_error_free(err);
		g_free(string);
		g_free(pattern);
		return result;
	}
	else
		result = TRUE;

	g_free(string);
	item->pattern = pattern;
	item->re = re;

	return result;
}

static void listform_update_caps(gchar *pattern, gboolean is_re, ArcData data)
{
	if (is_re)
	{
		if (strstr(pattern, "mYear") != NULL)
			data->form_caps |= FORM_YEAR;

		if (strstr(pattern, "mMonth") != NULL)
			data->form_caps |= FORM_MONTH;

		if (strstr(pattern, "mDay") != NULL)
			data->form_caps |= FORM_DAY;

		if (strstr(pattern, "mHour") != NULL)
			data->form_caps |= FORM_HOUR;

		if (strstr(pattern, "mMin") != NULL)
			data->form_caps |= FORM_MIN;

		if (strstr(pattern, "mSec") != NULL)
			data->form_caps |= FORM_SEC;

		if (strstr(pattern, "size") != NULL)
			data->form_caps |= FORM_SIZE;

		if (strstr(pattern, "packedSize") != NULL)
			data->form_caps |= FORM_PKSIZE;

		if (strstr(pattern, "attr") != NULL)
			data->form_caps |= FORM_ATTR;

		build_units_cache(data);
	}
	else
	{
		if (strchr(pattern, 'y') != NULL)
			data->form_caps |= FORM_YEAR;

		if (strchr(pattern, 't') != NULL || strchr(pattern, 'T') != NULL)
			data->form_caps |= FORM_MONTH;

		if (strchr(pattern, 'd') != NULL)
			data->form_caps |= FORM_DAY;

		if (strchr(pattern, 'h') != NULL)
			data->form_caps |= FORM_HOUR;

		if (strchr(pattern, 'm') != NULL)
			data->form_caps |= FORM_MIN;

		if (strchr(pattern, 's') != NULL)
			data->form_caps |= FORM_SEC;

		if (strchr(pattern, 'z') != NULL)
			data->form_caps |= FORM_SIZE;

		if (strchr(pattern, 'p') != NULL)
			data->form_caps |= FORM_PKSIZE;

		if (strchr(pattern, 'Z') != NULL || strchr(pattern, 'P') != NULL)
		{
			data->form_caps |= FORM_HUMAN;
			build_units_cache(data);
		}

		if (strchr(pattern, 'a') != NULL)
			data->form_caps |= FORM_ATTR;
	}

	if (data->form_caps & FORM_YEAR && data->form_caps & FORM_MONTH &&
	                data->form_caps & FORM_DAY && data->form_caps & FORM_HOUR &&
	                data->form_caps & FORM_MIN && data->form_caps & FORM_SEC)
		data->form_caps |= FORM_FILLDATE;
}

static gboolean check_if_pattern_regex(gchar *pattern, size_t len)
{
	return (len > 3 && pattern[0] == '/' && strchr(pattern + 1, '/') != NULL);
}

static gboolean set_listform(gchar *addon, ArcData data)
{
	gboolean result = FALSE;
	char key[23];

	data->units = addon_get_custom_crap(addon, "SizeUnits", ARRAY_SIZE(gUnits));
	data->months = addon_get_custom_crap(addon, "Months", 12);
	data->list_patterns = g_ptr_array_new_with_free_func(destroy_pattern_item);

	for (int i = 0; i <= MAX_MULTIKEYS; i++)
	{
		snprintf(key, sizeof(key), "Format%d", i);
		gchar *value = ADDON_GET_STRING(addon, key);

		if (!value)
			break;

		result = TRUE;

		size_t len = strlen(value);

		if (len == 0 && data->debug)
			dprintf(gDebugFd, "WARNING: %s is empty.\n", key);

		tListPattern *item = g_new0(tListPattern, 1);

		gboolean is_re = check_if_pattern_regex(value, len);

		if (is_re)
		{
			if (!set_regex_from_string(value, item, data))
			{
				g_free(item);
				g_free(data->units);
				g_free(data->months);
				data->units = NULL;
				data->months = NULL;

				if (data->list_patterns)
					g_ptr_array_free(data->list_patterns, TRUE);

				result = FALSE;
				break;
			}
		}
		else
			item->pattern = value;

		listform_update_caps(item->pattern, is_re, data);
		g_ptr_array_add(data->list_patterns, (gpointer)item);
	}

	if (!result)
		return result;

	data->ignore_stings = g_ptr_array_new_with_free_func(destroy_pattern_item);

	for (int i = 0; i <= MAX_MULTIKEYS; i++)
	{
		snprintf(key, sizeof(key), "IgnoreString%d", i);
		gchar *value = ADDON_GET_STRING(addon, key);

		if (!value)
			break;

		size_t len = strlen(value);

		if (len == 0 && data->debug)
			dprintf(gDebugFd, "WARNING: %s is empty.\n", key);

		tListPattern *item = g_new0(tListPattern, 1);

		gboolean is_re = check_if_pattern_regex(value, len);
		listform_update_caps(value, is_re, data);

		if (is_re)
		{
			if (!set_regex_from_string(value, item, data))
			{
				g_free(item);
				continue;
			}
		}
		else
			item->pattern = value;

		g_ptr_array_add(data->ignore_stings, (gpointer)item);
	}

	data->list_start = ADDON_GET_STRING(addon, "Start");

	if (data->list_start)
	{
		size_t len = strlen(data->list_start);
		data->process_lines = (len == 0);

		if (check_if_pattern_regex(data->list_start, len))
		{
			tListPattern item = {NULL, NULL};

			if (set_regex_from_string(data->list_start, &item, data))
			{
				data->list_start = item.pattern;
				data->list_start_re = item.re;
			}
		}
	}
	else
		data->process_lines = TRUE;

	data->list_end = ADDON_GET_STRING(addon, "End");

	if (data->list_end)
	{
		if (check_if_pattern_regex(data->list_start, strlen(data->list_end)))
		{
			tListPattern item = {NULL, NULL};

			if (set_regex_from_string(data->list_end, &item, data))
			{
				data->list_end = item.pattern;
				data->list_end_re = item.re;
			}
		}
	}

	gchar *basedir_re = ADDON_GET_STRING(addon, "ListBaseDirRegEx");

	if (basedir_re)
	{
		if (check_if_pattern_regex(basedir_re, strlen(basedir_re)))
		{
			tListPattern item = {NULL, NULL};

			if (set_regex_from_string(basedir_re, &item, data))
			{
				g_free(item.pattern);
				data->basedir_re = item.re;
			}
		}
	}

	if (result)
	{
		int form_mode = g_key_file_get_integer(gCfg, addon, "FormMode", NULL);

		if (form_mode & MAF_WIN_PATH)
			data->form_caps |= FORM_WINSEP;

		if (form_mode & MAF_UNIX_ATTR)
			data->form_caps |= FORM_UNIXATTR;

		if (g_key_file_get_boolean(gCfg, addon, "LazyParser", NULL))
			data->form_caps |= FORM_LAZY;

		if (g_key_file_get_boolean(gCfg, addon, "ListYearCanBeTime", NULL))
			data->form_caps |= FORM_YEARTIME;

		gchar* strip_chars = g_key_file_get_value(gCfg, addon, "SizeStripChars", NULL);

		if (strip_chars)
		{
			append_not_blank(strip_chars, 0, strlen(strip_chars), APPEND_DST(data->strip_chars));
			g_free(strip_chars);
		}
	}

	return result;
}

static gchar* get_substring(gchar *string, gchar *begin, gchar end, gboolean chomp)
{
	gchar *result = NULL;
	gchar *pos = strstr(string, begin);
	gchar *substr = pos;

	if (chomp)
		substr = pos + strlen(begin);

	if (pos != NULL && substr != NULL)
	{
		result = g_strdup(substr);
		pos = strchr(result, end);

		if (pos)
		{
			if (!chomp)
			{
				substr = pos + 1;

				if (substr)
					*substr = '\0';
			}
			else
				*pos = '\0';
		}
	}

	return result;
}


static gchar* replace_substring(gchar *string, gchar *wut, gchar *with)
{
	gchar **split = g_strsplit(string, wut, -1);
	gchar *result = g_strjoinv(with, split);

	g_free(string);
	g_strfreev(split);

	return result;
}

static gchar* replace_optional(gchar *pattern, gchar *var, gchar *value, gchar *brackets, gchar *substring)
{
	gchar *result = pattern;

	if (value && value[0] != '\0')
	{
		gchar *opt = replace_substring(substring, var, value);
		result = replace_substring(result, brackets, opt);
		g_free(opt);
	}
	else
	{
		result = replace_substring(result, brackets, "");
		g_free(substring);
	}

	return result;
}

static gboolean check_if_win_exe(gchar *executable)
{
	if (!executable)
		return FALSE;

	gchar *ext = NULL;
	char *dot = strrchr(executable, '.');

	if (dot)
		ext = g_ascii_strdown(dot + 1, -1);

	return (ext && strcmp(ext, "exe") == 0);
}

static gboolean check_if_exe_exists(gchar *executable)
{
	return (g_file_test(executable, check_if_win_exe(executable) ? G_FILE_TEST_EXISTS : G_FILE_TEST_IS_EXECUTABLE));
}

static gchar* replace_env_vars(gchar *string)
{
	gchar *result = string;
	gchar **envs = g_listenv();

	for (gchar **env = envs; *env != NULL; env++)
	{
		gchar *tmpl = g_strdup_printf("$%s", *env);

		if (strstr(result, tmpl) != NULL)
			result = replace_substring(result, tmpl, (char *)g_getenv(*env));

		g_free(tmpl);
	}

	g_strfreev(envs);
	return result;
}

static gchar* get_exe_path(gchar *executable)
{
	if (!executable)
		return NULL;

	if (executable[0] == '\0')
	{
		g_free(executable);
		return NULL;
	}

	if (executable[0] == '/')
	{
		if (check_if_exe_exists(executable))
			return executable;
		else
		{
			g_free(executable);
			return NULL;
		}
	}

	if (executable[0] == '~')
	{
		gchar *result = g_strdup_printf("%s%s", gHome, executable + 1);
		g_free(executable);

		if (check_if_exe_exists(result))
			return result;
		else
			executable = result;
	}

	const char tc_var[] = "%$MULTIARC%";

	if (strncmp(tc_var, executable, strlen(tc_var)) == 0)
	{
		gchar *result = g_strdup_printf("%s%s", gHome, executable + strlen(tc_var));
		g_free(executable);
		return result;
	}

	gchar *result = g_find_program_in_path(executable);

	if (result)
	{
		g_free(executable);
		return result;
	}

	executable = replace_env_vars(executable);

	if (check_if_exe_exists(executable))
		return executable;

	g_free(executable);
	return NULL;
}

static gchar* get_archiver(gchar *addon)
{
	gchar *result = get_exe_path(ADDON_GET_STRING(addon, "Archiver"));

	if (!result)
	{
		gchar *fallback = ADDON_GET_STRING(addon, "FallBackArchivers");

		if (!fallback)
			return NULL;

		gchar **exes = g_strsplit(fallback, ",", -1);

		for (gchar **exe = exes; *exe != NULL; exe++)
		{
			result = get_exe_path(*exe);

			if (result)
				break;
		}

		g_free(fallback);
	}

	return result;
}

intptr_t DCPCALL AskDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	char key[22];
	tAskData *data = (tAskData*)SendDlgMsg(pDlg, NULL, DM_GETDLGDATA, 0, 0);

	switch (Msg)
	{
	case DN_INITDIALOG:
	{
		SendDlgMsg(pDlg, NULL, DM_SETTEXT, (intptr_t)data->addon, 0);

		for (int i = 0; i < MAX_MULTIKEYS; i++)
		{
			snprintf(key, sizeof(key), "%s%d", data->key_prefix, i);
			gchar *string = ADDON_GET_STRING(data->addon, key);

			if (!string)
				break;

			gchar *comment = g_key_file_get_comment(gCfg, data->addon, key, NULL);

			if (comment)
				SendDlgMsg(pDlg, "lbAskHistory", DM_LISTADDSTR, (intptr_t)comment, 0);
			else
				SendDlgMsg(pDlg, "lbAskHistory", DM_LISTADDSTR, (intptr_t)string, 0);

			if (i == 0)
				SendDlgMsg(pDlg, "lbAskHistory", DM_SHOWITEM, 1, 0);

			g_free(string);
			g_free(comment);
		}

		if (!(data->mode & ASK_MULTIVOL))
		{
			gchar *value = ADDON_GET_STRING(data->addon, "CheatSheet");

			if (value && value[0] != '\0')
			{
				gchar *cheatsheet = NULL;

				if (value[0] == '`')
				{
					value = unquote_value(value, '`');

					gchar *archiver = get_archiver(data->addon);

					if (archiver && check_if_win_exe(archiver))
					{
						gchar *tmp = archiver;
						archiver = g_strdup_printf("env WINEPREFIX='%s' %s '%s/wine' '%s'",
						                           gWinePrefix, gWineArchWin32 ? "WINEARCH=win32" : "", gWineBin, archiver);
						g_free(tmp);
					}

					if (archiver)
					{
						value = replace_substring(value, "%P", archiver);
						g_free(archiver);
					}

					cheatsheet = get_output(value);
				}
				else
				{
					if (value[0] == '$')
						value = replace_env_vars(value);
					else if (value[0] != '/')
					{
						gchar *tmp = value;
						value = g_strdup_printf("%s/cheat_sheets/%s", gExtensions->PluginDir, tmp);
						g_free(tmp);
					}

					struct stat st;

					if (value && stat(value, &st) == 0)
					{
						if (st.st_size > 0 && st.st_size < MAX_FILE_RANGE)
							g_file_get_contents(value, &cheatsheet, NULL, NULL);
					}
				}

				if (cheatsheet)
				{
					SendDlgMsg(pDlg, "seCheatSheet", DM_SHOWITEM, 1, 0);
					SendDlgMsg(pDlg, "seCheatSheet", DM_SETTEXT, (intptr_t)cheatsheet, 0);
					g_free(cheatsheet);
				}
			}

			g_free(value);
		}

		break;
	}

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			int selected = SendDlgMsg(pDlg, "lbAskHistory", DM_LISTGETITEMINDEX, 0, 0);

			gchar *comment = g_strdup((char*)SendDlgMsg(pDlg, "edDescr", DM_GETTEXT, 0, 0));
			gchar *opts = g_strdup((char*)SendDlgMsg(pDlg, "edOpts", DM_GETTEXT, 0, 0));

			if (strcmp(data->key_prefix, "AskHistory") == 0)
				g_key_file_set_value(gCache, "Options", data->addon, opts);

			g_strlcpy(data->opts, opts, MAX_PATH);

			int num = 0;

			for (int i = 0; i <= MAX_MULTIKEYS; i++)
			{
				snprintf(key, sizeof(key), "%s%d", data->key_prefix, i);
				gchar *prev_opts = ADDON_GET_STRING(data->addon, key);

				if ((i == selected) && strcmp((char *)SendDlgMsg(pDlg, "edOpts", DM_GETTEXT, 0, 0), prev_opts) == 0)
				{
					g_free(prev_opts);
					g_key_file_remove_comment(gCfg, data->addon, key, NULL);
					g_key_file_remove_key(gCfg, data->addon, key, NULL);
					snprintf(key, sizeof(key), "%s%d", data->key_prefix, ++i);
					prev_opts = ADDON_GET_STRING(data->addon, key);
				}

				gchar *prev_comment = g_key_file_get_comment(gCfg, data->addon, key, NULL);
				g_key_file_remove_comment(gCfg, data->addon, key, NULL);
				g_key_file_remove_key(gCfg, data->addon, key, NULL);

				snprintf(key, sizeof(key), "%s%d", data->key_prefix, num++);
				g_key_file_set_value(gCfg, data->addon, key, opts);

				if (comment && comment[0] != '\0')
					g_key_file_set_comment(gCfg, data->addon, key, comment, NULL);

				g_free(opts);
				g_free(comment);

				if (!prev_opts)
					break;

				opts = prev_opts;
				comment = prev_comment;
			}
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "lbAskHistory") == 0)
		{
			int i = SendDlgMsg(pDlg, "lbAskHistory", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
			{
				snprintf(key, sizeof(key), "%s%d", data->key_prefix, i);
				gchar *string = ADDON_GET_STRING(data->addon, key);

				if (string)
				{
					gchar *comment = g_key_file_get_comment(gCfg, data->addon, key, NULL);
					SendDlgMsg(pDlg, "edOpts", DM_SETTEXT, (intptr_t)string, 0);
					SendDlgMsg(pDlg, "edDescr", DM_SETTEXT, (intptr_t)comment, 0);
					g_free(comment);
					g_free(string);
				}
			}
		}

		break;
	}

	return 0;
}

static gchar* show_ask_dialog(gchar *addon, int mode)
{
	gchar *result = NULL;

	char test[MAX_PATH] = "";

	gchar *opts = g_key_file_get_value(gCache, "Options", addon, NULL);

	if (mode & ASK_ONCE && mode & ASK_ASKED)
		result = opts;
	else
	{
		if (opts)
			g_strlcpy(test, opts, MAX_PATH);

		g_free(opts);

		tAskData userdata = { addon, "AskHistory", mode, TRUE, "" };

		if (gExtensions->DialogBoxParam(gLFMAsk, strlen(gLFMAsk), AskDlgProc, DB_FILENAME, (void *)&userdata, NULL) != 0)
		{
			result = g_key_file_get_value(gCache, "Options", addon, NULL);
		}

		if (mode & ASK_ONCE)
			mode |= ASK_ASKED;
		else
			mode &= ~ASK_ASKED;

		g_key_file_set_integer(gCache, "AskMode", addon, mode);
	}

	return result;
}

static gchar* command_set_dirs(gchar *command, char *subdir, char *tmpdir)
{
	if (!command)
		return NULL;

	gchar *brackets = NULL;
	gchar *result = g_strdup(command);
	gchar *quoted_subdir = NULL;
	gchar *quoted_tmpdir = NULL;

	if (subdir)
		quoted_subdir = g_shell_quote(subdir);

	if (tmpdir)
		quoted_tmpdir = g_shell_quote(tmpdir);

	while ((brackets = get_substring(result, "{", '}', FALSE)) != NULL)
	{
		gchar *data = get_substring(brackets, "{", '}', TRUE);

		if (strstr(data, "%R") != NULL)
			result = replace_optional(result, "%R", quoted_subdir, brackets, data);
		else if (strstr(data, "%T") != NULL)
			result = replace_optional(result, "%T", quoted_tmpdir, brackets, data);

		g_free(brackets);
	}

	if (strstr(result, "%R") != NULL)
		result = replace_substring(result, "%R", quoted_subdir ? quoted_subdir : "");

	if (strstr(result, "%T") != NULL)
		result = replace_substring(result, "%T", quoted_tmpdir ? quoted_tmpdir : "");

	g_free(quoted_subdir);
	g_free(quoted_tmpdir);

	return result;
}

static gchar* prepare_command(gchar *addon, int cmd, gint *err_lvl, gint *enc, gboolean is_wine, gchar *pass)
{
	const char *keys[] =
	{
		"List",
		"ExtractWithoutPath",
		"Extract",
		"Add",
		"AddSelfExtract",
		"Move",
		"Delete",
		"Test",
	};

	if (cmd >= OP_LAST)
		return NULL;

	gchar *result = ADDON_GET_STRING(addon, keys[cmd]);

	if (!result || result[0] == '\0')
	{
		g_free(result);
		return NULL;
	}

	if (is_wine)
	{
		gchar *pos = strstr(result, "%P ");

		if (pos)
		{
			gchar *wine_templ = g_strdup_printf("%s/wine %%P ", gWineBin);
			result = replace_substring(result, "%P ", wine_templ);
			g_free(wine_templ);
		}
		else
		{
			gchar *tmp = result;
			result = g_strdup_printf("%s/wine %s", gWineBin, tmp);
			g_free(tmp);
		}
	}


	gboolean is_askopts = FALSE;
	int mode = g_key_file_get_integer(gCache, "AskMode", addon, NULL);

	switch (cmd)
	{
	case OP_LIST:
		is_askopts = (mode & ASK_LIST);
		break;

	case OP_UNPACK:
	case OP_UNPACK_PATH:
		is_askopts = (mode & ASK_UNPACK);
		break;

	case OP_PACK:
	case OP_SFX:
	case OP_MOVE:
		is_askopts = (mode & ASK_PACK);
		break;

	case OP_DELETE:
		is_askopts = (mode & ASK_DELETE);
		break;

	case OP_TEST:
		is_askopts = (mode & ASK_TEST);
		break;
	};

	gchar *brackets = NULL;

	gchar volume[20] = "";

	gchar *opts = NULL;

	if (is_askopts)
		opts = show_ask_dialog(addon, mode);

	if (mode & ASK_MULTIVOL && strstr(result, "%V") != NULL)
	{
		tAskData userdata = { addon, "VolumeSize", mode, FALSE, "" };

		if (gExtensions->DialogBoxParam(gLFMAsk, strlen(gLFMAsk), AskDlgProc, DB_FILENAME, (void *)&userdata, NULL) != 0)
			g_strlcpy(volume, userdata.opts, sizeof(volume));
	}

	while ((brackets = get_substring(result, "{", '}', FALSE)) != NULL)
	{
		gchar *data = get_substring(brackets, "{", '}', TRUE);

		if (strstr(data, "%S") != NULL)
			result = replace_optional(result, "%S", opts, brackets, data);
		else if (strstr(data, "%W") != NULL)
			result = replace_optional(result, "%W", pass, brackets, data);
		else if (strstr(data, "%V") != NULL)
		{
			result = replace_optional(result, "%V", volume, brackets, data);
		}
		else if (strstr(data, "%T") == NULL && strstr(data, "%R") != NULL)
		{
			dprintf(gDebugFd, "[%s]: \"%s\" is not supported!\n", result, data);
			result = replace_substring(result, brackets, "");
			g_free(data);
		}

		g_free(brackets);
	}

	if (strstr(result, "%S") != NULL)
		result = replace_substring(result, "%S", opts ? opts : "");

	if (strstr(result, "%V") != NULL)
		result = replace_substring(result, "%V", volume);

	if (strstr(result, "%W") != NULL)
		result = replace_substring(result, "%W", pass ? pass : "");

	g_free(opts);

	gchar *string = get_substring(result, "%E", ' ', FALSE);

	if (string)
	{
		gchar *value = get_substring(string, "%E", ' ', TRUE);

		if (value)
		{
			*err_lvl = atoi(value);
			g_free(value);
		}

		result = replace_substring(result, string, "");
		g_free(string);
	}

	string = get_substring(result, "%O", ' ', FALSE);

	if (string)
	{
		if (string[2] == 'A')
			*enc = ENC_ANSI;
		else if (string[2] == 'O')
			*enc = ENC_OEM;
		else
			*enc = ENC_OEM;

		result = replace_substring(result, string, "");
		g_free(string);
	}

	return result;
}

static gchar* get_winepath(gchar *filename)
{
	gchar *result = NULL;
	replace_nix_sep(filename);

	gboolean is_quoted = value_is_quoted(filename, '"', strlen(filename) - 1);

	if (is_quoted)
		filename = unquote_value(filename, '"');

	if (filename[0] != '\\')
		result = filename;
	else
	{
		result = g_strdup_printf("%s%s", gWineDrive, filename);
		g_free(filename);
	}

	if (is_quoted)
		result = quote_and_free(result);

	return result;
}

static gboolean check_if_dir(gchar *filename)
{
	if (!filename)
		return FALSE;

	size_t len = strlen(filename);

	if (len == 0)
		return FALSE;

	return (filename[len - 1] == '/');
}

static gchar* filename_apply_modifiers(gchar *filename, int flags)
{
	if (!filename)
		return NULL;

	size_t len = strlen(filename);
	gchar *result = g_strdup(filename);

	if ((flags & ARG_ALLNONE || flags & ARG_ALLUNIX) && len > 3)
	{
		char mask[5];
		g_strlcpy(mask, result + (len - 4), 5);

		if (flags & ARG_ALLNONE && strcmp(mask, "/*.*") == 0)
			result[len - 4] = '\0';
		else if (flags & ARG_ALLUNIX && strcmp(mask, "/*.*") == 0)
			result[len - 2] = '\0';
	}

	if (flags & ARG_BASENAME)
	{
		gchar *tmp = result;
		result = g_path_get_dirname(tmp);
		g_free(tmp);
	}
	else if (flags & ARG_BASENAME)
	{
		gchar *tmp = result;
		result = g_path_get_basename(tmp);
		g_free(tmp);
	}

	if (flags & ARG_QUOTEALL || (flags & ARG_QUOTESPC && strchr(result, ' ') != NULL))
		result = quote_and_free(result);

	return result;
}

static gchar* chomp_modifiers(gchar *string, int *arg_flags, guint *max_files)
{
	int mods = 0;
	char count_mod[20] = "";
	const char magic_chars[] = "QqSMN*WPAOUF";

	int flags[] =
	{
		ARG_QUOTESPC,
		ARG_QUOTEALL,
		ARG_UNIXSEP,
		ARG_ALLWIN,
		ARG_ALLNONE,
		ARG_ALLUNIX,
		ARG_BASENAME,
		ARG_DIRNAME,
		ARG_ANSIENC,
		ARG_OEMENC,
		ARG_UTFENC,
		ARG_FILEONLY,
	};

	while (*string != '\0')
	{
		if (strchr(magic_chars, *string) == NULL && !isdigit(*string))
			break;

		if (isdigit(*string))
		{
			append_char(*string, APPEND_DST(count_mod));
		}
		else
		{
			for (int i = 0; magic_chars[i] != '\0'; i++)
			{
				if (*string == magic_chars[i])
				{
					mods |= flags[i];
					break;
				}
			}
		}

		string++;
	}

	if (max_files)
		*max_files = atoi(count_mod);

	if (arg_flags)
		*arg_flags = mods;

	return string;
}

static gchar* replace_arg_template(gchar *string, gchar *exe, gchar *archive, gchar **src_list, gchar **dst_list, gchar **tmpfile, gboolean is_wine, guint *items_got, guint *current, guint *max_files)
{
	gchar *new_string = NULL;
	guint got = *items_got;
	const char magic_chars[] = "PpAaFLlD";

	while (*string != '\0')
	{
		char c = *string;

		if (c == '%')
		{
			int flags = 0;
			char *end = NULL;
			char *filename = NULL;
			char *next = string + 1;

			if (strchr(magic_chars, *next) == NULL)
			{
				string++;
				continue;
			}

			*string = '\0';

			if (*next == 'P' || *next == 'p')
			{
				end = chomp_modifiers(next + 1, &flags, NULL);
				filename = filename_apply_modifiers(exe, flags);
			}
			else if (*next == 'A' || *next == 'a')
			{
				end = chomp_modifiers(next + 1, &flags, NULL);
				filename = filename_apply_modifiers(archive, flags);
			}
			else if (*next == 'F')
			{
				end = chomp_modifiers(next + 1, &flags, max_files);

				if (src_list && src_list[got])
				{
					if (flags & ARG_FILEONLY)
					{
						while (src_list[got])
						{
							if (!check_if_dir(src_list[got]))
								break;

							got++;
						}

						*current = got;

						if (src_list[got])
							filename = filename_apply_modifiers(src_list[got++], flags);
					}
					else
					{
						*current = got;
						filename = filename_apply_modifiers(src_list[got++], flags);
					}
				}
			}
			else if (*next == 'D')
			{
				end = chomp_modifiers(next + 1, &flags, NULL);

				if (dst_list && dst_list[*current])
					filename = filename_apply_modifiers(dst_list[*current], flags);
			}
			else if (*next == 'L' || *next == 'l')
			{
				if (!tmpfile)
				{
					dprintf(gDebugFd, "ERROR: L or l is present in the arguments, but the pointer to the location of the temporary file string is missing\n");
					return NULL;
				}

				end = chomp_modifiers(next + 1, &flags, NULL);

				if (src_list)
				{
					gint fd = -1;
					guint count = g_strv_length(src_list);

					if (*tmpfile != NULL)
						fd = open(*tmpfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
					else
					{
						gchar *arcname = g_path_get_basename(archive);
						gchar *templfile = g_strdup_printf("%s_XXXXXX.lst", arcname);
						g_free(arcname);
						GError *err = NULL;
						fd = g_file_open_tmp(templfile, tmpfile, &err);

						if (err)
						{
							dprintf(gDebugFd, "ERROR: failed to create temp filelist: %s\n", err->message);
							g_error_free(err);
						}
					}

					if (fd != -1)
					{
						filename = g_strdup(*tmpfile);

						for (guint i = 0; i < count; i++)
						{
							if (flags & ARG_FILEONLY && check_if_dir(src_list[i]))
								continue;

							gchar *line = filename_apply_modifiers(src_list[i], flags);

							if (line)
							{
								if (is_wine && !(flags & ARG_UNIXSEP))
									replace_nix_sep(line);

								if (flags & ARG_ANSIENC)
									line = string_convert_encoding(line, ENC_ANSI);
								else if (flags & ARG_OEMENC)
									line = string_convert_encoding(line, ENC_OEM);

								dprintf(fd, "%s%s", line, is_wine ? "\r\n" : "\n");
								g_free(line);
							}

							got++;
						}

						close(fd);
					}
				}
			}

			if (is_wine && filename)
				filename = get_winepath(filename);

			gchar *tmp = new_string;
			new_string = g_strdup_printf("%s%s", tmp ? tmp : "", filename ? filename : "");
			g_free(tmp);
			g_free(filename);
			string = end;
			*items_got = got;
			continue;
		}
		else
		{
			gchar *tmp = new_string;
			new_string = g_strdup_printf("%s%c", tmp ? tmp : "", *string);
			g_free(tmp);
		}

		string++;
	}

	return new_string;
}

static gchar** build_argv_from_template(gchar *templ, gchar *exe, gchar *archive, gchar **src_list, gchar **dst_list, gchar **tmpfile, gboolean is_wine, gboolean is_batch, guint *items_got)
{
	int argc;
	gchar** argv;
	gchar *cut_tmpl = NULL;
	int filemods = 0;
	GError *err = NULL;
	guint maxfiles = -1;
	guint items_added = 0;
	guint current = 0;

	if (!templ || templ[0] == '\0')
		return NULL;

	guint src_count = 0;

	if (src_list)
		src_count = g_strv_length(src_list);

	gboolean append_items = (is_batch && strstr(templ, "%F") != NULL && src_count > 1);

	gchar *replica = get_substring(templ, "[", ']', FALSE);

	if (replica)
	{
		gchar *tmp = replica;
		replica = get_substring(templ, "[", ']', TRUE);
		cut_tmpl = replace_substring(g_strdup(templ), tmp, "");
		g_free(tmp);
	}

	if (!g_shell_parse_argv(cut_tmpl ? cut_tmpl : templ, &argc, &argv, &err))
	{
		if (err)
		{
			dprintf(gDebugFd, "build_argv_from_template (%s): %s\n", templ, err->message);
			g_error_free(err);
		}

		g_free(replica);
		g_free(cut_tmpl);
		return NULL;
	}

	g_free(cut_tmpl);

	if (argc < 2)
	{
		dprintf(gDebugFd, "build_argv_from_template (%s): argc < 2\n", templ);

		if (argv)
			g_strfreev(argv);

		g_free(replica);

		return NULL;
	}

	for (int i = 0; i < argc; i++)
	{
		gchar *oldarg = argv[i];

		if (i == 0 && (strstr(argv[i], "%P") != NULL || strstr(argv[i], "%p") != NULL))
		{
			argv[i] = g_strdup(exe);
			g_free(oldarg);
		}
		else
		{
			argv[i] = unquote_value(replace_arg_template(oldarg, exe, archive, src_list,
			                        dst_list, tmpfile, is_wine, &items_added, &current, &maxfiles), '"');
		}
	}

	if (append_items && maxfiles == 1)
		append_items = FALSE;

	if (append_items || replica)
	{
		guint dst_count = -1;
		guint append_args = 0;
		guint max_args = sysconf(_SC_ARG_MAX) - g_strv_length(argv);
		gchar *arg_lines = NULL;

		if (replica)
		{
			int r_count;
			gchar **r_argv;

			if (!g_shell_parse_argv(replica, &r_count, &r_argv, NULL))
			{
				dprintf(gDebugFd, "build_argv_from_template (%%D): replica g_shell_parse_argv fail\n");

				if (argv)
					g_strfreev(argv);

				g_free(replica);
				return NULL;
			}

			if (dst_list)
				dst_count = g_strv_length(dst_list);

			if (strstr(replica, "%D") != NULL && src_count != dst_count)
			{
				dprintf(gDebugFd, "build_argv_from_template (%%D): items list != destnames list\n");

				if (argv)
					g_strfreev(argv);

				g_free(replica);
				return NULL;
			}

			for (guint i = items_added; i < src_count; i++)
			{
				if (maxfiles > 1 && items_added == maxfiles)
					break;

				gchar *new_args = NULL;

				for (int j = 0; j < r_count; j++)
				{
					gchar *line = replace_arg_template(g_strdup(r_argv[j]), exe, archive, src_list, dst_list,
					                                   tmpfile, is_wine, &items_added, &current, &maxfiles);
					append_args++;

					if (line)
						new_args = string_add_line(new_args, line, "\n");
				}

				if (append_args >= max_args)
				{
					items_added--;
					g_free(new_args);
					break;
				}

				if (new_args)
					arg_lines = string_add_line(arg_lines, new_args, "\n");
			}

		}
		else
		{
			for (guint i = 1; i < src_count; i++)
			{
				gchar *filename = filename_apply_modifiers(src_list[i], filemods);
				arg_lines = string_add_line(arg_lines, filename, "\n");
				append_args++;
				items_added++;

				if (append_args >= max_args || (maxfiles > 1 && items_added == maxfiles))
					break;
			}
		}

		if (arg_lines)
		{
			gchar *prev_args = g_strjoinv("\n", argv);
			g_strfreev(argv);
			arg_lines = string_add_line(prev_args, arg_lines, "\n");
			argv = g_strsplit(arg_lines, "\n", -1);
			g_free(arg_lines);
		}
	}

	if (items_got != NULL)
		*items_got = items_added;

	g_free(replica);

	return argv;
}

static int move_files(gchar *tempdir, gchar **src_list, gchar **dst_list, gboolean win_sep)
{
	int result = E_SUCCESS;
	guint len = g_strv_length(dst_list);

	for (guint i = 0; i < len; i++)
	{
		if (g_file_test(dst_list[i], G_FILE_TEST_EXISTS))
			remove_target(dst_list[i]);

		if (win_sep)
			replace_win_sep(src_list[i]);

		gchar *filename = g_strdup_printf("%s/%s", tempdir, src_list[i]);

		if (rename(filename, dst_list[i]) == -1)
		{
			int errsv = errno;
			dprintf(gDebugFd, "rename (%s -> %s): %s\n", filename, dst_list[i], strerror(errsv));
		}

		g_free(filename);
	}

	return result;
}

static gchar* ReplaceFarCommand(gchar *command, gchar *archiver, size_t len)
{
	if (archiver && strncmp(command, archiver, len) == 0)
	{
		gchar *tmp = command;
		command = g_strdup_printf("%%P%s", command + len);
		g_free(tmp);
	}

	command = replace_substring(command, "%%W", "%T");
	command = replace_substring(command, "%%P", "%W");
	command = replace_substring(command, "%%", "%");
	return command;
}

static void ReplaceCommand(gchar *addon, const char *key, gboolean is_far_addon, gchar *archiver, size_t len)
{
	gchar *string = g_key_file_get_value(gCfg, addon, key, NULL);

	if (string)
	{
		if (string[0] != '\0')
		{
			if (is_far_addon)
			{
				g_key_file_set_comment(gCfg, addon, key, string, NULL);
				string = ReplaceFarCommand(string, archiver, len);
			}

			if (value_is_quoted(string, '\'', strlen(string) - 1))
			{
				gchar *tmp = unquote_value(string, '\'');
				string = quote_and_free(tmp);
			}

			g_key_file_set_string(gCfg, addon, key, string);
		}

		g_free(string);
	}
}

static void ConvertAddon(GKeyFile *key_file, gchar *addon)
{
	gchar *string = NULL;

	gboolean is_dc_addon = g_key_file_has_key(gCfg, addon, "Flags", NULL);
	int form_mode = g_key_file_get_integer(gCfg, addon, "FormMode", NULL);

	gboolean is_tc_addon = g_key_file_has_key(gCfg, addon, "ExtractWithPath", NULL);
	gchar *archiver = ADDON_GET_STRING(addon, "Archiver");
	size_t len = -1;

	if (archiver)
	{
		if (archiver[0] != '\0')
			g_key_file_set_comment(gCfg, addon, "Archiver", archiver, NULL);

		len = strlen(archiver);
	}

	string = ADDON_GET_STRING(addon, "List");
	gboolean is_far_addon =  g_key_file_has_key(gCfg, addon, "TypeName", NULL);

	if (string)
	{
		if (string[0] != '\0')
		{
			gchar *oldstring = g_strdup(string);

			if (is_far_addon)
				string = ReplaceFarCommand(string, archiver, len);
			else if (value_is_quoted(string, '\'', strlen(string) - 1))
			{
				gchar *tmp = unquote_value(string, '\'');
				string = quote_and_free(tmp);
			}

			if (strstr(string, "%A") == NULL && strstr(string, "%a") == NULL)
			{
				gchar *tmp = string;
				string = g_strdup_printf("%s %%AQ", tmp);
				g_free(tmp);
			}

			g_key_file_set_string(gCfg, addon, "List", string);

			if (is_far_addon && !archiver)
			{
				char *pos = strchr(string, ' ');

				if (pos != NULL)
				{
					gchar *tmp = g_strdup(string);
					*pos = '\0';
					archiver = string;
					string = tmp;
					g_key_file_set_string(gCfg, addon, "Archiver", archiver);
					len = strlen(archiver);
					string = ReplaceFarCommand(string, archiver, len);
					g_key_file_set_string(gCfg, addon, "List", string);
				}
			}

			if (strcmp(string, oldstring) != 0)
				g_key_file_set_comment(gCfg, addon, "List", oldstring, NULL);

			g_free(oldstring);
			g_free(string);
		}
	}

	ReplaceCommand(addon, "ExtractWithoutPath", is_far_addon, archiver, len);

	if (is_tc_addon)
	{
		string = g_key_file_get_value(gCfg, addon, "Extract", NULL);

		if (string)
		{
			if (value_is_quoted(string, '\'', strlen(string) - 1))
			{
				gchar *tmp = unquote_value(string, '\'');
				string = quote_and_free(tmp);
			}

			g_key_file_set_comment(gCfg, addon, "Extract", string, NULL);
			g_key_file_set_string(gCfg, addon, "ExtractWithoutPath", string);
			g_free(string);
		}

		string = g_key_file_get_value(gCfg, addon, "ExtractWithPath", NULL);

		if (string)
		{
			if (value_is_quoted(string, '\'', strlen(string) - 1))
			{
				gchar *tmp = unquote_value(string, '\'');
				string = quote_and_free(tmp);
			}

			gchar *comment = g_strdup_printf("ExtractWithPath=%s", string);
			g_key_file_set_comment(gCfg, addon, "ExtractWithoutPath", comment, NULL);
			g_free(comment);
			g_key_file_set_string(gCfg, addon, "Extract", string);
			g_free(string);
		}

		g_key_file_remove_key(gCfg, addon, "ExtractWithPath", NULL);

		if (g_key_file_has_key(gCfg, addon, "UnixPath", NULL))
		{
			if (!g_key_file_get_boolean(gCfg, addon, "UnixPath", NULL))
				form_mode |= MAF_WIN_PATH;
			else
				form_mode |= MAF_UNIX_PATH;
		}
	}

	ReplaceCommand(addon, "Extract", is_far_addon, archiver, len);
	ReplaceCommand(addon, "Add", is_far_addon, archiver, len);

	if (is_far_addon)
	{
		string = g_key_file_get_value(gCfg, addon, "SFX", NULL);

		if (string)
		{
			string = ReplaceFarCommand(string, archiver, len);
			g_key_file_set_string(gCfg, addon, "AddSelfExtract", string);
			g_free(string);
		}
	}

	if (is_dc_addon || is_far_addon)
	{
		string = g_key_file_get_value(gCfg, addon, "Extract", NULL);
		g_key_file_set_integer(gCfg, addon, "BatchUnpack", (strstr(string, "%F") == NULL) ? 1 : 0);
		g_free(string);
	}

	if (is_dc_addon)
		g_key_file_set_integer(gCfg, addon, "UnixPath", (form_mode & MAF_UNIX_PATH));

	if (!is_dc_addon)
		ReplaceCommand(addon, "Move", is_far_addon, archiver, len);

	ReplaceCommand(addon, "Test", is_far_addon, archiver, len);
	ReplaceCommand(addon, "Delete", is_far_addon, archiver, len);

	g_free(archiver);

	string = g_key_file_get_value(gCfg, addon, "Extension", NULL);

	if (string)
	{
		gchar *lower_ext = g_ascii_strdown(string, -1);
		g_key_file_set_string(gCfg, addon, "Extension", lower_ext);
		g_free(lower_ext);
		g_free(string);
	}

	if (is_far_addon && !g_key_file_has_key(gCfg, addon, "FormMode", NULL))
	{
		for (int i = 0; i < MAX_MULTIKEYS; i++)
		{
			gchar *key = g_strdup_printf("Form%d", i);
			string = ADDON_GET_STRING(addon, key);

			if (!string)
			{
				g_free(key);
				break;
			}

			if (value_is_quoted(string, '\'', strlen(string) - 1))
			{
				gchar *tmp = unquote_value(string, '\'');
				string = quote_and_free(tmp);
			}

			if (string[0] == '/' || (strlen(string) > 1 && string[0] == '"' && string[1] == '/'))
			{
				// regex
			}
			else if (string[0] != '\0')
			{
				replace_char(string, 'e', '?');
				replace_char(string, 'x', '?');
			}

			g_key_file_set_string(gCfg, addon, key, string);
			g_free(string);
			g_free(key);
		}
	}

	if (!g_key_file_has_key(gCfg, addon, "Enabled", NULL))
		g_key_file_set_integer(gCfg, addon, "Enabled", 1);

	g_key_file_set_integer(gCfg, addon, "Debug", (int)g_key_file_get_boolean(gCfg, addon, "Debug", NULL));

	if (is_dc_addon)
		g_key_file_set_comment(gCfg, addon, "Debug", "This addon was converted from the DC addon.", NULL);

	if (is_tc_addon)
		g_key_file_set_comment(gCfg, addon, "Debug", "This addon was converted from the TC addon.", NULL);

	if (is_far_addon)
		g_key_file_set_comment(gCfg, addon, "Debug", "This addon was converted from the FAR addon.", NULL);

	g_key_file_set_integer(gCfg, addon, "FormMode", form_mode);
	g_key_file_set_comment(gCfg, addon, "FormMode", "2 - use Windows path delimiter, 4 - use Unix file attributes", NULL);
	g_key_file_set_integer(gCfg, addon, "AddonCannotModify", (int)g_key_file_get_boolean(gCfg, addon, "AddonCannotModify", NULL));
	g_key_file_set_integer(gCfg, addon, "DontTouchThisAddonAnymore", 1);
	g_key_file_save_to_file(gCfg, gConfName, NULL);
}

intptr_t DCPCALL ImpDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	switch (Msg)
	{
	case DN_INITDIALOG:
		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnLoad") == 0)
		{
			size_t len = 0;
			ssize_t lread;
			char *line = NULL;
			char *filename = (char*)SendDlgMsg(pDlg, "edFileName", DM_GETTEXT, 0, 0);
			SendDlgMsg(pDlg, "mContent", DM_SETTEXT, 0, 0);
			SendDlgMsg(pDlg, "lbAddons", DM_LISTCLEAR, 0, 0);
			GKeyFile *import = (GKeyFile*)SendDlgMsg(pDlg, NULL, DM_GETDLGDATA, 0, 0);

			if (import)
				g_key_file_free(import);

			SendDlgMsg(pDlg, NULL, DM_SETDLGDATA, 0, 0);

			if (filename[0] == '\0')
				return 0;


			GFile *gfile = g_file_new_for_path(filename);

			if (gfile)
			{
				GFileInfo *fileinfo = g_file_query_info(gfile, "standard::*", 0, NULL, NULL);

				if (fileinfo)
				{
					goffset size = g_file_info_get_size(fileinfo);

					if (size < 1 || size > MAX_FILE_RANGE)
					{
						MessageBox("Inappropriate file size.", PLUGNAME, 0);
						g_object_unref(fileinfo);
						g_object_unref(gfile);
						return 0;
					}

					const gchar *content_type = g_file_info_get_content_type(fileinfo);

					if (strcmp(content_type, "text/plain") != 0)
					{
						MessageBox("Content is not text/plain", PLUGNAME, 0);
						g_object_unref(fileinfo);
						g_object_unref(gfile);
						return 0;
					}

					g_object_unref(fileinfo);
				}
				else
				{
					MessageBox("Unable to retrieve file information.", PLUGNAME, 0);
					g_object_unref(gfile);
					return 0;
				}

				g_object_unref(gfile);
			}
			else
			{
				MessageBox("Unable to retrieve file information.", PLUGNAME, 0);
				return 0;
			}

			FILE *fp = fopen(filename, "r");

			if (fp != NULL)
			{
				while ((lread = getline(&line, &len, fp)) != -1)
				{
					if (line[lread - 1] == '\n')
						line[lread - 1] = '\0';

					if (line[lread - 2] == '\r')
						line[lread - 2] = '\0';

					if (line[0] == ';')
						line[0] = '#';

					SendDlgMsg(pDlg, "mContent", DM_LISTADDSTR, (intptr_t)line, 0);
				}

				free(line);
				fclose(fp);

				GError *err = NULL;
				GKeyFile *import = g_key_file_new();
				char *text = (char*)SendDlgMsg(pDlg, "mContent", DM_GETTEXT, 0, 0);

				if (!g_key_file_load_from_data(import, text, strlen(text), G_KEY_FILE_KEEP_COMMENTS, &err))
				{
					MessageBox(err->message, PLUGNAME, MB_OK | MB_ICONERROR);
					g_key_file_free(import);
					g_error_free(err);
					SendDlgMsg(pDlg, NULL, DM_SETDLGDATA, 0, 0);
				}
				else
				{
					gchar **groups = g_key_file_get_groups(import, NULL);

					for (gchar **addon = groups; *addon != NULL; addon++)
					{
						if (strcmp(*addon, "MultiArc") != 0)
							SendDlgMsg(pDlg, "lbAddons", DM_LISTADDSTR, (intptr_t) * addon, 0);
					}

					g_strfreev(groups);
					SendDlgMsg(pDlg, NULL, DM_SETDLGDATA, (intptr_t)import, 0);
				}
			}
		}
		else if (strcmp(DlgItemName, "btnImport") == 0)
		{
			SendDlgMsg(pDlg, "btnImport", DM_ENABLE, 0, 0);
			int selected = SendDlgMsg(pDlg, "lbAddons", DM_LISTGETITEMINDEX, 0, 0);

			if (selected == -1)
				MessageBox("Nothing was selected.", PLUGNAME, 0);
			else
			{
				char new_name[MAX_PATH];
				char *addon = (char*)SendDlgMsg(pDlg, "lbAddons", DM_LISTGETITEM, selected, 0);
				GKeyFile *import = (GKeyFile*)SendDlgMsg(pDlg, NULL, DM_GETDLGDATA, 0, 0);

				if (!g_key_file_has_key(import, addon, "List", NULL) && !g_key_file_has_key(import, addon, "Flags", NULL))
				{
					MessageBox("This section does not contain commands or flags to get a list of archived files, aborted.", PLUGNAME, 0);
					return 0;
				}

				g_strlcpy(new_name, addon, MAX_PATH);

				while (g_key_file_has_group(gCfg, new_name))
				{
					if (InputBox(PLUGNAME, "Already exists, please enter another name:", FALSE, new_name, MAX_PATH) == FALSE)
						return 0;
				}

				gchar **keys = g_key_file_get_keys(import, addon, NULL, NULL);

				for (gchar **key = keys; *key != NULL; key++)
				{
					gchar *value = g_key_file_get_value(import, addon, *key, NULL);
					gchar *comment = g_key_file_get_comment(import, addon, *key, NULL);
					g_key_file_set_value(gCfg, new_name, *key, value);

					if (comment)
					{
						g_key_file_set_comment(gCfg, new_name, *key, comment, NULL);
						g_free(comment);
					}

					g_free(value);
				}

				ConvertAddon(gCfg, new_name);
				SendDlgMsg(pDlg, "lbAddons", DM_LISTSETDATA, selected, 1);
				MessageBox("Import completed.", PLUGNAME, 0);
			}
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "lbAddons") == 0)
		{
			int selected = SendDlgMsg(pDlg, "lbAddons", DM_LISTGETITEMINDEX, 0, 0);

			if (selected != -1)
			{
				int imported = SendDlgMsg(pDlg, "lbAddons", DM_LISTGETDATA, selected, 0);
				SendDlgMsg(pDlg, "btnImport", DM_ENABLE, (imported != 1), 0);
			}
			else
				SendDlgMsg(pDlg, "btnImport", DM_ENABLE, 0, 0);
		}

		break;

	case DN_CLOSE:
	{
		GKeyFile *import = (GKeyFile*)SendDlgMsg(pDlg, NULL, DM_GETDLGDATA, 0, 0);

		if (import)
			g_key_file_free(import);
	}
	break;
	}

	return 0;
}

static void FillAddonsComboBox(uintptr_t pDlg)
{
	SendDlgMsg(pDlg, "cmbAddon", DM_LISTCLEAR, 0, 0);
	gchar **groups = g_key_file_get_groups(gCfg, NULL);

	if (groups)
	{

		for (gchar **addon = groups; *addon != NULL; addon++)
		{

			if (strcmp(*addon, "MultiArc") == 0)
				continue;

			if (!g_key_file_get_boolean(gCfg, *addon, "DontTouchThisAddonAnymore", NULL))
				ConvertAddon(gCfg, *addon);

			SendDlgMsg(pDlg, "cmbAddon", DM_LISTADDSTR, (intptr_t)*addon, 0);
		}

		g_strfreev(groups);
	}
}

intptr_t DCPCALL OptDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	char *names[] =
	{
		"cbAskPack",
		"cbAskExtract",
		"cbAskList",
		"cbAskDelete",
		"cbAskTest",
		"cbAskOnce",
		"cbAskListPass",
		"cbAskMultiVolume",
		NULL
	};

	int flags[] =
	{
		ASK_PACK,
		ASK_UNPACK,
		ASK_LIST,
		ASK_DELETE,
		ASK_TEST,
		ASK_ONCE,
		ASK_LISTPASS,
		ASK_MULTIVOL,
	};

	switch (Msg)
	{
	case DN_INITDIALOG:
	{
		FillAddonsComboBox(pDlg);
		break;
	}

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnSet") == 0)
		{
			char *addon = g_strdup((char*)SendDlgMsg(pDlg, "cmbAddon", DM_GETTEXT, 0, 0));

			if (!addon || addon[0] == '\0')
				break;

			gchar *comment = g_key_file_get_comment(gCfg, addon, "Archiver", NULL);

			if (!comment)
			{
				gchar *current = ADDON_GET_STRING(addon, "Archiver");
				g_key_file_set_comment(gCfg, addon, "Archiver", current, NULL);
				g_free(current);
			}

			g_free(comment);

			char *exe = g_strdup((char*)SendDlgMsg(pDlg, "edExe", DM_GETTEXT, 0, 0));
			g_key_file_set_string(gCfg, addon, "Archiver", exe);
			SendDlgMsg(pDlg, "lblExeError", DM_SHOWITEM, !check_if_exe_exists(exe), 0);
			SendDlgMsg(pDlg, "lblWine", DM_SHOWITEM, check_if_win_exe(exe), 0);
			g_free(addon);
			g_free(exe);
		}
		else if (strcmp(DlgItemName, "btnLog") == 0)
		{
			gchar *quoted = g_shell_quote(gDebugFileName);
			gchar *command = g_strdup_printf("xdg-open %s", quoted);
			g_free(quoted);
			g_spawn_command_line_async(command, NULL);
			g_free(command);
		}
		else if (strcmp(DlgItemName, "btnTrimLog") == 0)
		{
			ftruncate(gDebugFd, 0);
			lseek(gDebugFd, 0, SEEK_SET);
		}
		else if (strcmp(DlgItemName, "btnImport") == 0)
		{
			gExtensions->DialogBoxLFMFile(gLFMImport, ImpDlgProc);
			FillAddonsComboBox(pDlg);
		}
		else if (strcmp(DlgItemName, "btnEdit") == 0)
		{
			gchar *quoted = g_shell_quote(gConfName);
			gchar *command = g_strdup_printf("xdg-open %s", quoted);
			g_free(quoted);
			g_spawn_command_line_async(command, NULL);
			g_free(command);
			g_key_file_save_to_file(gCfg, gConfName, NULL);
			MessageBox("Press OK when done editing.", PLUGNAME, MB_ICONINFORMATION | MB_OK);
			g_key_file_load_from_file(gCfg, gConfName, G_KEY_FILE_KEEP_COMMENTS, NULL);
			FillAddonsComboBox(pDlg);
		}
		else if (strcmp(DlgItemName, "btnClose") == 0)
		{
			//g_key_file_save_to_file(gCfg, gConfName, NULL);
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "cmbAddon") == 0)
		{
			char *addon = g_strdup((char*)SendDlgMsg(pDlg, "cmbAddon", DM_GETTEXT, 0, 0));

			if (!addon || addon[0] == '\0')
				break;

			SendDlgMsg(pDlg, "edExt", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "edExe", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "cbDebug", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "cbDisable", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "gbAskOpts", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "btnSet", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "cbBlacklist", DM_ENABLE, 1, 0);

			gchar *string = ADDON_GET_STRING(addon, "Description");
			SendDlgMsg(pDlg, "lblDescr", DM_SETTEXT, (intptr_t)string, 0);
			g_free(string);
			string = ADDON_GET_STRING(addon, "Extension");
			SendDlgMsg(pDlg, "edExt", DM_SETTEXT, (intptr_t)string, 0);
			g_free(string);
			string = ADDON_GET_STRING(addon, "Archiver");
			SendDlgMsg(pDlg, "edExe", DM_SETTEXT, (intptr_t)string, 0);
			string = get_exe_path(string);
			SendDlgMsg(pDlg, "lblExeError", DM_SHOWITEM, (string == NULL), 0);
			SendDlgMsg(pDlg, "lblWine", DM_SHOWITEM, check_if_win_exe(string), 0);
			SendDlgMsg(pDlg, "cbDebug", DM_SETCHECK, g_key_file_get_boolean(gCfg, addon, "Debug", NULL), 0);
			SendDlgMsg(pDlg, "cbDisable", DM_SETCHECK, !g_key_file_get_boolean(gCfg, addon, "Enabled", NULL), 0);
			g_free(string);

			SendDlgMsg(pDlg, "cbAskPack", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbAskExtract", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbAskList", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbAskDelete", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbAskTest", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbAskOnce", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbAskListPass", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbMultiVolume", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cmbVolumeSize", DM_SETTEXT, 0, 0);
			SendDlgMsg(pDlg, "cbAskPack", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskExtract", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskList", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskDelete", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskTest", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskOnce", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskListPass", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbAskMultiVolume", DM_ENABLE, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_NEW", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_MODIFY", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_MULTIPLE", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_DELETE", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_BY_CONTENT", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_SEARCHTEXT", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_HIDE", DM_SETCHECK, 0, 0);
			SendDlgMsg(pDlg, "cbPK_CAPS_ENCRYPT", DM_SETCHECK, 0, 0);


			gboolean is_anyopt = FALSE;
			string = ADDON_GET_STRING(addon, "List");

			if (string && string[0] != '\0')
			{
				if (strstr(string, "%W") != NULL)
					SendDlgMsg(pDlg, "cbAskListPass", DM_ENABLE, 1, 0);

				if (strchr(string, '{') != NULL && strstr(string, "%S") != NULL)
				{
					SendDlgMsg(pDlg, "cbAskList", DM_ENABLE, 1, 0);
					is_anyopt = TRUE;
				}

				if (!(g_key_file_get_integer(gCfg, addon, "Flags", NULL) & MAF_TARBALL))
					SendDlgMsg(pDlg, "cbPK_CAPS_MULTIPLE", DM_SETCHECK, 1, 0);
			}

			g_free(string);
			string = ADDON_GET_STRING(addon, "Add");

			if (string && string[0] != '\0')
			{
				if (strchr(string, '{') != NULL && strstr(string, "%S") != NULL)
				{
					SendDlgMsg(pDlg, "cbAskPack", DM_ENABLE, 1, 0);
					is_anyopt = TRUE;
				}

				if (strstr(string, "%V") != NULL)
					SendDlgMsg(pDlg, "cbAskMultiVolume", DM_ENABLE, 1, 0);


				SendDlgMsg(pDlg, "cbPK_CAPS_NEW", DM_SETCHECK, 1, 0);

				if (!g_key_file_get_boolean(gCfg, addon, "AddonCannotModify", NULL))
					SendDlgMsg(pDlg, "cbPK_CAPS_MODIFY", DM_SETCHECK, 1, 0);

				if (strstr(string, "%W") != NULL)
					SendDlgMsg(pDlg, "cbPK_CAPS_ENCRYPT", DM_SETCHECK, 1, 0);
			}

			g_free(string);
			string = ADDON_GET_STRING(addon, "Delete");

			if (string && string[0] != '\0')
			{
				if (strchr(string, '{') != NULL && strstr(string, "%S") != NULL)
				{
					SendDlgMsg(pDlg, "cbAskDelete", DM_ENABLE, 1, 0);
					is_anyopt = TRUE;
				}

				SendDlgMsg(pDlg, "cbPK_CAPS_DELETE", DM_SETCHECK, 1, 0);
			}

			g_free(string);
			gboolean is_caps_search = FALSE;
			string = ADDON_GET_STRING(addon, "Extract");

			if (string && string[0] != '\0')
			{
				if (strchr(string, '{') != NULL && strstr(string, "%S") != NULL)
				{
					SendDlgMsg(pDlg, "cbAskExtract", DM_ENABLE, 1, 0);
					is_anyopt = TRUE;
				}

				is_caps_search = (strstr(string, "%L") == NULL);
			}

			g_free(string);
			string = ADDON_GET_STRING(addon, "ExtractWithoutPath");

			if (string && string[0] != '\0')
			{
				if (string && strchr(string, '{') != NULL && strstr(string, "%S") != NULL)
				{
					SendDlgMsg(pDlg, "cbAskExtract", DM_ENABLE, 1, 0);
					is_anyopt = TRUE;
				}

				is_caps_search = (strstr(string, "%L") == NULL);
			}

			if (is_caps_search && !g_key_file_get_boolean(gCfg, addon, "BatchUnpack", NULL))
				SendDlgMsg(pDlg, "cbPK_CAPS_SEARCHTEXT", DM_SETCHECK, 1, 0);

			g_free(string);
			string = ADDON_GET_STRING(addon, "Test");

			if (string && strchr(string, '{') != NULL && strstr(string, "%S") != NULL)
			{
				SendDlgMsg(pDlg, "cbAskTest", DM_ENABLE, 1, 0);
				is_anyopt = TRUE;
			}

			if (is_anyopt)
				SendDlgMsg(pDlg, "cbAskOnce", DM_ENABLE, 1, 0);

			g_free(string);
			string = ADDON_GET_STRING(addon, "ID");

			if (string && string[0] != '\0')
				SendDlgMsg(pDlg, "cbPK_CAPS_BY_CONTENT", DM_SETCHECK, 1, 0);

			g_free(string);

			if (g_key_file_get_boolean(gCfg, addon, "IDOnly", NULL))
				SendDlgMsg(pDlg, "cbPK_CAPS_HIDE", DM_SETCHECK, 1, 0);

			int mode = g_key_file_get_integer(gCache, "AskMode", addon, NULL);

			SendDlgMsg(pDlg, "gbAskOpts", DM_ENABLE, 0, 0);

			for (int i = 0; names[i] != NULL; i++)
				SendDlgMsg(pDlg, names[i], DM_SETCHECK, (mode & flags[i]) ? 1 : 0, 0);

			SendDlgMsg(pDlg, "gbAskOpts", DM_ENABLE, 1, 0);
			g_free(addon);
		}
		else if (strcmp(DlgItemName, "cbDebug") == 0)
		{
			char *addon = (char*)SendDlgMsg(pDlg, "cmbAddon", DM_GETTEXT, 0, 0);
			g_key_file_set_integer(gCfg, addon, "Debug", wParam);
		}
		else if (strcmp(DlgItemName, "cbDisable") == 0)
		{
			char *addon = (char*)SendDlgMsg(pDlg, "cmbAddon", DM_GETTEXT, 0, 0);
			g_key_file_set_integer(gCfg, addon, "Enabled", !wParam);
		}
		else if (strncmp(DlgItemName, "cbAsk", 5) == 0)
		{
			int is_ready = SendDlgMsg(pDlg, "gbAskOpts", DM_ENABLE, 1, 0);
			SendDlgMsg(pDlg, "gbAskOpts", DM_ENABLE, is_ready, 0);

			if (!is_ready)
				return 0;

			char *addon = g_strdup((char*)SendDlgMsg(pDlg, "cmbAddon", DM_GETTEXT, 0, 0));
			int mode = g_key_file_get_integer(gCache, "AskMode", addon, NULL);

			for (int i = 0; names[i] != NULL; i++)
			{
				if (SendDlgMsg(pDlg, names[i], DM_GETCHECK, 0, 0) == 1)
					mode |= flags[i];
				else if (mode & flags[i])
					mode &= ~flags[i];
			}

			g_key_file_set_integer(gCache, "AskMode", addon, mode);
			g_free(addon);
		}

		break;
	}

	return 0;
}

static void ClearParsedCache(ArcData data)
{
	data->form_index = 0;
	data->cur_attr[0] = '\0';
	data->cur_day[0] = '\0';
	data->cur_ext[0] = '\0';
	data->cur_hour[0] = '\0';
	data->cur_min[0] = '\0';
	data->cur_month[0] = '\0';
	data->cur_name[0] = '\0';
	data->cur_pm = FALSE;
	data->cur_sec[0] = '\0';
	data->cur_pksize[0] = '\0';
	data->cur_size[0] = '\0';
	data->cur_year[0] = '\0';
	data->pksize_mult = 0;
	data->size_mult = 0;
}

static void FillHeaderSize(ArcData data, tHeaderDataEx *HeaderDataEx)
{
	gdouble filesize = -1;

	if (data->cur_pksize[0] != '\0')
		filesize = get_filesize(data->cur_pksize, data->pksize_mult, data->units);

	if (filesize != -1)
	{
		HeaderDataEx->PackSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->PackSize = (int64_t)filesize & 0x00000000FFFFFFFF;
	}
	else
	{
		HeaderDataEx->PackSizeHigh = 0xFFFFFFFF;
		HeaderDataEx->PackSize = 0xFFFFFFFE;
	}

	filesize = -1;

	if (data->cur_size[0] != '\0')
		filesize = get_filesize(data->cur_size, data->size_mult, data->units);

	if (filesize != -1)
	{
		HeaderDataEx->UnpSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->UnpSize = (int64_t)filesize & 0x00000000FFFFFFFF;
	}
	else
	{
		HeaderDataEx->UnpSizeHigh = 0xFFFFFFFF;
		HeaderDataEx->UnpSize = 0xFFFFFFFE;
	}
}

static void FillHeaderDate(ArcData data, tHeaderDataEx *HeaderDataEx)
{
	gboolean result = TRUE;

	if (data->cur_year[0] == '\0' &&
	                data->cur_month[0] == '\0' &&
	                data->cur_day[0] == '\0' &&
	                data->cur_hour[0] == '\0' &&
	                data->cur_min[0] == '\0' &&
	                data->cur_sec[0] == '\0')
		return;

	struct tm filetime = {0};
	struct tm arctime = *localtime(&data->st.st_mtime);

	filetime.tm_sec = atoi(data->cur_sec);

	if (filetime.tm_sec < 0 || filetime.tm_sec > 59)
	{
		if (data->debug)
			dprintf(gDebugFd, "tm_sec = %d ", filetime.tm_sec);

		result = FALSE;
	}

	filetime.tm_min = atoi(data->cur_min);

	if (filetime.tm_min < 0 || filetime.tm_min > 59)
	{
		if (data->debug)
			dprintf(gDebugFd, "tm_min = %d ", filetime.tm_min);

		result = FALSE;
	}

	filetime.tm_hour = atoi(data->cur_hour);

	if (data->cur_pm)
		filetime.tm_hour = + 12;

	if (filetime.tm_hour < 0 || filetime.tm_hour > 23)
	{
		if (data->debug)
			dprintf(gDebugFd, "tm_hour = %d ", filetime.tm_hour);

		result = FALSE;
	}

	filetime.tm_mday = atoi(data->cur_day);

	if (filetime.tm_mday < 1 || filetime.tm_mday > 31)
	{
		if (data->debug)
			dprintf(gDebugFd, "tm_mday = %d ", filetime.tm_mday);

		result = FALSE;
	}

	filetime.tm_mon = month_to_num(data);

	if (filetime.tm_mon < 0 || filetime.tm_mon > 11)
	{
		if (data->debug)
			dprintf(gDebugFd, "tm_mon = %d ", filetime.tm_mon);

		result = FALSE;
	}

	if (data->cur_year[0] == '\0')
	{
		filetime.tm_year = arctime.tm_year + 1900;
	}
	else
	{
		char *timesep = strchr(data->cur_year, ':');

		if (data->form_caps & FORM_YEARTIME && timesep != NULL)
		{
			*timesep = '\0';
			filetime.tm_hour = atoi(data->cur_year);
			filetime.tm_min = atoi(timesep + 1);
			filetime.tm_year = arctime.tm_year + 1900;
		}
		else
			filetime.tm_year = atoi(data->cur_year);
	}

	if (filetime.tm_year > 1000)
		filetime.tm_year -= 1900;
	else if (filetime.tm_year > -1)
		filetime.tm_year = 2000 + filetime.tm_year - 1900;
	else
	{
		if (data->debug)
			dprintf(gDebugFd, "tm_year = %d ", filetime.tm_year);

		result = FALSE;
	}

	if (result)
		HeaderDataEx->FileTime = (int)mktime(&filetime);

	if (data->debug)
	{
		if (!result || HeaderDataEx->FileTime < 1)
			dprintf(gDebugFd, "FileTime has not been set.\n");
	}
}

static void FillHeaderAttr(ArcData data, tHeaderDataEx *HeaderDataEx)
{
	if (HeaderDataEx->FileName[strlen(HeaderDataEx->FileName) - 1] == '/')
		HeaderDataEx->FileAttr = S_IFDIR;

	if (data->cur_attr[0] != '\0')
	{
		int len = strlen(data->cur_attr);

		if (data->form_caps & FORM_UNIXATTR || len == 10 || (len > 1 && isdigit(data->cur_attr[1])))
		{
			gboolean is_octal = FALSE;

			if (data->cur_attr[0] == 'd')
				HeaderDataEx->FileAttr = S_IFDIR;
			else if (data->cur_attr[0] == 'b')
				HeaderDataEx->FileAttr = S_IFBLK;
			else if (data->cur_attr[0] == 'c')
				HeaderDataEx->FileAttr = S_IFCHR;
			else if (data->cur_attr[0] == 'f')
				HeaderDataEx->FileAttr = S_IFIFO;
			else if (data->cur_attr[0] == 'l')
				HeaderDataEx->FileAttr = S_IFLNK;
			else if (data->cur_attr[0] == 's')
				HeaderDataEx->FileAttr = S_IFSOCK;
			else if (isdigit(data->cur_attr[0]))
			{
				HeaderDataEx->FileAttr = strtol(data->cur_attr, NULL, 8);
				is_octal = TRUE;
			}

			int i = 1;

			if (isdigit(data->cur_attr[i]))
			{
				HeaderDataEx->FileAttr |= strtol(data->cur_attr + 1, NULL, 8);
				is_octal = TRUE;
			}

			if (!is_octal)
			{
				if (data->cur_attr[i++] == 'r')
					HeaderDataEx->FileAttr |= S_IRUSR;

				if (data->cur_attr[i++] == 'w')
					HeaderDataEx->FileAttr |= S_IWUSR;

				if (data->cur_attr[i] == 'x')
					HeaderDataEx->FileAttr |= S_IXUSR;
				else if (data->cur_attr[i] == 'S')
					HeaderDataEx->FileAttr |= S_ISUID;
				else if (data->cur_attr[i] == 's')
					HeaderDataEx->FileAttr |= S_ISUID | S_IXUSR;

				i++;

				if (data->cur_attr[i++] == 'r')
					HeaderDataEx->FileAttr |= S_IRGRP;

				if (data->cur_attr[i++] == 'w')
					HeaderDataEx->FileAttr |= S_IWGRP;

				if (data->cur_attr[i] == 'x')
					HeaderDataEx->FileAttr |= S_IXGRP;
				else if (data->cur_attr[i] == 'S')
					HeaderDataEx->FileAttr |= S_ISGID;
				else if (data->cur_attr[i] == 's')
					HeaderDataEx->FileAttr |= S_ISGID | S_IXGRP;

				i++;

				if (data->cur_attr[i++] == 'r')
					HeaderDataEx->FileAttr |= S_IROTH;

				if (data->cur_attr[i++] == 'w')
					HeaderDataEx->FileAttr |= S_IWOTH;

				if (data->cur_attr[i] == 'x')
					HeaderDataEx->FileAttr |= S_IXOTH;
				else if (data->cur_attr[i] == 'T')
					HeaderDataEx->FileAttr |= S_ISVTX;
				else if (data->cur_attr[i] == 't')
					HeaderDataEx->FileAttr |= S_ISVTX | S_IXOTH;
			}

		}
		else if (data->cur_attr[0] == 'd' || strchr(data->cur_attr, 'D'))
			HeaderDataEx->FileAttr = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		else if (strchr(data->cur_attr, 'R'))
			HeaderDataEx->FileAttr = S_IRUSR | S_IRGRP | S_IROTH;
		else
			HeaderDataEx->FileAttr = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	}
}

static gboolean FillHeader(ArcData data, tHeaderDataEx *HeaderDataEx)
{
	gboolean result = TRUE;

	g_strchomp(data->cur_name);
	g_strchomp(data->cur_ext);

	char *pos = strstr(data->cur_name, "->");

	if (pos)
	{
		*pos = '\0';

		if (data->debug)
			dprintf(gDebugFd, "WARNING: the part with '->' has been removed from the Name");
	}

	if (data->cur_name[0] == '\0')
		result = FALSE;
	else
	{
		if (data->basedir[0] != '\0')
		{
			char *bak = g_strdup(data->cur_name);
			data->cur_name[0] = '\0';
			append_str(data->basedir, APPEND_DST(data->cur_name));

			if (data->form_caps & FORM_WINSEP)
				append_char('\\', APPEND_DST(data->cur_name));
			else
				append_char('/', APPEND_DST(data->cur_name));

			append_str(bak, APPEND_DST(data->cur_name));
			g_free(bak);
		}

		if (!HeaderDataEx)
		{
			if (data->form_caps & FORM_WINSEP)
				replace_win_sep(data->cur_name);

			data->dirlst = string_add_line(data->dirlst, g_path_get_dirname(data->cur_name), "\n");
			ClearParsedCache(data);
			return TRUE;
		}

		g_strstrip(data->cur_pksize);
		g_strstrip(data->cur_size);
		g_strstrip(data->cur_attr);
		g_strstrip(data->cur_month);
		g_strstrip(data->cur_year);

		if (data->debug)
		{
			dprintf(gDebugFd, "Name: \"%s\"\n", data->cur_name);
			dprintf(gDebugFd, "Ext: \"%s\"\n", data->cur_ext);
			dprintf(gDebugFd, "PackSize: \"%s\"\n", data->cur_pksize);
			dprintf(gDebugFd, "UnpSize: \"%s\"\n", data->cur_size);
			dprintf(gDebugFd, "Attr: \"%s\"\n", data->cur_attr);
			dprintf(gDebugFd, "Day: \"%s\"\n", data->cur_day);
			dprintf(gDebugFd, "Month: \"%s\"\n", data->cur_month);
			dprintf(gDebugFd, "Year: \"%s\"\n", data->cur_year);
			dprintf(gDebugFd, "Hour: \"%s%s\n", data->cur_hour, data->cur_pm ? " p\"" : "\"");
			dprintf(gDebugFd, "Min: \"%s\"\n", data->cur_min);
			dprintf(gDebugFd, "Sec: \"%s\"\n\n", data->cur_sec);
		}

		g_strlcpy(data->lastitem, data->cur_name, sizeof(data->lastitem) - 1);

		if (data->form_caps & FORM_WINSEP)
			replace_win_sep(data->cur_name);

		if (data->cur_ext[0] == '\0')
			g_strlcpy(HeaderDataEx->FileName, data->cur_name, sizeof(HeaderDataEx->FileName) - 1);
		else
		{
			g_strlcpy(HeaderDataEx->FileName, data->cur_name, sizeof(HeaderDataEx->FileName) - 1);

			if (!data->cur_dot)
			{
				append_char('.', APPEND_DST(HeaderDataEx->FileName));
				append_char('.', APPEND_DST(data->lastitem));
			}

			append_str(data->cur_ext, APPEND_DST(HeaderDataEx->FileName));
			append_str(data->cur_ext, APPEND_DST(data->lastitem));
		}

		if (data->dirs && g_strv_contains((const gchar * const *)data->dirs, HeaderDataEx->FileName))
			append_char('/', APPEND_DST(HeaderDataEx->FileName));

		if (data->form_caps & FORM_SIZE || data->form_caps & FORM_PKSIZE)
			FillHeaderSize(data, HeaderDataEx);
		else
		{
			HeaderDataEx->UnpSizeHigh = 0xFFFFFFFF;
			HeaderDataEx->UnpSize = 0xFFFFFFFE;
			HeaderDataEx->PackSizeHigh = 0xFFFFFFFF;
			HeaderDataEx->PackSize = 0xFFFFFFFE;
		}

		FillHeaderDate(data, HeaderDataEx);

		if (data->form_caps & FORM_ATTR)
			FillHeaderAttr(data, HeaderDataEx);
	}

	ClearParsedCache(data);
	return result;
}

static gboolean ParseLineRegEx(tListPattern *item, gchar *line, ArcData data, tHeaderDataEx *HeaderDataEx)
{
	gboolean is_found = FALSE;
	GMatchInfo *match_info;

	if (data->debug)
		dprintf(gDebugFd, "Pattern \"%s\" is applied\n", item->pattern);

	is_found = g_regex_match(item->re, line, 0, &match_info);

	if (is_found && g_match_info_matches(match_info))
	{
		gchar *string = g_match_info_fetch_named(match_info, "name");

		if (string)
		{
			append_str(string, APPEND_DST(data->cur_name));
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "ext");

		if (string)
		{
			append_str(string, APPEND_DST(data->cur_ext));
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "size");

		if (string)
		{
			append_str(string, APPEND_DST(data->cur_size));
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "packedSize");

		if (string)
		{
			append_str(string, APPEND_DST(data->cur_pksize));
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "attr");

		if (string)
		{
			append_str(string, APPEND_DST(data->cur_attr));
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mYear");

		if (string)
		{
			g_strlcpy(data->cur_year, string, sizeof(data->cur_year) - 1);
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mDay");

		if (string)
		{
			g_strlcpy(data->cur_day, string, sizeof(data->cur_day) - 1);
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mMonth");

		if (string)
		{
			g_strlcpy(data->cur_month, string, sizeof(data->cur_month) - 1);
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mHour");

		if (string)
		{
			g_strlcpy(data->cur_hour, string, sizeof(data->cur_hour) - 1);
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mAMPM");

		if (string)
		{
			data->cur_pm = (string[0] == 'p' || string[0] == 'P');
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mMin");

		if (string)
		{
			g_strlcpy(data->cur_min, string, sizeof(data->cur_min) - 1);
			g_free(string);
		}

		string = g_match_info_fetch_named(match_info, "mSec");

		if (string)
		{
			g_strlcpy(data->cur_sec, string, sizeof(data->cur_sec) - 1);
			g_free(string);
		}

		g_match_info_free(match_info);
	}

	data->form_index++;

	if (data->form_index == data->list_patterns->len)
		return FillHeader(data, HeaderDataEx);

	return FALSE;
}

static gboolean ParseLine(gchar *line, ArcData data, tHeaderDataEx *HeaderDataEx)
{
	size_t line_pos = 0;
	size_t line_len = strlen(line);

	if (line_len < 1)
		return FALSE;

	tListPattern *item = (tListPattern*)g_ptr_array_index(data->list_patterns, data->form_index);

	if (item->re)
		return ParseLineRegEx(item, line, data, HeaderDataEx);

	gchar *form = item->pattern;
	size_t len = strlen(form);

	for (size_t i = 0; i < len; i++)
	{
		char c = form[i];
		size_t bak_pos = line_pos;

		if (c == '+' && i > 0)
		{
			c = form[i - 1];

			if (c == 'n')
			{
				if (i == len - 1 || form[i + 1] == '+')
				{
					append_str(line + line_pos, APPEND_DST(data->cur_name));

					if (data->debug)
						debug_dump_char(c, bak_pos, line_len);
				}
				else
				{
					line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_name));

					if (data->debug)
						debug_dump_char(c, bak_pos, line_pos);
				}
			}
			else if (c == 'e')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_ext));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'a')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_attr));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'z')
			{
				if (data->form_caps & FORM_LAZY)
					line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_size));
				else
					line_pos = append_digits(line, line_pos, line_len,
					                         data->strip_chars, FORM_SIZE, data->form_caps,
					                         APPEND_DST(data->cur_size));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'p')
			{
				if (data->form_caps & FORM_LAZY)
					line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_pksize));
				else
					line_pos = append_digits(line, line_pos, line_len,
					                         data->strip_chars, FORM_SIZE, data->form_caps,
					                         APPEND_DST(data->cur_pksize));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'd')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_day));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 't' || c == 'T')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_month));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'y')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_year));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'h')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_hour));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 'm')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_min));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == 's')
			{
				line_pos = append_not_blank(line, line_pos, line_len, APPEND_DST(data->cur_sec));

				if (data->debug)
					debug_dump_char(c, bak_pos, line_pos);
			}
			else if (c == ' ')
			{
				while (line_pos < line_len)
				{
					if (isblank(line[line_pos]))
						break;

					if (data->debug)
						dprintf(gDebugFd, " ");

					line_pos++;
				}
			}
		}
		else if (c == '=' && i > 0)
		{
			size_t j;
			char mult[20] = "";
			char prev = form[i - 1];

			if (prev == '+' && i > 1)
				prev = form[i - 2];

			if (prev == 'z' || prev == 'p')
			{
				for (j = i + 1; j < len; j++)
				{
					c = form[j];

					if (isdigit(c))
						append_char(c, APPEND_DST(mult));
					else
						break;
				}

				i = j - 1;

				if (prev == 'z')
					data->size_mult = atoi(mult);
				else if (prev == 'p')
					data->pksize_mult = atoi(mult);
			}

		}
		else if (c == 'Z')
		{
			gboolean append = TRUE;

			while (c == form[i])
			{
				if (append && strchr(data->units_cache, line[line_pos]) != NULL)
				{
					append_char(line[line_pos], APPEND_DST(data->cur_size));
					line_pos++;

					if (data->debug)
						dprintf(gDebugFd, "%c", c);
				}
				else
					append = FALSE;

				i++;

				if (line_pos == line_len)
					break;
			}

			i--;
		}
		else if (c == 'P')
		{
			gboolean append = TRUE;

			while (c == form[i])
			{
				if (append && strchr(data->units_cache, line[line_pos]) != NULL)
				{
					append_char(line[line_pos], APPEND_DST(data->cur_pksize));
					line_pos++;

					if (data->debug)
						dprintf(gDebugFd, "%c", c);
				}
				else
					append = FALSE;

				i++;

				if (line_pos == line_len)
					break;
			}

			i--;
		}
		else if (c == '*')
		{
			while (line_pos < line_len)
			{
				if (isblank(line[line_pos]))
					break;

				if (data->debug)
					dprintf(gDebugFd, "?");

				line_pos++;
			}
		}
		else if (c == '(' || c == ')')
		{
			dprintf(gDebugFd, "WARNING: format with '(' or ')' chars is not supported!");
		}
		else if (c == '.')
		{
			g_strchomp(data->cur_name);
			append_char('.', APPEND_DST(data->cur_name));
			data->cur_dot = TRUE;

			if (data->debug)
				dprintf(gDebugFd, "%c", c);
		}
		else if (c == '$')
		{
			while (line_pos < line_len)
			{
				if (!isblank(line[line_pos]))
					break;

				if (data->debug)
					dprintf(gDebugFd, " ");

				line_pos++;
			}
		}
		else
		{
			if (data->debug)
				dprintf(gDebugFd, "%c", c);

			if (c == 'n')
				append_char(line[line_pos], APPEND_DST(data->cur_name));
			else if (c == 'e')
				append_char(line[line_pos], APPEND_DST(data->cur_ext));
			else if (c == 'x' && !isspace(line[line_pos]))
			{
				if (data->debug)
					dprintf(gDebugFd, "\nRejected! (position %ld: received '%c', expected space)", line_pos, line[line_pos]);

				line_pos = -1;
			}
			else if (data->form_caps & FORM_LAZY)
			{
				if (c == 'a')
					append_char(line[line_pos], APPEND_DST(data->cur_attr));
				else if (c == 'z')
					append_char(line[line_pos], APPEND_DST(data->cur_size));
				else if (c == 'p')
					append_char(line[line_pos], APPEND_DST(data->cur_pksize));
				else if (c == 'd')
					append_char(line[line_pos], APPEND_DST(data->cur_day));
				else if (c == 't' || c == 'T')
					append_char(line[line_pos], APPEND_DST(data->cur_month));
				else if (c == 'y')
					append_char(line[line_pos], APPEND_DST(data->cur_year));
				else if (c == 'h')
					append_char(line[line_pos], APPEND_DST(data->cur_hour));
				else if (c == 'H' && (line[line_pos] == 'p' || line[line_pos] == 'P'))
					data->cur_pm = TRUE;
				else if (c == 'm')
					append_char(line[line_pos], APPEND_DST(data->cur_min));
				else if (c == 's')
					append_char(line[line_pos], APPEND_DST(data->cur_sec));
			}
			else
			{
				if (c == 'a')
					append_char(line[line_pos], APPEND_DST(data->cur_attr));
				else if (c == 'z')
				{
					if (isdigit(line[line_pos]) || (data->form_caps & FORM_HUMAN && line[line_pos] == '.'))
						append_char(line[line_pos], APPEND_DST(data->cur_size));
					else if (data->form_caps & FORM_HUMAN && line[line_pos] == ',')
						append_char('.', APPEND_DST(data->cur_size));
					else if ((strchr(data->strip_chars, line[line_pos]) == NULL) && !isblank(line[line_pos]) && line[line_pos] != '-')
					{
						if (data->debug)
							dprintf(gDebugFd, "\nRejected! (position %ld: '%c' received)", line_pos, line[line_pos]);

						line_pos = -1;
					}
				}
				else if (c == 'p')
				{
					if (isdigit(line[line_pos]) || (data->form_caps & FORM_HUMAN && line[line_pos] == '.'))
						append_char(line[line_pos], APPEND_DST(data->cur_pksize));
					else if (data->form_caps & FORM_HUMAN && line[line_pos] == ',')
						append_char('.', APPEND_DST(data->cur_pksize));
					else if ((strchr(data->strip_chars, line[line_pos]) == NULL) && !isblank(line[line_pos]) && line[line_pos] != '-')
					{
						if (data->debug)
							dprintf(gDebugFd, "\nRejected! (position %ld: '%c' received)", line_pos, line[line_pos]);

						line_pos = -1;
					}
				}
				else if (c == 'd')
					line_pos = append_digit(line[line_pos], line_pos, data->debug, APPEND_DST(data->cur_day));
				else if (c == 't')
					line_pos = append_digit(line[line_pos], line_pos, data->debug, APPEND_DST(data->cur_month));
				else if (c == 'T')
					append_char(line[line_pos], APPEND_DST(data->cur_month));
				else if (c == 'y')
					append_char(line[line_pos], APPEND_DST(data->cur_year));
				else if (c == 'h')
					line_pos = append_digit(line[line_pos], line_pos, data->debug, APPEND_DST(data->cur_hour));
				else if (c == 'H')
				{
					if (line[line_pos] == 'p' || line[line_pos] == 'P')
					{
						data->cur_pm = TRUE;
					}
					else if (line[line_pos] != 'a' && line[line_pos] != 'A' && !isblank(line[line_pos]))
					{
						if (data->debug)
							dprintf(gDebugFd, "\nRejected! (position %ld: '%c' received while parsing am/pm)", line_pos, line[line_pos]);

						line_pos = -1;
					}
				}
				else if (c == 'm')
					line_pos = append_digit(line[line_pos], line_pos, data->debug, APPEND_DST(data->cur_min));
				else if (c == 's')
					line_pos = append_digit(line[line_pos], line_pos, data->debug, APPEND_DST(data->cur_sec));
			}

			if (line_pos != -1)
				line_pos++;
		}

		if (line_pos == line_len && i + 1 < len)
		{
			char *rest = form + i + 1;
			char chars[] = "adtTyhms?";
			gboolean is_short_line = FALSE;

			while (*rest++)
			{
				is_short_line = (*rest != '\0' && strchr(chars, *rest) != NULL);

				if (is_short_line)
					break;
			}

			if (is_short_line)
			{
				if (data->debug)
					dprintf(gDebugFd, "\nRejected! (the line is too short)");

				line_pos = -1;
			}
		}

		if (line_pos == -1)
		{
			if (data->debug)
				dprintf(gDebugFd, "\n");

			i = data->form_index;
			ClearParsedCache(data);

			if (i > 0)
			{
				dprintf(gDebugFd, "\n%s\n", line);
				return ParseLine(line, data, HeaderDataEx);
			}

			return FALSE;
		}

		if (line_pos >= line_len)
			break;
	}

	if (data->debug)
		dprintf(gDebugFd, "\n");

	data->form_index++;

	if (data->form_index == data->list_patterns->len)
		return FillHeader(data, HeaderDataEx);

	return FALSE;
}

static gboolean CheckLine(gchar *line, gchar *pattern)
{
	if (!line || !pattern)
		return FALSE;

	size_t len = strlen(pattern);

	if (len < 1)
		return FALSE;

	if (len == 1 && pattern[0] == '^')
		return (strlen(line) == 0);

	if (len > 1)
	{
		if (pattern[len - 1] == '$')
			return (strncmp(line + (strlen(line) - len + 1), pattern, len - 1) == 0);
		else if (pattern[0] == '^')
			return (strncmp(line, pattern + 1, len - 1) == 0);
		else
			return (strstr(line, pattern) != NULL);
	}
	else
		return (strstr(line, pattern) != NULL);

	return FALSE;
}

static int ExecuteArchiver(char *workdir, char **argv, int encoding, char *pass_str, tProcessDataProc proc, char *proc_name, char *mask, GPtrArray **out, gboolean debug, char *addon)
{
	GPid pid;
	gint in_fd;
	gint out_fd;
	gint err_fd;
	gint status = 0;
	int result = E_HANDLED;
	gchar **envp = NULL;
	GError *err = NULL;
	int progress = 0;
	char num[4] = "";
	char display_name[MAX_PATH] = "";

	if (proc_name)
		g_strlcpy(display_name, proc_name, MAX_PATH - 1);

	int flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;

	envp = g_environ_setenv(g_get_environ(), "MULTIARC", gExtensions->PluginDir, TRUE);

	if (gWineArchWin32)
		envp = g_environ_setenv(envp, "WINEARCH", "win32", TRUE);

	envp = g_environ_setenv(envp, "WINEPREFIX", gWinePrefix, TRUE);
	envp = g_environ_setenv(envp, "WINEDEBUG", "-all", TRUE);

	if (g_key_file_get_boolean(gCfg, addon, "LANG_C", NULL))
		envp = g_environ_setenv(envp, "LANG", "C", TRUE);

	if (proc && proc(display_name, progress) == 0)
		return result;

	gchar *command = g_strjoinv(" ", argv);
	dprintf(gDebugFd, "%s\n", command);
	g_free(command);

	if (!g_spawn_async_with_pipes(workdir, argv, envp, flags, NULL, NULL, &pid, &in_fd, &out_fd, &err_fd, &err))
	{
		if (err)
			dprintf(gDebugFd, "ERROR (g_spawn_async_with_pipes): %s\n", err->message);

		MessageBox(err->message, addon,  MB_OK | MB_ICONERROR);
	}

	if (in_fd == -1 && debug)
		dprintf(gDebugFd, "ERROR: STDIN fd == -1\n");

	if (out_fd == -1 && debug)
		dprintf(gDebugFd, "ERROR: STDOUT fd == -1\n");

	if (err_fd == -1 && debug)
		dprintf(gDebugFd, "ERROR: STDERR fd == -1\n");

	if (!err)
	{
		clock_t clock_start = clock();
		clock_t hup_clock_start = -1;
		char buff[BUFSIZE];
		char outline[MAX_LINEBUF] = "";
		char errline[MAX_LINEBUF] = "";
		char pass[MAX_PATH] = "";

		ssize_t len;

		ssize_t line = 0;
		ssize_t eline = 0;
		ssize_t prev_eline = -1;

		ssize_t pwstrlen = -1;
		ssize_t masklen = -1;

		if (pass_str)
			pwstrlen = strlen(pass_str);

		if (mask)
			masklen = strlen(mask);

		int outlen = 0;
		int errlen = 0;
		gboolean is_smth_asked = FALSE;
		gboolean is_aborted = FALSE;
		gboolean is_in_closed = FALSE;

		struct pollfd fds[3];
		fds[0].fd = in_fd;
		fds[0].events = POLLOUT;
		fds[1].fd = out_fd;
		fds[1].events = POLLIN | POLLHUP;
		fds[2].fd = err_fd;
		fds[2].events = POLLIN | POLLHUP;

		if (debug)
			dprintf(gDebugFd, "%s\n", EXEC_SEP);

		for (;;)
		{
			if (poll(fds, 3, 100) < 1)
			{
				int errsv = errno;

				if (debug)
					dprintf(gDebugFd, "ERROR: poll fds %s\n", strerror(errsv));

				is_aborted = TRUE;
				break;
			}
			else if (fds[0].revents & POLLERR)
			{
				int errsv = errno;

				if (debug)
					dprintf(gDebugFd, "POLLERR event %s on STDIN fd, closed\n", (errsv != 0) ? strerror(errsv) : "");

				is_in_closed = (close(in_fd) != -1);
			}
			else if (fds[1].revents & POLLERR || fds[2].revents & POLLERR)
			{
				if (debug)
					dprintf(gDebugFd, "POLLERR event on %s fd, break\n", (fds[1].revents & POLLERR)  ? "STDOUT" : "STDERR");

				is_aborted = TRUE;
				break;
			}
			else if ((fds[1].revents & POLLHUP && fds[2].revents & POLLHUP) && !(fds[0].revents & POLLOUT || fds[1].revents & POLLIN || fds[2].revents & POLLIN))
				break;
			else if (fds[0].revents & POLLOUT && !(fds[1].revents & POLLIN || fds[2].revents & POLLIN))
			{
				if (!is_smth_asked && pwstrlen > 0)
				{
					gboolean is_stderr_askpass = (errlen > pwstrlen && strncmp(errline, pass_str, pwstrlen) == 0);
					gboolean is_stdout_askpass = (outlen > pwstrlen && strncmp(outline, pass_str, pwstrlen) == 0);

					if (is_stderr_askpass || is_stdout_askpass)
					{
						if (debug)
							dprintf(gDebugFd, "POLLOUT event on STDIN, PasswordQuery string found in %s: %s \n", is_stdout_askpass ? "STDOUT" : "STDERR", is_stdout_askpass ? outline : errline);

						is_smth_asked = TRUE;

						if (pass[0] == '\0' && InputBox(addon, MSG_PASSWD, TRUE, pass, MAX_PATH) == FALSE)
							break;
						else
						{
							write(in_fd, pass, sizeof(pass));
							write(in_fd, "\r\n", 2);
							is_smth_asked = TRUE;
							prev_eline = eline;
						}
					}
				}
			}

			if (fds[2].revents & POLLIN && (len = read(err_fd, buff, sizeof(buff))) > 0)
			{
				for (int i = 0; i < len; i++)
				{
					if (buff[i] == '\n')
					{

						errline[errlen] = '\0';
						errlen = 0;

						if (debug)
							dprintf(gDebugFd, "STDERR:%s\n", errline);

						if (is_smth_asked && eline > prev_eline)
						{
							MessageBox(errline, addon, 0);
							is_aborted = TRUE;
							break;
						}

						eline++;

						memset(errline, 0, sizeof(errline));
					}
					else if (buff[i] != '\r')
					{
						if (errlen < MAX_LINEBUF)
						{
							errline[errlen] = buff[i];
							errlen++;
						}
					}
				}
			}

			if (fds[1].revents & POLLIN && (len = read(out_fd, buff, sizeof(buff))) > 0)
			{
				for (int i = 0; i < len; i++)
				{
					if (buff[i] == '\n')
					{
						line++;
						outline[outlen] = '\0';
						outlen = 0;

						if (debug)
							dprintf(gDebugFd, "STDOUT:%s\n", outline);

						if (out)
							g_ptr_array_add(*out, (gpointer)string_convert_encoding(g_strdup(outline), encoding));

						int p = atoi(num);

						if (p > progress && p <= 100)
							p = progress;

						memset(outline, 0, sizeof(outline));
						memset(num, 0, sizeof(num));
					}
					else if (buff[i] != '\r')
					{
						if (outlen < MAX_LINEBUF)
						{
							outline[outlen] = buff[i];
							outlen++;
						}

						if (mask && proc)
						{
							if (masklen > outlen && mask[outlen] == 'p')
							{
								append_char(buff[i], APPEND_DST(num));
							}
						}
					}
				}
			}

			if (is_aborted)
				break;

			if (fds[1].revents & POLLHUP && !(fds[2].revents & POLLHUP))
			{
				if (hup_clock_start == -1)
					hup_clock_start = clock();
				else if ((clock() - hup_clock_start) / CLOCKS_PER_SEC >= HUP_TIMEOUT)
				{
					dprintf(gDebugFd, "ERROR: POLLHUP event on STDOUT but no POLLHUP event on STDERR for %d sec, aborted.\n", HUP_TIMEOUT);
					break;
				}
			}

			if (proc)
			{
				if (proc(display_name, progress) == 0)
				{
					is_aborted = TRUE;
					break;
				}
			}
			else if ((clock() - clock_start) / CLOCKS_PER_SEC >= gTimeout)
			{
				if (MessageBox(MSG_TIMEOUT, addon, MB_YESNO | MB_ICONWARNING) == ID_YES)
				{
					is_aborted = TRUE;
					break;
				}
				else
					clock_start = clock();
			}
		}

		if (debug)
			dprintf(gDebugFd, "%s\n", EXEC_SEP);

		close(out_fd);
		close(err_fd);

		if (!is_in_closed)
			close(in_fd);

		if (is_aborted && kill(pid, 0) == 0)
		{
			kill(pid, SIGKILL);
			dprintf(gDebugFd, "SIGKILL %d\n", pid);
		}

		if (waitpid((int)pid, &status, 0) == -1)
		{
			int errsv = errno;
			dprintf(gDebugFd, "ERROR (waitpid): %s\n", strerror(errsv));
		}

		if (!is_aborted)
			result = WEXITSTATUS(status);

		dprintf(gDebugFd, "PID %d, exec time %.2fs, WIFEXITED %d WEXITSTATUS %d\n",
		        pid, (double)(clock() - clock_start) / CLOCKS_PER_SEC, WIFEXITED(status), WEXITSTATUS(status));
	}

	if (err)
		g_error_free(err);

	return result;
}

static int ExtractWithoutPath(gchar **src_list, gchar **dst_list, gchar *dest_path, ArcData data)
{
	guint items = 0;
	gchar *tmpfile = NULL;
	int result = E_SUCCESS;

	gchar *command = command_set_dirs(data->command, dest_path, NULL);

	gchar **argv = build_argv_from_template(command, data->archiver, data->archive,
	                                        src_list, dst_list, &tmpfile, data->wine, FALSE, &items);
	gchar *workdir = g_path_get_dirname(dst_list[0]);
	int status = ExecuteArchiver(workdir, argv, data->enc, data->pass_str, data->ProcessDataProc,
	                             src_list[0], data->progress_mask, NULL, data->debug, data->addon);
	g_free(workdir);

	if (tmpfile)
	{
		if (!data->debug)
			remove_target(tmpfile);

		g_free(tmpfile);
	}

	if (status == E_HANDLED)
		result = E_EABORTED;
	else if (!data->ignore_err && status > data->error_level)
	{
		if (data->debug)
			dprintf(gDebugFd, "ERROR: exitstatus = %d (error level = %d)\n", status, data->error_level);

		result = E_EWRITE;
	}

	return result;
}

static int ExtractWithPath(gchar **src_list, gchar **dst_list, gchar *dest_path, ArcData data)
{
	int status;
	guint items = 0;
	guint proc_items = 0;
	int result = E_SUCCESS;
	guint all_items = g_strv_length(src_list);
	gchar *workdir = NULL;
	gchar *tmpfile = NULL;
	gchar *arcname = g_path_get_basename(data->archive);

	if (dst_list)
	{
		gchar *basedir = get_dirname_from_list(dst_list);
		workdir = make_tempdir(basedir, arcname);
		g_free(basedir);
	}

	if (!workdir || !build_temp_tree(workdir, src_list, NULL, NULL))
	{
		MessageBox("Error creating temporary folder.", data->addon, MB_OK | MB_ICONERROR);
		g_free(arcname);
		g_free(workdir);
		result = E_EABORTED;
	}

	gchar *command = command_set_dirs(data->command, dest_path, workdir);

	while (proc_items < all_items)
	{
		gchar **argv = build_argv_from_template(command, data->archiver, data->archive,
		                                        src_list + proc_items, dst_list ? dst_list + proc_items : NULL,
		                                        &tmpfile, data->wine, data->batch, &items);
		status = ExecuteArchiver(workdir, argv, data->enc, data->pass_str, data->ProcessDataProc,
		                         arcname, data->progress_mask, NULL, data->debug, data->addon);
		proc_items = proc_items + items;

		if (items == 0)
			break;
	}

	g_free(command);

	if (workdir)
	{
		move_files(workdir, src_list, dst_list, data->form_caps & FORM_WINSEP);
		remove_target(workdir);
		g_free(workdir);
	}

	if (tmpfile)
	{
		if (!data->debug)
			remove_target(tmpfile);

		g_free(tmpfile);
	}

	g_free(arcname);

	if (status == E_HANDLED)
		result = E_EABORTED;
	else if (!data->ignore_err && status > data->error_level)
	{
		if (data->debug)
			dprintf(gDebugFd, "ERROR: exitstatus = %d (error level = %d)\n", status, data->error_level);

		result = E_EWRITE;
	}

	return result;
}

static int ProcessBatch(gchar *addon, int op_type, gchar *exe, gchar *archive, gchar *workdir, gchar *subdir, gchar **src_list, gchar **dst_list, gchar *pass)
{
	int status;
	guint items = 0;
	guint proc_items = 0;
	int result = E_SUCCESS;
	int encoding, error_level;
	gboolean is_wine = check_if_win_exe(exe);
	guint all_items = g_strv_length(src_list);
	gchar *tmpfile = NULL;
	gchar *tmpdir = NULL;
	gchar *arcname = g_path_get_basename(archive);
	gchar *templ = prepare_command(addon, op_type, &error_level, &encoding, is_wine, pass);

	if (!templ)
	{
		g_free(arcname);
		return E_NOT_SUPPORTED;
	}

	if (op_type != OP_DELETE)
	{
		if (subdir && subdir[0] != '\0' && strstr(templ, "%R") == NULL && strstr(templ, "%D") == NULL)
		{
			tmpdir = make_tree_temp_copy(workdir, subdir, src_list, arcname);
			set_list_subdir(src_list, subdir);
		}
		else if (strstr(templ, "%F") == NULL && strstr(templ, "%L") == NULL)
			tmpdir = make_tree_temp_copy(workdir, NULL, src_list, arcname);
	}

	gchar *command = command_set_dirs(templ, subdir, tmpdir);
	g_free(templ);

	gchar *pass_str = ADDON_GET_STRING(addon, "PasswordQuery");
	gchar *progress_mask = ADDON_GET_STRING(addon, "AddonProgressMask");
	gboolean debug = g_key_file_get_boolean(gCfg, addon, "Debug", NULL);

	while (proc_items < all_items)
	{
		gchar **argv = build_argv_from_template(command, exe, archive,
		                                        src_list + proc_items, dst_list ? dst_list + proc_items : NULL,
		                                        &tmpfile, is_wine, TRUE, &items);
		status = ExecuteArchiver(tmpdir ? tmpdir : workdir, argv, encoding, pass_str,
		                         gProcessDataProc, arcname, progress_mask, NULL, debug, addon);
		proc_items = proc_items + items;

		if (items == 0)
			break;
	}

	if (tmpdir)
	{
		remove_target(tmpdir);
		g_free(tmpdir);
	}

	if (tmpfile)
	{
		if (!debug)
			remove_target(tmpfile);

		g_free(tmpfile);
	}

	g_free(progress_mask);
	g_free(pass_str);
	g_free(command);

	g_free(arcname);


	if (status == E_HANDLED)
		result = E_EABORTED;
	else if (!g_key_file_get_boolean(gCfg, addon, "IgnoreErrors", NULL) && status > error_level)
	{
		if (debug)
			dprintf(gDebugFd, "ERROR: exitstatus = %d (error level = %d)\n", status, error_level);

		result = E_EWRITE;
	}

	return result;
}

static gboolean CheckIDs(gchar *ids, gchar *idpos, gint64 range, int fd, unsigned char **map, gint64 end)
{
	if (ids[0] == '\0')
		return FALSE;

	gboolean result = FALSE;
	gint64 offset = 0;
	gchar **id_split = g_strsplit(ids, ",", -1);
	gchar **pos_split = g_strsplit(idpos, ",", -1);

	for (char **id = id_split; *id != NULL; id++)
	{
		g_strstrip(*id);

		if (strlen(*id) < 2)
			continue;

		size_t signature_size;
		unsigned char *signature = id_to_uchar(*id, &signature_size);

		if (signature_size < 1 || signature_size > range)
		{
			g_free(signature);
			continue;
		}

		unsigned char* buf = (unsigned char*)malloc(signature_size);

		for (char **pos = pos_split; *pos != NULL; pos++)
		{
			g_strstrip(*pos);

			if (*pos[0] == '\0')
				continue;

			//g_print("CheckIDs: ID %s, IDPos %s\n", *id, *pos);

			if (strcmp(*pos, "<SeekID>") != 0)
			{
				offset = g_ascii_strtoll(*pos, NULL, 0);

				if (offset < 0)
					offset = end + offset;

				if (lseek(fd, (off_t)offset, SEEK_SET) != -1)
				{
					if (read(fd, (unsigned char *)buf, signature_size) == signature_size)
					{
						if (memcmp(buf, signature, signature_size) == 0)
							result = TRUE;
					}
				}
			}
			else
			{
				if (!*map)
				{
					if (range < 0)
						dprintf(gDebugFd, "ERROR: IDSeekRange < 0 is not supported.");
					else
						*map = mmap(0, range, PROT_READ, MAP_PRIVATE, fd, 0);

				}

				if (*map && memmem(*map, range, signature, signature_size) != NULL)
					result = TRUE;
			}

			if (result == TRUE)
				break;

		}

		g_free(buf);
		g_free(signature);

		if (result == TRUE)
			break;
	}

	g_strfreev(id_split);
	g_strfreev(pos_split);

	return result;
}

static gboolean ProbeFile(char *archive, gchar *addon, int fd)
{
	gboolean result = FALSE;
	gint64 range = MAX_FILE_RANGE;
	unsigned char *map = NULL;

	//g_print("ProbeFile: %s\n", archive);

	if (fd == -1)
		return result;

	gint64 end = lseek64(fd, 0, SEEK_END);
	gchar *idrange = ADDON_GET_STRING(addon, "IDSeekRange");

	if (idrange && idrange[0] != '\0')
		range = g_ascii_strtoll(idrange, NULL, 0);

	g_free(idrange);

	if (range > end)
		range = end;

	gchar *ids = ADDON_GET_STRING(addon, "ID");

	if (ids)
	{
		gchar *idpos = ADDON_GET_STRING(addon, "IDPos");
		result = CheckIDs(ids, (!idpos || idpos[0] == '\0') ? "0" : idpos, range, fd, &map, end);
		g_free(idpos);
		g_free(ids);

		if (!result)
		{
			for (int i = 1; i < MAX_MULTIKEYS; i++)
			{
				char key[8];
				snprintf(key, sizeof(key), "ID%d", i);
				ids = ADDON_GET_STRING(addon, key);

				if (!ids)
					break;

				snprintf(key, sizeof(key), "IDPos%d", i);
				idpos = ADDON_GET_STRING(addon, key);
				result = CheckIDs(ids, (!idpos || idpos[0] == '\0') ? "0" : idpos, range, fd, &map, end);
				g_free(idpos);
				g_free(ids);

				if (result)
					break;
			}
		}
	}

	if (map)
		munmap(map, range);

	return result;
}

static gchar* FindAddon(char *archive)
{
	gchar *result = NULL;

	result = g_key_file_get_value(gCache, "Addons", archive, NULL);

	if (result)
	{
		dprintf(gDebugFd, "\n[%s] assigned to the archive %s\n", result, archive);
		return result;
	}

	gchar **addon = NULL;
	gboolean is_found = FALSE;
	gchar **groups = g_key_file_get_groups(gCfg, NULL);

	if (!groups)
		return NULL;

	int fd = open(archive, O_RDONLY);
	gchar *filename = g_path_get_basename(archive);
	gchar *lower_name = g_ascii_strdown(filename, -1);

	for (addon = groups; *addon != NULL; addon++)
	{
		if (strcmp(*addon, "MultiArc") == 0)
			continue;

		//g_print("FindAddon(%s): %s\n", archive, *addon);

		if (!g_key_file_get_boolean(gCfg, *addon, "DontTouchThisAddonAnymore", NULL))
			ConvertAddon(gCfg, *addon);

		if (!g_key_file_get_boolean(gCfg, *addon, "Enabled", NULL))
			continue;

		if (!is_found && !g_key_file_get_boolean(gCfg, *addon, "IDOnly", NULL))
		{
			gchar *pattern = ADDON_GET_STRING(*addon, "FileNamePattern");

			if (pattern && pattern[0] != '\0')
				is_found = g_pattern_match_simple(pattern, filename);

			if (!is_found)
			{
				gchar *exts = g_key_file_get_string(gCfg, *addon, "Extension", NULL);

				if (exts)
				{
					gchar **split = g_strsplit(exts, ",", -1);

					for (char **ext = split; *ext != NULL; ext++)
					{
						gchar *mask = g_strdup_printf("*.%s", *ext);
						is_found = g_pattern_match_simple(mask, lower_name);
						g_free(mask);

						if (is_found)
							break;
					}

					g_strfreev(split);
					g_free(exts);
				}
			}
		}

		if (!is_found)
			is_found = ProbeFile(archive, *addon, fd);

		if (is_found)
		{
			if (!g_key_file_has_key(gCache, "Archivers", *addon, NULL))
			{
				gchar *exe = get_archiver(*addon);

				if (!exe)
				{
					dprintf(gDebugFd, "FindAddon (%s): archiver not found or assigned, addon will be ignored\n", *addon);
					is_found = FALSE;
					continue;
				}

				g_key_file_set_string(gCache, "Archivers", *addon, exe);
				g_free(exe);
			}

			break;
		}
	}

	g_free(filename);
	g_free(lower_name);
	close(fd);

	if (is_found)
	{
		gchar *descr = ADDON_GET_STRING(*addon, "Description");
		dprintf(gDebugFd, "\nFound registered addon [%s] %s%s%s for archive %s\n", *addon,
		        (descr && descr[0] != '\0') ? "\"" : "",
		        (descr && descr[0] != '\0') ? descr : "",
		        (descr && descr[0] != '\0') ? "\"" : "",
		        archive);
		g_free(descr);
		result = g_strdup(*addon);
		g_key_file_set_string(gCache, "Addons", archive, result);
	}

	g_strfreev(groups);

	return result;
}


int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ArcData data = (ArcData)hArcData;

	if (data->tarball)
	{
		if (data->lastitem[0] == '\0')
		{
			gchar *filename = g_path_get_basename(data->archive);
			char *dot = strrchr(filename, '.');

			if (dot)
				*dot = '\0';

			g_strlcpy(HeaderDataEx->FileName, filename, sizeof(HeaderDataEx->FileName) - 1);
			g_strlcpy(data->lastitem, filename, sizeof(data->lastitem) - 1);

			g_free(filename);

			HeaderDataEx->PackSizeHigh = ((int64_t)data->st.st_size & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->PackSize = (int64_t)data->st.st_size & 0x00000000FFFFFFFF;
			HeaderDataEx->UnpSizeHigh = 0xFFFFFFFF;
			HeaderDataEx->UnpSize = 0xFFFFFFFE;

			return E_SUCCESS;
		}

		return E_END_ARCHIVE;
	}

	while (data->cur < data->count)
	{
		gchar *line = (char*)g_ptr_array_index(data->lines, data->cur);

		if (line[strlen(line) - 1] == '\r')
			line[strlen(line) - 1] = '\0';

		if (!data->process_lines)
		{
			if (data->list_start_re)
				data->process_lines = g_regex_match(data->list_start_re, line, 0, NULL);
			else
				data->process_lines = CheckLine(line, data->list_start);

			data->cur++;
			continue;
		}
		else
		{
			if (data->list_end_re)
			{
				if (g_regex_match(data->list_end_re, line, 0, NULL) == TRUE)
					return E_END_ARCHIVE;
			}
			else if (CheckLine(line, data->list_end) == TRUE)
				return E_END_ARCHIVE;
		}

		if (data->process_lines)
		{
			if (data->debug)
				dprintf(gDebugFd, "%s%s\n", (data->form_index == 0) ? "\n" : "", line);

			gboolean ignore_line = FALSE;

			if (data->basedir_re)
			{
				GMatchInfo *match_info;

				if (g_regex_match(data->basedir_re, line, 0, &match_info) && g_match_info_matches(match_info))
				{
					gchar *string = g_match_info_fetch(match_info, 1);

					if (string)
					{
						size_t len = strlen(string);

						if (len == 1 && string[0] == '.')
							data->basedir[0] = '\0';
						else if (len > 2 && string[0] == '.' && string[1] == '/')
							g_strlcpy(data->basedir, string + 2, sizeof(data->basedir));
						else
							g_strlcpy(data->basedir, string, sizeof(data->basedir));

						g_free(string);
					}

					if (data->debug)
						dprintf(gDebugFd, "BaseDir is now set to \"%s\"\n", data->basedir);

					data->cur++;
					continue;
				}

			}

			for (gsize i = 0; i < data->ignore_stings->len; i++)
			{
				tListPattern *item = (tListPattern*)g_ptr_array_index(data->ignore_stings, i);

				if (!item->re)
					ignore_line = CheckLine(line, item->pattern);
				else
					ignore_line = g_regex_match(item->re, line, 0, NULL);

				if (ignore_line)
				{
					if (data->debug)
						dprintf(gDebugFd, "[%s]: line ignored.", item->pattern);

					break;
				}
			}

			if (!ignore_line && ParseLine(line, data, HeaderDataEx))
				return E_SUCCESS;
		}

		data->cur++;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData data = (ArcData)hArcData;

	if (data->ProcessDataProc && data->ProcessDataProc(data->lastitem, -1000) == 0)
		return E_EABORTED;

	int result = E_SUCCESS;

	if (Operation != PK_SKIP)
	{
		if (data->op_type == 0)
		{
			if (Operation == PK_EXTRACT)
			{
				if (data->batch)
				{
					data->command = prepare_command(data->addon, OP_UNPACK_PATH, &data->error_level, &data->enc, data->wine, data->pass);

					if (!data->command)
					{
						dprintf(gDebugFd, "WARNING: BatchUnpack is disabled\n");
						data->batch = FALSE;
						data->op_type = OP_UNPACK;
					}
					else
						data->op_type = OP_UNPACK_PATH;
				}
				else
				{
					data->command = prepare_command(data->addon, OP_UNPACK, &data->error_level, &data->enc, data->wine, data->pass);

					if (!data->command)
						data->op_type = OP_UNPACK_PATH;
					else
						data->op_type = OP_UNPACK;
				}
			}
			else
				data->op_type = OP_TEST;
		}

		if (!data->command)
			data->command = prepare_command(data->addon, data->op_type, &data->error_level, &data->enc, data->wine, data->pass);

		if (!data->command)
			return E_NOT_SUPPORTED;
		else if (data->batch)
		{
			if (DestPath)
			{
				g_strlcpy(data->destpath, DestPath, sizeof(data->destpath));
				data->itemlist = string_add_line(data->itemlist, g_strdup(data->lastitem), "\n");

				if (DestName && DestName[0] != '\0')
					data->filelist = string_add_line(data->filelist, g_strdup(DestName), "\n");
			}
			else if (data->destpath[0] == '\0')
			{
				data->itemlist = string_add_line(data->itemlist, g_strdup(data->lastitem), "\n");

				if (DestName && DestName[0] != '\0')
					data->filelist = string_add_line(data->filelist, g_strdup_printf("%s%s", DestPath ? DestPath : "", DestName), "\n");
			}
		}
		else if (data->command)
		{
			gchar *src_list[] = {data->lastitem, NULL};
			gchar *dst_list[] = {DestName, NULL};

			if (data->op_type != OP_UNPACK_PATH)
				result = ExtractWithoutPath(src_list, dst_list, DestPath, data);
			else
				result = ExtractWithPath(src_list, dst_list, DestPath, data);

			data->ProcessDataProc(data->lastitem, -1100);
		}

		int progress = (int)(data->cur * 100 / data->count);
		data->ProcessDataProc(DestName, -progress);
	}

	data->cur++;

	return result;
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	gchar **argv;
	gint status = 0;
	clock_t clock_start = clock();

	struct stat st;

	if (stat(ArchiveData->ArcName, &st) != 0)
	{
		ArchiveData->OpenResult = E_EOPEN;
		return NULL;
	}

	gchar *addon = FindAddon(ArchiveData->ArcName);

	if (!addon)
	{
		ArchiveData->OpenResult = E_NOT_SUPPORTED;
		return NULL;
	}

	gboolean debug = g_key_file_get_boolean(gCfg, addon, "Debug", NULL);
	gchar *archiver = g_key_file_get_string(gCache, "Archivers", addon, NULL);

	if (!archiver)
	{
		dprintf(gDebugFd, "Executable not found\n");
		ArchiveData->OpenResult = E_NOT_SUPPORTED;
		return NULL;
	}

	int dc_flags = g_key_file_get_integer(gCfg, addon, "Flags", NULL);

	if (dc_flags & MAF_TARBALL)
	{
		tArcData *data = (tArcData*)malloc(sizeof(tArcData));

		if (data == NULL)
		{
			g_free(addon);
			g_free(archiver);
			ArchiveData->OpenResult = E_NO_MEMORY;
			return NULL;
		}

		memset(data, 0, sizeof(tArcData));
		data->tarball = TRUE;
		data->archive = g_strdup(ArchiveData->ArcName);
		data->archiver = archiver;
		data->addon = addon;
		data->debug = debug;
		data->st = st;

		if (debug)
			dprintf(gDebugFd, "The archive name without last extension is used to represent the data in the archive.\n");

		return data;
	}

	char pass[MAX_PATH] = "";

	gchar *cached_pass = g_key_file_get_string(gCache, "Passwords", ArchiveData->ArcName, NULL);

	if (cached_pass)
		g_strlcpy(pass, cached_pass, MAX_PATH);

	g_free(cached_pass);

	if (ArchiveData->OpenMode == PK_OM_LIST)
	{
		int mode = g_key_file_get_integer(gCache, "AskMode", addon, NULL);

		if (mode & ASK_LISTPASS)
		{
			if (InputBox(addon, MSG_PASSWD, TRUE, pass, MAX_PATH) != FALSE)
				g_key_file_set_string(gCache, "Passwords", ArchiveData->ArcName, pass);
		}
	}

	int error_level = g_key_file_get_integer(gCfg, addon, "Errorlevel", NULL), encoding = ENC_DEF;
	gboolean is_wine = check_if_win_exe(archiver);
	gchar *templ = prepare_command(addon, OP_LIST, &error_level, &encoding, is_wine, pass);
	gchar *command = command_set_dirs(templ, NULL, NULL);
	g_free(templ);
	argv = build_argv_from_template(command, archiver, ArchiveData->ArcName, NULL, NULL, NULL, is_wine, FALSE, NULL);
	g_free(command);

	if (!argv)
	{
		g_strfreev(argv);

		if (debug)
			dprintf(gDebugFd, "ERROR: failed to get argument vector from command template.\n");

		g_free(addon);
		g_free(archiver);
		ArchiveData->OpenResult = E_NOT_SUPPORTED;
		return NULL;
	}

	gchar *workdir = g_path_get_dirname(ArchiveData->ArcName);
	gchar *pass_str = ADDON_GET_STRING(addon, "PasswordQuery");
	GPtrArray *lines = g_ptr_array_new_with_free_func(g_free);
	status = ExecuteArchiver(workdir, argv, encoding, pass_str, NULL, NULL, NULL, &lines, debug, addon);
	g_free(workdir);

	if (argv)
		g_strfreev(argv);

	if (status == E_HANDLED)
	{
		g_free(addon);
		g_free(archiver);

		if (lines)
			g_ptr_array_free(lines, TRUE);

		ArchiveData->OpenResult = E_HANDLED;
		return NULL;
	}

	if (!g_key_file_get_boolean(gCfg, addon, "IgnoreErrors", NULL) && status > error_level)
	{
		if (debug)
			dprintf(gDebugFd, "ERROR: exitstatus = %d (error level = %d)\n", status, error_level);

		g_free(addon);
		g_free(archiver);

		if (lines)
			g_ptr_array_free(lines, TRUE);

		ArchiveData->OpenResult = E_EOPEN;
		return NULL;
	}

	if (lines->len < 1)
	{
		g_free(addon);
		g_free(archiver);

		if (lines)
			g_ptr_array_free(lines, TRUE);

		ArchiveData->OpenResult = E_NO_FILES;
		return NULL;
	}

	tArcData *data = (tArcData*)malloc(sizeof(tArcData));

	if (data == NULL)
	{
		g_free(addon);
		g_free(archiver);

		if (lines)
			g_ptr_array_free(lines, TRUE);

		ArchiveData->OpenResult = E_NO_MEMORY;
		return NULL;
	}

	memset(data, 0, sizeof(tArcData));

	data->addon = addon;
	data->debug = debug;
	data->archiver = archiver;
	data->clock_start = clock_start;


	if (set_listform(addon, data))
	{
		data->archive = g_strdup(ArchiveData->ArcName);
		data->lines = lines;
		data->count = lines->len;
		data->wine = is_wine;
		data->error_level = error_level;
		data->st = st;
		data->batch = g_key_file_get_boolean(gCfg, addon, "BatchUnpack", NULL);

		if (g_key_file_get_boolean(gCfg, addon, "SearchForUglyDirs", NULL))
		{
			gboolean process_lines = data->process_lines;

			while (ReadHeaderEx(data, NULL) == E_SUCCESS)
				ProcessFile(data, PK_SKIP, NULL, NULL);

			data->dirs = g_strsplit(data->dirlst, "\n", -1);
			g_free(data->dirlst);
			ClearParsedCache(data);
			data->cur = 0;
			data->lastitem[0] = '\0';
			data->process_lines = process_lines;
		}
	}
	else
	{
		if (debug)
			dprintf(gDebugFd, "ERROR: list format is missing.\n");

		if (lines)
			g_ptr_array_free(lines, TRUE);

		g_free(data->units);
		g_free(data->months);
		g_free(archiver);
		g_free(addon);
		g_free(data);
		ArchiveData->OpenResult = E_BAD_DATA;
		return NULL;
	}

	return data;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	if (data->batch && data->itemlist)
	{
		gchar **src_list = g_strsplit(data->itemlist, "\n", -1);
		g_free(data->itemlist);

		gchar **dst_list = NULL;

		if (data->filelist)
		{
			dst_list = g_strsplit(data->filelist, "\n", -1);
			g_free(data->filelist);
		}

		int result = ExtractWithPath(src_list, dst_list, data->destpath, data);
		g_strfreev(src_list);
		g_strfreev(dst_list);

		if (result == E_EWRITE)
			MessageBox("Error while processing archive.", data->addon, MB_OK | MB_ICONERROR);
	}

	clock_t clock_start = data->clock_start;

	if (data->list_start_re)
		g_regex_unref(data->list_start_re);

	if (data->list_end_re)
		g_regex_unref(data->list_end_re);

	if (data->basedir_re)
		g_regex_unref(data->basedir_re);

	if (data->lines)
		g_ptr_array_free(data->lines, TRUE);

	if (data->list_patterns)
		g_ptr_array_free(data->list_patterns, TRUE);

	if (data->ignore_stings)
		g_ptr_array_free(data->ignore_stings, TRUE);

	if (data->dirs)
		g_strfreev(data->dirs);

	if (data->months)
		g_strfreev(data->months);

	if (data->units)
		g_strfreev(data->units);

	g_free(data->archive);
	g_free(data->addon);
	g_free(data->command);
	g_free(data->list_start);
	g_free(data->list_end);
	g_free(data->units_cache);
	g_free(data);

	dprintf(gDebugFd, "OpenArchive>{ReadHeaderEx+ProcessFile}>CloseArchive time %.2fs", (double)(clock() - clock_start) / CLOCKS_PER_SEC);

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	gchar *lines = NULL;
	gchar *dst_lines = NULL;
	char pass[MAX_PATH] = "";

	if (!(Flags & PK_PACK_SAVE_PATHS))
		return E_NOT_SUPPORTED;

	gchar *addon = 	FindAddon(PackedFile);

	if (!addon)
		return E_NOT_SUPPORTED;

	if (Flags & PK_PACK_ENCRYPT)
	{
		if (InputBox(addon, MSG_PASSWD, TRUE, pass, MAX_PATH) == TRUE)
			g_key_file_set_string(gCache, "Passwords", PackedFile, pass);
	}

	while (*AddList)
	{
		lines = string_add_line(lines, g_strdup(AddList), "\n");

		if (SubPath)
			dst_lines = string_add_line(dst_lines, g_strdup_printf("%s/%s", SubPath, AddList), "\n");
		else
			dst_lines = string_add_line(dst_lines, g_strdup(AddList), "\n");

		while (*AddList++);
	}

	gchar **src_list = g_strsplit(lines, "\n", -1);
	g_free(lines);
	gchar **dst_list = g_strsplit(dst_lines, "\n", -1);
	g_free(dst_lines);
	gchar *exe = g_key_file_get_string(gCache, "Archivers", addon, NULL);
	int op_type = (Flags & PK_PACK_MOVE_FILES) ? OP_MOVE : OP_PACK;

	int result = ProcessBatch(addon, op_type, exe, PackedFile, SrcPath, SubPath, src_list, dst_list, pass);

	g_free(exe);
	g_free(addon);
	g_strfreev(src_list);
	g_strfreev(dst_list);

	return result;
}

int DCPCALL DeleteFiles(char *PackedFile, char *DeleteList)
{
	gchar *lines = NULL;
	char pass[MAX_PATH] = "";

	gchar *addon = 	FindAddon(PackedFile);

	if (!addon)
		return E_NOT_SUPPORTED;


	gchar *cached_pass = g_key_file_get_string(gCache, "Passwords", PackedFile, NULL);

	if (cached_pass)
		g_strlcpy(pass, cached_pass, MAX_PATH);

	g_free(cached_pass);

	while (*DeleteList)
	{
		lines = string_add_line(lines, g_strdup(DeleteList), "\n");

		while (*DeleteList++);
	}

	gchar **src_list = g_strsplit(lines, "\n", -1);
	g_free(lines);
	gchar *exe = g_key_file_get_string(gCache, "Archivers", addon, NULL);

	int result = ProcessBatch(addon, OP_DELETE, exe, PackedFile, NULL, NULL, src_list, NULL, pass);

	g_free(exe);
	g_free(addon);
	g_strfreev(src_list);

	return result;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gProcessDataProc = pProcessDataProc;
	else
		data->ProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc)
{
	ArcData data = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		gChangeVolProc = pChangeVolProc;
	else
		data->ChangeVolProc = pChangeVolProc;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_MULTIPLE | PK_CAPS_OPTIONS;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	gchar *addon = FindAddon(FileName);

	if (!addon)
		return FALSE;

	g_free(addon);
	return TRUE;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	if (g_file_test(gLFMCfg, G_FILE_TEST_EXISTS))
		gExtensions->DialogBoxLFMFile(gLFMCfg, OptDlgProc);
	else
		MessageBox(MSG_LFM_MISSIND, PLUGNAME, MB_OK | MB_ICONERROR);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
		sprintf(gLFMCfg, "%.*s%s", PATH_MAX - (int)strlen(LFM_CFG) - 2, gExtensions->PluginDir, LFM_CFG);
		sprintf(gLFMAsk, "%.*s%s", PATH_MAX - (int)strlen(LFM_ASK) - 2, gExtensions->PluginDir, LFM_ASK);
		sprintf(gLFMImport, "%.*s%s", PATH_MAX - (int)strlen(LFM_IMPORT) - 2, gExtensions->PluginDir, LFM_IMPORT);
		g_setenv("MULTIARC", gExtensions->PluginDir, TRUE);

		gDebugFd = g_file_open_tmp(PLUGNAME "_XXXXXX.log", &gDebugFileName, NULL);

		gCache = g_key_file_new();

		gboolean is_cfg_update = FALSE;

		gCfg = g_key_file_new();
		gConfName = g_strdup_printf("%s/%s.ini", gExtensions->PluginConfDir, PLUGNAME);

		if (!g_key_file_load_from_file(gCfg, gConfName, G_KEY_FILE_KEEP_COMMENTS, NULL))
		{
			gchar *defconf = g_strdup_printf("%s/settings_default.ini", gExtensions->PluginDir);
			g_key_file_load_from_file(gCfg, defconf, G_KEY_FILE_KEEP_COMMENTS, NULL);
			g_free(defconf);
			is_cfg_update = TRUE;
		}

		gHome = g_getenv("HOME");

		gEncodingANSI = g_key_file_get_string(gCfg, "MultiArc", "ANSI", NULL);
		gEncodingOEM = g_key_file_get_string(gCfg, "MultiArc", "OEM", NULL);

		if (!gEncodingANSI || !gEncodingOEM)
		{
			const gchar* lang = g_getenv("LANG");

			if (strncmp(lang, "ru_RU", 5) == 0)
			{
				g_key_file_set_string(gCfg, "MultiArc", "ANSI", "CP1251");
				g_key_file_set_string(gCfg, "MultiArc", "OEM", "CP866");
			}
			else
			{
				g_key_file_set_string(gCfg, "MultiArc", "ANSI", "CP1252");
				g_key_file_set_string(gCfg, "MultiArc", "OEM", "CP437");
			}

			g_free(gEncodingANSI);
			g_free(gEncodingOEM);
			gEncodingANSI = g_key_file_get_string(gCfg, "MultiArc", "ANSI", NULL);
			gEncodingOEM = g_key_file_get_string(gCfg, "MultiArc", "OEM", NULL);
			is_cfg_update = TRUE;
		}

		int timeout = g_key_file_get_integer(gCfg, "MultiArc", "ListTimeout", NULL);

		if (timeout > 5)
			gTimeout = timeout;
		else
		{
			g_key_file_set_integer(gCfg, "MultiArc", "ListTimeout", gTimeout);
			is_cfg_update = TRUE;
		}

		gchar *string = g_key_file_get_string(gCfg, "MultiArc", "WineRootFSDrive", NULL);

		if (string && strlen(string) == 2 && string[1] == ':')
			g_strlcpy(gWineDrive, string, sizeof(gWineDrive));
		else
		{
			g_key_file_set_string(gCfg, "MultiArc", "WineRootFSDrive", "Z:");
			is_cfg_update = TRUE;
		}

		g_free(string);

		gWinePrefix = g_key_file_get_string(gCfg, "MultiArc", "WINEPREFIX", NULL);

		if (!gWinePrefix || strlen(gWinePrefix) < 3)
		{
			g_key_file_set_string(gCfg, "MultiArc", "WINEPREFIX", "$HOME/.wine_" PLUGNAME);
			gWinePrefix = g_key_file_get_string(gCfg, "MultiArc", "WINEPREFIX", NULL);
			is_cfg_update = TRUE;
		}

		gWineBin = g_key_file_get_string(gCfg, "MultiArc", "WineBin", NULL);

		if (!gWineBin)
		{
			g_key_file_set_string(gCfg, "MultiArc", "WineBin", "/usr/bin");
			gWineBin = g_key_file_get_string(gCfg, "MultiArc", "WineBin", NULL);
			is_cfg_update = TRUE;
		}

		if (gWinePrefix[0] != '/')
		{
			if (gWinePrefix[0] == '~')
			{
				gchar *tmp = gWinePrefix;
				gWinePrefix = g_strdup_printf("%s%s", gHome, tmp + 1);
				g_free(tmp);
			}
			else
				gWinePrefix = replace_env_vars(gWinePrefix);
		}

		if (!g_key_file_has_key(gCfg, "MultiArc", "WineArchWin32", NULL))
		{
			g_key_file_set_boolean(gCfg, "MultiArc", "WineArchWin32", FALSE);
			is_cfg_update = TRUE;
		}

		if (!g_key_file_has_key(gCfg, "MultiArc", "WineServerPersistent", NULL))
		{
			g_key_file_set_boolean(gCfg, "MultiArc", "WineServerPersistent", TRUE);
			is_cfg_update = TRUE;
		}

		gWineArchWin32 = g_key_file_get_boolean(gCfg, "MultiArc", "WineArchWin32", NULL);
		gboolean is_persistent = g_key_file_get_boolean(gCfg, "MultiArc", "WineServerPersistent", NULL);

		if (is_persistent)
		{
			gchar *command = g_strdup_printf("env WINEPREFIX='%s' %s '%s/wineserver' -p",
			                                 gWinePrefix, gWineArchWin32 ? "WINEARCH=win32" : "", gWineBin);
			gWineServerCalled = g_spawn_command_line_async(command, NULL);

			if (gWineServerCalled)
				dprintf(gDebugFd, "Starting persistent wineserver:\n%s\n%s\n", command, gWineServerCalled ? "OK" : "FAIL");

			g_free(command);
		}

		if (is_cfg_update)
			g_key_file_save_to_file(gCfg, gConfName, NULL);
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
		free(gExtensions);

	gExtensions = NULL;

	if (gCfg != NULL)
	{
		g_key_file_save_to_file(gCfg, gConfName, NULL);
		g_key_file_free(gCfg);
	}

	gCfg = NULL;

	if (gConfName != NULL)
		g_free(gConfName);

	gConfName = NULL;

	if (gCache != NULL)
		g_key_file_free(gCache);

	gCache = NULL;

	if (gEncodingANSI != NULL)
		g_free(gEncodingANSI);

	gEncodingANSI = NULL;

	if (gEncodingOEM != NULL)
		g_free(gEncodingOEM);

	gEncodingOEM = NULL;

	if (gWinePrefix != NULL)
	{
		if (gWineServerCalled)
		{
			gchar *command = g_strdup_printf("env WINEPREFIX='%s' %s '%s/wineserver' -k",
			                                 gWinePrefix, gWineArchWin32 ? "WINEARCH=win32" : "", gWineBin);
			g_spawn_command_line_async(command, NULL);
			g_free(command);
		}

		g_free(gWinePrefix);
	}

	gWinePrefix = NULL;

	if (gWineBin != NULL)
		g_free(gWineBin);

	gWineBin = NULL;

	if (gDebugFd != -1)
	{
		close(gDebugFd);
		remove(gDebugFileName);
		g_free(gDebugFileName);
	}
}

