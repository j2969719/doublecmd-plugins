#include <stdio.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <zlib.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <string.h>
#include "wdxplugin.h"

#define _detectstring "EXT=\"*\""

typedef struct _field
{
	char *name;
	int type;
	char *unit;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"CRC32",		ft_string,		""},
	{"MD5",			ft_string,		""},
	{"SHA1",		ft_string,		""},
	{"SHA256",		ft_string,		""},
	{"SHA512",		ft_string,		""},
};

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
	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nomorefields;

	strncpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	strncpy(Units, fields[FieldIndex].unit, maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetDetectString(char* DetectString, int maxlen)
{
	strlcpy(DetectString, _detectstring, maxlen);
	return 0;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (flags && CONTENT_DELAYIFSLOW)
		return ft_delayed;

	int i, bytes;
	unsigned char data[1024];
	char result[128];
	struct stat buf;

	if (stat(FileName, &buf))
		return ft_fileerror;

	if (!S_ISREG(buf.st_mode))
		return ft_fileerror;

	FILE *inFile = fopen(FileName, "rb");

	if (inFile == NULL)
		return ft_fileerror;

	if (FieldIndex == 0)
	{
		uLong c = crc32(0L, Z_NULL, 0);

		while ((bytes = fread(data, 1, 1024, inFile)) != 0)
			c = crc32(c, data, bytes);

		sprintf(result, "%02lx", c);
	}
	else if (FieldIndex == 1)
	{
		unsigned char c[SHA256_DIGEST_LENGTH];
		MD5_CTX md5;
		MD5_Init(&md5);

		while ((bytes = fread(data, 1, 1024, inFile)) != 0)
			MD5_Update(&md5, data, bytes);

		MD5_Final(c, &md5);

		for (i = 0; i < MD5_DIGEST_LENGTH; i++)
			sprintf(&result[i * 2], "%02x", c[i]);
	}
	else if (FieldIndex == 2)
	{
		unsigned char c[SHA_DIGEST_LENGTH];
		SHA_CTX sha1;
		SHA1_Init(&sha1);

		while ((bytes = fread(data, 1, 1024, inFile)) != 0)
			SHA1_Update(&sha1, data, bytes);

		SHA1_Final(c, &sha1);

		for (i = 0; i < SHA_DIGEST_LENGTH; i++)
			sprintf(&result[i * 2], "%02x", c[i]);
	}
	else if (FieldIndex == 3)
	{
		unsigned char c[SHA256_DIGEST_LENGTH];
		SHA256_CTX sha256;
		SHA256_Init(&sha256);

		while ((bytes = fread(data, 1, 1024, inFile)) != 0)
			SHA256_Update(&sha256, data, bytes);

		SHA256_Final(c, &sha256);

		for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
			sprintf(&result[i * 2], "%02x", c[i]);
	}
	else if (FieldIndex == 4)
	{
		unsigned char c[SHA512_DIGEST_LENGTH];
		SHA512_CTX sha512;
		SHA512_Init(&sha512);

		while ((bytes = fread(data, 1, 1024, inFile)) != 0)
			SHA512_Update(&sha512, data, bytes);

		SHA512_Final(c, &sha512);

		for (i = 0; i < SHA512_DIGEST_LENGTH; i++)
			sprintf(&result[i * 2], "%02x", c[i]);
	}

	fclose(inFile);
	strncpy((char*)FieldValue, result, maxlen - 1);

	return fields[FieldIndex].type;

}
