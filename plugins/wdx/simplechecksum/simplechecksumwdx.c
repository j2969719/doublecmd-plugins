#include <stdio.h>
#include <gcrypt.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <string.h>
#include "wdxplugin.h"

#define BUFF_SIZE 1024

typedef struct _field
{
	char *name;
	int type;
	int algo;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"CRC32",		ft_string,		GCRY_MD_CRC32},
	{"MD5",			ft_string,		  GCRY_MD_MD5},
	{"SHA1",		ft_string,		 GCRY_MD_SHA1},
	{"SHA256",		ft_string,	       GCRY_MD_SHA256},
	{"SHA512",		ft_string,	       GCRY_MD_SHA512},
	{"BLAKE2B 256",		ft_string,	  GCRY_MD_BLAKE2B_256},
	{"BLAKE2B 512",		ft_string,	  GCRY_MD_BLAKE2B_512},
	{"BLAKE2B 384",		ft_string,	  GCRY_MD_BLAKE2B_384},
	{"BLAKE2B 160",		ft_string,	  GCRY_MD_BLAKE2B_160},
	{"BLAKE2S 256",		ft_string,	  GCRY_MD_BLAKE2S_256},
	{"BLAKE2S 224",		ft_string,	  GCRY_MD_BLAKE2S_224},
	{"BLAKE2S 160",		ft_string,	  GCRY_MD_BLAKE2S_160},
	{"BLAKE2S 128",		ft_string,	  GCRY_MD_BLAKE2S_128},
	{"SHA384",		ft_string,	       GCRY_MD_SHA384},
	{"SHA224",		ft_string,	       GCRY_MD_SHA224},
	{"RMD160",		ft_string,	       GCRY_MD_RMD160},
	{"MD2",			ft_string,		  GCRY_MD_MD2},
	{"MD4",			ft_string,		  GCRY_MD_MD4},
	{"TIGER",		ft_string,		GCRY_MD_TIGER},
	{"HAVAL",		ft_string,		GCRY_MD_HAVAL},
	{"CRC32 RFC1510",	ft_string,	GCRY_MD_CRC32_RFC1510},
	{"CRC24 RFC2440",	ft_string,	GCRY_MD_CRC24_RFC2440},
	{"TIGER1",		ft_string,	       GCRY_MD_TIGER1},
	{"TIGER2",		ft_string,	       GCRY_MD_TIGER2},
	{"STRIBOG256",		ft_string,	   GCRY_MD_STRIBOG256},
	{"STRIBOG512",		ft_string,	   GCRY_MD_STRIBOG512},
	{"GOSTR3411_CP",	ft_string,	 GCRY_MD_GOSTR3411_CP},
	{"GOSTR3411_94 ",	ft_string,	 GCRY_MD_GOSTR3411_94},
	{"WHIRLPOOL",		ft_string,	    GCRY_MD_WHIRLPOOL},
	{"SHA3 224",		ft_string,	     GCRY_MD_SHA3_224},
	{"SHA3 256",		ft_string,	     GCRY_MD_SHA3_256},
	{"SHA3 384",		ft_string,	     GCRY_MD_SHA3_384},
	{"SHA3 512",		ft_string,	     GCRY_MD_SHA3_512},
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

	strlcpy(FieldName, fields[FieldIndex].name, maxlen - 1);
	strlcpy(Units, "", maxlen - 1);
	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	if (flags && CONTENT_DELAYIFSLOW)
		return ft_delayed;

	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_nosuchfield;

	size_t i, bytes;
	unsigned char data[BUFF_SIZE];
	char* result = NULL;
	unsigned char* digest;
	unsigned int digestlen;
	struct stat buf;
	gcry_md_hd_t h;
	gcry_error_t err;

	if (stat(FileName, &buf))
		return ft_fileerror;

	if (!S_ISREG(buf.st_mode))
		return ft_fileerror;

	FILE *inFile = fopen(FileName, "rb");

	if (inFile == NULL)
		return ft_fileerror;

	err = gcry_md_open(&h, fields[FieldIndex].algo, 0);

	if (gcry_err_code(err))
	{
		fclose(inFile);
		return ft_fieldempty;
	}

	while ((bytes = fread(data, 1, BUFF_SIZE, inFile)) != 0)
		gcry_md_write(h, data, bytes);

	digest = gcry_md_read(h, fields[FieldIndex].algo);
	digestlen = gcry_md_get_algo_dlen(fields[FieldIndex].algo);

	size_t res_size = (size_t)digestlen * sizeof(char) * 2 + 1;
	result = (char*)malloc(res_size);

	if (result != NULL)
	{
		memset(result, 0, res_size);

		for (i = 0; i < digestlen; i++)
			sprintf(&result[i * 2], "%02x", digest[i]);
	}

	fclose(inFile);
	gcry_md_close(h);

	if (result != NULL)
	{
		strlcpy((char*)FieldValue, result, maxlen - 1);
		free(result);
	}
	else
		return ft_fieldempty;

	return fields[FieldIndex].type;
}
