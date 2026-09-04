#include <cstdint>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <array>
#include <random>
#include <algorithm>

#include <zarchive/zarchivereader.h>
#include <zarchive/zarchivewriter.h>

#include "wcxplugin.h"
#include "extension.h"

using namespace std;
namespace fs = std::filesystem;

#define MessageBox dc_extensions->MessageBox
#define BUFF_SIZE 8192
#define PACKER_CAPS PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_DELETE | PK_CAPS_MULTIPLE | PK_CAPS_SEARCHTEXT
#define DOESNT_CONTAIN(LIST, STRING) find(LIST.begin(), LIST.end(), STRING) == LIST.end()
#define MAX_ATTEMPTS 300
#define CURRENT_PROGRESS 1000 //0
#define TOTAL_PROGRESS 0 //1000

typedef struct
{
	string path;
	bool is_dir;
	uint64_t size;
} FileInfo;

typedef struct
{
	ZArchiveReader *reader;
	vector<FileInfo> files;
	uint32_t index;
	uint32_t count;
	fs::path arc_path;
	string last_path;
	ofstream out;
	tProcessDataProc progress;
} ArcData;

typedef void *HINSTANCE;

tProcessDataProc progress = nullptr;
tExtensionStartupInfo* dc_extensions = nullptr;

const unsigned char FILE_MAGIC[] = {0x28, 0xB5, 0x2F, 0xFD, 0x60, 0xFF};

#ifdef __cplusplus
extern "C" {
#endif

#include <vector>
#include <string>

static void build_filelist(ZArchiveReader* reader, ZArchiveNodeHandle parent, const string& path, vector<FileInfo>& files)
{
	if (!reader || parent == ZARCHIVE_INVALID_NODE)
		return;

	uint32_t count = reader->GetDirEntryCount(parent);

	for (uint32_t i = 0; i < count; ++i)
	{
		ZArchiveReader::DirEntry entry;

		if (reader->GetDirEntry(parent, i, entry))
		{
			FileInfo info;
			info.path = path.empty() ? string(entry.name) : path + "/" + string(entry.name);
			info.is_dir = entry.isDirectory;
			info.size = entry.size;
			files.push_back(info);

			if (entry.isDirectory)
			{
				ZArchiveNodeHandle child = reader->LookUp(info.path, entry.isFile, true);
				build_filelist(reader, child, info.path, files);
			}
		}
	}
}

static bool append_items_from_archive(ZArchiveWriter& writer, char *archive, ArcData *data, vector<string>& blacklist)
{
	vector<uint8_t> buf;
	buf.resize(BUFF_SIZE);
	ZArchiveNodeHandle parent;
	try
	{
		data->reader = ZArchiveReader::OpenFromFile(archive);

		if (data->reader && (parent = data->reader->LookUp("", false, true)) != ZARCHIVE_INVALID_NODE)
		{
			build_filelist(data->reader, parent, "", data->files);
			uint32_t count = data->files.size();

			for (uint32_t i = 0; i < count; i++)
			{
				FileInfo info = data->files[i];

				if (DOESNT_CONTAIN(blacklist, info.path))
				{
					double percent =  i * 100 / count;
					char *filename = (char*)info.path.c_str();
					progress(filename, -(TOTAL_PROGRESS + percent));

					if (info.is_dir)
						writer.MakeDir(filename, true);
					else if (writer.StartNewFile(filename) && info.size > 0)
					{
						uint64_t read = 0;
						uint64_t offset = 0;
						ZArchiveNodeHandle file = data->reader->LookUp(info.path, true, false);

						while ((read = data->reader->ReadFromFile(file, offset, BUFF_SIZE, buf.data())) > 0)
						{
							writer.AppendData(buf.data(), read);
							offset += read;
							percent =  offset * 100 / info.size;

							if (progress(filename, -(CURRENT_PROGRESS + percent)) == 0)
								return false;
						}
					}
				}
			}
		}
		else
			return false;
	}
	catch (...)
	{
		return false;
	}

	return true;
}


static string get_tempname(char *filename)
{
	fs::path src(filename);
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> dis(10000000, 99999999);
	string dir = src.parent_path().string();
	string ext = src.extension().string();
	string tempname;
	int attempts = 0;

	do
	{
		tempname = dir + "/" + to_string(dis(gen)) + ext;

		if (attempts++ > MAX_ATTEMPTS)
			return {};
	}
	while (fs::exists(tempname));

	return tempname;
}

static void new_file_cb(const int32_t index, void* userdata)
{
	return;
}

static void write_file_cb(const void* buf, size_t len, void* userdata)
{
	ArcData *data = (ArcData*)userdata;
	data->out.write((const char*)buf, len);
}

HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	ArcData *data = new ArcData{};

	try 
	{
		ZArchiveNodeHandle parent;
		data->arc_path = fs::path(ArchiveData->ArcName);
		data->reader = ZArchiveReader::OpenFromFile(data->arc_path);

		if (!data->reader || (parent = data->reader->LookUp("", false, true)) == ZARCHIVE_INVALID_NODE)
		{
			ArchiveData->OpenResult = E_BAD_DATA;
			delete data;
			return nullptr;
		}

		build_filelist(data->reader, parent, "", data->files);
		data->count = data->files.size();
		data->index = 0;
		ArchiveData->OpenResult = 0;
	}
	catch (...) 
	{
		ArchiveData->OpenResult = E_EOPEN;
		delete data;
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
	ArcData *data = (ArcData*)hArcData;

	memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));
	HeaderDataEx->PackSizeHigh = 0xFFFFFFFF;
	HeaderDataEx->PackSize = 0xFFFFFFFE;

	try
	{
		if (data->index < data->count)
		{
			FileInfo info = data->files[data->index];
			strncpy(HeaderDataEx->FileName, info.path.c_str(), sizeof(HeaderDataEx->FileName) - 1);
			data->last_path = info.path;

			if (info.is_dir)
				HeaderDataEx->FileAttr = 16877;
			else
			{
				HeaderDataEx->UnpSizeHigh = (info.size & 0xFFFFFFFF00000000) >> 32;
				HeaderDataEx->UnpSize = info.size & 0x00000000FFFFFFFF;
			}

			return E_SUCCESS;
		}
	}
	catch (const std::exception& ex)
	{
		cerr << PLUGNAME << " ("<<data->arc_path.filename() << ") skip index #" << data->index << " : " << ex.what() << endl;
		data->index++;
	}

	return E_END_ARCHIVE;
}

