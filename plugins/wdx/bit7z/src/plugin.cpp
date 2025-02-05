#include <bit7z/bit7z.hpp>
#include <string.h>
#include "wdxplugin.h"

using namespace std;
using namespace bit7z;

#define fieldcount (sizeof(gFields)/sizeof(tField))

typedef struct sField
{
	const char *name;
	int type;
	const char *units;
} tField;

tField gFields[] =
{
	{"Item Count",		ft_numeric_32,	""},
	{"File Count",		ft_numeric_32,	""},
	{"Folder Count",	ft_numeric_32,	""},
	{"Size",		ft_numeric_64,	""},
	{"Packed Size",		ft_numeric_64,	""},
	{"Solid",		ft_boolean,	""},
	{"Number Volumes",	ft_numeric_32,	""},
	{"Comment",		ft_string,	""},
	{"Method",		ft_string,	""},
	{"Characts",		ft_string,	""},
	{"Error Flags",		ft_numeric_32,	""},
};

// Bit7zLibrary gBit7zLib { "/usr/lib/p7zip/7z.so" };
Bit7zLibrary gBit7zLib { "/usr/lib/7zip/7z.so" };

string gLastFile;
bool gLastFileRead = false;
map< BitProperty, BitPropVariant > gProps;
int gItems = 0, gFiles = 0, gFolders = 0;
int64_t gSize = 0, gPackedSize = 0;


char* strlcpy(char* p, const char* p2, int maxlen)
{
	if ((int)strlen(p2) >= maxlen)
	{
		strncpy(p, p2, maxlen);
		p[maxlen] = 0;
	}
	else
		strcpy(p, p2);

	return p;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= (int)fieldcount)
		return ft_nomorefields;

	strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);
	strlcpy(Units, gFields[FieldIndex].units, maxlen - 1);
	return gFields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	string file(FileName);

	if (file != gLastFile || !gLastFileRead)
	{
		gLastFile = file;

		try
		{
			BitArchiveReader reader{ gBit7zLib, file, BitFormat::Auto };
			gProps = reader.archiveProperties();
			gItems = reader.itemsCount();
			gFiles = reader.filesCount();
			gFolders = reader.foldersCount();
			gSize = (int64_t)reader.size();
			gPackedSize = (int64_t)reader.packSize();
		}
		catch (const bit7z::BitException& ex)
		{
			printf("%s (%s): %s\n", PLUGNAME, FileName, ex.what());
			gLastFileRead = false;
			return ft_fileerror;
		}

		gLastFileRead = true
		                ;
	}

	if (!gLastFileRead)
		return ft_fileerror;

	switch (FieldIndex)
	{
	case 0:
		*(int*)FieldValue = gItems;
		break;

	case 1:
		*(int*)FieldValue = gFiles;
		break;

	case 2:
		*(int*)FieldValue = gFolders;
		break;

	case 3:
		*(int64_t*)FieldValue = gSize;
		break;

	case 4:
		*(int64_t*)FieldValue = gPackedSize;
		break;

	case 5:
		try
		{
			*(int*)FieldValue = (int)gProps[BitProperty::Solid].getBool();
		}
		catch (const bit7z::BitException& ex)
		{
			return ft_fieldempty;
		}

		break;

	case 6:
		try
		{
			*(int*)FieldValue = (int)gProps[BitProperty::NumVolumes].getUInt32();
		}
		catch (const bit7z::BitException& ex)
		{
			return ft_fieldempty;
		}

		break;

	case 7:
		strlcpy((char*)FieldValue, gProps[BitProperty::Comment].toString().c_str(), maxlen - 1);
		break;

	case 8:
		strlcpy((char*)FieldValue, gProps[BitProperty::Method].toString().c_str(), maxlen - 1);
		break;

	case 9:
		strlcpy((char*)FieldValue, gProps[BitProperty::Characts].toString().c_str(), maxlen - 1);
		break;
	case 10:
		try
		{
			*(int*)FieldValue = (int)gProps[BitProperty::ErrorFlags].getUInt32();
		}
		catch (const bit7z::BitException& ex)
		{
			return ft_fieldempty;
		}
		break;

	default:
		return ft_nosuchfield;
	}

	return gFields[FieldIndex].type;
}
