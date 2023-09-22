#define _GNU_SOURCE
#include <glib.h>
#include <math.h>
#include <time.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/uri.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include "wfxplugin.h"
#include "extension.h"

#define Int32x32To64(a,b) ((gint64)(a)*(gint64)(b))
#define DEFAULT_URL "https://vc.kiev.ua/download/"

typedef struct sVFSDirData
{
	xmlXPathObjectPtr obj;
	xmlNodeSetPtr nodeset;
	gsize index;
	CURL *curl;
} tVFSDirData;

typedef struct sOutFile
{
	char *filename;
	xmlChar *url;
	FILE *fp;
	size_t total;
	size_t cur;
} tOutFile;

typedef struct sMemBuf
{
	char *buf;
	size_t size;
} tMemBuf;

typedef struct sPlugSettings
{
	gboolean init;
	gboolean ask_sizes;
	gboolean follow;
	gboolean verbose;
	gboolean noquery;
	gboolean fail;
	long max_redir;
	long timeout;
	char url[PATH_MAX];
	char user_agent[MAX_PATH];
} tPlugSettings;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;
xmlDocPtr gDoc = NULL;
CURL *gCurl = NULL;
clock_t gClock;

static char gLFMPath[PATH_MAX];
static char gHistoryFile[PATH_MAX];
static char gSelectedFile[PATH_MAX];
tPlugSettings gSettings;


static gboolean UnixTimeToFileTime(unsigned long mtime, LPFILETIME ft)
{
	gint64 ll = Int32x32To64(mtime, 10000000) + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
	return TRUE;
}

static void SetCurrentFileTime(LPFILETIME ft)
{
	gint64 ll = g_get_real_time();
	ll = ll * 10 + 116444736000000000;
	ft->dwLowDateTime = (DWORD)ll;
	ft->dwHighDateTime = ll >> 32;
}

static void errmsg(char *msg)
{
	gLogProc(gPluginNr, MSGTYPE_IMPORTANTERROR, msg);
	g_print("%s\n", msg);
}
/*
static void errmsg_dialog(char *msg)
{

	if (gDialogApi)
		gDialogApi->MessageBox(msg, "Double Commander", MB_OK | MB_ICONERROR);
	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, msg, NULL, 0);
	else
		g_print("%s\n", msg);
}
*/
static size_t write_to_file_cb(void *buffer, size_t size, size_t nitems, void *userdata)
{
	tOutFile *data = (tOutFile*)userdata;

	if (!data->fp)
	{
		data->fp = fopen(data->filename, "wb");

		if (!data->fp)
		{
			errmsg(strerror(EBADFD));
			return 0;
		}
	}

	int done = 0;
	size_t out = fwrite(buffer, size, nitems, data->fp);

	if (data->total > 0)
	{
		data->cur += out;

		if (data->cur > 0)
			done = (int)(data->cur * 100 / data->total);

		if (done > 100)
			done = 100;
	}

	if (gProgressProc(gPluginNr, (char*)data->url, data->filename, done))
		return 0;

	return out;
}

static size_t write_to_buffer_cb(void *buffer, size_t size, size_t nitems, void *userdata)
{
	size_t realsize = size * nitems;
	tMemBuf *mem = (tMemBuf*)userdata;

	char *ptr = realloc(mem->buf, mem->size + realsize + 1);

	if (ptr == NULL)
	{
		errmsg(strerror(ENOMEM));
		return 0;
	}

	mem->buf = ptr;
	memcpy(&(mem->buf[mem->size]), buffer, realsize);
	mem->size += realsize;
	mem->buf[mem->size] = 0;

	return realsize;
}

static size_t do_nothing_cb(void *buffer, size_t size, size_t nitems, void *userdata)
{
	return size * nitems;
}

static xmlChar *name_to_url(char *name, xmlDocPtr doc)
{
	xmlChar *url = NULL;

	gchar *xpath = g_strdup_printf("/links/link[@name=\"%s\"]", name);
	xmlXPathContextPtr context = xmlXPathNewContext(doc);
	xmlXPathObjectPtr obj = xmlXPathEvalExpression((xmlChar*)xpath, context);
	xmlXPathFreeContext(context);
	g_free(xpath);

	if (obj)
	{
		xmlNodeSetPtr nodeset = obj->nodesetval;

		if (!xmlXPathNodeSetIsEmpty(nodeset))
			url = xmlNodeGetContent(nodeset->nodeTab[0]);

		xmlXPathFreeObject(obj);
	}

	return url;
}