int DCPCALL ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
{
	int result = E_SUCCESS;
	ArcData *data = (ArcData*)hArcData;

	if (Operation == PK_EXTRACT) 
	{
		ZArchiveNodeHandle file = data->reader->LookUp(data->last_path, true, false);

		if (file == ZARCHIVE_INVALID_NODE)
			return E_BAD_DATA;

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
			else
			{
				std::vector<uint8_t> buffer;
				buffer.resize(BUFF_SIZE);
				uint64_t read = 0;
				uint64_t offset = 0;

				while ((read = data->reader->ReadFromFile(file, offset, BUFF_SIZE, buffer.data())) > 0)
				{

					out.write((const char*)buffer.data(), read);
					offset += read;

					if (data->progress(DestName, read) == 0)
					{
						result =  E_EABORTED;
						break;
					}
				}

				out.close();
			}

		} 
		catch (const std::exception& ex)
		{
			string msg(data->last_path + ": " + ex.what());
			MessageBox((char*)msg.c_str(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
			result = E_EABORTED;
		}
	}

	data->index++;

	return result;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData *data = (ArcData*)hArcData;

	if (data) 
	{
		delete data->reader;
		delete data;
	}

	return E_SUCCESS;
}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	bool result = false;
	ifstream file(FileName, ios::binary);
	unsigned char header[sizeof(FILE_MAGIC)];

	if (!file.is_open())
		return result;

	if (file.read(reinterpret_cast<char*>(header), sizeof(FILE_MAGIC)))
		result = (memcmp(header, FILE_MAGIC, sizeof(FILE_MAGIC)) == 0);

	file.close();

	return result;
}

