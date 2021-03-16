#define _GNU_SOURCE
#include <glib.h>
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

typedef struct sVFSDirData
{
	xmlXPathObjectPtr obj;
	xmlNodeSetPtr nodeset;
	gsize index;
} tVFSDirData;

typedef struct sOutFile
{
	char *filename;
	xmlChar *url;
	FILE *fp;
} tOutFile;

typedef struct sMemBuf
{
	char *buf;
	size_t size;
} tMemBuf;

int gPluginNr;
tProgressProc gProgressProc = NULL;
tLogProc gLogProc = NULL;
tRequestProc gRequestProc = NULL;
tExtensionStartupInfo* gDialogApi = NULL;
xmlDocPtr g_doc = NULL;
CURL *g_curl;

static char g_url[PATH_MAX] = "https://vc.kiev.ua/download/";
static char g_lfm_path[PATH_MAX];
static char g_history_file[PATH_MAX];


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
	if (gDialogApi)
		gDialogApi->MessageBox(msg, "Double Commander", MB_OK | MB_ICONERROR);
	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_MsgOK, NULL, msg, NULL, 0);
	else
		g_print("%s\n", msg);
}

static size_t write_to_file_cb(void *buffer, size_t size, size_t nmemb, void *stream)
{
	tOutFile *out = (tOutFile*)stream;

	if (gProgressProc(gPluginNr, out->url, out->filename, 0))
		return 0;

	if (!out->fp)
	{
		out->fp = fopen(out->filename, "wb");

		if (!out->fp)
		{
			errmsg(strerror(EBADFD));
			return 0;
		}
	}

	return fwrite(buffer, size, nmemb, out->fp);
}

static size_t write_to_buffer_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	tMemBuf *mem = (tMemBuf*)userp;

	char *ptr = realloc(mem->buf, mem->size + realsize + 1);

	if (ptr == NULL)
	{
		errmsg(strerror(ENOMEM));
		return 0;
	}

	mem->buf = ptr;
	memcpy(&(mem->buf[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->buf[mem->size] = 0;

	return realsize;
}

static xmlChar *name_to_url(char *name)
{
	xmlChar *url = NULL;

	gchar *xpath = g_strdup_printf("/links/link[@name=\"%s\"]", name);
	xmlXPathContextPtr context = xmlXPathNewContext(g_doc);
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

static xmlNodePtr get_links(gchar *url)
{
	htmlDocPtr doc;
	CURLcode res;
	char name[MAX_PATH];
	xmlNodePtr result = NULL;
	tMemBuf membuf;

	if (!g_curl)
		return NULL;

	membuf.buf = malloc(1);
	membuf.size = 0;

	curl_easy_setopt(g_curl, CURLOPT_URL, url);
	curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_to_buffer_cb);
	curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, (void *)&membuf);

	res = curl_easy_perform(g_curl);

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

	xmlChar *xpath = (xmlChar*)"//a/@href";
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
		result = xmlNewNode(NULL, BAD_CAST "links");

		for (int i = 0; i < nodeset->nodeNr; i++)
		{
			xmlNodePtr link = xmlNewNode(NULL, BAD_CAST "link");

			const xmlNode *node = nodeset->nodeTab[i]->parent;
			xmlChar *href = xmlGetProp(node, "href");

			if (strchr((char*)href, '?') != NULL)
			{
				continue;
				xmlFree(href);
			}

			if (href[strlen((char*)href) - 1] != '/' && strncmp("http", (char*)href, 4) != 0 && strchr((char*)href, '.'))
			{
				gchar **split = g_strsplit((char*)href, "/", -1);
				gchar *result = g_strjoinv("_", split);
				g_strlcpy(name, (char*)result, MAX_PATH);
				g_free(result);
				g_strfreev(split);
			}
			else
			{
				xmlChar *text = xmlNodeGetContent(node);
				g_strstrip((char*)text);
				g_strlcpy(name, (char*)text, MAX_PATH);


				if (name[strlen(name) - 1] == '/')
					name[strlen(name) - 1] = '\0';

				xmlFree(text);
			}

			if (strlen(name) < 1)
			{
				xmlFree(href);
				continue;
			}

			if (strcmp("..", (char*)name) == 0)
				xmlNewProp(link, "name", "<Parent Directory>");
			else
				xmlNewProp(link, "name", name);

			if (node->next && node->next->type == XML_TEXT_NODE)
			{
				//g_printf("%s", (char *)node->next->content);
			}

			xmlChar *orig = href;
			href = xmlBuildURI(href, (xmlChar *) url);
			xmlFree(orig);
			xmlNodeSetContent(link, href);
			xmlFree(href);
			xmlAddChild(result, link);
		}
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
		if ((fp = fopen(g_history_file, "r")) != NULL)
		{
			while ((read = getline(&line, &len, fp)) != -1 && count < MAX_PATH)
			{
				if (line[read - 1] == '\n')
					line[read - 1] = '\0';

				gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTADD, (intptr_t)line, 0);
				count++;
			}

			fclose(fp);
		}

		if (count == 0)
			gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTADD, (intptr_t)g_url, 0);


		gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_SETTEXT, (intptr_t)g_url, 0);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			g_strlcpy(g_url, (char*)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_GETTEXT, 0, 0), PATH_MAX);

			if ((fp = fopen(g_history_file, "w")) != NULL)
			{
				fprintf(fp, "%s\n", g_url);
				count = (int)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETCOUNT, 0, 0);

				for (int i = 0; i < count; i++)
				{
					line = g_strdup((char*)gDialogApi->SendDlgMsg(pDlg, "cbURL", DM_LISTGETITEM, i, 0));

					if (line && (strcmp(g_url, line) != 0))
						fprintf(fp, "%s\n", line);

					g_free(line);
				}

				fclose(fp);
			}

			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_OK, 0);
		}
		else if (strcmp(DlgItemName, "btnCancel") == 0)
			gDialogApi->SendDlgMsg(pDlg, DlgItemName, DM_CLOSE, ID_CANCEL, 0);
	}

	return 0;
}

