#include <fstream>
#include <bit7z/bit7z.hpp>
#include <bit7z/bitarchivewriter.hpp>
#include <bit7z/bitarchiveeditor.hpp>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <utime.h>
#include <string.h>
#include "wcxplugin.h"
#include "extension.h"

using namespace std;
using namespace bit7z;

#ifdef BIT7Z_AUTO_FORMAT
#define PACKERCAPS PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT | PK_CAPS_BY_CONTENT
#else
#define PACKERCAPS PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_MULTIPLE | PK_CAPS_DELETE | PK_CAPS_ENCRYPT | PK_CAPS_SEARCHTEXT | PK_CAPS_OPTIONS | PK_CAPS_BY_CONTENT
#endif

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox
#define LIST_ITEMS(L) (sizeof(L)/sizeof(tListItem))
#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))

#ifdef BIT7Z_AUTO_FORMAT
#define BITFORMAT BitFormat::Auto
#else
#define BITFORMAT BitFormat::SevenZip
#endif

typedef struct sArcData
{
	BitArchiveReader *reader;
	uint32_t index;
	uint32_t count;
	uint64_t total;
	char procfile[PATH_MAX];
	bool solid;
	vector<BitArchiveItemInfo> items;
	map<uint32_t, string> *dst_names;
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef struct sListItem
{
	const char *text;
	long int data;
} tListItem;

typedef tArcData* ArcData;
typedef void *HINSTANCE;

int gCryptoNr;
int gCryptoFlags;
tPkCryptProc gPkCryptProc  = nullptr;
tChangeVolProc gChangeVolProc  = nullptr;
tProcessDataProc gProcessDataProc = nullptr;
tExtensionStartupInfo* gExtensions = nullptr;

// Bit7zLibrary gBit7zLib { "/usr/lib/p7zip/7z.so" };
Bit7zLibrary gBit7zLib { "/usr/lib/7zip/7z.so" };

char gPass[PATH_MAX];
uint64_t gTotalSize = 0;
char gProcFile[PATH_MAX];
char gPassMsg[] = "Enter password:";

bool gSolid = true;
bool gCryptHeaders = false;
int gComprLevel = (int)BitCompressionLevel::Normal;
int gComprMethod = (int)BitCompressionMethod::Lzma2;
int gDictSize = 0;
int gWordSize = 0;
uint64_t gVolumeSize = 0;
int gThreadCount = 0;

tListItem gComprLevels[] =
{
	{"None",       (int)BitCompressionLevel::None},
	{"Fastest", (int)BitCompressionLevel::Fastest},
	{"Fast",       (int)BitCompressionLevel::Fast},
	{"Normal",   (int)BitCompressionLevel::Normal},
	{"Max", 	(int)BitCompressionLevel::Max},
	{"Ultra",     (int)BitCompressionLevel::Ultra},
};

tListItem gComprMethods[] =
{
	{"Copy",   (int)BitCompressionMethod::Copy},
	{"BZip2", (int)BitCompressionMethod::BZip2},
	{"Lzma",   (int)BitCompressionMethod::Lzma},
	{"Lzma2", (int)BitCompressionMethod::Lzma2},
	{"Ppmd",   (int)BitCompressionMethod::Ppmd},

};

tListItem gDictSizes[] =
{
	{"64 KB",	     65536},
	{"1 MB",	   1048576},
	{"2 MB",	   2097152},
	{"3 MB",	   3145728},
	{"4 MB",	   4194304},
	{"6 MB",	   6291456},
	{"8 MB",	   8388608},
	{"12 MB",	  12582912},
	{"16 MB",	  16777216},
	{"24 MB",	  25165824},
	{"32 MB",	  33554432},
	{"48 MB",	  50331648},
	{"64 MB",	  67108864},
	{"96 MB",	 100663296},
	{"128 MB",	 134217728},
	{"192 MB",	 201326592},
	{"256 MB",	 268435456},
	{"384 MB",	 402653184},
	{"512 MB",	 536870912},
	{"768 MB",	 805306368},
	{"1024 MB",	1073741824},
	{"1536 MB",	1610612736},
};

int gWordSizes[] = {8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 273};

tListItem gVolumeSizes[] =
{

	{"360 KB",	     362496},
	{"720 KB",	     730112},
	{"1.2 MB",	    1213952},
	{"10 MB",	   10485760},
	{"1.44 MB",	    1457664},
	{"128 MB",	  134217728},
	{"256 MB",	  268435456},
	{"512 MB",	  536870912},
	{"640 MB",	  671088640},
	{"700 MB",	  734003200},
	{"1 GB",	 1073741824},
	{"2 GB",	 2147483648},
	{"4 GB",	 4294967296},
	{"4480 MB",	 4697620480},
	{"8128 MB",	 8522825728},
	{"23040 MB",	24159191040},
};

static char *ask_password(void)
{
	if (!InputBox((char *)PLUGNAME, gPassMsg, true, gPass, PATH_MAX))
		gPass[0] = '\0';

	return gPass;
}

static bool show_progress(uint64_t size)
{
	int progress = 0;

	if (gTotalSize > 0)
		progress = 0 - size * 100 / gTotalSize;

	return (gProcessDataProc(gProcFile, progress) != 0);
}
/*
static void show_ratio(uint64_t input, uint64_t output)
{
	if (input > 0)
		gProcessDataProc(gProcFile, -1000 - output * 100 / input);
}
*/
static void set_totalsize(uint64_t size)
{
	gTotalSize = size;
}

static void set_filename(string filename)
{
	snprintf(gProcFile, PATH_MAX, "%s", filename.c_str());
}

static void fix_attr(tArcData *data, uint32_t index, char *DestName)
{
	struct utimbuf ubuf;
	auto item = data->items[index];
	int attr = item.attributes() >> 16;

	if (attr == 0)
	{
		if (item.isDir())
			attr = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		else
			attr = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	}

	chmod(DestName, attr);
	ubuf.actime = chrono::system_clock::to_time_t(item.lastAccessTime());
	ubuf.modtime = chrono::system_clock::to_time_t(item.lastWriteTime());
	utime(DestName, &ubuf);
}

static bool extract_single(tArcData *data, uint32_t index, char *DestName)
{
	bool result = true;
	ofstream outfile(DestName);

	try
	{
		data->reader->extractTo(outfile, index);
	}
	catch (const bit7z::BitException& ex)
	{
		if (ex.posixCode() != ECANCELED)
			MessageBox((char *)ex.what(), nullptr,  MB_OK | MB_ICONERROR);

		result = false;
	}

	outfile.close();
	fix_attr(data, index, DestName);

	return result;
}

intptr_t DCPCALL OptionsDlgProc(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam)
{
	int index = 0;

	switch (Msg)
	{
	case DN_INITDIALOG:

		for (index = 0; index < (int)LIST_ITEMS(gComprLevels); index++)
		{
			SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTADD, (intptr_t)gComprLevels[index].text, (intptr_t)gComprLevels[index].data);

			if (gComprLevel == gComprLevels[index].data)
				SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTSETITEMINDEX, index, 0);
		}

		for (index = 0; index < (int)LIST_ITEMS(gComprMethods); index++)
		{
			SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTADD, (intptr_t)gComprMethods[index].text, (intptr_t)gComprMethods[index].data);

			if (gComprMethod == gComprMethods[index].data)
				SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTSETITEMINDEX, index, 0);
		}

		for (index = 0; index < (int)LIST_ITEMS(gDictSizes); index++)
		{
			SendDlgMsg(pDlg, (char*)"cbDictionarySize", DM_LISTADD, (intptr_t)gDictSizes[index].text, (intptr_t)gDictSizes[index].data);

			if (gDictSize == gDictSizes[index].data)
				SendDlgMsg(pDlg, (char*)"cbDictionarySize", DM_LISTSETITEMINDEX, index, 0);
		}

		for (index = 0; index < (int)ARRAY_SIZE(gWordSizes); index++)
		{
			SendDlgMsg(pDlg, (char*)"cbWordSize", DM_LISTADD, (intptr_t)to_string(gWordSizes[index]).c_str(), (intptr_t)gWordSizes[index]);

			if (gWordSize == gWordSizes[index])
				SendDlgMsg(pDlg, (char*)"cbWordSize", DM_LISTSETITEMINDEX, index, 0);
		}

		for (index = 0; index < (int)LIST_ITEMS(gVolumeSizes); index++)
			SendDlgMsg(pDlg, (char*)"cbVolumeSize", DM_LISTADD, (intptr_t)gVolumeSizes[index].text, (intptr_t)gVolumeSizes[index].data);

		if (gVolumeSize > 0)
			SendDlgMsg(pDlg, (char*)"cbVolumeSize", DM_SETTEXT, (intptr_t)to_string(gVolumeSize).c_str(), 0);

		for (index = 1; index < 5; index++)
			SendDlgMsg(pDlg, (char*)"cbThreadsCount", DM_LISTADD, (intptr_t)to_string(index).c_str(), index);

		if (gThreadCount > 0)
			SendDlgMsg(pDlg, (char*)"cbThreadsCount", DM_LISTSETITEMINDEX, gThreadCount, 0);

		SendDlgMsg(pDlg, (char*)"ckSolid", DM_SETCHECK, (intptr_t)gSolid, 0);
		SendDlgMsg(pDlg, (char*)"ckCryptHeaders", DM_SETCHECK, (intptr_t)gCryptHeaders, 0);

		break;

	case DN_CLICK:
		if (strcmp(DlgItemName, "btnOK") == 0)
		{
			index = (int)SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gComprLevel = (int)SendDlgMsg(pDlg, (char*)"cbLevel", DM_LISTGETDATA, index, 0);

			index = (int)SendDlgMsg(pDlg, (char*)"cbMethod", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gComprMethod = (int)SendDlgMsg(pDlg, (char *)"cbMethod", DM_LISTGETDATA, index, 0);

			index = (int)SendDlgMsg(pDlg, (char*)"cbDictionarySize", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gDictSize = (int)SendDlgMsg(pDlg, (char*)"cbDictionarySize", DM_LISTGETDATA, index, 0);

			index = (int)SendDlgMsg(pDlg, (char*)"cbWordSize", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gWordSize = (int)SendDlgMsg(pDlg, (char*)"cbWordSize", DM_LISTGETDATA, index, 0);

			char *text = (char*)SendDlgMsg(pDlg, (char*)"cbVolumeSize", DM_GETTEXT, 0, 0);
			gVolumeSize = (int64_t)strtoll(text, nullptr, 10);

			index = (int)SendDlgMsg(pDlg, (char*)"cbThreadsCount", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
				gThreadCount = (int)SendDlgMsg(pDlg, (char*)"cbThreadsCount", DM_LISTGETDATA, index, 0);

			gSolid = (bool)SendDlgMsg(pDlg, (char*)"ckSolid", DM_GETCHECK, 0, 0);
			gCryptHeaders = (bool)SendDlgMsg(pDlg, (char*)"ckCryptHeaders", DM_GETCHECK, 0, 0);
		}

		break;

	case DN_KEYDOWN:
		if (strcmp(DlgItemName, "cbVolumeSize") == 0)
		{
			int16_t *key = (int16_t*)wParam;

			if ((*key < 48 || *key > 57) && *key != 8 && *key != 9 && *key != 13 && *key != 37 && *key != 38 && *key != 39 && *key != 40) // omfg
				*key = 0;
		}

		break;

	case DN_CHANGE:
		if (strcmp(DlgItemName, "cbVolumeSize") == 0)
		{
			index = (int)SendDlgMsg(pDlg, (char*)"cbVolumeSize", DM_LISTGETITEMINDEX, 0, 0);

			if (index != -1)
			{
				uint64_t data = (uint64_t)SendDlgMsg(pDlg, (char*)"cbVolumeSize", DM_LISTGETDATA, index, 0);
				SendDlgMsg(pDlg, (char*)"cbVolumeSize", DM_SETTEXT, (intptr_t)to_string(data).c_str(), 0);
			}
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
		try
		{
			data->reader = new BitArchiveReader { gBit7zLib, std::filesystem::path(ArchiveData->ArcName), BITFORMAT };
		}
		catch (const bit7z::BitException& ex)
		{
			char pass[PATH_MAX] = "";

			if (ex.posixCode() == 1)
			{
				if (!gCryptoFlags & PK_CRYPTOPT_MASTERPASS_SET)
				{
					int ret = gPkCryptProc(gCryptoNr, PK_CRYPT_LOAD_PASSWORD_NO_UI, ArchiveData->ArcName, pass, PATH_MAX);

					if (ret != 0 && ArchiveData->OpenMode == PK_OM_LIST)
						ret = gPkCryptProc(gCryptoNr, PK_CRYPT_LOAD_PASSWORD, ArchiveData->ArcName, pass, PATH_MAX);
				}

				if (pass[0] != '\0' || InputBox((char*)PLUGNAME, (char*)ex.what(), true, pass, PATH_MAX))
					data->reader = new BitArchiveReader { gBit7zLib, ArchiveData->ArcName, BITFORMAT, pass };
			}
		}

		if (data->reader)
		{
			data->reader->setPasswordCallback(ask_password);
			data->reader->setTotalCallback([data](uint64_t size)
			{
				data->total = size;
			});

			data->reader->setFileCallback([data](string filename)
			{
				snprintf(data->procfile, PATH_MAX, "%s", filename.c_str());
			});

			data->reader->setProgressCallback([data](uint64_t size)
			{
				int progress = 0;
				int mode = data->solid ? 0 : -1000;

				if (data->total > 0)
					progress = mode - size * 100 / data->total;

				return (data->ProcessDataProc(data->procfile, progress) != 0);
			});

			data->count = data->reader->itemsCount();
			data->items = data->reader->items();

			if (ArchiveData->OpenMode == PK_OM_EXTRACT)
			{
				data->solid = data->reader->isSolid();

				if (data->solid)
				{
					data->dst_names = new map<uint32_t, string>;
					snprintf(data->procfile, PATH_MAX, "%s", ArchiveData->ArcName);
				}
			}
		}
		else
		{
			free(data);
			ArchiveData->OpenResult = E_EOPEN;
			return E_SUCCESS;
		}
	}
	catch (const bit7z::BitException& ex)
	{
		printf("%s (%s): %s\n", PLUGNAME, ArchiveData->ArcName, ex.what());

		free(data);

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
		auto item = data->items[data->index];
		snprintf(HeaderDataEx->FileName, sizeof(HeaderDataEx->FileName), "%s", item.path().c_str());

		HeaderDataEx->FileAttr = item.attributes() >> 16;

		if (HeaderDataEx->FileAttr == 0)
		{
			if (item.isDir())
				HeaderDataEx->FileAttr = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
			else
				HeaderDataEx->FileAttr = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		}

		HeaderDataEx->PackSizeHigh = (item.packSize() & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->PackSize = item.packSize() & 0x00000000FFFFFFFF;
		HeaderDataEx->UnpSizeHigh = (item.size() & 0xFFFFFFFF00000000) >> 32;
		HeaderDataEx->UnpSize = item.size() & 0x00000000FFFFFFFF;
		HeaderDataEx->FileTime = (int)chrono::system_clock::to_time_t(item.lastWriteTime());
		HeaderDataEx->FileCRC = (int)item.crc();
		return E_SUCCESS;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	ArcData data = (ArcData)hArcData;

	int totalProgress = data->count > 0 ? -(data->index * 100 / data->count) : 0;

	if (!data->solid && data->ProcessDataProc(DestName, totalProgress) == 0)
		return E_EABORTED;

	if (Operation == PK_EXTRACT)
	{
		if (data->solid)
			data->dst_names->insert(pair<uint32_t, string>(data->index, string(DestName)));
		else if (extract_single(data, data->index, DestName) == false)
			return E_EWRITE;
	}
	else if (Operation == PK_TEST && data->index == 0)
	{
		try
		{
			data->reader->test();
		}
		catch (const bit7z::BitException& ex)
		{
			if (ex.posixCode() != ECANCELED)
				MessageBox((char*)ex.what(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
		}
	}

	data->index++;

	return E_SUCCESS;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	if (data->solid)
	{
		if (data->dst_names->size() > 1)
		{
			try
			{
				vector<uint32_t> indices;
				std::filesystem::path p(data->dst_names->begin()->second);
				string outdir = string(p.parent_path()) + "/unpk_XXXXXX";
				char *tmppath = mkdtemp((char*)outdir.c_str());

				if (tmppath != nullptr)
				{
					outdir = string(tmppath);

					for (auto i = data->dst_names->begin(); i != data->dst_names->end(); i++)
						indices.push_back(i->first);

					try
					{
						data->reader->extractTo(outdir, indices);

						for (auto i = data->dst_names->begin(); i != data->dst_names->end(); i++)
						{
							string arc_path = data->items[i->first].path();
							string outpath = outdir + fs::path::preferred_separator + arc_path;
							fix_attr(data, i->first, (char*)outpath.c_str());

							try
							{
								fs::rename(outpath, i->second);
							}
							catch (fs::filesystem_error& ex)
							{
								int ret = MessageBox((char*)ex.what(), (char*)PLUGNAME,  MB_OKCANCEL | MB_ICONERROR);

								if (ret == ID_CANCEL)
									break;
							}
						}
					}
					catch (const bit7z::BitException& ex)
					{
						if (ex.posixCode() != ECANCELED)
							MessageBox((char*)ex.what(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
					}

					fs::remove_all(outdir);
				}
				else
					MessageBox((char*)"Cannot create a temp directory", (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
			}
			catch (fs::filesystem_error& ex)
			{
				MessageBox((char*)ex.what(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
			}
		}
		else if (data->dst_names->size() == 1)
			extract_single(data, data->dst_names->begin()->first, (char*)data->dst_names->begin()->second.c_str());

		delete data->dst_names;
	}

	delete data->reader;
	free(data);

	return E_SUCCESS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	string src(SrcPath);
	string arc(PackedFile);

	if (arc.substr(arc.length() - 4) == ".run" || Flags & PK_PACK_MOVE_FILES || !(Flags & PK_PACK_SAVE_PATHS))
		return E_NOT_SUPPORTED;

	char pass[PATH_MAX] = "";

	try
	{
		BitArchiveWriter writer{ gBit7zLib, arc, BitFormat::SevenZip, pass };
		writer.setUpdateMode(UpdateMode::Update);

		writer.setCompressionLevel((BitCompressionLevel)gComprLevel);
		writer.setCompressionMethod((BitCompressionMethod)gComprMethod);
		writer.setDictionarySize(gDictSize);
		writer.setWordSize(gWordSize);
		writer.setStoreSymbolicLinks(true);
		writer.setSolidMode(gSolid);
		writer.setVolumeSize(gVolumeSize);
		writer.setThreadsCount(gThreadCount);

		writer.setPasswordCallback(ask_password);
		writer.setProgressCallback(show_progress);
		gTotalSize = 0;
		gProcFile[0] = '\0';
		writer.setTotalCallback(set_totalsize);
		writer.setFileCallback(set_filename);
		//writer.setRatioCallback(show_ratio);

		if (Flags & PK_PACK_ENCRYPT)
		{
			pass[0] = '\0';

			if (InputBox(nullptr, gPassMsg, true, pass, PATH_MAX))
				writer.setPassword(pass, gCryptHeaders);

			if (gCryptHeaders)
				gPkCryptProc(gCryptoNr, PK_CRYPT_SAVE_PASSWORD, PackedFile, pass, PATH_MAX);
		}

		map<string, string> items;

		while (*AddList)
		{
			if (gProcessDataProc(AddList, 0) == 0)
				return E_EABORTED;

			string target(AddList);
			string filename = src + target;

			if (SubPath)
				target.insert(0, 1, '/').insert(0,  string(SubPath));

			if (std::filesystem::exists(filename))
			{
				if (std::filesystem::is_symlink(filename))
					items[filename] = target;
				else if (std::filesystem::is_directory(filename))
				{
					try
					{
						if (std::filesystem::is_empty(filename))
							items[filename] = target;
					}
					catch (...)
					{
						printf("%s: SKIP -> %s\n", PLUGNAME, filename.c_str());
					}
				}
				else
					items[filename] = target;
			}
			else
				printf("%s: SKIP -> %s\n", PLUGNAME, filename.c_str());

			while (*AddList++);
		}

		writer.addItems(items);
		writer.compressTo(PackedFile);
	}
	catch (const bit7z::BitException& ex)
	{
		if (ex.posixCode() != ECANCELED)
			MessageBox((char*)ex.what(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);

		return E_EABORTED;
	}

	return E_SUCCESS;
}

int DCPCALL DeleteFiles(char *PackedFile, char *DeleteList)
{
	char pass[PATH_MAX] = "";

	try
	{
		BitArchiveReader reader{ gBit7zLib, PackedFile, BitFormat::SevenZip, pass };
		BitArchiveEditor writer{ gBit7zLib, PackedFile, BitFormat::SevenZip, pass };
		auto items = reader.items();
		auto count = reader.itemsCount();
		writer.setOverwriteMode(OverwriteMode::Overwrite);
		writer.setPasswordCallback(ask_password);
		writer.setProgressCallback(show_progress);
		gTotalSize = 0;
		gProcFile[0] = '\0';
		writer.setTotalCallback(set_totalsize);
		writer.setFileCallback(set_filename);

		while (*DeleteList)
		{
			if (gProcessDataProc(DeleteList, 0) == 0)
				return E_EABORTED;

			auto target = string(DeleteList);

			try
			{
				if (target.substr(target.length() - 4) == "/*.*")
				{
					target.erase(target.length() - 3);

					for (uint32_t i = 0; i < count; i++)
					{
						if (items[i].path().compare(0, target.size(), target) == 0)
							writer.deleteItem(i);
					}

				}
				else
					writer.deleteItem(target);
			}
			catch (const bit7z::BitException& ex)
			{
				if (ex.posixCode() != ECANCELED)
				{
					string msg = target + ": " + ex.what();
					MessageBox((char*)msg.c_str(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
				}
			}

			while (*DeleteList++);
		}

		writer.applyChanges();
	}
	catch (const bit7z::BitException& ex)
	{
		if (ex.posixCode() != ECANCELED)
			MessageBox((char*)ex.what(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);

		return E_EABORTED;
	}

	return E_SUCCESS;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	try
	{
		BitArchiveReader reader { gBit7zLib, FileName, BITFORMAT };
	}
	catch (const bit7z::BitException& ex)
	{
		return false;
	}

	return true;
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
