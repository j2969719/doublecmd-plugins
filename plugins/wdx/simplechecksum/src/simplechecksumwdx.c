#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#include <sys/stat.h>
#include <string.h>
#include "wdxplugin.h"

#define BUFF_SIZE 4096

typedef struct _field
{
	char *name;
	int type;
	int algo;
} FIELD;

#define fieldcount (sizeof(fields)/sizeof(FIELD))

FIELD fields[] =
{
	{"BLAKE2b 160",		ft_string,	321   /* GCRY_MD_BLAKE2B_160 */},
	{"BLAKE2b 256",		ft_string,	320   /* GCRY_MD_BLAKE2B_256 */},
	{"BLAKE2b 384",		ft_string,	319   /* GCRY_MD_BLAKE2B_384 */},
	{"BLAKE2b 512",		ft_string,	318   /* GCRY_MD_BLAKE2B_512 */},
	{"BLAKE2s 128",		ft_string,	325   /* GCRY_MD_BLAKE2S_128 */},
	{"BLAKE2s 160",		ft_string,	324   /* GCRY_MD_BLAKE2S_160 */},
	{"BLAKE2s 224",		ft_string,	323   /* GCRY_MD_BLAKE2S_224 */},
	{"BLAKE2s 256",		ft_string,	322   /* GCRY_MD_BLAKE2S_256 */},
	{"CRC32",		ft_string,	302	    /* GCRY_MD_CRC32 */},
	{"CRC24 RFC2440",	ft_string,	304 /* GCRY_MD_CRC24_RFC2440 */},
	{"CRC32 RFC1510",	ft_string,	303 /* GCRY_MD_CRC32_RFC1510 */},
	{"GOST R 34.11-94",	ft_string,	308  /* GCRY_MD_GOSTR3411_94 */},
	{"GOST R 34.11-CP",	ft_string,	311  /* GCRY_MD_GOSTR3411_CP */},
	{"HAVAL",		ft_string,	7	    /* GCRY_MD_HAVAL */},
	{"MD2",			ft_string,	5	      /* GCRY_MD_MD2 */},
	{"MD4",			ft_string,	301	      /* GCRY_MD_MD4 */},
	{"MD5",			ft_string,	1 	      /* GCRY_MD_MD5 */},
	{"RIPEMD-160",		ft_string,	3	   /* GCRY_MD_RMD160 */},
	{"SHA1",		ft_string,	2	     /* GCRY_MD_SHA1 */},
	{"SHA2 224",		ft_string,	11	   /* GCRY_MD_SHA224 */},
	{"SHA2 256",		ft_string,	8 	   /* GCRY_MD_SHA256 */},
	{"SHA2 384",		ft_string,	9	   /* GCRY_MD_SHA384 */},
	{"SHA2 512",		ft_string,	10	   /* GCRY_MD_SHA512 */},
	{"SHA2 512/224",	ft_string,	328    /* GCRY_MD_SHA512_224 */},
	{"SHA2 512/256",	ft_string,	327    /* GCRY_MD_SHA512_256 */},
	{"SHA3 224",		ft_string,	312	 /* GCRY_MD_SHA3_224 */},
	{"SHA3 256",		ft_string,	313	 /* GCRY_MD_SHA3_256 */},
	{"SHA3 384",		ft_string,	314	 /* GCRY_MD_SHA3_384 */},
	{"SHA3 512",		ft_string,	315	 /* GCRY_MD_SHA3_512 */},
//	{"SHAKE128",		ft_string,	316	 /* GCRY_MD_SHAKE128 */},
//	{"SHAKE256",		ft_string,	317	 /* GCRY_MD_SHAKE256 */},
	{"SM3",			ft_string,	326	      /* GCRY_MD_SM3 */},
	{"Stribog 256",		ft_string,	309    /* GCRY_MD_STRIBOG256 */},
	{"Stribog 512",		ft_string,	310    /* GCRY_MD_STRIBOG512 */},
	{"Tiger",		ft_string,	6	    /* GCRY_MD_TIGER */},
	{"Tiger1",		ft_string,	306	   /* GCRY_MD_TIGER1 */},
	{"Tiger2",		ft_string,	307	   /* GCRY_MD_TIGER2 */},
	{"Whirlpool",		ft_string,	305	/* GCRY_MD_WHIRLPOOL */},
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

	if (gcry_md_get_algo_dlen(fields[FieldIndex].algo) == 0)
		strlcpy(Units, "<not supported>", maxlen - 1);
	else
		strlcpy(Units, "Lowercase|Uppercase", maxlen - 1);

	return fields[FieldIndex].type;
}

int DCPCALL ContentGetValue(char* FileName, int FieldIndex, int UnitIndex, void* FieldValue, int maxlen, int flags)
{
	size_t i, bytes, res_size;
	unsigned char data[BUFF_SIZE];
	char* result = NULL;
	unsigned char* digest;
	unsigned int digestlen;
	struct stat buf;
	gcry_md_hd_t h;
	gcry_error_t err;

	if (FieldIndex < 0 || FieldIndex >= fieldcount)
		return ft_fieldempty;

	if (lstat(FileName, &buf) != 0)
		return ft_fileerror;

	if (!S_ISREG(buf.st_mode))
		return ft_fileerror;

	if (flags && CONTENT_DELAYIFSLOW)
	{
		strlcpy((char*)FieldValue, "???", maxlen - 1);
		return ft_delayed;
		//return ft_ondemand;
	}

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

	if (digestlen > 0)
	{
		res_size = (size_t)digestlen * sizeof(char) * 2 + 1;
		result = (char*)malloc(res_size);
	}

	if (result != NULL)
	{
		memset(result, 0, res_size);

		for (i = 0; i < digestlen; i++)
		{
			if (UnitIndex == 1)
				sprintf(&result[i * 2], "%02X", digest[i]);
			else
				sprintf(&result[i * 2], "%02x", digest[i]);
		}
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