static void curl_set_additional_options(CURL *curl)
{
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, (long)gSettings.follow);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)gSettings.verbose);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, gSettings.max_redir);

	if (gSettings.user_agent[0] != '\0')
		curl_easy_setopt(curl, CURLOPT_USERAGENT, gSettings.user_agent);

	curl_easy_setopt(curl, CURLOPT_FAILONERROR, (long)gSettings.fail);
}

static gchar *parse_date(char *text, gint index)
{
	struct tm tm = {0};
	gchar *formats[] =
	{
		"%d-%b-%Y %R",
		"%Y-%m-%d %R",
		"%Y.%m.%d %R",
		"%d-%b-%Y  %k:%M",
		NULL
	};

	if (!formats[index])
		return NULL;

	if (!strptime(text, formats[index], &tm))
		return NULL;

	return g_strdup_printf("%ld", (long)mktime(&tm));
}

static gchar *parse_size(char *text)
{
	if (!g_regex_match_simple("^[\\d.]+[kKmMgGtT]?$", text, 0, 0))
		return NULL;

	gdouble size = g_ascii_strtod(text, NULL);

	gchar last = text[strlen(text) - 1];

	if (last == 'k' || last == 'K')
		size = size * 1024;
	else if (last == 'm' || last == 'M')
		size = size * pow(1024, 2);
	else if (last == 'g' || last == 'G')
		size = size * pow(1024, 3);
	else if (last == 't' || last == 'T')
		size = size * pow(1024, 4);

	return g_strdup_printf("%.0f", size);
}

static void parse_fileinfo(char *text, xmlNodePtr link, xmlDocPtr doc)
{
	GRegex *regex;
	GMatchInfo *match_info;
	guint re_index = 0;
	gboolean found = FALSE;

	gchar *regexs[] =
	{
		"(\\d{2}\\-\\w{3}\\-\\d{4}\\s\\d{2}:\\d{2})\\s+([\\d\\.]*[-kKmMgGtT]?)",
		"(\\d{4}\\-\\d{2}\\-\\d{2}\\s\\d{2}:\\d{2})\\s+([\\d\\.]*[-kKmMgGtT]?)",
		"([\\d\\.]*[-kKmMgGtT]?)\\s+(\\d{4}\\.\\d{2}\\.\\d{2}\\s\\d{2}:\\d{2})",
		"(\\d\\d?\\-\\w{3}\\-\\d{4}\\s{2}\\d\\d?:\\d{2})\\s+([\\d\\.]*[-kKmMgGtT]?).*",
		NULL
	};

	for (gchar **p = regexs; *p != NULL; p++)
	{
		regex = g_regex_new(*p, 0, 0, NULL);
		g_regex_match(regex, text, 0, &match_info);

		if (g_match_info_matches(match_info))
		{
			for (gint i = 1; i <= 2; i++)
			{
				gchar *match = g_match_info_fetch(match_info, i);

				gchar *size = parse_size(match);

				if (size)
				{
					xmlNewProp(link, (xmlChar*)"size", (xmlChar*)size);
					g_free(size);
					found = TRUE;
				}
				else
				{
					gchar *date = parse_date(match, re_index);

					if (date)
					{
						xmlNewProp(link, (xmlChar*)"date", (xmlChar*)date);
						g_free(date);
						found = TRUE;
					}
				}

				g_free(match);
			}
		}

		g_match_info_free(match_info);
		g_regex_unref(regex);

		if (found)
			break;

		re_index++;
	}
}

static gboolean is_reparse_point(char *url)
{
	gboolean result = FALSE;
	char *link = g_strdup(url);

	char *pos = strrchr(link, '?');

	if (pos)
		*pos = '\0';

	if (url[strlen(link) - 1] == '/')
	{
		g_free(link);
		return TRUE;
	}

	char *dot = strrchr(link, '.');

	if (!dot)
	{
		g_free(link);
		return FALSE;
	}

	char *exts[] =
	{
		".htm",
		".html",
		".php",
		".xml",
		NULL
	};

	for (char **p = exts; *p != NULL; p++)
	{
		if (g_strcmp0(*p, dot) == 0)
		{
			result = TRUE;
			break;
		}
	}

	g_free(link);

	return result;
}

static gboolean is_ext_candidate(char *text, char *dot)
{
	if (strcmp("..", text) == 0 || strcasecmp("Parent Directory", text) == 0)
		return FALSE;

	char *exts[] =
	{
		".htm",
		".html",
		".php",
		".xml",
		".zip",
		".gz",
		".png",
		".jpg",
		".json",
		".css",
		".js",
		NULL
	};

	for (char **p = exts; *p != NULL; p++)
	{
		if (g_strcmp0(*p, dot) == 0 && !g_strrstr(text, dot))
			return TRUE;
	}

	return FALSE;
}

