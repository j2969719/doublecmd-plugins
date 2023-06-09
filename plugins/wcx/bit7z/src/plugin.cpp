#include <fstream>
#include <bit7z/bit7z.hpp>
#include <bit7z/bitarchivewriter.hpp>
#include <bit7z/bitarchiveeditor.hpp>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

using namespace std;
using namespace bit7z;

#define PACKERCAPS PK_CAPS_NEW | PK_CAPS_MULTIPLE | PK_CAPS_DELETE | PK_CAPS_ENCRYPT | PK_CAPS_SEARCHTEXT | PK_CAPS_OPTIONS
#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox


typedef struct sArcData
{
	BitArchiveReader *reader;
	uint32_t index;
	uint32_t count;
	vector<bit7z::BitArchiveItemInfo> arc_items;
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

int gCryptoNr;
int gCryptoFlags;
tPkCryptProc gPkCryptProc  = nullptr;
tChangeVolProc gChangeVolProc  = nullptr;
tProcessDataProc gProcessDataProc = nullptr;
tExtensionStartupInfo* gExtensions = nullptr;
Bit7zLibrary gBit7zLib;

char gPass[PATH_MAX];
char gPassMsg[] = "Enter password:";

bool gSolid = false;
int gComppLevel = (int)BitCompressionLevel::Normal;
int gComppMethod = (int)BitCompressionMethod::Lzma2;

static char *ask_password(void)
{
	if (!InputBox(nullptr, gPassMsg, true, gPass, PATH_MAX))
		gPass[0] = '\0';

	return gPass;

}

static bool show_progress(uint64_t size)
{
	return (gProcessDataProc(nullptr, 0) != 0);
}

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	int index = 0;

	switch (Msg)
	{
	case DN_INITDIALOG:
		SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)"None", (intptr_t)BitCompressionLevel::None);

		if (gComppLevel == (int)BitCompressionLevel::None)
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)"Fastest", (intptr_t)BitCompressionLevel::Fastest);

		if (gComppLevel == (int)BitCompressionLevel::Fastest)
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)"Fast", (intptr_t)BitCompressionLevel::Fast);

		if (gComppLevel == (int)BitCompressionLevel::Fast)
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)"Normal", (intptr_t)BitCompressionLevel::Normal);

		if (gComppLevel == (int)BitCompressionLevel::Normal)
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)"Max", (intptr_t)BitCompressionLevel::Max);

		if (gComppLevel == (int)BitCompressionLevel::Max)
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)"Ultra", (intptr_t)BitCompressionLevel::Ultra);

		if (gComppLevel == (int)BitCompressionLevel::Ultra)
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);

		index = 0;
		SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTADD, (intptr_t)"Copy", (intptr_t)BitCompressionMethod::Copy);

		if (gComppMethod == (int)BitCompressionMethod::Copy)
			SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTADD, (intptr_t)"BZip2", (intptr_t)BitCompressionMethod::BZip2);

		if (gComppMethod == (int)BitCompressionMethod::BZip2)
			SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTADD, (intptr_t)"Lzma", (intptr_t)BitCompressionMethod::Lzma);

		if (gComppMethod == (int)BitCompressionMethod::Lzma)
			SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTADD, (intptr_t)"Lzma2", (intptr_t)BitCompressionMethod::Lzma2);

		if (gComppMethod == (int)BitCompressionMethod::Lzma2)
			SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTSETITEMINDEX, index, 0);

		index++;
		SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTADD, (intptr_t)"Ppmd", (intptr_t)BitCompressionMethod::Ppmd);

		if (gComppMethod == (int)BitCompressionMethod::Ppmd)
			SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTSETITEMINDEX, index, 0);

		SendDlgMsg(pDlg, (char*)"ckSolid", DM_SETCHECK, (intptr_t)gSolid, 0);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			index = (int)SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gComppLevel = (int)SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTGETDATA, index, 0);

			index = (int)SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gComppMethod = (int)SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTGETDATA, index, 0);

			gSolid = (bool)SendDlgMsg(pDlg, (char*)"ckSolid", DM_GETCHECK, 0, 0);
		}

		break;
	}

	return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData *data = (tArcData *)malloc(sizeof(tArcData));

	if (data == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(data, 0, sizeof(tArcData));

	try
	{
		data->reader = new BitArchiveReader{ gBit7zLib, ArchiveData->ArcName, BitFormat::SevenZip };
		data->reader->setPasswordCallback(ask_password);
		data->count = data->reader->itemsCount();
		data->arc_items = data->reader->items();
	}
	catch (const bit7z::BitException& ex)
	{
		printf("%s: %s\n", ArchiveData->ArcName, ex.what());
		ArchiveData->OpenResult = E_EOPEN;
		return E_SUCCESS;
	}

	return (HANDLE)data;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ArcData data = (ArcData)hArcData;

	memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));

	if (data->index < data->count)
	{
		auto item = data->arc_items[data->index];
		strncpy(HeaderDataEx->FileName, item.path().c_str(), sizeof(HeaderDataEx->FileName) - 1);

		if (item.isDir())
			HeaderDataEx->FileAttr = S_IFDIR;

		HeaderDataEx->PackSizeHigh = (item.packSize() & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->PackSize = item.packSize() & 0x00000000FFFFFFFF;
		HeaderDataEx->UnpSizeHigh = (item.size() & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->UnpSize = item.size() & 0x00000000FFFFFFFF;
		HeaderDataEx->FileTime = (int)chrono::system_clock::to_time_t(item.lastWriteTime());
		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData data = (ArcData)hArcData;

	if (data->ProcessDataProc(DestName, -1000) == 0)
		return E_EABORTED;

	if (Operation == PK_EXTRACT)
	{
		ofstream outfile(DestName);

		try
		{
			data->reader->extract(outfile, data->index);
		}
		catch (const bit7z::BitException& ex)
		{
			MessageBox((char*)ex.what(), nullptr,  MB_OK | MB_ICONERROR);
		}

		outfile.close();
	}

	data->index++;

	return E_SUCCESS;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	delete data->reader;
	free(data);

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	struct stat buf;
	string src(SrcPath);

	try
	{
		BitArchiveWriter writer{ gBit7zLib, PackedFile, BitFormat::SevenZip };
		writer.setUpdateMode(UpdateMode::Update);
		writer.setCompressionLevel((BitCompressionLevel)gComppLevel);
		writer.setCompressionMethod((BitCompressionMethod)gComppMethod);
		writer.setSolidMode(gSolid);
		writer.setUpdateMode(UpdateMode::Update);
		writer.setPasswordCallback(ask_password);
		writer.setProgressCallback(show_progress);

		if (Flags & PK_PACK_ENCRYPT)
		{
			char pass[PATH_MAX] = "";

			if (InputBox(nullptr, gPassMsg, true, pass, PATH_MAX))
				writer.setPassword(pass);
		}

		while (*AddList)
		{
			if (gProcessDataProc(AddList, 0) == 0)
				return E_EABORTED;

			string target(AddList);
			string filename = src + '/' + target;

			if (SubPath)
				target.insert(0, 1, '/').insert(0,  string(SubPath));

			if (stat(filename.c_str(), &buf) == 0 && !S_ISDIR(buf.st_mode))
				writer.addFile(filename, target);

			while (*AddList++);
		}


		writer.compressTo(PackedFile);
	}
	catch (const bit7z::BitException& ex)
	{
		MessageBox((char*)ex.what(), nullptr,  MB_OK | MB_ICONERROR);
	}

	return E_SUCCESS;
}

int DCPCALL DeleteFiles(char *PackedFile, char *DeleteList)
{
	BitArchiveEditor writer{ gBit7zLib, PackedFile, BitFormat::SevenZip };
	writer.setOverwriteMode(OverwriteMode::Overwrite);
	writer.setPasswordCallback(ask_password);
	writer.setProgressCallback(show_progress);

	while (*DeleteList)
	{
		if (gProcessDataProc(DeleteList, 0) == 0)
			return E_EABORTED;

		auto target = string(DeleteList);

		if (target.substr(target.length() - 4) == "/*.*")
			target.erase(target.length() - 4);

		try
		{
			writer.deleteItem(target);
		}
		catch (const bit7z::BitException& ex)
		{
			string msg = target + ": " + ex.what();
			MessageBox((char*)msg.c_str(), nullptr,  MB_OK | MB_ICONERROR);
		}

		while (*DeleteList++);
	}

	try
	{
		writer.applyChanges();
	}
	catch (const bit7z::BitException& ex)
	{
		MessageBox((char*)ex.what(), nullptr,  MB_OK | MB_ICONERROR);
	}

	return E_SUCCESS;
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

void DCPCALL PkSetCryptCallback(tPkCryptProc pPkCryptProc, int CryptoNr, int Flags)
{
	gCryptoNr = CryptoNr;
	gCryptoFlags = Flags;
	gPkCryptProc = pPkCryptProc;
}

int DCPCALL GetPackerCaps(void)
{
	return PACKERCAPS;
}

void DCPCALL ConfigurePacker(HWND Parent, HINSTANCE DllInstance)
{
	string lfmpath(gExtensions->PluginDir);
	lfmpath.append("dialog.lfm");
	gExtensions->DialogBoxLFMFile((char*)lfmpath.c_str(), OptionsDlgProc);
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		gExtensions = (tExtensionStartupInfo*)malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
	{
		free(gExtensions);
	}

	gExtensions = NULL;
}

#ifdef __cplusplus
}
#endif