static void ShowCfgURLDlg(gboolean init)
{
	if (gDialogApi)
	{
		if (g_file_test(g_lfm_path, G_FILE_TEST_EXISTS))
			gDialogApi->DialogBoxLFMFile(g_lfm_path, DlgProc);
		else
			gDialogApi->InputBox("Double Commander", "URL:", FALSE, g_url, PATH_MAX);
	}

	else if (gRequestProc)
		gRequestProc(gPluginNr, RT_URL, NULL, NULL, g_url, PATH_MAX);
}

static gboolean SetFindData(tVFSDirData *dirdata, WIN32_FIND_DATAA *FindData)
{
	xmlNodeSetPtr nodeset = dirdata->nodeset;
	memset(FindData, 0, sizeof(WIN32_FIND_DATAA));

	if (nodeset->nodeNr <= dirdata->index)
		return FALSE;

	const xmlNode *node = nodeset->nodeTab[dirdata->index++];
	xmlChar *name = xmlGetProp(node, "name");
	g_strlcpy(FindData->cFileName, (char*)name, sizeof(FindData->cFileName));

	xmlChar *url = xmlNodeGetContent(node);

	if (url)
	{
		if (url[strlen(url) - 1] == '/')
			FindData->dwFileAttributes = FILE_ATTRIBUTE_REPARSE_POINT;

		xmlFree(url);
	}

	SetCurrentFileTime(&FindData->ftLastWriteTime);
	FindData->nFileSizeLow = 0;
	return TRUE;
}

int DCPCALL FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
	gPluginNr = PluginNr;
	gProgressProc = pProgressProc;
	gLogProc = pLogProc;
	gRequestProc = pRequestProc;
	g_doc = xmlNewDoc(BAD_CAST "1.0");
	g_curl = curl_easy_init();
	ShowCfgURLDlg(TRUE);

	return 0;
}