static gchar *prepare_name(char *text, xmlDocPtr doc)
{
	gint index = 1;
	xmlChar *url = NULL;
	gchar **split = NULL;
	gchar *temp = g_strdup(text);

	if (temp[strlen((char*)temp) - 1] == '/')
		temp[strlen((char*)temp) - 1] = '\0';

	if (strchr((char*)temp, '/') != NULL)
	{
		gchar *end = g_path_get_basename(temp);
		g_free(temp);
		temp = end;
	}

	gchar *grabage[] =
	{
		"https://",
		"http://",
		"ftp://",
		"\a",
		"\b",
		"\f",
		"\n",
		"\r",
		"\t",
		"\v",
		"\\",
		"\"",
		"/",
		"?",
		"→",
		"  ",
		NULL
	};

	for (gchar **p = grabage; *p != NULL; p++)
	{
		split = g_strsplit(temp, *p, -1);
		g_free(temp);
		temp = g_strjoinv(" ", split);
		g_strfreev(split);
	}

	gchar *name = g_utf8_make_valid(temp, -1);
	g_free(temp);
	g_strstrip(name);
	gchar *result = g_strdup(name);

	char *ext = "";
	char *dot = strrchr(name, '.');

	if (dot)
	{
		ext = dot + 1;
		*dot = '\0';
	}

	while ((url = name_to_url(result, doc)) != NULL && index < MAX_PATH)
	{
		xmlFree(url);
		g_free(result);
		result = g_strdup_printf("%s (%d)%c%s", name, index, ext[0] != '\0' ? '.' : '\0', ext);
		index++;
	}

	g_free(name);

	return result;
}

