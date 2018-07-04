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
#include <nss.h>
#include <nss3/pk11pub.h>
#include <nss3/hasht.h>
#include <nss3/blapit.h>
#include <nspr4/prinit.h>
#include <nspr4/prerror.h>

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
	static const char name[] = "NSS";
	return name;
}

//---------------------------------------------------------
// Initialize/Terminate
//---------------------------------------------------------
bool k2h_crypt_lib_initialize(void)
{
	if(NSS_IsInitialized()){
		WAN_K2HPRN("Already initialized NSS");
		return false;
	}
	if(SECSuccess != NSS_NoDB_Init(NULL)){
		ERR_K2HPRN("Failed NSS_NoDB_Init");
		return false;
	}
	return true;
}

bool k2h_crypt_lib_terminate(void)
{
	if(SECSuccess != NSS_Shutdown()){
		ERR_K2HPRN("Failed NSS_Shutdown, probabry anyone handles NSS context or objects.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// MD5
//---------------------------------------------------------
string to_md5_string(const char* str)
{
	PK11Context*	md5ctx;
	unsigned char	md5hex[MD5_LENGTH];
	unsigned int	md5outlen;

	// md5
	if(NULL == (md5ctx = PK11_CreateDigestContext(SEC_OID_MD5))){
		ERR_K2HPRN("Could not create context for md5.");
		return string("");
	}

	if(SECSuccess != PK11_DigestBegin(md5ctx)){
		ERR_K2HPRN("Could not begin digest for MD5.");
		PK11_DestroyContext(md5ctx, PR_TRUE);
		return string("");
	}
	for(size_t length = strlen(str); !ISEMPTYSTR(str) && 0 < length; length -= min(length, K2H_CVT_MD_PARTSIZE), str = &str[min(length, K2H_CVT_MD_PARTSIZE)]){
		PK11_DigestOp(md5ctx, reinterpret_cast<const unsigned char*>(str), min(length, K2H_CVT_MD_PARTSIZE));
	}
	PK11_DigestFinal(md5ctx, md5hex, &md5outlen, MD5_LENGTH);
	PK11_DestroyContext(md5ctx, PR_TRUE);

	// base64
	return to_base64(md5hex, MD5_LENGTH);
}

//---------------------------------------------------------
// SHA256
//---------------------------------------------------------
string to_sha256_string(const unsigned char* bin, size_t length)
{
	PK11Context*	sha256ctx;
	unsigned int	sha256outlen;
	unsigned int	digest_len	= SHA256_LENGTH;
	unsigned char	sha256hex[SHA256_LENGTH];

	// sha256
	if(NULL == (sha256ctx = PK11_CreateDigestContext(SEC_OID_SHA256))){
		ERR_K2HPRN("Could not create context for sha256.");
		return string("");
	}

	if(SECSuccess != PK11_DigestBegin(sha256ctx)){
		ERR_K2HPRN("Could not begin digest for sha256.");
		PK11_DestroyContext(sha256ctx, PR_TRUE);
		return string("");
	}
	for(size_t onelength = 0; 0 < length; length -= onelength, bin = &bin[onelength]){
		onelength = min(length, K2H_CVT_MD_PARTSIZE);
		if(SECSuccess != PK11_DigestOp(sha256ctx, bin, onelength)){
			ERR_K2HPRN("Could not update context for sha256 digest.");
			PK11_DestroyContext(sha256ctx, PR_TRUE);
			return string("");
		}
	}
	if(SECSuccess != PK11_DigestFinal(sha256ctx, sha256hex, &sha256outlen, digest_len)){
		ERR_K2HPRN("Could not final context for sha256 digest.");
		PK11_DestroyContext(sha256ctx, PR_TRUE);
		return string("");
	}

	PK11_DestroyContext(sha256ctx, PR_TRUE);

	// base64
	return to_base64(sha256hex, static_cast<size_t>(digest_len));
}

//---------------------------------------------------------
// AES256 CBC PADDING by PBKDF1(PCKS#5 v1.5)
//---------------------------------------------------------
//
// Create Key and IV for PBKDF1(PCKS#5 v1.5)
//
// [NOTE]
// This function calculates and returns Key and IV value according to the
// argument. NSS calculates Key and IV values as PCKS#5 v2(PBKDF2) and
// uses those randomly for AES256.
// However, K2HASH was originally implemented only in OpenSSL, using the
// EVP_BytesToKey function which is not recommended.
// That function calculates Key and IV value from salt and returns those values
// as PCKS#5 v1.5(PBKDF1). Therefore, we also use those Key and IV values
// calculated equivalently from salt for NSS and implement so that data created
// with K2HASH of OpenSSL version can be read out.
// Thus, this function mimics a part of OpenSSL EVP_BytesToKey(PBKDF1) for NSS.
// In the future, we will move to PCKS#5 v2(PBKDF2) soon.
// For future encryption, this function will no longer be used.(For decoding,
// it is used for compatibility, but it will not be supported for a long time.)
//
static bool create_pbkdf1_key_iv(const unsigned char* pSalt, const unsigned char* pPass, size_t lenPass, unsigned int iteratecnt, unsigned char* pKey, unsigned int lenCipherKey, unsigned char* pIV, int lenIV)
{
	bool			result = false;
	PK11Context*	md5ctx;
	unsigned char	md5buf[MD5_BLOCK_LENGTH];							// == SHA512_LENGTH
	unsigned int	md5len = 0;
	bool			is_first_loop;
	unsigned int	cnt;

	if(!pSalt || !pPass || lenPass <= 0 || !pKey || lenCipherKey == 0 || !pIV || lenIV <= 0){
		ERR_K2HPRN("parameters are wrong.");
		return result;
	}
	if(NULL == (md5ctx = PK11_CreateDigestContext(SEC_OID_MD5))){		// MD5
		ERR_K2HPRN("Could not create digest context.");
		return result;
	}

	for(result = true, is_first_loop = true; true; is_first_loop = false){
		if(SECSuccess != PK11_DigestBegin(md5ctx)){
			PK11_DestroyContext(md5ctx, PR_TRUE);
			result = false;
			break;
		}

		if(!is_first_loop){
			PK11_DigestOp(md5ctx, &(md5buf[0]), md5len);
		}
		PK11_DigestOp(md5ctx, pPass, lenPass);
		PK11_DigestOp(md5ctx, pSalt, K2H_ENCRYPT_SALT_LENGTH);			// K2H_ENCRYPT_SALT_LENGTH = 8
		PK11_DigestFinal(md5ctx, &(md5buf[0]), &md5len, sizeof(md5buf));

		for(cnt = 1; cnt < iteratecnt; ++cnt){
			PK11_DigestOp(md5ctx, &(md5buf[0]), md5len);
			PK11_DigestFinal(md5ctx, &(md5buf[0]), &md5len, sizeof(md5buf));
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
	PK11_DestroyContext(md5ctx, PR_TRUE);
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

	unsigned char*	encryptdata;
	unsigned char*	saltpos;
	unsigned char*	setdatapos;
	int				encbodylen = 0;
	unsigned int	enclastlen = 0;

	// allocated for encrypted data area
	if(NULL == (encryptdata = reinterpret_cast<unsigned char*>(malloc(orglen + K2H_ENCRYPTED_DATA_EX_LENGTH)))){
		ERR_K2HPRN("Could not allcation memory.");
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

	// SEC OID Tag & Cipher
	SECOidTag			cipherOid = SEC_OID_AES_256_CBC;
	CK_MECHANISM_TYPE	cipherMech;
	if(CKM_INVALID_MECHANISM == (cipherMech = PK11_AlgtagToMechanism(cipherOid))){
		ERR_K2HPRN("could not get SEC_OID_AES_256_CBC Cipher.");
		K2H_Free(encryptdata);
		return NULL;
	}else{
		CK_MECHANISM_TYPE	paddedMech = PK11_GetPadMechanism(cipherMech);
		if(CKM_INVALID_MECHANISM == paddedMech || cipherMech == paddedMech){
			ERR_K2HPRN("could not get Padding SEC_OID_AES_256_CBC Cipher.");
			K2H_Free(encryptdata);
			return NULL;
		}
		cipherMech = paddedMech;
	}

	// Passphrase(key) and Salt Items
	SECItem			keyItem;
	keyItem.type	= siBuffer;
	keyItem.data	= const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(pass));
	keyItem.len		= strlen(pass);

	SECItem			saltItem;
	saltItem.type	= siBuffer;
	saltItem.data	= saltpos;
	saltItem.len	= K2H_ENCRYPT_SALT_LENGTH;

	// Algorithm ID
	SECAlgorithmID*	algid;
	if(NULL == (algid = PK11_CreatePBEV2AlgorithmID(cipherOid, cipherOid, SEC_OID_HMAC_SHA1, 32, 1, &saltItem))){
		ERR_K2HPRN("could not get Algorithm ID.");
		K2H_Free(encryptdata);
		return NULL;
	}

	// Slot
	PK11SlotInfo*	Slot;
	if(NULL == (Slot = PK11_GetBestSlot(cipherMech, NULL))){
		ERR_K2HPRN("could not get PKCS#11 slot.");
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// Symmetric key
	//
	// [NOTE]
	// This key acquisition is done solely to obtain the key length for the Cipher.
	// This key is temporary key to the last, it is released immediately.
	//
	PK11SymKey*		pKey;
	if(NULL == (pKey = PK11_PBEKeyGen(Slot, algid, &keyItem, PR_FALSE, NULL))){
		ERR_K2HPRN("could not get Symmetric Key.");
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// key and IV size
	unsigned int	keySize	= PK11_GetKeyLength(pKey);
	int				ivSize	= PK11_GetIVLength(cipherMech);
	if(ivSize <= 0 || K2H_ENCRYPT_IV_LENGTH < ivSize){
		ERR_K2HPRN("IV size is wrong(%d)", ivSize);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	//
	// Get key and IV(for PBKDF1)
	//
	unsigned char	key[32];						// always key size is 32 on this case
	unsigned char	iv[K2H_ENCRYPT_IV_LENGTH];		// always IV size is 16 == ivSize on this case, this likes EVP_MAX_IV_LENGTH in OpenSSL.
	if(!create_pbkdf1_key_iv(saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, keySize, iv, ivSize)){
		ERR_K2HPRN("could not create IV.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// switch new key
	SECItem			newkeyItem;
	newkeyItem.type	= siBuffer;
	newkeyItem.data	= key;
	newkeyItem.len	= keySize;

	PK11SymKey*		pNewKey;
	if(NULL == (pNewKey = PK11_ImportSymKey(Slot, cipherMech, PK11_OriginUnwrap, CKA_WRAP, &newkeyItem, NULL))){
		ERR_K2HPRN("could not get new Symmetric Key.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}
	PK11_FreeSymKey(pKey);
	pKey = pNewKey;

	// Make SEC parameter
	SECItem		ivItem;
	ivItem.type	= siBuffer;
	ivItem.data	= iv;
	ivItem.len	= ivSize;

	// SEC Parameter
	SECItem*	SecParam;
	if(NULL == (SecParam = PK11_ParamFromIV(cipherMech, &ivItem))){
		ERR_K2HPRN("could not get SEC Parameter.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// Context
	PK11Context*	Context;
	if(NULL == (Context = PK11_CreateContextBySymKey(cipherMech, CKA_ENCRYPT, pKey, SecParam))){
		ERR_K2HPRN("could not create Context by PK11_CreateContextBySymKey.");
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// do encrypt
	if(SECSuccess != PK11_CipherOp(Context, setdatapos, &encbodylen, orglen + K2H_ENCRYPT_MAX_PADDING_LENGTH, orgdata, orglen)){
		ERR_K2HPRN("Failed to AES256 CBC encrypt.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// last encrypt
	if(SECSuccess != PK11_DigestFinal(Context, (setdatapos + encbodylen), &enclastlen, ((orglen + K2H_ENCRYPT_MAX_PADDING_LENGTH) - encbodylen))){
		ERR_K2HPRN("Failed to AES256 CBC encrypt finally.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	PK11_DestroyContext(Context, PR_TRUE);
	SECITEM_FreeItem(SecParam, PR_TRUE);
	PK11_FreeSymKey(pKey);
	PK11_FreeSlot(Slot);
	SECOID_DestroyAlgorithmID(algid, PR_TRUE);

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

	unsigned char*			decryptdata;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH));
	int						decbodylen	= 0;
	unsigned int			declastlen	= 0;

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(encbodylen)))){	// decbodylen < encbodylen
		ERR_K2HPRN("Could not allcation memory.");
		return NULL;
	}

	// SEC OID Tag & Cipher
	SECOidTag			cipherOid = SEC_OID_AES_256_CBC;
	CK_MECHANISM_TYPE	cipherMech;
	if(CKM_INVALID_MECHANISM == (cipherMech = PK11_AlgtagToMechanism(cipherOid))){
		ERR_K2HPRN("could not get SEC_OID_AES_256_CBC Cipher.");
		K2H_Free(decryptdata);
		return NULL;
	}else{
		CK_MECHANISM_TYPE	paddedMech = PK11_GetPadMechanism(cipherMech);
		if(CKM_INVALID_MECHANISM == paddedMech || cipherMech == paddedMech){
			ERR_K2HPRN("could not get Padding SEC_OID_AES_256_CBC Cipher.");
			K2H_Free(decryptdata);
			return NULL;
		}
		cipherMech = paddedMech;
	}

	// Passphrase(key) and Salt Items
	SECItem			keyItem;
	keyItem.type	= siBuffer;
	keyItem.data	= const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(pass));
	keyItem.len		= strlen(pass);

	SECItem			saltItem;
	saltItem.type	= siBuffer;
	saltItem.data	= const_cast<unsigned char*>(saltpos);
	saltItem.len	= K2H_ENCRYPT_SALT_LENGTH;

	// Algorithm ID
	SECAlgorithmID*	algid;
	if(NULL == (algid = PK11_CreatePBEV2AlgorithmID(cipherOid, cipherOid, SEC_OID_HMAC_SHA1, 32, 1, &saltItem))){
		ERR_K2HPRN("could not get Algorithm ID.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// Slot
	PK11SlotInfo*	Slot;
	if(NULL == (Slot = PK11_GetBestSlot(cipherMech, NULL))){
		ERR_K2HPRN("could not get PKCS#11 slot.");
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// Symmetric key
	//
	// [NOTE]
	// This key acquisition is done solely to obtain the key length for the Cipher.
	// This key is temporary key to the last, it is released immediately.
	//
	PK11SymKey*		pKey;
	if(NULL == (pKey = PK11_PBEKeyGen(Slot, algid, &keyItem, PR_FALSE, NULL))){
		ERR_K2HPRN("could not get Symmetric Key.");
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// key and IV size
	unsigned int	keySize	= PK11_GetKeyLength(pKey);
	int				ivSize	= PK11_GetIVLength(cipherMech);
	if(ivSize <= 0 || K2H_ENCRYPT_IV_LENGTH < ivSize){
		ERR_K2HPRN("IV size is wrong(0)");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	//
	// Get key and IV(for PBKDF1)
	//
	unsigned char	key[32];						// always key size is 32 on this case
	unsigned char	iv[K2H_ENCRYPT_IV_LENGTH];		// always IV size is 16 == ivSize on this case, this likes EVP_MAX_IV_LENGTH in OpenSSL.
	if(!create_pbkdf1_key_iv(saltpos, reinterpret_cast<const unsigned char*>(pass), strlen(pass), 1, key, keySize, iv, ivSize)){
		ERR_K2HPRN("could not create IV.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// switch new key
	SECItem			newkeyItem;
	newkeyItem.type	= siBuffer;
	newkeyItem.data	= key;
	newkeyItem.len	= keySize;

	PK11SymKey*		pNewKey;
	if(NULL == (pNewKey = PK11_ImportSymKey(Slot, cipherMech, PK11_OriginUnwrap, CKA_WRAP, &newkeyItem, NULL))){
		ERR_K2HPRN("could not get new Symmetric Key.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}
	PK11_FreeSymKey(pKey);
	pKey = pNewKey;

	// Make SEC param
	SECItem		ivItem;
	ivItem.type	= siBuffer;
	ivItem.data	= iv;
	ivItem.len	= ivSize;

	// SEC Parameter
	SECItem*	SecParam;
	if(NULL == (SecParam = PK11_ParamFromIV(cipherMech, &ivItem))){
		ERR_K2HPRN("could not get SEC Parameter.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// Context
	PK11Context*	Context;
	if(NULL == (Context = PK11_CreateContextBySymKey(cipherMech, CKA_DECRYPT, pKey, SecParam))){
		ERR_K2HPRN("could not create Context by PK11_CreateContextBySymKey.");
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// do decrypt
	if(SECSuccess != PK11_CipherOp(Context, decryptdata, &decbodylen, encbodylen, encbodypos, encbodylen)){
		ERR_K2HPRN("Failed to AES256 CBC decrypt.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// last decrypt
	if(SECSuccess != PK11_DigestFinal(Context, (decryptdata + decbodylen), &declastlen, (encbodylen - decbodylen))){
		ERR_K2HPRN("Failed to AES256 CBC decrypt finally.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	PK11_DestroyContext(Context, PR_TRUE);
	SECITEM_FreeItem(SecParam, PR_TRUE);
	PK11_FreeSymKey(pKey);
	PK11_FreeSlot(Slot);
	SECOID_DestroyAlgorithmID(algid, PR_TRUE);

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

	unsigned char	iv[K2H_ENCRYPT_IV_LENGTH];		// always IV size is 16 == ivSize on this case, this likes EVP_MAX_IV_LENGTH in OpenSSL.
	unsigned char*	encryptdata;
	unsigned char*	saltpos;
	unsigned char*	ivpos;
	unsigned char*	iterpos;
	unsigned char*	setdatapos;
	int				encbodylen = 0;
	unsigned int	enclastlen = 0;

	// allocated for encrypted data area
	if(NULL == (encryptdata = reinterpret_cast<unsigned char*>(malloc(orglen + K2H_ENCRYPTED_DATA_EX2_LENGTH)))){
		ERR_K2HPRN("Could not allcation memory.");
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
	ivpos	= &saltpos[K2H_ENCRYPT_SALT_LENGTH];

	// make iv and copy it
	if(!k2h_generate_iv(iv, K2H_ENCRYPT_IV_LENGTH)){					// always IV size is 16 == ivSize on this case, this likes EVP_MAX_IV_LENGTH in OpenSSL.
		ERR_K2HPRN("Could not make iv.");
		K2H_Free(encryptdata);
		return NULL;
	}
	memcpy(ivpos, iv, K2H_ENCRYPT_IV_LENGTH);							// must be K2H_ENCRYPT_IV_LENGTH = 16
	iterpos	= &ivpos[K2H_ENCRYPT_IV_LENGTH];

	// copy iter
	if(!k2h_copy_iteration_count(iterpos, iter, K2H_ENCRYPT_ITER_LENGTH)){
		ERR_K2HPRN("Could not save iteration count.");
		K2H_Free(encryptdata);
		return NULL;
	}
	setdatapos	= &iterpos[K2H_ENCRYPT_ITER_LENGTH];

	// SEC OID Tag & Cipher
	SECOidTag			cipherOid = SEC_OID_AES_256_CBC;
	CK_MECHANISM_TYPE	cipherMech;
	if(CKM_INVALID_MECHANISM == (cipherMech = PK11_AlgtagToMechanism(cipherOid))){
		ERR_K2HPRN("could not get SEC_OID_AES_256_CBC Cipher.");
		K2H_Free(encryptdata);
		return NULL;
	}else{
		CK_MECHANISM_TYPE	paddedMech = PK11_GetPadMechanism(cipherMech);
		if(CKM_INVALID_MECHANISM == paddedMech || cipherMech == paddedMech){
			ERR_K2HPRN("could not get Padding SEC_OID_AES_256_CBC Cipher.");
			K2H_Free(encryptdata);
			return NULL;
		}
		cipherMech = paddedMech;
	}

	// Passphrase(key) and Salt Items
	SECItem			keyItem;
	keyItem.type	= siBuffer;
	keyItem.data	= const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(pass));
	keyItem.len		= strlen(pass);

	SECItem			saltItem;
	saltItem.type	= siBuffer;
	saltItem.data	= saltpos;
	saltItem.len	= K2H_ENCRYPT_SALT_LENGTH;

	// Algorithm ID
	SECAlgorithmID*	algid;
	if(NULL == (algid = PK11_CreatePBEV2AlgorithmID(cipherOid, cipherOid, SEC_OID_HMAC_SHA512, 32, iter, &saltItem))){
		ERR_K2HPRN("could not get Algorithm ID.");
		K2H_Free(encryptdata);
		return NULL;
	}

	// Slot
	PK11SlotInfo*	Slot;
	if(NULL == (Slot = PK11_GetBestSlot(cipherMech, NULL))){
		ERR_K2HPRN("could not get PKCS#11 slot.");
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// Symmetric key
	PK11SymKey*		pKey;
	if(NULL == (pKey = PK11_PBEKeyGen(Slot, algid, &keyItem, PR_FALSE, NULL))){
		ERR_K2HPRN("could not get Symmetric Key.");
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}
	// [NOTE]
	// If you need to print raw key binary, you can print it here.
	//
	// ex: print following as (rawKey->data and rawKey->len)
	//		PK11_ExtractKeyValue(pKey);
	//		SECItem*	rawKey = PK11_GetKeyData(pKey);
	//

	// get IV size
	int	ivSize	= PK11_GetIVLength(cipherMech);
	if(ivSize <= 0 || K2H_ENCRYPT_IV_LENGTH < ivSize){
		ERR_K2HPRN("IV size is wrong(%d)", ivSize);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// Make SEC Parameter
	SECItem		ivItem;
	ivItem.type	= siBuffer;
	ivItem.data	= iv;
	ivItem.len	= ivSize;

	// SEC Parameter
	SECItem*	SecParam;
	if(NULL == (SecParam = PK11_ParamFromIV(cipherMech, &ivItem))){
		ERR_K2HPRN("could not get SEC Parameter.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// Context
	PK11Context*	Context;
	if(NULL == (Context = PK11_CreateContextBySymKey(cipherMech, CKA_ENCRYPT, pKey, SecParam))){
		ERR_K2HPRN("could not create Context by PK11_CreateContextBySymKey.");
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// do encrypt
	if(SECSuccess != PK11_CipherOp(Context, setdatapos, &encbodylen, orglen + K2H_ENCRYPT_MAX_PADDING_LENGTH, orgdata, orglen)){
		ERR_K2HPRN("Failed to AES256 CBC encrypt.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	// last encrypt
	if(SECSuccess != PK11_DigestFinal(Context, (setdatapos + encbodylen), &enclastlen, ((orglen + K2H_ENCRYPT_MAX_PADDING_LENGTH) - encbodylen))){
		ERR_K2HPRN("Failed to AES256 CBC encrypt finally.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(encryptdata);
		return NULL;
	}

	PK11_DestroyContext(Context, PR_TRUE);
	SECITEM_FreeItem(SecParam, PR_TRUE);
	PK11_FreeSymKey(pKey);
	PK11_FreeSlot(Slot);
	SECOID_DestroyAlgorithmID(algid, PR_TRUE);

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

	unsigned char*			decryptdata;
	int						iter;
	const unsigned char*	saltpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH];
	const unsigned char*	iv			= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH];
	const unsigned char*	iterpos		= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH];
	const unsigned char*	encbodypos	= &encdata[K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH];
	int						encbodylen	= static_cast<int>(enclen - (K2H_ENCRYPT_SALT_PREFIX_LENGTH + K2H_ENCRYPT_SALT_LENGTH + K2H_ENCRYPT_IV_LENGTH + K2H_ENCRYPT_ITER_LENGTH));
	int						decbodylen	= 0;
	unsigned int			declastlen	= 0;

	// allocated for decrypted data area
	if(NULL == (decryptdata = reinterpret_cast<unsigned char*>(malloc(encbodylen)))){	// decbodylen < encbodylen
		ERR_K2HPRN("Could not allcation memory.");
		return NULL;
	}

	// get iteration count
	if(-1 == (iter = k2h_get_iteration_count(iterpos))){
		ERR_K2HPRN("Could not get iteration count.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// SEC OID Tag & Cipher
	SECOidTag			cipherOid = SEC_OID_AES_256_CBC;
	CK_MECHANISM_TYPE	cipherMech;
	if(CKM_INVALID_MECHANISM == (cipherMech = PK11_AlgtagToMechanism(cipherOid))){
		ERR_K2HPRN("could not get SEC_OID_AES_256_CBC Cipher.");
		K2H_Free(decryptdata);
		return NULL;
	}else{
		CK_MECHANISM_TYPE	paddedMech = PK11_GetPadMechanism(cipherMech);
		if(CKM_INVALID_MECHANISM == paddedMech || cipherMech == paddedMech){
			ERR_K2HPRN("could not get Padding SEC_OID_AES_256_CBC Cipher.");
			K2H_Free(decryptdata);
			return NULL;
		}
		cipherMech = paddedMech;
	}

	// Passphrase(key) and Salt Items
	SECItem			keyItem;
	keyItem.type	= siBuffer;
	keyItem.data	= const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(pass));
	keyItem.len		= strlen(pass);

	SECItem			saltItem;
	saltItem.type	= siBuffer;
	saltItem.data	= const_cast<unsigned char*>(saltpos);
	saltItem.len	= K2H_ENCRYPT_SALT_LENGTH;

	// Algorithm ID
	SECAlgorithmID*	algid;
	if(NULL == (algid = PK11_CreatePBEV2AlgorithmID(cipherOid, cipherOid, SEC_OID_HMAC_SHA512, 32, iter, &saltItem))){
		ERR_K2HPRN("could not get Algorithm ID.");
		K2H_Free(decryptdata);
		return NULL;
	}

	// Slot
	PK11SlotInfo*	Slot;
	if(NULL == (Slot = PK11_GetBestSlot(cipherMech, NULL))){
		ERR_K2HPRN("could not get PKCS#11 slot.");
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// Symmetric key
	PK11SymKey*		pKey;
	if(NULL == (pKey = PK11_PBEKeyGen(Slot, algid, &keyItem, PR_FALSE, NULL))){
		ERR_K2HPRN("could not get Symmetric Key.");
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}
	// [NOTE]
	// If you need to print raw key binary, you can print it here.
	//
	// ex: print following as (rawKey->data and rawKey->len)
	//		PK11_ExtractKeyValue(pKey);
	//		SECItem*	rawKey = PK11_GetKeyData(pKey);
	//

	// IV size
	int	ivSize	= PK11_GetIVLength(cipherMech);
	if(ivSize <= 0 || K2H_ENCRYPT_IV_LENGTH < ivSize){
		ERR_K2HPRN("IV size is wrong(%d)", ivSize);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// Make SEC Parameter
	SECItem		ivItem;
	ivItem.type	= siBuffer;
	ivItem.data	= const_cast<unsigned char*>(iv);
	ivItem.len	= ivSize;

	// SEC Parameter
	SECItem*	SecParam;
	if(NULL == (SecParam = PK11_ParamFromIV(cipherMech, &ivItem))){
		ERR_K2HPRN("could not get SEC Parameter.");
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// Context
	PK11Context*	Context;
	if(NULL == (Context = PK11_CreateContextBySymKey(cipherMech, CKA_DECRYPT, pKey, SecParam))){
		ERR_K2HPRN("could not create Context by PK11_CreateContextBySymKey.");
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// do decrypt
	if(SECSuccess != PK11_CipherOp(Context, decryptdata, &decbodylen, encbodylen, encbodypos, encbodylen)){
		ERR_K2HPRN("Failed to AES256 CBC decrypt.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	// last decrypt
	if(SECSuccess != PK11_DigestFinal(Context, (decryptdata + decbodylen), &declastlen, (encbodylen - decbodylen))){
		ERR_K2HPRN("Failed to AES256 CBC decrypt finally.");
		PK11_DestroyContext(Context, PR_TRUE);
		SECITEM_FreeItem(SecParam, PR_TRUE);
		PK11_FreeSymKey(pKey);
		PK11_FreeSlot(Slot);
		SECOID_DestroyAlgorithmID(algid, PR_TRUE);
		K2H_Free(decryptdata);
		return NULL;
	}

	PK11_DestroyContext(Context, PR_TRUE);
	SECITEM_FreeItem(SecParam, PR_TRUE);
	PK11_FreeSymKey(pKey);
	PK11_FreeSlot(Slot);
	SECOID_DestroyAlgorithmID(algid, PR_TRUE);

	// length
	declen = static_cast<size_t>(decbodylen + declastlen);

	return decryptdata;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
