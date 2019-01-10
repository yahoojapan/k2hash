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
#include <nettle/md5.h>
#include <nettle/sha2.h>
#include <nettle/hmac.h>
#include <nettle/pbkdf2.h>
#include <nettle/aes.h>
#include <nettle/cbc.h>

#include "k2hcommon.h"
#include "k2hcryptcommon.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Structure for CBC
//---------------------------------------------------------
struct local_cbc_ctx	CBC_CTX(struct aes256_ctx, AES_BLOCK_SIZE);		// IV size is as same as AES_BLOCK_SIZE = K2H_ENCRYPT_IV_LENGTH(16)

//---------------------------------------------------------
// Get crypt library name
//---------------------------------------------------------
const char* k2h_crypt_lib_name(void)
{
	static const char name[] = "nettle";
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
	struct md5_ctx	ctx_md5;
	unsigned char	md5hex[MD5_DIGEST_SIZE];

	// md5
	md5_init(&ctx_md5);
	for(size_t length = strlen(str); !ISEMPTYSTR(str) && 0 < length; length -= min(length, K2H_CVT_MD_PARTSIZE), str = &str[min(length, K2H_CVT_MD_PARTSIZE)]){
		md5_update(&ctx_md5, min(length, K2H_CVT_MD_PARTSIZE), reinterpret_cast<const unsigned char*>(str));
	}
	md5_digest(&ctx_md5, MD5_DIGEST_SIZE, md5hex);

	// base64
	return to_base64(md5hex, MD5_DIGEST_SIZE);
}

//---------------------------------------------------------
// SHA256
//---------------------------------------------------------
string to_sha256_string(const unsigned char* bin, size_t length)
{
	struct sha256_ctx	ctx_sha256;
	unsigned char		buf[K2H_CVT_MD_PARTSIZE];
	unsigned int		digest_len	= SHA256_DIGEST_SIZE;
	unsigned char		sha256hex[SHA256_DIGEST_SIZE];

	memset(buf, 0, K2H_CVT_MD_PARTSIZE);

	// sha256
	sha256_init(&ctx_sha256);

	for(size_t onelength = 0; 0 < length; length -= onelength, bin = &bin[onelength]){
		onelength = min(length, K2H_CVT_MD_PARTSIZE);
		sha256_update(&ctx_sha256, onelength, bin);
		memset(buf, 0, K2H_CVT_MD_PARTSIZE);
	}
	sha256_digest(&ctx_sha256, digest_len, sha256hex);

	// base64
	return to_base64(sha256hex, static_cast<size_t>(digest_len));
}