static xmlNodePtr get_links(gchar *url)
{
	htmlDocPtr doc;
	CURLcode res;
	char name[MAX_PATH];
	xmlNodePtr result = NULL;
	tMemBuf membuf;

	if (!gCurl)
		return NULL;

	membuf.buf = malloc(1);
	membuf.size = 0;

	curl_easy_setopt(gCurl, CURLOPT_URL, url);
	curl_set_additional_options(gCurl);
	curl_easy_setopt(gCurl, CURLOPT_WRITEFUNCTION, write_to_buffer_cb);
	curl_easy_setopt(gCurl, CURLOPT_WRITEDATA, (void *)&membuf);
	curl_easy_setopt(gCurl, CURLOPT_TIMEOUT, gSettings.timeout);

	res = curl_easy_perform(gCurl);

	if (res != CURLE_OK)
	{
		errmsg(curl_easy_strerror(res) ? (char*)curl_easy_strerror(res) : "unknown error");
		free(membuf.buf);
		return NULL;
	}

	doc = htmlReadMemory(membuf.buf, membuf.size, url, NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);

	free(membuf.buf);

	if (!doc)
		return NULL;

	xmlChar *xpath = (xmlChar*)"//@href";
	xmlXPathContextPtr context = xmlXPathNewContext(doc);
	xmlXPathObjectPtr obj = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);

	if (!obj)
	{
		xmlFreeDoc(doc);
		return NULL;
	}

	xmlNodeSetPtr nodeset = obj->nodesetval;

	if (!xmlXPathNodeSetIsEmpty(nodeset))
	{
		xmlDocPtr tmpdoc = xmlNewDoc(BAD_CAST "1.0");
		result = xmlNewNode(NULL, BAD_CAST "links");
		xmlDocSetRootElement(tmpdoc, result);

		for (int i = 0; i < nodeset->nodeNr; i++)
		{
			clock_t cur_time = (double)(clock() - gClock) / CLOCKS_PER_SEC;

			if ((long)cur_time >= gSettings.timeout)
			{
				errmsg("Retrieving link list aborted: timed out");
				break;
			}

			xmlNodePtr link = xmlNewNode(NULL, BAD_CAST "link");

			const xmlNode *node = nodeset->nodeTab[i]->parent;
			xmlChar *href = xmlGetProp(node, (xmlChar*)"href");

			if (!href)
				continue;

			if (href[0] == '\0' || href[0] == '#' || strchr((char*)href, '{') || (gSettings.noquery && strrchr((char*)href, '?')))
			{
				continue;
				xmlFree(href);
			}

			xmlChar *text = xmlNodeGetContent(node);
			gchar *temp = NULL;

			if (!text || text[0] == '\0')
				temp = prepare_name((char*)href, tmpdoc);
			else
			{
				size_t len = strlen((char*)text);

				if (len > 3)
				{
					char *p = (char*)text + (len - 3);

					if (g_strcmp0(p, "..>") == 0)
						temp = prepare_name((char*)href, tmpdoc);
					else
					{
						char *dot = strrchr((char*)href, '.');

						if (dot && is_ext_candidate((char*)text, dot))
						{
							if (text[strlen((char*)text) - 1] == '/')
								text[strlen((char*)text) - 1] = '\0';

							gchar *concat = g_strconcat((char*)text, dot, NULL);
							temp = prepare_name((char*)concat, tmpdoc);
							g_free(concat);
						}
					}
				}

				if (!temp)
					temp = prepare_name((char*)text, tmpdoc);
			}

			xmlFree(text);
			g_strlcpy(name, temp, MAX_PATH);
			g_free(temp);


			if (strlen(name) < 1)
			{
				xmlFree(href);
				continue;
			}

			if (strcmp("..", (char*)name) == 0 || strcasecmp("Parent Directory", (char*)name) == 0)
				xmlNewProp(link, (xmlChar*)"name", (xmlChar*)"<Parent Directory>");
			else
				xmlNewProp(link, (xmlChar*)"name", (xmlChar*)name);

			if (node->next && node->next->type == XML_TEXT_NODE)
			{
				xmlNewProp(link, (xmlChar*)"extra", (xmlChar*)g_strstrip((char *)node->next->content));
				parse_fileinfo((char*)node->next->content, link, tmpdoc);
			}
			else if (node->parent)
			{
				xmlNodePtr parent = node->parent;

				if (parent->name && strcmp((char*)parent->name, "td") == 0)
				{
					gboolean extra2_is_text = FALSE;
					xmlNodePtr extra1 = parent->next;

					if (extra1 && extra1->name && strcmp((char*)extra1->name, "td") == 0)
					{
						if (extra1->children && extra1->children->type == XML_TEXT_NODE)
						{
							xmlNodePtr extra2 = extra1->next;

							if (extra2 && extra2->name && strcmp((char*)extra2->name, "td") == 0)
							{
								if (extra2->children && extra2->children->type == XML_TEXT_NODE)
								{
									gchar *temp = g_strjoin(" ", (char *)extra1->children->content, (char *)extra2->children->content, NULL);
									xmlNewProp(link, (xmlChar*)"extra", (xmlChar*)g_strstrip(temp));
									parse_fileinfo(temp, link, tmpdoc);
									extra2_is_text = TRUE;
									g_free(temp);
								}

							}

							if (!extra2_is_text)
							{
								xmlNewProp(link, (xmlChar*)"extra", (xmlChar*)g_strstrip((char *)extra1->children->content));
								parse_fileinfo((char*)extra1->children->content, link, tmpdoc);
							}
						}
					}
				}
			}

			xmlChar *orig = href;
			href = xmlBuildURI(href, (xmlChar*)url);
			xmlFree(orig);
			xmlNodeSetContent(link, href);
			xmlFree(href);
			xmlAddChild(result, link);
		}

		xmlUnlinkNode(result);
		xmlFreeDoc(tmpdoc);
	}

	xmlXPathFreeObject(obj);
	xmlFreeDoc(doc);

	return result;
}

