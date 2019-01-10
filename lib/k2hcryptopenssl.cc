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
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include "k2hcommon.h"
#include "k2hcryptcommon.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Get crypt library name
//---------------------------------------------------------
const char* k2h_crypt_lib_name(void)
{
	static const char name[] = "OpenSSL";
	return name;
}

//---------------------------------------------------------
// Initialize/Terminate
//---------------------------------------------------------
bool k2h_crypt_lib_initialize(void)
{
	// [NOTE]
	// Now(after 1.1.0) it does not need to call OpenSSL_add_all_algorithms() etc.
	//
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
	MD5_CTX			md5ctx;
	unsigned char	md5hex[MD5_DIGEST_LENGTH];

	// md5
	MD5_Init(&md5ctx);
	for(size_t length = strlen(str); !ISEMPTYSTR(str) && 0 < length; length -= min(length, K2H_CVT_MD_PARTSIZE), str = &str[min(length, K2H_CVT_MD_PARTSIZE)]){
		MD5_Update(&md5ctx, str, min(length, K2H_CVT_MD_PARTSIZE));
	}
	MD5_Final(md5hex, &md5ctx);

	// base64
	return to_base64(md5hex, MD5_DIGEST_LENGTH);
}

//---------------------------------------------------------
// SHA256
//---------------------------------------------------------
string to_sha256_string(const unsigned char* bin, size_t length)
{
	EVP_MD_CTX*		sha256ctx	= EVP_MD_CTX_create();
	unsigned int	digest_len	= SHA256_DIGEST_LENGTH;
	unsigned char	sha256hex[SHA256_DIGEST_LENGTH];

	if(!sha256ctx){
		ERR_K2HPRN("Could not create context for sha256.");
		return string("");
	}
	EVP_MD_CTX_set_flags(sha256ctx, EVP_MD_CTX_FLAG_ONESHOT);

	// sha256
	if(1 != EVP_DigestInit_ex(sha256ctx, EVP_sha256(), NULL)){
		ERR_K2HPRN("Could not initialize context for sha256 digest.");
		EVP_MD_CTX_destroy(sha256ctx);
		return string("");
	}

	for(size_t onelength = 0; 0 < length; length -= onelength, bin = &bin[onelength]){
		onelength = min(length, K2H_CVT_MD_PARTSIZE);
		if(1 != EVP_DigestUpdate(sha256ctx, bin, onelength)){
			ERR_K2HPRN("Could not update context for sha256 digest.");
			EVP_MD_CTX_destroy(sha256ctx);
			return string("");
		}
	}
	if(1 != EVP_DigestFinal_ex(sha256ctx, sha256hex, &digest_len)){
		ERR_K2HPRN("Could not final context for sha256 digest.");
		EVP_MD_CTX_destroy(sha256ctx);
		return string("");
	}
	EVP_MD_CTX_destroy(sha256ctx);

	// base64
	return to_base64(sha256hex, static_cast<size_t>(digest_len));
}

