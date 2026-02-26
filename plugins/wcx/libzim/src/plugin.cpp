#include <cstdint>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <array>

#include <zim/archive.h>
#include <zim/item.h>
#include <zim/entry.h>

#include "wcxplugin.h"
#include "extension.h"

using namespace std;
namespace fs = std::filesystem;

#define MessageBox gExtensions->MessageBox
#define BUFF_SIZE 8192

typedef struct sArcData
{
	zim::Archive* archive;
	uint32_t index;
	uint32_t count;
	fs::path arc_path;
	string last_path;
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

const unsigned char FILE_MAGIC[] = {0x5A, 0x49, 0x4D, 0x04};

#ifdef __cplusplus
extern "C" {
#endif

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	tArcData *data = new tArcData{};
	try 
	{
		data->archive = new zim::Archive(ArchiveData->ArcName);
		data->count = data->archive->getEntryCount();
		data->index = 0;
		data->arc_path = fs::path(ArchiveData->ArcName);
		ArchiveData->OpenResult = 0;
	}
	catch (...) 
	{
		ArchiveData->OpenResult = E_EOPEN;
		return nullptr;
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

	while (data->index < data->count) 
	{
		try
		{
			auto entry = data->archive->getEntryByPath(data->index);

			if (entry.isRedirect())
			{
				data->index++;
				continue;
			}

			data->last_path = entry.getPath();
			strncpy(HeaderDataEx->FileName, data->last_path.c_str(), sizeof(HeaderDataEx->FileName) - 1);
			HeaderDataEx->UnpSizeHigh = (entry.getItem().getSize() & 0xFFFFFFFF00000000) >> 32;
			HeaderDataEx->UnpSize = entry.getItem().getSize() & 0x00000000FFFFFFFF;
			return E_SUCCESS;
		} 
		catch (const std::exception& ex)
		{
			cerr << PLUGNAME << " ("<<data->arc_path.filename() << ") skip index #" << data->index << " : " << ex.what() << endl;
			data->index++;
		}
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	int result = E_SUCCESS;
	ArcData data = (ArcData)hArcData;

	if (Operation == PK_EXTRACT) 
	{
		auto entry = data->archive->getEntryByPath(data->last_path);
		auto blob = entry.getItem().getData();

		string path;

		if (DestPath && strlen(DestPath) > 0) 
			path = string(DestPath) + "/" + string(DestName);
		else
			path = string(DestName);

		try
		{
			ofstream out(path, ios::binary);

			if (!out.is_open())
				result = E_EWRITE;

			size_t total = blob.size();
			size_t written = 0;

			while (written < total) 
			{
				size_t len = std::min((size_t)BUFF_SIZE, total - written);
				out.write(&blob.data()[written], len);
				written += len;

				if (data->ProcessDataProc(DestName, written) == 0)
				{
					result =  E_EABORTED;
					break;
				}
			}

			out.close();

		} 
		catch (const std::exception& ex)
		{
			cerr << PLUGNAME << " ("<<data->arc_path.filename() << ") " << data->last_path << " : " << ex.what() << endl;
			result = E_EWRITE;
		}
	}

	data->index++;

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	if (data) 
	{
		delete data->archive;
		delete data;
	}

	return E_SUCCESS;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	ifstream file(FileName, ios::binary);
	unsigned char header[sizeof(FILE_MAGIC)];

	if (!file.is_open())
		return false;

	if (file.read(reinterpret_cast<char*>(header), sizeof(FILE_MAGIC)))
		return (memcmp(header, FILE_MAGIC, sizeof(FILE_MAGIC)) == 0);

	return false;
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
	return;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_MULTIPLE | PK_CAPS_BY_CONTENT | PK_CAPS_HIDE;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == nullptr)
	{
		gExtensions = (tExtensionStartupInfo*)malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != nullptr)
	{
		free(gExtensions);
	}

	gExtensions = nullptr;
}

#ifdef __cplusplus
}

#endif
