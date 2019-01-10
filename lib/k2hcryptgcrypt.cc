/*
 * K2HASH
 *
 * Copyright 2018 Yahoo Japan Corporation.
 *
 * K2HASH is key-valuew store base libraries.
 * K2HASH is made for the purpose of the construction of
 * original KVS system and the offer of the library.
 * The characteristic is this KVS library which Key can
 * layer. And can support multi-processing and multi-thread,
 * and is provided safely as available KVS.
 *
 * For the full copyright and license information, please view
 * the license file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Mon May 7 2018
 * REVISION:
 *
 */

#include <string.h>
#include <gcrypt.h>

#include "k2hcommon.h"
#include "k2hcryptcommon.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	MD5_DIGEST_SIZE				16
#define SHA256_DIGEST_SIZE			32
#define	GCRY_PBKDF1_KEY_LENGTH		32
#define	GCRY_PBKDF2_KEY_LENGTH		32

//---------------------------------------------------------
// Get crypt library name
//---------------------------------------------------------
const char* k2h_crypt_lib_name(void)
{
	static const char name[] = "gcrypt";
	return name;
}

//---------------------------------------------------------
// Initialize/Terminate
//---------------------------------------------------------
bool k2h_crypt_lib_initialize(void)
{
	return true;
}

bool k2h_crypt_lib_terminate(void)
{
	return true;
}

//---------------------------------------------------------
// MD5
//---------------------------------------------------------
string to_md5_string(const char* str)
{
	gcry_md_hd_t	ctx_md5;
	gcry_error_t	err;
	unsigned char	md5hex[MD5_DIGEST_SIZE];

	// md5
	if(GPG_ERR_NO_ERROR != (err = gcry_md_open(&ctx_md5, GCRY_MD_MD5, 0))){
		ERR_K2HPRN("MD5 context creation failure: %s/%s", gcry_strsource(err), gcry_strerror(err));
		return string("");
	}

	for(size_t length = strlen(str); !ISEMPTYSTR(str) && 0 < length; length -= min(length, K2H_CVT_MD_PARTSIZE), str = &str[min(length, K2H_CVT_MD_PARTSIZE)]){
		gcry_md_write(ctx_md5, reinterpret_cast<const unsigned char*>(str), min(length, K2H_CVT_MD_PARTSIZE));
	}
	memcpy(md5hex, gcry_md_read(ctx_md5, 0), MD5_DIGEST_SIZE);
	gcry_md_close(ctx_md5);

	// base64
	return to_base64(md5hex, MD5_DIGEST_SIZE);
}

//---------------------------------------------------------
// SHA256
//---------------------------------------------------------
string to_sha256_string(const unsigned char* bin, size_t length)
{
	gcry_md_hd_t		ctx_sha256;
	gcry_error_t		err;
	unsigned char		sha256hex[SHA256_DIGEST_SIZE];

	// sha256
	if(GPG_ERR_NO_ERROR != (err = gcry_md_open(&ctx_sha256, GCRY_MD_SHA256, 0))){
		ERR_K2HPRN("SHA256 context creation failure: %s/%s", gcry_strsource(err), gcry_strerror(err));
		return string("");
	}

	for(size_t onelength = 0; 0 < length; length -= onelength, bin = &bin[onelength]){
		onelength = min(length, K2H_CVT_MD_PARTSIZE);
		gcry_md_write(ctx_sha256, bin, onelength);
	}
	memcpy(sha256hex, gcry_md_read(ctx_sha256, 0), SHA256_DIGEST_SIZE);
	gcry_md_close(ctx_sha256);

	// base64
	return to_base64(sha256hex, SHA256_DIGEST_SIZE);
}

