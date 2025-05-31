#include <cmath>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <limits.h>
#include <MediaInfoDLL/MediaInfoDLL.h>
#include "wdxplugin.h"

using namespace std;
using namespace MediaInfoDLL;

enum
{
	PRE_NONE,
	PRE_TO_SEC,
	PRE_SHORT_TIME,
	PRE_FLOAT_BPS,
	PRE_UNITS_BPS,
	PRE_APROX_BPS,
};

typedef struct sfield
{
	const char *name;
	int type;
	const char *units;
	const char *info;
	int pre;
	char *value;
} tfield;

typedef struct saproxbps
{
	const char *text;
	double value;
	double delta;
	int64_t unit;
} taproxbps;

#define ARRAY_SIZE(arr) (int)(sizeof(arr) / sizeof((arr)[0]))
#define FIELDCOUNT ARRAY_SIZE(gFields)
#define ONE_KILO 1000
#define ONE_MEGA 1000000
#define ONE_GIGA 1000000000 //CHAD

taproxbps gBpsChoices[] =
{
	// name,	value	+/-	unit
	{"16 KBps",	 16,	  2,	ONE_KILO},
	{"32 KBps",	 32,	  2,	ONE_KILO},
	{"96 KBps",	 96,	  2,	ONE_KILO},
	{"128 KBps",	128,	  2,	ONE_KILO},
	{"160 KBps",	160,	  2,	ONE_KILO},
	{"192 KBps",	192,	  2,	ONE_KILO},
	{"256 KBps",	256,	  2,	ONE_KILO},
	{"320 KBps",	320,	  2,	ONE_KILO},
	{"400 KBps",	320,	  2,	ONE_KILO},
	{"750 KBps",	320,	  2,	ONE_KILO},
	{"1 MBps",	  1,	0.5,	ONE_MEGA},
	{"2.5 MBps",	2.5,	0.5,	ONE_MEGA},
	{"3.8 MBps",	3.8,	0.5,	ONE_MEGA},
	{"4.5 MBps",	4.5,	0.5,	ONE_MEGA},
	{"6.8 MBps",	6.8,	0.5,	ONE_MEGA},
	{"9.8 MBps",	9.8,	0.5,	ONE_MEGA},
	{"18 MBps",	 18,	0.5,	ONE_MEGA},
	{"25 MBps",	 25,	  2,	ONE_MEGA},
	{"40 MBps",	 40,	  2,	ONE_MEGA},
	{"250 MBps",	250,	  2,	ONE_MEGA},
	{"1.4 GBps",	1.4,	0.5,	ONE_GIGA},
};

