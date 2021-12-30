|ext|algo|comment|
|---|---|---|
|`blake2b_512`|`GCRY_MD_BLAKE2B_512`|or simply `blake2b` ext|
|`blake2b_384`|`GCRY_MD_BLAKE2B_384`||
|`blake2b_256`|`GCRY_MD_BLAKE2B_256`||
|`blake2b_160`|`GCRY_MD_BLAKE2B_160`||
|`blake2s_256`|`GCRY_MD_BLAKE2S_256`|or simply `blake2s` ext|
|`blake2s_224`|`GCRY_MD_BLAKE2S_224`||
|`blake2s_160`|`GCRY_MD_BLAKE2S_160`||
|`blake2s_128`|`GCRY_MD_BLAKE2S_128`||
|`crc24_rfc2440`|`GCRY_MD_CRC24_RFC2440`||
|`crc32`|`GCRY_MD_CRC32`||
|`crc32_rfc1510`|`GCRY_MD_CRC32_RFC1510`||
|`gostr3411_94`|`GCRY_MD_GOSTR3411_94`|GOST R 34.11-94.|
|`gostr3411_cp`|`GCRY_MD_GOSTR3411_CP`|GOST R 34.11-94 with CryptoPro-A S-Box.|
|`haval`|`GCRY_MD_HAVAL`|HAVAL, 5 pass, 160 bit|
|`md2`|`GCRY_MD_MD2`||
|`md4`|`GCRY_MD_MD4`||
|`md5`|`GCRY_MD_MD5`||
|`rmd160`|`GCRY_MD_RMD160`||
|`sha1`|`GCRY_MD_SHA1`||
|`sha224`|`GCRY_MD_SHA224`||
|`sha256`|`GCRY_MD_SHA256`||
|`sha384`|`GCRY_MD_SHA384`||
|`sha512`|`GCRY_MD_SHA512`||
|`sha512_224`|`GCRY_MD_SHA512_224`||
|`sha512_256`|`GCRY_MD_SHA512_256`||
|`sha3_224`|`GCRY_MD_SHA3_224`||
|`sha3_256`|`GCRY_MD_SHA3_256`||
|`sha3_384`|`GCRY_MD_SHA3_384`||
|`sha3_512`|`GCRY_MD_SHA3_512`||
|`shake128`|`GCRY_MD_SHAKE128`||
|`shake256`|`GCRY_MD_SHAKE256`||
|`sm3`|`GCRY_MD_SM3`||
|`stribog256`|`GCRY_MD_STRIBOG256`|GOST R 34.11-2012, 256 bit.|
|`stribog512`|`GCRY_MD_STRIBOG512`|GOST R 34.11-2012, 512 bit.|
|`tiger`|`GCRY_MD_TIGER`|TIGER/192 as used by gpg <= 1.3.2|
|`tiger1`|`GCRY_MD_TIGER1`|TIGER fixed.|
|`tiger2`|`GCRY_MD_TIGER2`|TIGER2 variant. |
|`whirlpool`|`GCRY_MD_WHIRLPOOL`||

**Note about minimal `libgcrypt` version:**
- BLAKE2: `libgcrypt` >= 1.8.0;
- MD2: `libgcrypt` >= 1.7.0;
- SHA-3 (digest sizes: 224, 256, 384 and 512): `libgcrypt` >= 1.7.0.

**Note:** If your Unix-like distribution has an old version of `libgcrypt`, you can compile this plugin without errors, but some hash algorithms from the list just won't work.
