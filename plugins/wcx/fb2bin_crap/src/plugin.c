#include <glib.h>
#include <string.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include "wcxplugin.h"
#include "extension.h"

typedef struct sArcData
{
	char arcname[PATH_MAX];
	char filename[MAX_PATH];
	xmlDoc *document;
	xmlNode *node;
	gsize out_len;
	guchar *data;
	time_t filetime;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

#define BUFF_SIZE 8192

tProcessDataProc gProcessDataProc = NULL;

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	struct stat buf;
	tArcData * handle;
	handle = malloc(sizeof(tArcData));
	memset(handle, 0, sizeof(tArcData));
	snprintf(handle->arcname, PATH_MAX, "%s", ArchiveData->ArcName);
	handle->document = xmlReadFile(ArchiveData->ArcName, NULL, 0);

	if (handle->document == NULL)
	{
		free(handle);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	xmlNode *root = xmlDocGetRootElement(handle->document);

	if (strcmp((char*)root->name, "FictionBook") != 0)
	{
		xmlFreeDoc(handle->document);
		free(handle);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	if (stat(ArchiveData->ArcName, &buf) == 0)
		handle->filetime = buf.st_mtime;

	handle->node = root->children;
	return (HANDLE)handle;

}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	memset(HeaderData, 0, sizeof(tHeaderData));
	ArcData handle = (ArcData)hArcData;

	xmlNode *node = handle->node;
	handle->data = NULL;
	memset(handle->filename, 0, sizeof(handle->filename));

	if (node)
	{
		if (strcmp((char*)node->name, "binary") == 0)
		{
			xmlChar *bin_id, *bin_content;
			bin_id = xmlGetProp(node, (xmlChar*)"id");

			char *pdot = strchr((char*)bin_id, '.');

			if (pdot != NULL)
			{
				snprintf(handle->filename, MAX_PATH, "%s", (char*)bin_id);
			}
			else
			{
				bin_content = xmlGetProp(node, (xmlChar*)"content-type");
				char *p = strchr((char*)bin_content, '/');

				if (p != NULL)
					snprintf(handle->filename, PATH_MAX, "%s.%s", (char*)bin_id, p + 1);

				xmlFree(bin_content);
			}

			xmlFree(bin_id);

			if (handle->filename[0] != '\0')
			{
				snprintf(HeaderData->FileName, MAX_PATH, "%s", handle->filename);
				HeaderData->FileTime = handle->filetime;
				handle->data = g_base64_decode((char*)node->children->content, &handle->out_len);
				HeaderData->UnpSize = handle->out_len;
				HeaderData->PackSize = handle->out_len;
				HeaderData->FileAttr = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
			}
		}

		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData handle = (ArcData)hArcData;
	xmlNode *node = handle->node;

	if (Operation == PK_EXTRACT)
	{
		FILE *fp = fopen(DestName, "w");

		if (fp)
		{
			for (gsize i = 0; i < handle->out_len; i += BUFF_SIZE)
			{
				int len = (int)fwrite(handle->data + i, sizeof(guchar), BUFF_SIZE, fp);

				if (handle->ProcessDataProc)
					handle->ProcessDataProc(handle->filename, len);
			}

			fclose(fp);
		}
	}

	if (handle->data != NULL)
		g_free(handle->data);

	handle->node = node->next;

	return E_SUCCESS;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData handle = (ArcData)hArcData;
	xmlFreeDoc(handle->document);
	free(handle);
	return E_SUCCESS;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_HIDE;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData handle = (ArcData)hArcData;

	if ((int)(long)hArcData == -1 || !handle)
		gProcessDataProc = pProcessDataProc;
	else
		handle->ProcessDataProc = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{
	return;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	char *pdot = strchr(FileName, '.');

	if (pdot != NULL)
	{
		if (strcmp(pdot, ".fb2") == 0)
			return TRUE;
	}

	return FALSE;
}