//---------------------------------------------------------
// AES256 CBC PADDING by PBKDF1(PCKS#5 v1.5)
//---------------------------------------------------------
//
// gcry_cipher_encrypt is not padding for CBC, then we padding as PCKS#7 here.
//
static unsigned char* k2h_pcks7_padding(const unsigned char* orgdata, size_t orglen, size_t& padlen, size_t blocksize)
{
	// [NOTE]
	// If original length is as same as blocksize, adding blocksize.
	//
	padlen = ((orglen / blocksize) * blocksize) + blocksize;

	unsigned char	data	= static_cast<unsigned char>(padlen - orglen) & 0xff;
	unsigned char*	paddata;
	if(NULL == (paddata = reinterpret_cast<unsigned char*>(malloc(padlen)))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	memcpy(paddata, orgdata, orglen);
	for(size_t fill = 0; (fill + orglen) < padlen; ++fill){
		paddata[orglen + fill] = data;
	}
	return paddata;
}

//
// gcry_cipher_encrypt is not padding for CBC, then we padding as PCKS#7 here.
//
bool k2h_pcks7_remove_padding(const unsigned char* data, int& length)
{
	if(!data || length <= 0){
		ERR_K2HPRN("parameters are wrong.");
		return false;
	}

	unsigned char	lastdata = data[length - 1];
	if(length < static_cast<int>(lastdata)){
		ERR_K2HPRN("padding data length is over all data length.");
		return false;
	}
	length = length - static_cast<int>(lastdata);

	return true;
}

//
// Create Key and IV for PBKDF1(PCKS#5 v1.5)
//
// [NOTE]
// This function calculates and returns Key and IV value according to the
// argument. gcrypt calculates Key and IV values as PCKS#5 v2(PBKDF2) and
// uses those randomly for AES256.
// However, K2HASH was originally implemented only in OpenSSL, using the
// EVP_BytesToKey function which is not recommended.
// That function calculates Key and IV value from salt and returns those values
// as PCKS#5 v1.5(PBKDF1). Therefore, we also use those Key and IV values
// calculated equivalently from salt for gcrypt and implement so that data created
// with K2HASH of OpenSSL version can be read out.
// Thus, this function mimics a part of OpenSSL EVP_BytesToKey(PBKDF1) for gcrypt.
// In the future, we will move to PCKS#5 v2(PBKDF2) soon.
// For future encryption, this function will no longer be used.(For decoding,
// it is used for compatibility, but it will not be supported for a long time.)
//
// On gcrypt, we should use gcry_kdf_derive with GCRY_KDF_PBKDF1, but we need
// to generate IV and key from passphrase and salt. That function does not generate
// IV, then we use following function.
//
bool create_pbkdf1_key_iv(const unsigned char* pSalt, const unsigned char* pPass, size_t lenPass, unsigned int iteratecnt, unsigned char* pKey, unsigned int lenCipherKey, unsigned char* pIV, int lenIV)
{
	bool			result = false;
	gcry_md_hd_t	md5ctx;
	gcry_error_t	err;
	unsigned char	md5buf[MD5_DIGEST_SIZE * 4];									// == 64(for margin)
	unsigned int	md5len = MD5_DIGEST_SIZE;										// == 16
	bool			is_first_loop;
	unsigned int	cnt;

	if(!pSalt || !pPass || lenPass <= 0 || !pKey || lenCipherKey == 0 || !pIV || lenIV <= 0){
		ERR_K2HPRN("parameters are wrong.");
		return result;
	}

	for(result = true, is_first_loop = true; true; is_first_loop = false){
		if(GPG_ERR_NO_ERROR != (err = gcry_md_open(&md5ctx, GCRY_MD_MD5, 0))){		// MD5
			ERR_K2HPRN("MD5 context creation failure: %s/%s", gcry_strsource(err), gcry_strerror(err));
			result = false;
			break;
		}

		if(!is_first_loop){
			gcry_md_write(md5ctx, &(md5buf[0]), md5len);
		}
		gcry_md_write(md5ctx, pPass, lenPass);
		gcry_md_write(md5ctx, pSalt, K2H_ENCRYPT_SALT_LENGTH);						// K2H_ENCRYPT_SALT_LENGTH = 8
		memcpy(md5buf, gcry_md_read(md5ctx, 0), md5len);

		for(cnt = 1; cnt < iteratecnt; ++cnt){
			gcry_md_write(md5ctx, &(md5buf[0]), md5len);
			memcpy(md5buf, gcry_md_read(md5ctx, 0), md5len);
		}
		gcry_md_close(md5ctx);
		if(!result){
			break;
		}

		for(cnt = 0; 0 < lenCipherKey && cnt != md5len; --lenCipherKey, ++cnt){
			*(pKey++) = md5buf[cnt];
		}
		for(; 0 < lenIV && cnt != md5len; --lenIV, ++cnt){
			*(pIV++) = md5buf[cnt];
		}

		if(0 == lenCipherKey && 0 == lenIV){
			break;
		}
    }
	return result;
}

// These provide encryption of AES256 CBC PADDING.
// However, the common key is created with PBKDF1(PCKS#5 v1.5).
// These functions are the old type of encryption with K2HASH,
// and now the type of PBKDF2 is used.
//
// [DATA FORMAT]
// Encrypted binary data is formatted following:
//	"Salted__xxxxxxxxEEEE....EEEEPPPP..."
//
// SALT Prefix:		start with "Salted__" with salt(8byte)
// Encrypted data:	EEEE....EEEE, it is same original data length.
// Padding:			Encrypted data is 16byte block(AES256 CBC), then
//					max 31 bytes for padding.(== 32 bytes)
//
unsigned char* k2h_encrypt_aes256_cbc(const char* pass, const unsigned char* orgdata, size_t orglen, size_t& enclen)
{
	static const char	salt_prefix[] = K2H_ENCRYPT_SALT_PREFIX;

	if(ISEMPTYSTR(pass) || !orgdata || 0 == orglen){
		ERR_K2HPRN("parameters are wrong.");
		return NULL;
	}

	int					cipher		= GCRY_CIPHER_AES256;
	size_t				cipherBlock	= gcry_cipher_get_algo_blklen(cipher);
	int					keySize		= gcry_cipher_get_algo_keylen(cipher);
	gcry_cipher_hd_t	gcryCipherHd;
	gcry_error_t		gcryError;
	unsigned char		key[GCRY_PBKDF1_KEY_LENGTH];	// always key size is GCRY_PBKDF1_KEY_LENGTH(32) on this case
	unsigned char		iv[K2H_ENCRYPT_IV_LENGTH];		// always IV size is K2H_ENCRYPT_IV_LENGTH(16).
	unsigned char*		newdata;
	unsigned char*		encryptdata;
	unsigned char*		saltpos;
	unsigned char*		setdatapos;
	size_t				encbodylen;

	// check key size
	if(keySize <= 0 || GCRY_PBKDF1_KEY_LENGTH < keySize){
		ERR_K2HPRN("key size is wrong(%d)", keySize);
		return NULL;
	}

	// Do padding(PCKS#7)
	if(NULL == (newdata = k2h_pcks7_padding(orgdata, orglen, encbodylen, cipherBlock))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// allocated for encrypted data area(size is after padding)
	if(NULL == (encryptdata = reinterpret_cast<unsigned char*>(malloc(encbodylen + K2H_ENCRYPTED_DATA_EX_LENGTH)))){
		ERR_K2HPRN("Could not allocation memory.");
		K2H_Free(newdata);
		return NULL;
	}

	// copy salt prefix
	memcpy(encryptdata, salt_prefix, K2H_ENCRYPT_SALT_PREFIX_LENGTH);
	saltpos = &encryptdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];

	// make salt
	if(!k2h_pkcs5_salt(saltpos, K2H_ENCRYPT_SALT_LENGTH)){
		ERR_K2HPRN("Could not make salt.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	setdatapos = &saltpos[K2H_ENCRYPT_SALT_LENGTH];

	//
	// generate PBKDF1 key and iv
	//
	if(!create_pbkdf1_key_iv(saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, keySize, iv, K2H_ENCRYPT_IV_LENGTH)){
		ERR_K2HPRN("could not create IV.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}

	// initialize
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_open(&gcryCipherHd, cipher, GCRY_CIPHER_MODE_CBC, 0))){
		ERR_K2HPRN("Failed to open gcrypt cipher by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
    }

	// set symmetric key
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setkey(gcryCipherHd, key, GCRY_PBKDF2_KEY_LENGTH))){
		ERR_K2HPRN("Failed to set symmetric key by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
    }

	// set IV
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setiv(gcryCipherHd, iv, K2H_ENCRYPT_IV_LENGTH))){
		ERR_K2HPRN("Failed to set IV by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}

	// do encrypt
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_encrypt(gcryCipherHd, setdatapos, static_cast<int>(encbodylen), newdata, static_cast<int>(encbodylen)))){
		ERR_K2HPRN("Failed to AES256 CBC encrypt by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	K2H_Free(newdata);

	// destroy context
	gcry_cipher_close(gcryCipherHd);

	// length
	enclen = static_cast<size_t>(K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + encbodylen);

	return encryptdata;
}

unsigned char* k2h_decrypt_aes256_cbc(const char* pass, const unsigned char* encdata, size_t enclen, size_t& declen)
{
	if(ISEMPTYSTR(pass) || !encdata || 0 == enclen){
		ERR_K2HPRN("parameters are wrong.");
		return NULL;
	}

	int						cipher		= GCRY_CIPHER_AES256;
	int						keySize		= gcry_cipher_get_algo_keylen(cipher);
	gcry_cipher_hd_t		gcryCipherHd;
	gcry_error_t			gcryError;
	unsigned char			key[GCRY_PBKDF1_KEY_LENGTH];	// always key size is GCRY_PBKDF1_KEY_LENGTH(32) on this case
	unsigned char			iv[K2H_ENCRYPT_IV_LENGTH];		// always IV size is K2H_ENCRYPT_IV_LENGTH(16).
	unsigned char*			decryptdata;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH));
	int						decbodylen	= encbodylen;

	// check key size
	if(keySize <= 0 || GCRY_PBKDF1_KEY_LENGTH < keySize){
		ERR_K2HPRN("key size is wrong(%d)", keySize);
		return NULL;
	}

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(decbodylen)))){	// decbodylen < encbodylen
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	//
	// generate PBKDF1 key and iv
	//
	if(!create_pbkdf1_key_iv(saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, keySize, iv, K2H_ENCRYPT_IV_LENGTH)){
		ERR_K2HPRN("could not create IV.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// initialize
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_open(&gcryCipherHd, cipher, GCRY_CIPHER_MODE_CBC, 0))){
		ERR_K2HPRN("Failed to open gcrypt cipher by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		K2H_Free(decryptdata);
		return NULL;
    }

	// set symmetric key
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setkey(gcryCipherHd, key, GCRY_PBKDF2_KEY_LENGTH))){
		ERR_K2HPRN("Failed to set symmetric key by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(decryptdata);
		return NULL;
    }

	// set IV
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setiv(gcryCipherHd, iv, K2H_ENCRYPT_IV_LENGTH))){
		ERR_K2HPRN("Failed to set IV by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(decryptdata);
		return NULL;
	}

	// do decrypt
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_decrypt(gcryCipherHd, decryptdata, decbodylen, encbodypos, encbodylen))){
		ERR_K2HPRN("Failed to AES256 CBC decrypt by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(decryptdata);
		return NULL;
	}

	// destroy context
	gcry_cipher_close(gcryCipherHd);

	// remove padding data
	if(!k2h_pcks7_remove_padding(decryptdata, decbodylen)){
		K2H_Free(decryptdata);
		return NULL;
	}

	// length
	declen = static_cast<size_t>(decbodylen);

	return decryptdata;
}