intptr_t DCPCALL DlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	FILE *fp;
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;
	int count = 0;

	switch (Msg)
	{
	case DN_INITDIALOG:

		if (!gSettings.init)
			gDialogApi->SendDlgMsg(pDlg, "lblInfo", DM_SHOWITEM, 0, 0);

		if ((fp = fopen(gHistoryFile, "r")) != NULL)
		{
			while ((read = getline(&line, &len, fp)) != -1 && count < MAX_PATH)
			{
				if (line[read - 1] == '\n')
					line[read - 1] = '\0';

				if (strlen(line) > 0)
				{
					gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTADD, (intptr_t)line, 0);
					count++;
				}
			}

			g_free(line);
			fclose(fp);
		}

		if (count == 0 && gSettings.url[0] != '\0')
			gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTADD, (intptr_t)gSettings.url, 0);

		if (gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETCOUNT, 0, 0) == 0)
			gDialogApi->SendDlgMsg(pDlg, "btnRemove", DM_ENABLE, 0, 0);

		if (gSettings.url[0] == '\0')
			gDialogApi->SendDlgMsg(pDlg, "btnBrowser", DM_ENABLE, 0, 0);

		gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_SETTEXT, (intptr_t)gSettings.url, 0);

		if (gSelectedFile[0] != '\0' && strcmp("/", gSelectedFile) != 0 && strcmp("..", gSelectedFile) != 0)
		{
			gDialogApi->SendDlgMsg(pDlg, "gbCurrent", DM_SHOWITEM, 1, 0);
			gDialogApi->SendDlgMsg(pDlg, "edName", DM_SETTEXT, (intptr_t)gSelectedFile, 0);
			xmlChar *url = name_to_url(gSelectedFile, gDoc);
			gDialogApi->SendDlgMsg(pDlg, "edURL", DM_SETTEXT, (intptr_t)url, 0);
			gDialogApi->SendDlgMsg(pDlg, "btnSelected", DM_ENABLE, 1, 0);
			xmlFree(url);
			char extra[256];

			if (FsContentGetValue(gSelectedFile, 1, 0, (void*)extra, 256, 0) == ft_string)
				gDialogApi->SendDlgMsg(pDlg, "edExtra", DM_SETTEXT, (intptr_t)extra, 0);
			else
			{
				gDialogApi->SendDlgMsg(pDlg, "edExtra", DM_SHOWITEM, 0, 0);
				gDialogApi->SendDlgMsg(pDlg, "lblExtra", DM_SHOWITEM, 0, 0);
			}

		}
		else
			gDialogApi->SendDlgMsg(pDlg, "gbCurrent", DM_SHOWITEM, 0, 0);

		gDialogApi->SendDlgMsg(pDlg, "ckSizes", DM_SETCHECK, (intptr_t)gSettings.ask_sizes, 0);
		gDialogApi->SendDlgMsg(pDlg, "ckFollow", DM_SETCHECK, (intptr_t)gSettings.follow, 0);
		gDialogApi->SendDlgMsg(pDlg, "ckVerbose", DM_SETCHECK, (intptr_t)gSettings.verbose, 0);
		gDialogApi->SendDlgMsg(pDlg, "ckFail", DM_SETCHECK, (intptr_t)gSettings.fail, 0);
		gDialogApi->SendDlgMsg(pDlg, "ckNoQuery", DM_SETCHECK, (intptr_t)gSettings.noquery, 0);

		gchar *num = g_strdup_printf("%ld", gSettings.max_redir);
		gDialogApi->SendDlgMsg(pDlg, "edMaxRedir", DM_SETTEXT, (intptr_t)num, 0);
		g_free(num);

		num = g_strdup_printf("%ld", gSettings.timeout);
		gDialogApi->SendDlgMsg(pDlg, "edTimeout", DM_SETTEXT, (intptr_t)num, 0);
		g_free(num);

		gDialogApi->SendDlgMsg(pDlg, "cbUserAgent", DM_SETTEXT, (intptr_t)gSettings.user_agent, 0);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			g_strlcpy(gSettings.url, (char*)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_GETTEXT, 0, 0), PATH_MAX);

			if ((fp = fopen(gHistoryFile, "w")) != NULL)
			{
				if (gSettings.url[0] != '\0')
					fprintf(fp, "%s\n", gSettings.url);

				count = (int)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETCOUNT, 0, 0);

				for (int i = 0; i < count; i++)
				{
					line = (char*)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETITEM, i, 0);

					if (line && (strcmp(gSettings.url, line) != 0))
						fprintf(fp, "%s\n", line);

				}

				fclose(fp);
			}

			gSettings.ask_sizes = (gboolean)gDialogApi->SendDlgMsg(pDlg, "ckSizes", DM_GETCHECK, 0, 0);
			gSettings.follow = (gboolean)gDialogApi->SendDlgMsg(pDlg, "ckFollow", DM_GETCHECK, 0, 0);
			gSettings.verbose = (gboolean)gDialogApi->SendDlgMsg(pDlg, "ckVerbose", DM_GETCHECK, 0, 0);
			gSettings.fail = (gboolean)gDialogApi->SendDlgMsg(pDlg, "ckFail", DM_GETCHECK, 0, 0);
			gSettings.noquery = (gboolean)gDialogApi->SendDlgMsg(pDlg, "ckNoQuery", DM_GETCHECK, 0, 0);

			line = (char*)gDialogApi->SendDlgMsg(pDlg, "edMaxRedir", DM_GETTEXT, 0, 0);
			gSettings.max_redir = (long)g_ascii_strtod((char*)line, NULL);

			line = (char*)gDialogApi->SendDlgMsg(pDlg, "edTimeout", DM_GETTEXT, 0, 0);
			gSettings.timeout = (long)g_ascii_strtod((char*)line, NULL);

			line = (char*)gDialogApi->SendDlgMsg(pDlg, "cbUserAgent", DM_GETTEXT, 0, 0);
			g_strlcpy(gSettings.user_agent, line, MAX_PATH);

			if (gSettings.init)
				gSettings.init = FALSE;

			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}
		else if (strcmp(DlgItemName, "btnCancel") == 0)
		{
			if (gSettings.init)
				gSettings.url[0] = '\0';

			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_CANCEL, 0);
		}
		else if (strcmp(DlgItemName, "btnGo") == 0)
		{
			line = (char*)gDialogApi->SendDlgMsg(pDlg, "edURL", DM_GETTEXT, 0, 0);

			if (line && line[0] != '\0')
			{
				gchar *command = g_strdup_printf("xdg-open %s", line);
				system(command);
			}
		}
		else if (strcmp(DlgItemName, "btnBrowser") == 0)
		{
			line = (char*)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_GETTEXT, 0, 0);

			if (line && line[0] != '\0')
			{
				gchar *command = g_strdup_printf("xdg-open %s", line);
				system(command);
			}
		}
		else if (strcmp(DlgItemName, "btnSelected") == 0)
		{
			line = (char*)gDialogApi->SendDlgMsg(pDlg, "edURL", DM_GETTEXT, 0, 0);

			if (line && line[0] != '\0')
			{
				gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_SETTEXT, (intptr_t)line, 0);
			}
		}
		else if (strcmp(DlgItemName, "btnRemove") == 0)
		{
			int i = (int)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETITEMINDEX, 0, 0);

			if (i != -1)
				gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTDELETE, (intptr_t)i, 0);

			count = (int)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETCOUNT, 0, 0);

			if (count > 0)
				gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTSETITEMINDEX, 0, 0);
			else
			{
				gDialogApi->SendDlgMsg(pDlg, "btnBrowser", DM_ENABLE, 0, 0);
				gDialogApi->SendDlgMsg(pDlg, "btnRemove", DM_ENABLE, 0, 0);
			}
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "cbURL") == 0)
		{
			line = (char*)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_GETTEXT, 0, 0);

			if (strncmp(line, "http", 4) != 0)
				gDialogApi->SendDlgMsg(pDlg, "btnBrowser", DM_ENABLE, 0, 0);
			else
				gDialogApi->SendDlgMsg(pDlg, "btnBrowser", DM_ENABLE, 1, 0);

			if (gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETITEMINDEX, 0, 0) == -1)
				gDialogApi->SendDlgMsg(pDlg, "btnRemove", DM_ENABLE, 0, 0);
			else
				gDialogApi->SendDlgMsg(pDlg, "btnRemove", DM_ENABLE, 1, 0);
		}

		break;
	}

	return 0;
}

