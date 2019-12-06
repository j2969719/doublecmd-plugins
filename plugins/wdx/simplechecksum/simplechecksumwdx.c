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
	{"BLAKE2b 160",		ft_string,	  GCRY_MD_BLAKE2B_160},
	{"BLAKE2b 256",		ft_string,	  GCRY_MD_BLAKE2B_256},
	{"BLAKE2b 384",		ft_string,	  GCRY_MD_BLAKE2B_384},
	{"BLAKE2b 512",		ft_string,	  GCRY_MD_BLAKE2B_512},
	{"BLAKE2s 128",		ft_string,	  GCRY_MD_BLAKE2S_128},
	{"BLAKE2s 160",		ft_string,	  GCRY_MD_BLAKE2S_160},
	{"BLAKE2s 224",		ft_string,	  GCRY_MD_BLAKE2S_224},
	{"BLAKE2s 256",		ft_string,	  GCRY_MD_BLAKE2S_256},
	{"CRC32",		ft_string,		GCRY_MD_CRC32},
	{"CRC24 RFC2440",	ft_string,	GCRY_MD_CRC24_RFC2440},
	{"CRC32 RFC1510",	ft_string,	GCRY_MD_CRC32_RFC1510},
	{"GOST R 34.11-94 ",	ft_string,	 GCRY_MD_GOSTR3411_94},
	{"GOST R 34.11-CP",	ft_string,	 GCRY_MD_GOSTR3411_CP},
	{"HAVAL",		ft_string,		GCRY_MD_HAVAL},
	{"MD2",			ft_string,		  GCRY_MD_MD2},
	{"MD4",			ft_string,		  GCRY_MD_MD4},
	{"MD5",			ft_string,		  GCRY_MD_MD5},
	{"RIPEMD-160",		ft_string,	       GCRY_MD_RMD160},
	{"SHA1",		ft_string,		 GCRY_MD_SHA1},
	{"SHA2 224",		ft_string,	       GCRY_MD_SHA224},
	{"SHA2 256",		ft_string,	       GCRY_MD_SHA256},
	{"SHA2 384",		ft_string,	       GCRY_MD_SHA384},
	{"SHA2 512",		ft_string,	       GCRY_MD_SHA512},
	{"SHA3 224",		ft_string,	     GCRY_MD_SHA3_224},
	{"SHA3 256",		ft_string,	     GCRY_MD_SHA3_256},
	{"SHA3 384",		ft_string,	     GCRY_MD_SHA3_384},
	{"SHA3 512",		ft_string,	     GCRY_MD_SHA3_512},
	{"Stribog 256",		ft_string,	   GCRY_MD_STRIBOG256},
	{"Stribog 512",		ft_string,	   GCRY_MD_STRIBOG512},
	{"Tiger",		ft_string,		GCRY_MD_TIGER},
	{"Tiger1",		ft_string,	       GCRY_MD_TIGER1},
	{"Tiger2",		ft_string,	       GCRY_MD_TIGER2},
	{"Whirlpool",		ft_string,	    GCRY_MD_WHIRLPOOL},
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