HANDLE DCPCALL FsFindFirst(char* Path, WIN32_FIND_DATAA *FindData)
{
	tVFSDirData *dirdata;
	xmlNodePtr root = xmlDocGetRootElement(g_doc);

	if (root)
	{
		xmlUnlinkNode(root);
		xmlFreeNode(root);
	}

	root = get_links(g_url);

	if (!root)
		return (HANDLE)(-1);

	xmlDocSetRootElement(g_doc, root);

	xmlChar *xpath = (xmlChar*)"/links/link";
	xmlXPathContextPtr context = xmlXPathNewContext(g_doc);
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

	g_free(dirdata);

	return 0;
}

int DCPCALL FsGetFile(char* RemoteName, char* LocalName, int CopyFlags, RemoteInfoStruct* ri)
{
	if (gProgressProc(gPluginNr, RemoteName, LocalName, 0))
		return FS_FILE_USERABORT;

	if (CopyFlags == 0 && g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_EXISTS;

	xmlChar *url = name_to_url(RemoteName + 1);

	if (!url)
		return FS_FILE_READERROR;

	CURLcode res;
	tOutFile output = { LocalName, url, NULL };

	curl_easy_setopt(g_curl, CURLOPT_URL, url);
	curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_to_file_cb);
	curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &output);

	res = curl_easy_perform(g_curl);

	if (output.fp)
		fclose(output.fp);

	xmlFree(url);

	if (CURLE_OK != res)
		return FS_FILE_WRITEERROR;

	if (!g_file_test(LocalName, G_FILE_TEST_EXISTS))
		return FS_FILE_WRITEERROR;

	gProgressProc(gPluginNr, RemoteName, LocalName, 100);

	return FS_FILE_OK;
}

int DCPCALL FsExecuteFile(HWND MainWin, char* RemoteName, char* Verb)
{
	int result = FS_EXEC_ERROR;

	if (strcmp(Verb, "open") == 0)
	{
		xmlChar *url = name_to_url(RemoteName + 1);

		if (url)
		{
			if (url[strlen(url) - 1] == '/')
			{
				g_strlcpy(g_url, (char*)url, PATH_MAX);
				result = FS_EXEC_OK;
			}

			xmlFree(url);
		}
		else
			result = FS_EXEC_YOURSELF;
	}
	else if (strcmp(Verb, "properties") == 0)
		ShowCfgURLDlg(FALSE);

	return result;
}

void DCPCALL FsGetDefRootName(char* DefRootName, int maxlen)
{
	g_strlcpy(DefRootName, "Hypertext REFerence", maxlen - 1);
}

void DCPCALL FsSetDefaultParams(FsDefaultParamStruct* dps)
{
	FILE *fp;
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;
	Dl_info dlinfo;
	const char* lfm_name = "dialog.lfm";

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(lfm_name, &dlinfo) != 0)
	{
		g_strlcpy(g_lfm_path, dlinfo.dli_fname, PATH_MAX);
		char *pos = strrchr(g_lfm_path, '/');

		if (pos)
			strcpy(pos + 1, lfm_name);
	}

	g_strlcpy(g_history_file, dps->DefaultIniName, PATH_MAX);
	char *pos = strrchr(g_history_file, '/');

	if (pos)
		strcpy(pos + 1, "history_href.txt");

	if ((fp = fopen(g_history_file, "r")) != NULL)
	{
		if ((read = getline(&line, &len, fp)) != -1)
		{
			if (line[read - 1] == '\n')
				line[read - 1] = '\0';

			g_strlcpy(g_url, line, PATH_MAX);
		}

		fclose(fp);
	}
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gDialogApi == NULL)
	{
		gDialogApi = malloc(sizeof(tExtensionStartupInfo));
		memcpy(gDialogApi, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	xmlCleanupParser();

	if (g_doc)
		xmlFreeDoc(g_doc);

	if (g_curl)
	{
		curl_easy_cleanup(g_curl);
		curl_global_cleanup();
	}

	if (gDialogApi != NULL)
		free(gDialogApi);

	gDialogApi = NULL;
}