tfield gFields[] =
{
	// field name, 					  field type,			 units,  info,				    prepocess,	cache
	{"General: Title",				  ft_string,			    "", "General;%Title%",		     PRE_NONE, nullptr},
	{"General: Format",				  ft_string,			    "", "General;%Format%",		     PRE_NONE, nullptr},
//	{"General: Duration, ms",			  ft_numeric_64,		    "", "General;%Duration%",		     PRE_NONE, nullptr},
	{"General: Duration, s", 			  ft_numeric_32,		    "", "General;%Duration%",		   PRE_TO_SEC, nullptr},
	{"General: Duration, HH:MM:SS",			  ft_time,			    "", "General;%Duration%",		   PRE_TO_SEC, nullptr},
//	{"General: Duration, HH:MM:SS",			  ft_string,			    "", "General;%Duration/String3%",  PRE_SHORT_TIME, nullptr},
	{"General: Duration, HH:MM:SS.MMM",		  ft_string,			    "", "General;%Duration/String3%",	     PRE_NONE, nullptr},
	{"General: Overall bit rate",			  ft_numeric_floating, "Bps|KBps|MBps", "General;%OverallBitRate%",	PRE_UNITS_BPS, nullptr},
	{"General: Overall bit rate, auto Bps/KBps/MBps", ft_string,			    "", "General;%OverallBitRate%",	PRE_FLOAT_BPS, nullptr},
	{"General: Overall bit rate, aprox",		  ft_multiplechoice,		    "", "General;%OverallBitRate%",	PRE_APROX_BPS, nullptr},
	{"General: Count of streams",			  ft_numeric_64,		    "", "General;%StreamCount%",	     PRE_NONE, nullptr},
	{"General: Number of audio streams",		  ft_numeric_64,		    "", "General;%AudioCount%",		     PRE_NONE, nullptr},
	{"General: Number of text streams",		  ft_numeric_64,		    "", "General;%TextCount%",		     PRE_NONE, nullptr},
	{"General: Number of image streams",		  ft_numeric_64,		    "", "General;%ImageCount%",		     PRE_NONE, nullptr},
	{"General: Number of menu streams",		  ft_numeric_64,		    "", "General;%MenuCount%",		     PRE_NONE, nullptr},
	{"General: Encoded",				  ft_datetime,			    "", "General;%Encoded_Date%",	     PRE_NONE, nullptr},
	{"General: Tagged",				  ft_datetime,			    "", "General;%Tagged_Date%",	     PRE_NONE, nullptr},
	{"Video: Format",				  ft_string,			    "", "Video;%Format%",		     PRE_NONE, nullptr},
	{"Video: Format profile",			  ft_string,			    "", "Video;%Format_Profile%",	     PRE_NONE, nullptr},
	{"Video: Codec ID",				  ft_string,			    "", "Video;%CodecID%",		     PRE_NONE, nullptr},
	{"Video: Streamsize in bytes",			  ft_numeric_64,		    "", "Video;%StreamSize%",		     PRE_NONE, nullptr},
	{"Video: Bit rate",				  ft_numeric_floating, "Bps|KBps|MBps", "Video;%BitRate%",	        PRE_UNITS_BPS, nullptr},
	{"Video: Bit rate, auto Bps/KBps/MBps",		  ft_string,			    "", "Video;%BitRate%",	        PRE_FLOAT_BPS, nullptr},
	{"Video: Bit rate, aprox",			  ft_multiplechoice,		    "", "Video;%BitRate%",	        PRE_APROX_BPS, nullptr},
	{"Video: Width",				  ft_numeric_64,		    "", "Video;%Width%",		     PRE_NONE, nullptr},
	{"Video: Height",				  ft_numeric_64,		    "", "Video;%Height%", 		     PRE_NONE, nullptr},
	{"Video: Resolution",				  ft_string,			    "", "Video;%Width%x%Height%", 	     PRE_NONE, nullptr},
	{"Video: Display aspect ratio",			  ft_numeric_floating,		    "", "Video;%DisplayAspectRatio%",	     PRE_NONE, nullptr},
	{"Video: Display aspect ratio (string)",	  ft_string,			    "", "Video;%DisplayAspectRatio/String%", PRE_NONE, nullptr},
	{"Video: Pixel aspect ratio",			  ft_numeric_floating,		    "", "Video;%PixelAspectRatio%",	     PRE_NONE, nullptr},
	{"Video: Frame rate mode",			  ft_string,			    "", "Video;%FrameRate_Mode/String%",     PRE_NONE, nullptr},
	{"Video: Frame rate",				  ft_numeric_floating,		    "", "Video;%FrameRate%",		     PRE_NONE, nullptr},
	{"Video: Number of frames",			  ft_numeric_64,		    "", "Video;%FrameCount%",		     PRE_NONE, nullptr},
	{"Video: Bits/(Pixel*Frame)",			  ft_numeric_floating,		    "", "Video;%Bits-(Pixel*Frame)%",	     PRE_NONE, nullptr},
	{"Video: Rotation",				  ft_numeric_64,		    "", "Video;%Rotation%",		     PRE_NONE, nullptr},
	{"Video: Writing library",			  ft_string,			    "", "Video;%Encoded_Library/String%",    PRE_NONE, nullptr},
	{"Audio: Format",				  ft_string,			    "", "Audio;%Format%",		     PRE_NONE, nullptr},
	{"Audio: Format profile",			  ft_string,			    "", "Audio;%Format_Profile%",	     PRE_NONE, nullptr},
	{"Audio: Codec ID",				  ft_string,			    "", "Audio;%CodecID%",		     PRE_NONE, nullptr},
	{"Audio: Streamsize in bytes",			  ft_numeric_64,		    "", "Audio;%StreamSize%",		     PRE_NONE, nullptr},
	{"Audio: Bit rate",				  ft_numeric_floating, "Bps|KBps|MBps", "Audio;%BitRate%",		PRE_UNITS_BPS, nullptr},
	{"Audio: Bit rate, auto Bps/KBps/MBps",		  ft_string,			    "", "Audio;%BitRate%", 		PRE_FLOAT_BPS, nullptr},
	{"Audio: Bit rate, aprox",			  ft_multiplechoice,		    "", "Audio;%BitRate%",		PRE_APROX_BPS, nullptr},
	{"Audio: Bit rate mode",			  ft_string,			    "", "Audio;%BitRate_Mode%",		     PRE_NONE, nullptr},
	{"Audio: Channel(s)",				  ft_numeric_64,		    "", "Audio;%Channel(s)%",		     PRE_NONE, nullptr},
	{"Audio: Sampling rate, KHz",			  ft_numeric_floating,		    "", "Audio;%SamplingRate/String%",	     PRE_NONE, nullptr},
//	{"Audio: Sampling rate, KHz",			  ft_string,			    "", "Audio;%SamplingRate/String%",	     PRE_NONE, nullptr},
	{"Audio: Bit depth",				  ft_numeric_64,		    "", "Audio;%BitDepth%",		     PRE_NONE, nullptr},
	{"Audio: Delay in the stream",			  ft_numeric_64,		    "", "Audio;%Delaykfq%",		     PRE_NONE, nullptr},
	{"Audio: Title",				  ft_string,			    "", "Audio;%Title%",		     PRE_NONE, nullptr},
	{"Audio: Language",				  ft_string,			    "", "Audio;%Language%",		     PRE_NONE, nullptr},
};