//---------------------------------------------------------
// AES256 CBC PADDING by PBKDF1(PCKS#5 v1.5)
//---------------------------------------------------------
//
// CBC_ENCRYPT is not padding for CBC, then we padding as PCKS#7 here.
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
// [NOTE]
// CBC_DECRYPT is not padding for CBC, then we padding as PCKS#7 here.
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
// argument. nettle calculates Key and IV values as PCKS#5 v2(PBKDF2) and
// uses those randomly for AES256.
// However, K2HASH was originally implemented only in OpenSSL, using the
// EVP_BytesToKey function which is not recommended.
// That function calculates Key and IV value from salt and returns those values
// as PCKS#5 v1.5(PBKDF1). Therefore, we also use those Key and IV values
// calculated equivalently from salt for nettle and implement so that data created
// with K2HASH of OpenSSL version can be read out.
// Thus, this function mimics a part of OpenSSL EVP_BytesToKey(PBKDF1) for nettle.
// In the future, we will move to PCKS#5 v2(PBKDF2) soon.
// For future encryption, this function will no longer be used.(For decoding,
// it is used for compatibility, but it will not be supported for a long time.)
//
bool create_pbkdf1_key_iv(const unsigned char* pSalt, const unsigned char* pPass, size_t lenPass, unsigned int iteratecnt, unsigned char* pKey, unsigned int lenCipherKey, unsigned char* pIV, int lenIV)
{
	bool			result = false;
	struct md5_ctx	md5ctx;
	unsigned char	md5buf[MD5_DIGEST_SIZE * 4];					// == 64(for margin)
	unsigned int	md5len = MD5_DIGEST_SIZE;						// == 16
	bool			is_first_loop;
	unsigned int	cnt;

	if(!pSalt || !pPass || lenPass <= 0 || !pKey || lenCipherKey == 0 || !pIV || lenIV <= 0){
		ERR_K2HPRN("parameters are wrong.");
		return result;
	}

	for(result = true, is_first_loop = true; true; is_first_loop = false){
		md5_init(&md5ctx);

		if(!is_first_loop){
			md5_update(&md5ctx, md5len, &(md5buf[0]));
		}
		md5_update(&md5ctx, lenPass, pPass);
		md5_update(&md5ctx, K2H_ENCRYPT_SALT_LENGTH, pSalt);		// K2H_ENCRYPT_SALT_LENGTH = 8
		md5_digest(&md5ctx, md5len, md5buf);

		for(cnt = 1; cnt < iteratecnt; ++cnt){
			md5_update(&md5ctx, md5len, &(md5buf[0]));
			md5_digest(&md5ctx, md5len, md5buf);
		}
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

	struct local_cbc_ctx	aes256_cbc_ctx;
	unsigned char			key[AES256_KEY_SIZE];					// key size is 32
	unsigned char			iv[AES_BLOCK_SIZE];						// AES_BLOCK_SIZE = K2H_ENCRYPT_IV_LENGTH(16)
	unsigned char*			newdata;
	unsigned char*			encryptdata;
	unsigned char*			saltpos;
	unsigned char*			setdatapos;
	size_t					encbodylen = 0;

	// Do padding(PCKS#7)
	if(NULL == (newdata = k2h_pcks7_padding(orgdata, orglen, encbodylen, AES_BLOCK_SIZE))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// allocated for encrypted data area
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
	if(!create_pbkdf1_key_iv(saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, AES256_KEY_SIZE, iv, AES_BLOCK_SIZE)){
		ERR_K2HPRN("could not create IV.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}

	// Context
	aes256_set_encrypt_key(&(aes256_cbc_ctx.ctx), key);
	CBC_SET_IV(&aes256_cbc_ctx, iv);

	// do encrypt
	CBC_ENCRYPT(&aes256_cbc_ctx, aes256_encrypt, encbodylen, setdatapos, newdata);

	K2H_Free(newdata);

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

	struct local_cbc_ctx	aes256_cbc_ctx;
	unsigned char			key[AES256_KEY_SIZE];					// key size is 32
	unsigned char			iv[AES_BLOCK_SIZE];						// AES_BLOCK_SIZE = K2H_ENCRYPT_IV_LENGTH(16)
	unsigned char*			decryptdata;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH));
	int						decbodylen	= encbodylen;

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(decbodylen)))){	// decbodylen < encbodylen
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	//
	// generate PBKDF1 key and iv
	//
	if(!create_pbkdf1_key_iv(saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, AES256_KEY_SIZE, iv, AES_BLOCK_SIZE)){
		ERR_K2HPRN("could not create IV.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// Context
	aes256_set_decrypt_key(&(aes256_cbc_ctx.ctx), key);
	CBC_SET_IV(&aes256_cbc_ctx, iv);

	// do encrypt
	CBC_DECRYPT(&aes256_cbc_ctx, aes256_decrypt, decbodylen, decryptdata, encbodypos);

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

	struct hmac_sha512_ctx	sha512_ctx;
	struct local_cbc_ctx	aes256_cbc_ctx;
	unsigned char			key[AES256_KEY_SIZE];					// key size is 32
	unsigned char			iv[AES_BLOCK_SIZE];						// AES_BLOCK_SIZE = K2H_ENCRYPT_IV_LENGTH(16)
	unsigned char*			newdata;
	unsigned char*			encryptdata;
	unsigned char*			saltpos;
	unsigned char*			ivpos;
	unsigned char*			iterpos;
	unsigned char*			setdatapos;
	size_t					encbodylen = 0;

	// Do padding(PCKS#7)
	if(NULL == (newdata = k2h_pcks7_padding(orgdata, orglen, encbodylen, AES_BLOCK_SIZE))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// allocated for encrypted data area
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
	if(!k2h_generate_iv(iv, AES_BLOCK_SIZE)){						// always IV size is AES_BLOCK_SIZE = K2H_ENCRYPT_IV_LENGTH(16).
		ERR_K2HPRN("Could not make iv.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	memcpy(ivpos, iv, K2H_ENCRYPT_IV_LENGTH);						// must be K2H_ENCRYPT_IV_LENGTH = AES_BLOCK_SIZE(16)
	iterpos	= &ivpos[K2H_ENCRYPT_IV_LENGTH];

	// copy iter
	if(!k2h_copy_iteration_count(iterpos, iter, K2H_ENCRYPT_ITER_LENGTH)){
		ERR_K2HPRN("Could not save iteration count.");
		K2H_Free(encryptdata);
		K2H_Free(newdata);
		return NULL;
	}
	setdatapos	= &iterpos[K2H_ENCRYPT_ITER_LENGTH];

	// make key
	hmac_sha512_set_key(&sha512_ctx, strlen(pass), reinterpret_cast<const unsigned char*>(pass));
	PBKDF2(&sha512_ctx, hmac_sha512_update, hmac_sha512_digest, SHA512_DIGEST_SIZE, iter, K2H_ENCRYPT_SALT_LENGTH, saltpos, AES256_KEY_SIZE, key);

	// Context
	aes256_set_encrypt_key(&(aes256_cbc_ctx.ctx), key);
	CBC_SET_IV(&aes256_cbc_ctx, iv);

	// do encrypt
	CBC_ENCRYPT(&aes256_cbc_ctx, aes256_encrypt, encbodylen, setdatapos, newdata);

	K2H_Free(newdata);

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

	struct hmac_sha512_ctx	sha512_ctx;
	struct local_cbc_ctx	aes256_cbc_ctx;
	unsigned char			key[AES256_KEY_SIZE];					// key size is 32
	unsigned char*			decryptdata;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	iv			= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	const unsigned char*	iterpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH));
	int						decbodylen	= encbodylen;
	int						iter;

	// get iteration count
	if(-1 == (iter = k2h_get_iteration_count(iterpos))){
		ERR_K2HPRN("Could not get iteration count.");
		return NULL;
	}

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(encbodylen)))){	// decbodylen < encbodylen
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// make key
	hmac_sha512_set_key(&sha512_ctx, strlen(pass), reinterpret_cast<const unsigned char*>(pass));
	PBKDF2(&sha512_ctx, hmac_sha512_update, hmac_sha512_digest, SHA512_DIGEST_SIZE, iter, K2H_ENCRYPT_SALT_LENGTH, saltpos, AES256_KEY_SIZE, key);

	// Context
	aes256_set_decrypt_key(&(aes256_cbc_ctx.ctx), key);
	CBC_SET_IV(&aes256_cbc_ctx, iv);

	// do encrypt
	CBC_DECRYPT(&aes256_cbc_ctx, aes256_decrypt, decbodylen, decryptdata, encbodypos);

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