static void ShowCfgURLDlg(void)
{
	if (gDialogApi)
	{
		if (g_file_test(gLFMPath, G_FILE_TEST_EXISTS))
			gDialogApi->DialogBoxLFMFile(gLFMPath, DlgProc);
		else
			gDialogApi->InputBox("Double Commander", "URL:", FALSE, gSettings.url, PATH_MAX);
	}

	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_URL, NULL, NULL, gSettings.url, PATH_MAX);
}

static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	long filetime = -1;
	double filesize = 0.0;
	CURL *curl =  dirdata->curl;
	xmlNodeSetPtr nodeset = dirdata->nodeset;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (nodeset->nodeNr <= dirdata->index)
		return FALSE;

	const xmlNode *node = nodeset->nodeTab[dirdata->index++];
	xmlChar *name = xmlGetProp(node, (xmlChar*)"name");
	g_strlcpy(FindData->cFileName, (char*)name, sizeof(FindData->cFileName));
	FindData->ftCreationTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftCreationTime.dwLowDateTime = 0xFFFFFFFE;
	FindData->ftLastAccessTime.dwHighDateTime = 0xFFFFFFFF;
	FindData->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFE;

	xmlChar *url = xmlNodeGetContent(node);

	if (url)
	{
		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_URL, url);

			if (curl_easy_perform(curl) == CURLE_OK)
			{
				curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime);
				curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &filesize);
			}
		}
		else
		{
			xmlChar *size = xmlGetProp(node, (xmlChar*)"size");

			if (size && size[0] != '\0')
				filesize = g_ascii_strtod((char*)size, NULL);

			xmlFree(size);

			xmlChar *date = xmlGetProp(node, (xmlChar*)"date");

			if (date && date[0] != '\0')
				filetime = (long)g_ascii_strtod((char*)date, NULL);

			xmlFree(date);
		}

		if (is_reparse_point((char*)url))
			FindData->dwFileAttributes = FILE_ATTRIBUTE_REPARSE_POINT;

		xmlFree(url);
	}

	if (filetime < 0)
	{
		FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
		FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
		//SetCurrentFileTime(&FindData->ftLastWriteTime);
	}
	else
		UnixTimeToFileTime((time_t)filetime, &FindData->ftLastWriteTime);

	if (filesize > 0)
	{
		FindData->nFileSizeHigh = ((int64_t)filesize & 0xFFFFFFFF00000000) >> 32;
		FindData->nFileSizeLow = (int64_t)filesize & 0x00000000FFFFFFFF;
	}
	else
	{
		FindData->nFileSizeHigh = 0xFFFFFFFF;
		FindData->nFileSizeLow = 0xFFFFFFFE;
	}


	xmlFree(name);

	return TRUE;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	FILE *fp;
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;

	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	gDoc = xmlNewDoc(BAD_CAST "1.0");
	gCurl = curl_easy_init();

	if ((fp = fopen(gHistoryFile, "r")) != NULL)
	{
		if ((read = getline(&line, &len, fp)) != -1)
		{
			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			if (strlen(line) > 0)
				g_strlcpy(gSettings.url, line, PATH_MAX);
			else
				g_strlcpy(gSettings.url, DEFAULT_URL, PATH_MAX);
		}

		fclose(fp);
	}
	else
		g_strlcpy(gSettings.url, DEFAULT_URL, PATH_MAX);

	gSettings.ask_sizes = FALSE;
	gSettings.follow = TRUE;
	gSettings.verbose = FALSE;
	gSettings.max_redir = 3;
	gSettings.noquery = TRUE;
	gSettings.fail = TRUE;

	gSettings.timeout = 30;

	if (gSettings.init)
		ShowCfgURLDlg();

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;

	if (gSettings.url[0] == '\0')
		return (HANDLE)(-1);

	xmlNodePtr root = xmlDocGetRootElement(gDoc);

	if (root)
	{
		xmlUnlinkNode(root);
		xmlFreeNode(root);
	}

	gClock = clock();

	root = get_links(gSettings.url);

	if (!root)
		return (HANDLE)(-1);

	xmlDocSetRootElement(gDoc, root);

	xmlChar *xpath = (xmlChar*)"/links/link";
	xmlXPathContextPtr context = xmlXPathNewContext(gDoc);
	xmlXPathObjectPtr obj = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);

	if (!obj)
		return (HANDLE)(-1);

	xmlNodeSetPtr nodeset = obj->nodesetval;

	if (xmlXPathNodeSetIsEmpty(nodeset))
	{
		xmlXPathFreeObject(obj);
		return (HANDLE)(-1);
	}

	dirdata = g_new0(tVFSDirData, 1);

	if (dirdata == NULL)
		return (HANDLE)(-1);

	dirdata->obj = obj;
	dirdata->nodeset = nodeset;
	dirdata->index = 0;

	if (gSettings.ask_sizes)
	{
		dirdata->curl = curl_easy_init();
		curl_easy_setopt(dirdata->curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(dirdata->curl, CURLOPT_FILETIME, 1L);
		curl_easy_setopt(dirdata->curl, CURLOPT_HEADERFUNCTION, do_nothing_cb);
		curl_easy_setopt(dirdata->curl, CURLOPT_HEADER, 0L);
		curl_easy_setopt(dirdata->curl, CURLOPT_TIMEOUT, gSettings.timeout);
		curl_set_additional_options(dirdata->curl);
	}

	if (!SetFindData(dirdata, FindData))
	{
		xmlXPathFreeObject(dirdata->obj);
		g_free(dirdata);
		return (HANDLE)(-1);
	}

	return dirdata;
}

