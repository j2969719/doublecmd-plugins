#include <glib.h>
#include <unistd.h>

#if PLUGTARGET == 6
#include <wand/MagickWand.h>
#else
#include <MagickWand/MagickWand.h>
#endif

#include "wcxplugin.h"
#include "extension.h"

#define SendDlgMsg gExtensions->SendDlgMsg
#define MessageBox gExtensions->MessageBox
#define InputBox gExtensions->InputBox

typedef struct sArcData
{
	uint32_t i;
	uint32_t num;
	MagickWand *magick_wand;
	tChangeVolProc ChangeVolProc;
	tProcessDataProc ProcessDataProc;
} tArcData;

typedef tArcData* ArcData;

tProcessDataProc gProcessDataProc = NULL;
tExtensionStartupInfo* gExtensions = NULL;


HANDLE DCPCALL OpenArchive(tOpenArchiveData *ArchiveData)
{
	ArcData data = malloc(sizeof(tArcData));

	if (data == NULL)
	{
		ArchiveData->OpenResult = E_NO_MEMORY;
		return E_SUCCESS;
	}

	memset(data, 0, sizeof(tArcData));
	data->magick_wand = NewMagickWand();

	if (MagickReadImage(data->magick_wand, ArchiveData->ArcName) == MagickFalse)
	{
		data->magick_wand = DestroyMagickWand(data->magick_wand);
		free(data);
		ArchiveData->OpenResult = E_UNKNOWN_FORMAT;
		return E_SUCCESS;
	}

	MagickResetIterator(data->magick_wand);
	data->num = MagickGetNumberImages(data->magick_wand);

	return (HANDLE)data;
}

int DCPCALL ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
{
	return E_NOT_SUPPORTED;
}

int DCPCALL ReadHeaderEx(HANDLE hArcData, tHeaderDataEx *HeaderDataEx)
{
	ArcData data = (ArcData)hArcData;

	if (data->i < data->num)
	{
		memset(HeaderDataEx, 0, sizeof(&HeaderDataEx));
		MagickSetIteratorIndex(data->magick_wand, data->i);
		size_t width = MagickGetImageWidth(data->magick_wand);
		size_t height = MagickGetImageHeight(data->magick_wand);
		snprintf(HeaderDataEx->FileName, sizeof(HeaderDataEx->FileName) - 1, "[%d] %ldx%ld.png", data->i + 1, width, height);
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
		MagickWriteImage(data->magick_wand, DestName);

	data->i++;
	return E_SUCCESS;
}

int DCPCALL CloseArchive(HANDLE hArcData)
{
	ArcData data = (ArcData)hArcData;

	data->magick_wand = DestroyMagickWand(data->magick_wand);
	free(data);

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

void DCPCALL SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1)
{

}

BOOL DCPCALL CanYouHandleThisFile(char *FileName)
{
	if (g_str_has_suffix(FileName, ".ico"))
		return TRUE;

	return FALSE;
}

int DCPCALL GetPackerCaps(void)
{
	return PK_CAPS_NEW | PK_CAPS_HIDE;
}

int DCPCALL PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags)
{
	int result = E_SUCCESS;
	int sizes[] = { 16, 20, 24, 32, 40, 48, 128, 256, 0};

	if (access(PackedFile, F_OK) == 0)
		return E_ECREATE;

	MagickWand *magick_wand = NewMagickWand();
	MagickWand *out = NewMagickWand();

	MagickResetIterator(magick_wand);

	while (*AddList)
	{
		if (AddList[strlen(AddList) - 1] != '/')
		{
			gchar *path = g_strdup_printf("%s%s", SrcPath, AddList);

			if (MagickReadImage(magick_wand, path) == MagickFalse)
			{
				g_free(path);
				result = E_NOT_SUPPORTED;
				break;
			}

			size_t width = MagickGetImageWidth(magick_wand);
			size_t height = MagickGetImageHeight(magick_wand);

			for (int i = 0; sizes[i] != 0; i++)
			{
				if (sizes[i] <= width && sizes[i] <= height)
					if (MagickAddImage(out, magick_wand) == MagickTrue)
#if PLUGTARGET == 6
//fixme
						MagickResizeImage(out, sizes[i], sizes[i], 0);
#else
						MagickResizeImage(out, sizes[i], sizes[i], LanczosFilter);
#endif
			}

			g_free(path);

			if (gProcessDataProc(AddList, -1100) == 0)
			{
				return E_EABORTED;
				break;
			}
		}

		while (*AddList++);
	}

	if (result == E_SUCCESS)
		MagickWriteImages(out, PackedFile, MagickTrue);

	if (out)
		out = DestroyMagickWand(out);

	if (magick_wand)
		magick_wand = DestroyMagickWand(magick_wand);

	return result;
}

void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo)
{
	if (gExtensions == NULL)
	{
		MagickWandGenesis();
		gExtensions = (tExtensionStartupInfo*)malloc(sizeof(tExtensionStartupInfo));
		memcpy(gExtensions, StartupInfo, sizeof(tExtensionStartupInfo));
	}
}

void DCPCALL ExtensionFinalize(void* Reserved)
{
	if (gExtensions != NULL)
	{
		free(gExtensions);
		MagickWandTerminus();
	}

}