//---------------------------------------------------------
// AES256 CBC PADDING by PBKDF2(PCKS#5 v2)
//---------------------------------------------------------
// These provide encryption of AES256 CBC PADDING and the common
// key is created with PBKDF2(PCKS#5 v2).
//
// [DATA FORMAT]
// Encrypted binary data is formatted following:
//	"Salted__xxxxxxxxyyyyyyyyyyyyyyyyzzzzzzzzzzzzzzzzEEEE....EEEEPPPP..."
//
// SALT Prefix:		start with "Salted__" with salt(8byte)
// IV value:        initialize vector - IV(16byte)
// Iteration:       iteration count value(16byte)
// Encrypted data:	EEEE....EEEE, it is same original data length.
// Padding:			Encrypted data is 16byte block(AES256 CBC), then
//					max 31 bytes for padding.(== 32 bytes)
//
unsigned char* k2h_encrypt_aes256_cbc_pbkdf2(const char* pass, int iter, const unsigned char* orgdata, size_t orglen, size_t& enclen)
{
	static const char	salt_prefix[] = K2H_ENCRYPT_SALT_PREFIX;

	if(ISEMPTYSTR(pass) || iter < 1 || !orgdata || 0 == orglen){
		ERR_K2HPRN("parameters are wrong.");
		return NULL;
	}

	int					cipher		= GCRY_CIPHER_AES256;
	size_t				cipherBlock	= gcry_cipher_get_algo_blklen(cipher);
	int					keySize		= gcry_cipher_get_algo_keylen(cipher);
	gcry_cipher_hd_t	gcryCipherHd;
	gcry_error_t		gcryError;
	unsigned char*		newdata;
	unsigned char		key[GCRY_PBKDF2_KEY_LENGTH];
	unsigned char		iv[K2H_ENCRYPT_IV_LENGTH];
	unsigned char*		encryptdata;
	unsigned char*		saltpos;
	unsigned char*		ivpos;
	unsigned char*		iterpos;
	unsigned char*		setdatapos;
	size_t				encbodylen;

	// check key size
	if(keySize <= 0 || GCRY_PBKDF2_KEY_LENGTH < keySize){
		ERR_K2HPRN("key size is wrong(%d)", keySize);
		return NULL;
	}

	// Do padding(PCKS#7)
	if(NULL == (newdata = k2h_pcks7_padding(orgdata, orglen, encbodylen, cipherBlock))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// allocated for encrypted data area(size is after padding)
	if(NULL == (encryptdata = reinterpret_cast<unsigned char*>(malloc(encbodylen + K2H_ENCRYPTED_DATA_EX2_LENGTH)))){
		ERR_K2HPRN("Could not allocation memory.");
		K2H_Free(newdata);
		return NULL;
	}

	// copy salt prefix
	memcpy(encryptdata, salt_prefix, K2H_ENCRYPT_SALT_PREFIX_LENGTH);
	saltpos = &encryptdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];

	// make salt
	if(!k2h_pkcs5_salt(saltpos, K2H_ENCRYPT_SALT_LENGTH)){
		ERR_K2HPRN("Could not make salt.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	ivpos	= &saltpos[K2H_ENCRYPT_SALT_LENGTH];

	// make iv and copy it
	if(!k2h_generate_iv(iv, K2H_ENCRYPT_IV_LENGTH)){					// always IV size is K2H_ENCRYPT_IV_LENGTH(16).
		ERR_K2HPRN("Could not make iv.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	memcpy(ivpos, iv, K2H_ENCRYPT_IV_LENGTH);							// must be K2H_ENCRYPT_IV_LENGTH = 16
	iterpos	= &ivpos[K2H_ENCRYPT_IV_LENGTH];

	// copy iter
	if(!k2h_copy_iteration_count(iterpos, iter, K2H_ENCRYPT_ITER_LENGTH)){
		ERR_K2HPRN("Could not save iteration count.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	setdatapos	= &iterpos[K2H_ENCRYPT_ITER_LENGTH];

	// create key from pass/salt/iteration
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_kdf_derive(pass, strlen(pass), GCRY_KDF_PBKDF2, GCRY_MD_SHA512, saltpos, K2H_ENCRYPT_SALT_LENGTH, iter, keySize, key))){
		ERR_K2HPRN("Failed to make PKBDF2 key from pass/salt/iteration by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}

	// initialize
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_open(&gcryCipherHd, cipher, GCRY_CIPHER_MODE_CBC, 0))){
		ERR_K2HPRN("Failed to open gcrypt cipher by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
    }

	// set symmetric key
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setkey(gcryCipherHd, key, keySize))){
		ERR_K2HPRN("Failed to set symmetric key by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
    }

	// set IV
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setiv(gcryCipherHd, iv, K2H_ENCRYPT_IV_LENGTH))){
		ERR_K2HPRN("Failed to set IV by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}

	// do encrypt
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_encrypt(gcryCipherHd, setdatapos, static_cast<int>(encbodylen), newdata, static_cast<int>(encbodylen)))){
		ERR_K2HPRN("Failed to AES256 CBC encrypt by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	K2H_Free(newdata);

	// destroy context
	gcry_cipher_close(gcryCipherHd);

	// length
	enclen = static_cast<size_t>(K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH + encbodylen);

	return encryptdata;
}

unsigned char* k2h_decrypt_aes256_cbc_pbkdf2(const char* pass, const unsigned char* encdata, size_t enclen, size_t& declen)
{
	if(ISEMPTYSTR(pass) || !encdata || 0 == enclen){
		ERR_K2HPRN("parameters are wrong.");
		return NULL;
	}

	int						cipher		= GCRY_CIPHER_AES256;
	int						keySize		= gcry_cipher_get_algo_keylen(cipher);
	gcry_cipher_hd_t		gcryCipherHd;
	gcry_error_t			gcryError;
	unsigned char			key[GCRY_PBKDF2_KEY_LENGTH];
	unsigned char*			decryptdata;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	iv			= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	const unsigned char*	iterpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH));
	int						decbodylen	= encbodylen;
	int						iter;

	// check key size
	if(keySize <= 0 || GCRY_PBKDF2_KEY_LENGTH < keySize){
		ERR_K2HPRN("key size is wrong(%d)", keySize);
		return NULL;
	}

	// get iteration count
	if(-1 == (iter = k2h_get_iteration_count(iterpos))){
		ERR_K2HPRN("Could not get iteration count.");
		return NULL;
	}

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(decbodylen)))){	// decbodylen < encbodylen
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// create key from pass/salt/iteration
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_kdf_derive(pass, strlen(pass), GCRY_KDF_PBKDF2, GCRY_MD_SHA512, saltpos, K2H_ENCRYPT_SALT_LENGTH, iter, keySize, key))){
		ERR_K2HPRN("Failed to make PKBDF2 key from pass/salt/iteration by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		K2H_Free(decryptdata);
		return NULL;
	}

	// initialize
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_open(&gcryCipherHd, cipher, GCRY_CIPHER_MODE_CBC, 0))){
		ERR_K2HPRN("Failed to open gcrypt cipher by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		K2H_Free(decryptdata);
		return NULL;
    }

	// set symmetric key
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setkey(gcryCipherHd, key, keySize))){
		ERR_K2HPRN("Failed to set symmetric key by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(decryptdata);
		return NULL;
    }

	// set IV
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_setiv(gcryCipherHd, iv, K2H_ENCRYPT_IV_LENGTH))){
		ERR_K2HPRN("Failed to set IV by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(decryptdata);
		return NULL;
	}

	// do decrypt
	if(GPG_ERR_NO_ERROR != (gcryError = gcry_cipher_decrypt(gcryCipherHd, decryptdata, decbodylen, encbodypos, encbodylen))){
		ERR_K2HPRN("Failed to AES256 CBC decrypt by %s/%s", gcry_strsource(gcryError), gcry_strerror(gcryError));
		gcry_cipher_close(gcryCipherHd);
		K2H_Free(decryptdata);
		return NULL;
	}

	// destroy context
	gcry_cipher_close(gcryCipherHd);

	// remove padding data
	if(!k2h_pcks7_remove_padding(decryptdata, decbodylen)){
		K2H_Free(decryptdata);
		return NULL;
	}

	// length
	declen = static_cast<size_t>(decbodylen);

	return decryptdata;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