BOOL DCPCALL FsFindNext(HANDLE Hdl, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	return SetFindData(dirdata, FindData);
}

int DCPCALL FsFindClose(HANDLE Hdl)
{
	tVFSDirData *dirdata = (tVFSDirData*)Hdl;

	if (dirdata->obj)
		xmlXPathFreeObject(dirdata->obj);

	if (dirdata->curl)
		curl_easy_cleanup(dirdata->curl);

	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	xmlChar *url = name_to_url(RemoteName + 1, gDoc);

	if (!url)
		return FS_FILE_READERROR;

	CURLcode res;
	tOutFile output = { LocalName, url, NULL, (size_t)((int64_t)ri->SizeHigh << 32 | ri->SizeLow), 0 };

	curl_easy_setopt(gCurl, CURLOPT_URL, url);
	curl_set_additional_options(gCurl);
	curl_easy_setopt(gCurl, CURLOPT_WRITEFUNCTION, write_to_file_cb);
	curl_easy_setopt(gCurl, CURLOPT_WRITEDATA, &output);
	curl_easy_setopt(gCurl, CURLOPT_TIMEOUT, 0);

	res = curl_easy_perform(gCurl);

	if (output.fp)
		fclose(output.fp);

	if (CURLE_OK != res)
		errmsg(curl_easy_strerror(res) ? (char*)curl_easy_strerror(res) : "unknown error");

	xmlFree(url);

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	if (!g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_WRITEERROR;

	return FS_FILE_OK;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;

	if (strcmp(Verb, "open") == 0)
	{
		xmlChar *url = name_to_url(RemoteName + 1, gDoc);

		if (url)
		{
			if (is_reparse_point((char*)url))
			{
				g_strlcpy(gSettings.url, (char*)url, PATH_MAX);
				result = FS_EXEC_OK;
			}
			else
				result = FS_EXEC_YOURSELF;

			xmlFree(url);
		}
	}
	else if (strcmp(Verb, "properties") == 0)
	{
		if (RemoteName[1] == '\0' && gSettings.init)
			return result;
		else
		{
			g_strlcpy(gSelectedFile, RemoteName + 1, PATH_MAX);
			ShowCfgURLDlg();
		}

		result = FS_EXEC_OK;
	}
	else if (strncmp(Verb, "quote", 5) == 0)
	{
		if (strncmp(Verb + 6, "sizes", 9) == 0)
			gSettings.ask_sizes = !gSettings.ask_sizes;

		result = FS_EXEC_OK;
	}

	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Hypertext REFerence", maxlen - 1);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	const char* lfm_name = "dialog.lfm";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(lfm_name, &dlinfo) != 0)
	{
		g_strlcpy(gLFMPath, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(gLFMPath, '/');

		if (pos)
			strcpy(pos + 1, lfm_name);
	}

	g_strlcpy(gHistoryFile, dps->DefaultIniName, PATH_MAX);
	char *pos = strrchr(gHistoryFile, '/');

	if (pos)
		strcpy(pos + 1, "history_href.txt");

}

int DCPCALL FsContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	switch (FieldIndex)
	{
	case 0:
		g_strlcpy(FieldName, "URL", maxlen - 1);
		break;

	case 1:
		g_strlcpy(FieldName, "Extra", maxlen - 1);
		break;

	default:
		return ft_nomorefields;
	}

	Units[0] = '\0';

	return ft_string;
}

int DCPCALL FsContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (FieldIndex < 0 || FieldIndex > 1)
		return ft_nosuchfield;

	int result = ft_fieldempty;

	gchar *xpath = g_strdup_printf("/links/link[@name=\"%s\"]", FileName[0] == '/' ? FileName + 1 : FileName);
	xmlXPathContextPtr context = xmlXPathNewContext(gDoc);
	xmlXPathObjectPtr obj = xmlXPathEvalExpression((xmlChar*)xpath, context);
	xmlXPathFreeContext(context);
	g_free(xpath);

	if (obj)
	{
		xmlNodeSetPtr nodeset = obj->nodesetval;

		if (!xmlXPathNodeSetIsEmpty(nodeset))
		{
			xmlChar *value = NULL;

			if (FieldIndex == 0)
				value = xmlNodeGetContent(nodeset->nodeTab[0]);
			else
				value = xmlGetProp(nodeset->nodeTab[0], (xmlChar*)"extra");

			if (value && value[0] != '\0')
			{
				g_strlcpy((char*)FieldValue, (char*)value, maxlen - 1);
				result = ft_string;
			}

			xmlFree(value);
		}

		xmlXPathFreeObject(obj);
	}

	return result;
}

BOOL DCPCALL FsContentGetDefaultView(char* ViewContents, char* ViewHeaders, char* ViewWidths, char* ViewOptions, int maxlen)
{
	return FALSE;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gSettings.init = TRUE;
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	xmlCleanupParser();

	if (gDoc)
		xmlFreeDoc(gDoc);

	if (gCurl)
	{
		curl_easy_cleanup(gCurl);
		curl_global_cleanup();
	}

	if (gDialogApi != NULL)
		free(gDialogApi);

	gDialogApi = NULL;
}
