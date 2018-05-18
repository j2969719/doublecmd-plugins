#include <glib.h>
#include <poppler.h>
#include "wdxplugin.h"

#define _detectstring "EXT=\"PDF\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"Title",	ft_string,	""},
	{"Author",	ft_string,	""},
	{"Subject",	ft_string,	""},
	{"Creator",	ft_string,	""},
	{"Producer",	ft_string,	""},
	{"Keywords",	ft_string,	""},
	{"Version",	ft_string,	""},
	{"Total Pages",	ft_numeric_32,	""},
};

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	g_strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	g_strlcpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	g_strlcpy(DetectString, _detectstring, maxlen);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	char *strvalue;
	gboolean vempty = FALSE;
	gchar* fileUri = g_filename_to_uri(FileName, NULL, NULL);
	PopplerDocument *document = poppler_document_new_from_file(fileUri, NULL, NULL);

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

	default:
		vempty = TRUE;

		break;
	}

	if (vempty)
		return ft_fieldempty;
	else
		return fields[FieldIndex].type;
}