//---------------------------------------------------------
// AES256 CBC PADDING by PBKDF1(PCKS#5 v1.5)
//---------------------------------------------------------
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

	EVP_CIPHER_CTX*		cictx;
	const EVP_CIPHER*	cipher = EVP_aes_256_cbc();
	unsigned char		key[EVP_MAX_KEY_LENGTH];
	unsigned char		iv[EVP_MAX_IV_LENGTH];
	unsigned char*		encryptdata;
	unsigned char*		saltpos;
	unsigned char*		setdatapos;
	int					encbodylen = 0;
	int					enclastlen = 0;

	// allocated for encrypted data area
	if(NULL == (encryptdata = reinterpret_cast<unsigned char*>(malloc(orglen + K2H_ENCRYPTED_DATA_EX_LENGTH)))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// copy salt prefix
	memcpy(encryptdata, salt_prefix, K2H_ENCRYPT_SALT_PREFIX_LENGTH);
	saltpos = &encryptdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];

	// make salt
	if(!k2h_pkcs5_salt(saltpos, K2H_ENCRYPT_SALT_LENGTH)){
		ERR_K2HPRN("Could not make salt.");
		K2H_Free(encryptdata);
		return NULL;
	}
	setdatapos = &saltpos[K2H_ENCRYPT_SALT_LENGTH];

	// make common encryption key(iteration count = 1)
	if(0 == EVP_BytesToKey(cipher, EVP_md5(), saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, iv)){
		ERR_K2HPRN("Failed to make AES256 key from pass.");
		K2H_Free(encryptdata);
		return NULL;
	}

	// initialize
	if(NULL == (cictx = EVP_CIPHER_CTX_new())){
		ERR_K2HPRN("Failed to make EVP cipher context.");
		K2H_Free(encryptdata);
		return NULL;
	}

	// initialize context
	if(1 != EVP_EncryptInit_ex(cictx, cipher, NULL, key, iv)){						// type is normally supplied by a function such as EVP_aes_256_cbc().
		ERR_K2HPRN("Could not initialize EVP context.");
		K2H_Free(encryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// do encrypt
	if(1 != EVP_EncryptUpdate(cictx, setdatapos, &encbodylen, orgdata, static_cast<int>(orglen))){
		ERR_K2HPRN("Failed to AES256 CBC encrypt.");
		K2H_Free(encryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// last encrypt
	if(1 != EVP_EncryptFinal_ex(cictx, &setdatapos[encbodylen], &enclastlen)){		// padding is on as default
		ERR_K2HPRN("Failed to AES256 CBC encrypt finally.");
		K2H_Free(encryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// destroy context
	EVP_CIPHER_CTX_free(cictx);

	// length
	enclen = static_cast<size_t>(K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + encbodylen + enclastlen);

	return encryptdata;
}

unsigned char* k2h_decrypt_aes256_cbc(const char* pass, const unsigned char* encdata, size_t enclen, size_t& declen)
{
	if(ISEMPTYSTR(pass) || !encdata || 0 == enclen){
		ERR_K2HPRN("parameters are wrong.");
		return NULL;
	}

	EVP_CIPHER_CTX*			cictx;
	const EVP_CIPHER*		cipher = EVP_aes_256_cbc();
	unsigned char			key[EVP_MAX_KEY_LENGTH];
	unsigned char			iv[EVP_MAX_IV_LENGTH];
	unsigned char*			decryptdata;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH));
	int						decbodylen	= 0;
	int						declastlen	= 0;

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(enclen)))){	// declen < enclen
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// make common encryption key(iteration count = 1)
	if(0 == EVP_BytesToKey(cipher, EVP_md5(), saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, iv)){
		ERR_K2HPRN("Failed to make AES256 key from pass.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// initialize
	if(NULL == (cictx = EVP_CIPHER_CTX_new())){
		ERR_K2HPRN("Failed to make EVP cipher context.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// initialize context
	if(1 != EVP_DecryptInit_ex(cictx, cipher, NULL, key, iv)){						// type is normally supplied by a function such as EVP_aes_256_cbc().
		ERR_K2HPRN("Could not initialize EVP context.");
		K2H_Free(decryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// do decrypt
	if(1 != EVP_DecryptUpdate(cictx, decryptdata, &decbodylen, encbodypos, encbodylen)){
		ERR_K2HPRN("Failed to AES256 CBC decrypt.");
		K2H_Free(decryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// last decrypt
	if(1 != EVP_DecryptFinal_ex(cictx, &decryptdata[decbodylen], &declastlen)){
		ERR_K2HPRN("Failed to AES256 CBC decrypt finally.");
		K2H_Free(decryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// destroy context
	EVP_CIPHER_CTX_free(cictx);

	// length
	declen = static_cast<size_t>(decbodylen + declastlen);

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

	EVP_CIPHER_CTX*		cictx;
	const EVP_CIPHER*	cipher = EVP_aes_256_cbc();
	unsigned char*		key;
	unsigned char		iv[EVP_MAX_IV_LENGTH];
	unsigned char*		encryptdata;
	unsigned char*		saltpos;
	unsigned char*		ivpos;
	unsigned char*		iterpos;
	unsigned char*		setdatapos;
	int					encbodylen = 0;
	int					enclastlen = 0;

	// allocated for encrypted data area
	if(NULL == (encryptdata = reinterpret_cast<unsigned char*>(malloc(orglen + K2H_ENCRYPTED_DATA_EX2_LENGTH)))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// copy salt prefix
	memcpy(encryptdata, salt_prefix, K2H_ENCRYPT_SALT_PREFIX_LENGTH);
	saltpos	= &encryptdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];

	// make salt
	if(!k2h_pkcs5_salt(saltpos, K2H_ENCRYPT_SALT_LENGTH)){
		ERR_K2HPRN("Could not make salt.");
		K2H_Free(encryptdata);
		return NULL;
	}
	ivpos	= &saltpos[K2H_ENCRYPT_SALT_LENGTH];

	// make iv and copy it
	if(!k2h_generate_iv(iv, EVP_MAX_IV_LENGTH)){						// EVP_CIPHER_iv_length(cipher) = EVP_MAX_IV_LENGTH
		ERR_K2HPRN("Could not make iv.");
		K2H_Free(encryptdata);
		return NULL;
	}
	memcpy(ivpos, iv, K2H_ENCRYPT_IV_LENGTH);							// must be K2H_ENCRYPT_IV_LENGTH = EVP_MAX_IV_LENGTH = 16
	iterpos	= &ivpos[K2H_ENCRYPT_IV_LENGTH];

	// copy iter
	if(!k2h_copy_iteration_count(iterpos, iter, K2H_ENCRYPT_ITER_LENGTH)){
		ERR_K2HPRN("Could not save iteration count.");
		K2H_Free(encryptdata);
		return NULL;
	}
	setdatapos	= &iterpos[K2H_ENCRYPT_ITER_LENGTH];

	// allocated for key
	if(NULL == (key = reinterpret_cast<unsigned char*>(malloc(EVP_CIPHER_key_length(cipher))))){	// <= EVP_MAX_KEY_LENGTH
		ERR_K2HPRN("Could not allocation memory.");
		K2H_Free(encryptdata);
		return NULL;
	}

	// create key from pass/salt/iteration
	//
	// Key length is specified by EVP_CIPHER_key_length(cipher)
	// Slat is MAX = 16, and this case is 16.
	//
	if(0 == PKCS5_PBKDF2_HMAC(pass, strlen(pass), saltpos, K2H_ENCRYPT_SALT_LENGTH, iter, EVP_sha512(), EVP_CIPHER_key_length(cipher), key)){
		ERR_K2HPRN("Failed to make PBKDF2 key from pass/salt/iteration.");
		K2H_Free(key);
		K2H_Free(encryptdata);
		return NULL;
	}

	// initialize
	if(NULL == (cictx = EVP_CIPHER_CTX_new())){
		ERR_K2HPRN("Failed to make EVP cipher context.");
		K2H_Free(key);
		K2H_Free(encryptdata);
		return NULL;
	}

	// initialize context
	if(1 != EVP_EncryptInit_ex(cictx, cipher, NULL, key, iv)){						// type is normally supplied by a function such as EVP_aes_256_cbc().
		ERR_K2HPRN("Could not initialize EVP context.");
		K2H_Free(key);
		K2H_Free(encryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// do encrypt
	if(1 != EVP_EncryptUpdate(cictx, setdatapos, &encbodylen, orgdata, static_cast<int>(orglen))){
		ERR_K2HPRN("Failed to AES256 CBC encrypt.");
		K2H_Free(key);
		K2H_Free(encryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}
	// last encrypt
	if(1 != EVP_EncryptFinal_ex(cictx, &setdatapos[encbodylen], &enclastlen)){		// padding is on as default
		ERR_K2HPRN("Failed to AES256 CBC encrypt finally.");
		K2H_Free(key);
		K2H_Free(encryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// free and destroy context
	K2H_Free(key);
	EVP_CIPHER_CTX_free(cictx);

	// length
	enclen = static_cast<size_t>(K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH + encbodylen + enclastlen);

	return encryptdata;
}

unsigned char* k2h_decrypt_aes256_cbc_pbkdf2(const char* pass, const unsigned char* encdata, size_t enclen, size_t& declen)
{
	if(ISEMPTYSTR(pass) || !encdata || 0 == enclen){
		ERR_K2HPRN("parameters are wrong.");
		return NULL;
	}

	EVP_CIPHER_CTX*			cictx;
	const EVP_CIPHER*		cipher = EVP_aes_256_cbc();
	unsigned char*			key;
	unsigned char*			decryptdata;
	int						iter;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	iv			= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	const unsigned char*	iterpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH));
	int						decbodylen	= 0;
	int						declastlen	= 0;

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(enclen)))){	// declen < enclen
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// get iteration count
	if(-1 == (iter = k2h_get_iteration_count(iterpos))){
		ERR_K2HPRN("Could not get iteration count.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// allocated for key
	if(NULL == (key = reinterpret_cast<unsigned char*>(malloc(EVP_CIPHER_key_length(cipher))))){	// <= EVP_MAX_KEY_LENGTH
		ERR_K2HPRN("Could not allocation memory.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// create key from pass/salt/iteration
	//
	// Key length is specified by EVP_CIPHER_key_length(cipher)
	// Slat is MAX = 16, and this case is 16.
	//
	if(0 == PKCS5_PBKDF2_HMAC(pass, strlen(pass), saltpos, K2H_ENCRYPT_SALT_LENGTH, iter, EVP_sha512(), EVP_CIPHER_key_length(cipher), key)){
		ERR_K2HPRN("Failed to make PBKDF2 key from pass/salt/iteration.");
		K2H_Free(key);
		K2H_Free(decryptdata);
		return NULL;
	}

	// initialize
	if(NULL == (cictx = EVP_CIPHER_CTX_new())){
		ERR_K2HPRN("Failed to make EVP cipher context.");
		K2H_Free(key);
		K2H_Free(decryptdata);
		return NULL;
	}

	// initialize context
	if(1 != EVP_DecryptInit_ex(cictx, cipher, NULL, key, iv)){						// type is normally supplied by a function such as EVP_aes_256_cbc().
		ERR_K2HPRN("Could not initialize EVP context.");
		K2H_Free(key);
		K2H_Free(decryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// do decrypt
	if(1 != EVP_DecryptUpdate(cictx, decryptdata, &decbodylen, encbodypos, encbodylen)){
		ERR_K2HPRN("Failed to AES256 CBC decrypt.");
		K2H_Free(key);
		K2H_Free(decryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}
	// last decrypt
	if(1 != EVP_DecryptFinal_ex(cictx, &decryptdata[decbodylen], &declastlen)){
		ERR_K2HPRN("Failed to AES256 CBC decrypt finally.");
		K2H_Free(key);
		K2H_Free(decryptdata);
		EVP_CIPHER_CTX_free(cictx);
		return NULL;
	}

	// free and destroy context
	K2H_Free(key);
	EVP_CIPHER_CTX_free(cictx);

	// length
	declen = static_cast<size_t>(decbodylen + declastlen);

	return decryptdata;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