MediaInfo gMI;
char gLastFile[PATH_MAX] = "";
locale_t gNumericC;

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

const wchar_t *toWChar(char *c_str)
{
	const size_t size = strlen(c_str) + 1;
	wchar_t *result = new wchar_t[size];
	mbstowcs(result, c_str, size);
	return result;
}

bool toFileTime(char *string, LPFILETIME FileTime)
{
	struct tm t{0};

	if (!strptime(string, "%Y-%m-%d %H:%M:%S", &t))
		return false;

	time_t unixtime = mktime(&t);

	if (unixtime == -1)
		return false;

	unixtime = (unixtime * 10000000) + 0x019DB1DED53E8000;
	FileTime->dwLowDateTime = (DWORD)unixtime;
	FileTime->dwHighDateTime = unixtime >> 32;
	return true;
}

int DCPCALL ContentGetSupportedField(int FieldIndex, char* FieldName, char* Units, int maxlen)
{
	if (FieldIndex < 0 || FieldIndex >= FIELDCOUNT)
		return ft_nomorefields;

	strlcpy(FieldName, gFields[FieldIndex].name, maxlen - 1);

	if (gFields[FieldIndex].pre == PRE_APROX_BPS)
	{
		string units;

		for (int i = 0; i < ARRAY_SIZE(gBpsChoices); i++)
		{
			if (i > 0)
				units.append("|");

			units.append(gBpsChoices[i].text);
		}

		strlcpy(Units, units.c_str(), maxlen - 1);
	}
	else
		strlcpy(Units, gFields[FieldIndex].units, maxlen - 1);

	return gFields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	double num;
	char *pos = nullptr;
	char *end = nullptr;

	if (strcmp(gLastFile, FileName) != 0)
	{
		for (int i = 0; i < FIELDCOUNT; i++)
		{
			free(gFields[i].value);
			gFields[i].value = nullptr;
		}

		gMI.Open(toWChar(FileName));

		for (int i = 0; i < FIELDCOUNT; i++)
		{
			gMI.Option(__T("Inform"),  toWChar((char*)gFields[i].info));
			String out = gMI.Inform();
			wstring ws(out);
			string str(ws.begin(), ws.end());
			gFields[i].value = strdup(str.c_str());

			char bps[42] = "";

			if (gFields[i].value[0] != '\0')
			{
				if (gFields[i].pre == PRE_FLOAT_BPS)
				{
					num = strtod(gFields[i].value, &end);
					free(gFields[i].value);
					gFields[i].value = nullptr;

					if (num >= ONE_GIGA)
						snprintf(bps, sizeof(bps), "%.1f GBps", num / ONE_GIGA);
					else if (num > ONE_MEGA)
						snprintf(bps, sizeof(bps), "%.1f MBps", num / ONE_MEGA);
					else if (num == ONE_MEGA)
						snprintf(bps, sizeof(bps), "1 MBps");
					else if (num >= ONE_KILO)
						snprintf(bps, sizeof(bps), "%.0f KBps", num / ONE_KILO);
					else
						snprintf(bps, sizeof(bps), "%.0f Bps", num);

					gFields[i].value = strdup(bps);
				}
				else if (gFields[i].pre == PRE_SHORT_TIME)
				{
					pos = strrchr(gFields[i].value, '.');

					if (pos)
						*pos = '\0';
				}
				else if (gFields[i].pre == PRE_APROX_BPS)
				{
					bool is_found = false;
					num = strtod(gFields[i].value, &end);
					free(gFields[i].value);
					gFields[i].value = nullptr;

					for (int u = 0; u < ARRAY_SIZE(gBpsChoices); u++)
					{
						if (num / gBpsChoices[u].unit > gBpsChoices[u].value - gBpsChoices[u].delta && \
						                num / gBpsChoices[u].unit < gBpsChoices[u].value + gBpsChoices[u].delta)
						{

							is_found = true;
							gFields[i].value = strdup(gBpsChoices[u].text);
							break;
						}
					}

					if (!is_found)
					{
						if (num >= ONE_GIGA)
							snprintf(bps, sizeof(bps), "%.1f GBps", num / ONE_GIGA);
						else if (num >= ONE_MEGA)
							snprintf(bps, sizeof(bps), "%.1f MBps", num / ONE_MEGA);
						else if (num >= ONE_KILO)
							snprintf(bps, sizeof(bps), "%.0f KBps", num / ONE_KILO);
						else
							snprintf(bps, sizeof(bps), "%.0f Bps", num);

						gFields[i].value = strdup(bps);
					}
				}
			}
		}

		strlcpy(gLastFile, FileName, PATH_MAX);
		gMI.Close();
	}

	if (!gFields[FieldIndex].value || gFields[FieldIndex].value[0] == '\0')
		return ft_fieldempty;

	ttimeformat time_val;

	switch (gFields[FieldIndex].type)
	{
	case ft_string:
	case ft_multiplechoice:
		strlcpy((char*)FieldValue, gFields[FieldIndex].value, maxlen - 1);
		break;

	case ft_numeric_32:
		if (gFields[FieldIndex].pre == PRE_TO_SEC)
		{
			num = strtod(gFields[FieldIndex].value, &end);
			*(int*)FieldValue = (int)floor(num / 1000);
		}
		else if (gFields[FieldIndex].pre == PRE_UNITS_BPS && UnitIndex > 0)
		{
			num = strtod(gFields[FieldIndex].value, &end);

			if (UnitIndex == 1)
				*(int *)FieldValue = (int)floor(num / ONE_KILO);
			else if (UnitIndex == 2)
				*(int *)FieldValue = (int)floor(num / ONE_MEGA);
		}
		else
			*(int*)FieldValue = atoi(gFields[FieldIndex].value);

		break;

	case ft_numeric_64:
		num = strtod_l(gFields[FieldIndex].value, &end, gNumericC);

		if (gFields[FieldIndex].pre == PRE_UNITS_BPS && UnitIndex > 0)
		{
			if (UnitIndex == 1)
				*(int64_t *)FieldValue = (int64_t)floor(num / ONE_KILO);
			else if (UnitIndex == 2)
				*(int64_t *)FieldValue = (int64_t)floor(num / ONE_MEGA);
		}
		else
			*(int64_t*)FieldValue = (int64_t)num;

		break;

	case ft_numeric_floating:
		num = strtod_l(gFields[FieldIndex].value, &end, gNumericC);

		if (gFields[FieldIndex].pre == PRE_UNITS_BPS && UnitIndex > 0)
		{
			if (UnitIndex == 1)
				*(double *)FieldValue = num / ONE_KILO;
			else if (UnitIndex == 2)
				*(double *)FieldValue = num / ONE_MEGA;
		}
		else
			*(double*)FieldValue = num;

		break;

	case ft_time:
		if (gFields[FieldIndex].pre == PRE_TO_SEC)
		{
			num = strtod(gFields[FieldIndex].value, &end);
			chrono::seconds sec((int)floor(num / 1000));
			time_val.wHour = chrono::duration_cast<chrono::hours>(sec).count();
			time_val.wMinute = chrono::duration_cast<chrono::minutes>(sec).count() % 60;
			time_val.wSecond = sec.count() % 60;
			memcpy(FieldValue, (const void*)&time_val, sizeof(time_val));
			break;
		}
		else if (gFields[FieldIndex].pre == PRE_SHORT_TIME)
		{
			char time_str[12];
			strlcpy(time_str, gFields[FieldIndex].value, sizeof(time_str));
			pos = strrchr(time_str, ':');

			if (pos)
			{
				time_val.wSecond = atoi(pos + 1);
				*pos = '\0';
				pos = strrchr(time_str, ':');

				if (pos)
				{
					time_val.wMinute = atoi(pos + 1);
					*pos = '\0';
					time_val.wHour = atoi(time_str);
					memcpy(FieldValue, (const void*)&time_val, sizeof(time_val));
					break;
				}
			}
		}

		return ft_fieldempty;

	case ft_datetime:
		if (!toFileTime(gFields[FieldIndex].value, (FILETIME*)FieldValue))
			return ft_fieldempty;

		break;

	default:
		return ft_fieldempty;
	}

	return gFields[FieldIndex].type;
}

void DCPCALL ContentSetDefaultParams(ContentDefaultParamStruct* dps)
{
	gNumericC = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
}

void DCPCALL ContentPluginUnloading(void)
{
	for (int i = 0; i < FIELDCOUNT; i++)
	{
		free(gFields[i].value);
		gFields[i].value = nullptr;
	}

	freelocale(gNumericC);
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	strlcpy(DetectString, DETECT_STRING, maxlen - 1);
	return 0;
}