void DCPCALL SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc)
{
	ArcData *data = (ArcData*)hArcData;

	if ((int)(long)hArcData == -1 || !data)
		progress = pProcessDataProc;
	else
		data->progress = pProcessDataProc;
}

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc)
{
	return;
}

int DCPCALL GetPackerCaps(void)
{
	return PACKER_CAPS;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	ArcData data;
	char buf[BUFF_SIZE];
	string archive(PackedFile);
	vector<string> blacklist;
	int result = E_SUCCESS;

	bool is_repack = fs::exists(PackedFile);

	if (is_repack)
	{
		archive = get_tempname(PackedFile);

		if (archive.empty())
			return E_ECREATE;
	}

	data.out.open(archive, ios::binary | ios::out);

	if (!data.out.is_open())
		return E_ECREATE;

	string src(SrcPath);
	ZArchiveWriter writer(new_file_cb, write_file_cb, &data);

	if (SubPath)
		writer.MakeDir(SubPath, true);

	while (*AddList)
	{
		string filename(AddList);
		string target = SubPath ? string(SubPath) + "/" + filename : filename;
		char *item = (char*)target.c_str();
		data.last_path = src + filename;

		try
		{
			if (fs::is_directory(data.last_path))
				writer.MakeDir(item, true);
			else if (writer.StartNewFile(item))
			{
				streamsize read;
				ifstream in(data.last_path, std::ios::binary);

				if (in.is_open())
				{
					while (in.read(buf, BUFF_SIZE) || in.gcount() > 0)
					{
						read = in.gcount();
						writer.AppendData(buf, read);

						if (progress(item, (int)read) == 0)
						{
							result = E_EABORTED;
							break;
						}
					}

					in.close();
				}
				else
				{
					result = E_EREAD;
					break;
				}
			}
		}
		catch(const exception& ex)
		{
			string msg(target + ": " + ex.what());
			MessageBox((char*)msg.c_str(), (char*)PLUGNAME,  MB_OK | MB_ICONERROR);
			result = E_EABORTED;
		}

		if (result != E_SUCCESS)
			break;

		if (is_repack)
			blacklist.push_back(data.last_path);

		while (*AddList++);
	}

	if (is_repack && result == E_SUCCESS)
	{
		if (!append_items_from_archive(writer, PackedFile, &data, blacklist))
			result = E_EABORTED;
	}

	writer.Finalize();
	data.out.close();

	if (is_repack)
	{
		if (result == E_SUCCESS)
		{
			fs::remove(PackedFile);
			fs::rename(archive, PackedFile);
		}
		else
			fs::remove(archive);
	}

	return result;
}

int DCPCALL DeleteFiles(char *PackedFile, char *DeleteList)
{
	ArcData data;
	vector<string> blacklist;
	int result = E_SUCCESS;

	string archive = get_tempname(PackedFile);

	if (archive.empty())
		return E_EWRITE;

	data.out.open(archive, ios::binary | ios::out);

	if (!data.out.is_open())
		return E_EWRITE;

	ZArchiveWriter writer(new_file_cb, write_file_cb, &data);

	while (*DeleteList)
	{
		string path(DeleteList);
		int len = path.length();

		if (len > 4 && path.substr(len - 4) == "/*.*")
			path.resize(len - 4);

		blacklist.push_back(path);
		while (*DeleteList++);
	}

	if (!append_items_from_archive(writer, PackedFile, &data, blacklist))
		result = E_EABORTED;

	writer.Finalize();
	data.out.close();

	if (result == E_SUCCESS)
	{
		fs::remove(PackedFile);
		fs::rename(archive, PackedFile);
	}
	else
		fs::remove(archive);

	return result;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (dc_extensions == nullptr)
	{
		dc_extensions = (tExtensionStartupInfo*)malloc(sizeof(tExtensionStartupInfo));
		memcpy(dc_extensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (dc_extensions != nullptr)
	{
		free(dc_extensions);
		dc_extensions = nullptr;
	}
}

#ifdef __cplusplus
}

#endif
