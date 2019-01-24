#define _GNU_SOURCE

#include <glib.h>
#include <gio/gio.h>
#include <poppler.h>
#include "wdxplugin.h"

#include <dlfcn.h>

#include <glib/gi18n.h>
#include <locale.h>
#define GETTEXT_PACKAGE "plugins"

#define _detectstring "EXT=\"PDF\""
#define _validcontent "application/pdf"

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{N_("Title"),			ft_string,	""},
	{N_("Author"),			ft_string,	""},
	{N_("Subject"),			ft_string,	""},
	{N_("Creator"),			ft_string,	""},
	{N_("Producer"),		ft_string,	""},
	{N_("Keywords"),		ft_string,	""},
	{N_("Version"),			ft_string,	""},
	{N_("Total Pages"),		ft_numeric_32,	""},
	{N_("Creation date"),		ft_datetime,	""},
	{N_("Modification date"),	ft_datetime,	""},
	{N_("Attachments"),		ft_boolean,	""},
	{N_("Linearized"),		ft_boolean,	""},
	{N_("Password protected"),	ft_boolean,	""},
	{N_("Damaged"),			ft_boolean,	""},
	{N_("Can be printer"),		ft_boolean,	""},
	{N_("Can be modified"),		ft_boolean,	""},
	{N_("Can be copied"),		ft_boolean,	""},
	{N_("OK to add notes"),		ft_boolean,	""},
	{N_("OK to fill form"),		ft_boolean,	""},
	{N_("Text"),			ft_fulltext,	""},
};

static gchar *doc_text;
static gsize pos;

char* GetDocumentText(PopplerDocument *document)
{
	gchar *text = "";
	gsize index, pages;
	PopplerPage *ppage;
	pages = poppler_document_get_n_pages(document);
	for (index = 0; index < pages; index++)
	{
		ppage = poppler_document_get_page(document, index);
		text = g_strconcat(text, "\n", poppler_page_get_text(ppage), NULL);
	}
	return g_strconcat(text, "\0", NULL);
}

gboolean UnixTimeToFileTime(uint64_t unix_time, LPFILETIME FileTime)
{
	if (unix_time == -1)
		return FALSE;

	unix_time = (unix_time * 10000000) + 0x019DB1DED53E8000;
	FileTime->dwLowDateTime = (DWORD)unix_time;
	FileTime->dwHighDateTime = unix_time >> 32;
	return TRUE;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, gettext(fields[FieldIndex].name), maxlen - 1);
	g_strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen-1);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	char *strvalue;
	uint64_t timevalue;
	gboolean vempty = FALSE;
	gboolean tmpbool = FALSE;
	GError *err = NULL;
	gchar* content_type = g_content_type_guess(FileName, NULL, 0, &tmpbool);
	gchar* fileUri = g_filename_to_uri(FileName, NULL, NULL);

	if (!g_content_type_equals(content_type, _validcontent))
		return ft_fileerror;

	PopplerDocument *document = poppler_document_new_from_file(fileUri, NULL, &err);

	if (err)
	{
		if ((FieldIndex == 12) && (err->code == POPPLER_ERROR_ENCRYPTED))
		{
			g_error_free(err);
			*(int*)FieldValue = 1;
			return fields[FieldIndex].type;
		}
		else if ((FieldIndex == 13) && (err->code == POPPLER_ERROR_DAMAGED))
		{
			g_error_free(err);
			*(int*)FieldValue = 1;
			return fields[FieldIndex].type;
		}

		g_error_free(err);
	}

	if (document == NULL)
		return ft_fileerror;

	switch (FieldIndex)
	{
	case 0:
		strvalue = poppler_document_get_title(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 1:
		strvalue = poppler_document_get_author(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 2:
		strvalue = poppler_document_get_subject(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 3:
		strvalue = poppler_document_get_creator(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 4:
		strvalue = poppler_document_get_producer(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 5:
		strvalue = poppler_document_get_keywords(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 6:
		strvalue = poppler_document_get_pdf_version_string(document);

		if (strvalue)
			g_strlcpy((char*)FieldValue, strvalue, maxlen - 1);
		else
			vempty = TRUE;

		break;

	case 7:
		*(int*)FieldValue = poppler_document_get_n_pages(document);
		break;

	case 8:
		timevalue = poppler_document_get_creation_date(document);

		if (!UnixTimeToFileTime(timevalue, (FILETIME*)FieldValue))
			vempty = TRUE;

	case 9:
		timevalue = poppler_document_get_modification_date(document);

		if (!UnixTimeToFileTime(timevalue, (FILETIME*)FieldValue))
			vempty = TRUE;

	case 10:
		if (poppler_document_has_attachments(document))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 11:
		if (poppler_document_is_linearized(document))
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 12:
		*(int*)FieldValue = 0;
		break;

	case 14:
		if (poppler_document_get_permissions(document) & POPPLER_PERMISSIONS_OK_TO_PRINT)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 15:
		if (poppler_document_get_permissions(document) & POPPLER_PERMISSIONS_OK_TO_MODIFY)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 16:
		if (poppler_document_get_permissions(document) & POPPLER_PERMISSIONS_OK_TO_COPY)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 17:
		if (poppler_document_get_permissions(document) & POPPLER_PERMISSIONS_OK_TO_ADD_NOTES)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;

	case 18:
		if (poppler_document_get_permissions(document) & POPPLER_PERMISSIONS_OK_TO_FILL_FORM)
			*(int*)FieldValue = 1;
		else
			*(int*)FieldValue = 0;

		break;
	case 19:
		if (UnitIndex == 0)
		{
			doc_text = GetDocumentText(document);
			g_strlcpy(FieldValue, doc_text, maxlen-1);
			pos = maxlen-2;
		}
		else if (UnitIndex == -1)
		{
			doc_text = NULL;
			pos = 0;
			return ft_fieldempty;
		}
		else
		{
			if (strlen(doc_text+pos) > 0)
			{
				g_strlcpy((char*)FieldValue, doc_text+pos, maxlen-1);
				pos += maxlen-2;
			}
			else
				return ft_fieldempty;
		}
		return ft_fulltext;

		break;

	default:
		vempty = TRUE;

		break;
	}

	if (vempty)
		return ft_fieldempty;
	else
		return fields[FieldIndex].type;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	Dl_info dlinfo;

	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(fields, &dlinfo) != 0)
	{
		setlocale (LC_ALL, "");
		bindtextdomain(GETTEXT_PACKAGE, g_strdup_printf("%s/langs", g_path_get_dirname(dlinfo.dli_fname)));
		textdomain(GETTEXT_PACKAGE);
	}
}
