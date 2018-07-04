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

#ifndef	K2HCRYPTCOMMON_H
#define	K2HCRYPTCOMMON_H

#include <time.h>
#include <string>

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	K2H_CVT_MD_PARTSIZE					static_cast<size_t>(512)

#define	K2H_PKCS5_SALT_LENGTH				8						// [NOTE] OpenSSL has this symbol as PKCS5_SALT_LEN, but others do not have this value.
#define K2H_ENCRYPT_SALT_PREFIX				"Salted__"				// prefix for salt
#define K2H_ENCRYPT_SALT_PREFIX_LENGTH		8						// = strlen(K2H_ENCRYPT_SALT_PREFIX)
#define K2H_ENCRYPT_SALT_LENGTH				K2H_PKCS5_SALT_LENGTH
#define	K2H_ENCRYPT_IV_LENGTH				16						// [NOTE] this value should be 16 byte for all crypt libraries
#define	K2H_ENCRYPT_ITER_LENGTH				16						//
#define K2H_ENCRYPT_MAX_PADDING_LENGTH		32						// max 31 bytes

// For PBKDF1
#define	K2H_ENCRYPTED_DATA_EX_LENGTH		(K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_MAX_PADDING_LENGTH)

// For PBKDF2
#define	K2H_ENCRYPTED_DATA_EX2_LENGTH		(K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH + K2H_ENCRYPT_MAX_PADDING_LENGTH)

//---------------------------------------------------------
// Functions
//---------------------------------------------------------
std::string k2h_get_uniqid_for_history(const struct timespec& rtime);
bool k2h_pkcs5_salt(unsigned char* salt, size_t length);
bool k2h_generate_iv(unsigned char* iv, size_t length);
bool k2h_copy_iteration_count(unsigned char* setpos, int iter, size_t count);
int k2h_get_iteration_count(const unsigned char* getpos);

const char* k2h_crypt_lib_name(void);
bool k2h_crypt_lib_initialize(void);
bool k2h_crypt_lib_terminate(void);
std::string to_base64(const unsigned char* data, size_t length);
std::string to_md5_string(const char* str);
std::string to_sha256_string(const unsigned char* bin, size_t length);
unsigned char* k2h_encrypt_aes256_cbc(const char* pass, const unsigned char* orgdata, size_t orglen, size_t& enclen);
unsigned char* k2h_decrypt_aes256_cbc(const char* pass, const unsigned char* encdata, size_t enclen, size_t& declen);
unsigned char* k2h_encrypt_aes256_cbc_pbkdf2(const char* pass, int iter, const unsigned char* orgdata, size_t orglen, size_t& enclen);
unsigned char* k2h_decrypt_aes256_cbc_pbkdf2(const char* pass, const unsigned char* encdata, size_t enclen, size_t& declen);

#endif	// K2HCRYPTCOMMON_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
