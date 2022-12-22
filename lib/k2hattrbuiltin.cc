/*
 * K2HASH
 *
 * Copyright 2013 Yahoo Japan Corporation.
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
 * CREATE:   Fri Dec 18 2015
 * REVISION:
 *
 */

#include <fullock/flckutil.h>

#include <fstream>

#include "k2hcommon.h"
#include "k2hcryptcommon.h"
#include "k2hattrbuiltin.h"
#include "k2hattrs.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	K2HATTR_BUILTIN_VERSION						"K2H ATTR BUILTIN"
#define	K2HATTR_ENV_MTIME							"K2HATTR_MTIME"
#define	K2HATTR_ENV_HISTORY							"K2HATTR_HISTORY"
#define	K2HATTR_ENV_EXPIRE_SEC						"K2HATTR_EXPIRE_SEC"
#define	K2HATTR_ENV_DEFAULT_ENC						"K2HATTR_DEFENC"
#define	K2HATTR_ENV_ENCFILE							"K2HATTR_ENCFILE"
#define	K2HATTR_ENV_ENC_TYPE						"K2HATTR_ENC_TYPE"
#define	K2HATTR_ENV_ENC_ITER						"K2HATTR_ENC_ITER"

#define	K2HATTR_ENV_VAL_NO							"NO"
#define	K2HATTR_ENV_VAL_OFF							"OFF"
#define	K2HATTR_ENV_VAL_YES							"YES"
#define	K2HATTR_ENV_VAL_ON							"ON"
#define	K2HATTR_ENV_VAL_ENCTYPE_AES256_PBKDF1		"AES256_PBKDF1"
#define	K2HATTR_ENV_VAL_ENCTYPE_AES256_PBKDF2		"AES256_PBKDF2"

#define	K2HATTR_ENCFILE_COMMENT_CHAR				'#'

#define	K2HATTR_COMMON_MTIME						"mtime"
#define	K2HATTR_COMMON_EXPIRE						"expire"
#define	K2HATTR_COMMON_AES256_MD5					"aes256md5"
#define	K2HATTR_COMMON_AES256_PBKDF2				"aes256_cbc_pad.pbkdf2.sha256"		// <DATA ENC CIPHER>.<PCKS#5 v2>.<KEY HASH TYPE>
#define	K2HATTR_COMMON_UNIQ_ID						"uniqid"
#define	K2HATTR_COMMON_PARENT_UNIQ_ID				"parentuniqid"
#define	K2HATTR_COMMON_HISTORY_MARKER				"hismark"

//---------------------------------------------------------
// K2hCryptContext Class
//---------------------------------------------------------
// This class initializes the Crypt library when the k2hash
// library is loaded and discards it at the timing of unloading.
//
// This class is used in K2hAttrBuiltin class as singleton.
// This function provides for initializing crypt library when
// starting k2hash library and forking in child process.
//
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
class K2hCryptContext
{
	protected:
		static void PreforkHandler(void);

	public:
		K2hCryptContext(void);
		virtual ~K2hCryptContext(void);
};

void K2hCryptContext::PreforkHandler(void)
{
	if(!k2h_crypt_lib_initialize()){
		ERR_K2HPRN("Something error occurred in initializing crypt library at child process in fork().");
	}else{
		MSG_K2HPRN("Succeed initializing crypt library at child process in fork().");
	}
}

K2hCryptContext::K2hCryptContext(void)
{
	int	result;
	if(0 != (result = pthread_atfork(NULL, NULL, K2hCryptContext::PreforkHandler))){
		ERR_K2HPRN("Failed to set handler for forking(errno=%d), but continue...", result);
	}
	if(!k2h_crypt_lib_initialize()){
		ERR_K2HPRN("Something error occurred in initializing crypt library.");
	}else{
		MSG_K2HPRN("Succeed initializing crypt library.");
	}
}

K2hCryptContext::~K2hCryptContext(void)
{
	if(!k2h_crypt_lib_terminate()){
		ERR_K2HPRN("Something error occurred in terminating crypt library.");
	}else{
		MSG_K2HPRN("Succeed terminating crypt library.");
	}
}

//---------------------------------------------------------
// K2hAttrBuiltin Class variables
//---------------------------------------------------------
const char*		K2hAttrBuiltin::ATTR_ENV_MTIME		= K2HATTR_ENV_MTIME;
const char*		K2hAttrBuiltin::ATTR_ENV_HISTORY	= K2HATTR_ENV_HISTORY;
const char*		K2hAttrBuiltin::ATTR_ENV_EXPIRE_SEC	= K2HATTR_ENV_EXPIRE_SEC;
const char*		K2hAttrBuiltin::ATTR_ENV_DEFAULT_ENC= K2HATTR_ENV_DEFAULT_ENC;
const char*		K2hAttrBuiltin::ATTR_ENV_ENCFILE	= K2HATTR_ENV_ENCFILE;
const char*		K2hAttrBuiltin::ATTR_ENV_ENC_TYPE	= K2HATTR_ENV_ENC_TYPE;
const char*		K2hAttrBuiltin::ATTR_ENV_ENC_ITER	= K2HATTR_ENV_ENC_ITER;

const int		K2hAttrBuiltin::TYPE_ATTRBUILTIN;
const char*		K2hAttrBuiltin::ATTR_BUILTIN_VERSION= K2HATTR_BUILTIN_VERSION;

const char*		K2hAttrBuiltin::ATTR_MTIME			= K2HATTR_COMMON_MTIME;
const char*		K2hAttrBuiltin::ATTR_EXPIRE			= K2HATTR_COMMON_EXPIRE;
const char*		K2hAttrBuiltin::ATTR_UNIQID			= K2HATTR_COMMON_UNIQ_ID;
const char*		K2hAttrBuiltin::ATTR_PUNIQID		= K2HATTR_COMMON_PARENT_UNIQ_ID;
const char*		K2hAttrBuiltin::ATTR_AES256_MD5		= K2HATTR_COMMON_AES256_MD5;
const char*		K2hAttrBuiltin::ATTR_AES256_PBKDF2	= K2HATTR_COMMON_AES256_PBKDF2;
const char*		K2hAttrBuiltin::ATTR_HISMARK		= K2HATTR_COMMON_HISTORY_MARKER;

const time_t	K2hAttrBuiltin::NOT_EXPIRE;
const int		K2hAttrBuiltin::ATTR_MASK_NO;
const int		K2hAttrBuiltin::ATTR_MASK_MTIME;
const int		K2hAttrBuiltin::ATTR_MASK_ENCRYPT;
const int		K2hAttrBuiltin::ATTR_MASK_HISTORY;
const int		K2hAttrBuiltin::ATTR_MASK_EXPIRE;
const int		K2hAttrBuiltin::ATTR_MASK_EXPIRE_KP;

//---------------------------------------------------------
// Utility macros
//---------------------------------------------------------
#define	IS_ATTR_MASK(mask, value)					(value == (mask & value))

#define	IS_ATTR_MASK_MTIME(mask)					IS_ATTR_MASK(mask, K2hAttrBuiltin::ATTR_MASK_MTIME)
#define	IS_ATTR_MASK_ENCRYPT(mask)					IS_ATTR_MASK(mask, K2hAttrBuiltin::ATTR_MASK_ENCRYPT)
#define	IS_ATTR_MASK_HISTORY(mask)					IS_ATTR_MASK(mask, K2hAttrBuiltin::ATTR_MASK_HISTORY)
#define	IS_ATTR_MASK_EXPIRE(mask)					IS_ATTR_MASK(mask, K2hAttrBuiltin::ATTR_MASK_EXPIRE)
#define	IS_ATTR_MASK_EXPIRE_KP(mask)				IS_ATTR_MASK(mask, K2hAttrBuiltin::ATTR_MASK_EXPIRE_KP)

#define	ATTR_MASK_FILTER(mask)						(mask & (K2hAttrBuiltin::ATTR_MASK_MTIME | K2hAttrBuiltin::ATTR_MASK_ENCRYPT | K2hAttrBuiltin::ATTR_MASK_HISTORY | K2hAttrBuiltin::ATTR_MASK_EXPIRE | K2hAttrBuiltin::ATTR_MASK_EXPIRE_KP))

//---------------------------------------------------------
// K2hAttrBuiltin Class Methods
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
K2hCryptContext& K2hAttrBuiltin::GetCryptLibContext(void)
{
	static K2hCryptContext	cryptlibcontext;		// singleton
	return cryptlibcontext;
}

// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
k2hbapackmap_t& K2hAttrBuiltin::GetAttrPackMap(void)
{
	static k2hbapackmap_t	AttrPackMap;			// singleton
	(void)K2hAttrBuiltin::GetCryptLibContext();		// for initialize singleton if not initializing(this method call in K2hAttrBuiltin::Initialize)
	return AttrPackMap;
}

PK2HBATTRPACK K2hAttrBuiltin::GetBuiltinAttrPack(const K2HShm* pshm)
{
	if(!pshm){
		ERR_K2HPRN("Parameter is wrong.");
		return NULL;
	}
	k2hbapackmap_t::iterator	iter = K2hAttrBuiltin::GetAttrPackMap().find(pshm);
	if(K2hAttrBuiltin::GetAttrPackMap().end() == iter){
		return NULL;
	}
	return iter->second;
}

bool K2hAttrBuiltin::CleanAttrBuiltin(const K2HShm* pshm)
{
	if(!pshm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	k2hbapackmap_t::iterator	iter = K2hAttrBuiltin::GetAttrPackMap().find(pshm);
	if(K2hAttrBuiltin::GetAttrPackMap().end() == iter){
		WAN_K2HPRN("There is no builtin attribute pack structure in map.");
		return true;	// already unload
	}

	PK2HBATTRPACK	pPack	= iter->second;
	iter->second			= NULL;
	K2H_Delete(pPack);
	K2hAttrBuiltin::GetAttrPackMap().erase(iter);

	return true;
}

bool K2hAttrBuiltin::ClearEncryptPassMap(PK2HBATTRPACK pPack)
{
	if(!pPack){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	pPack->DefaultPassMD5.clear();

	return true;
}

int K2hAttrBuiltin::LoadEncryptPassMap(PK2HBATTRPACK pPack, const char* pfile)
{
	if(!pPack || ISEMPTYSTR(pfile)){
		ERR_K2HPRN("parameters are wrong.");
		return -1;
	}

	// make real path
	string	abspath;
	{
		char*	rpath;
		if(NULL == (rpath = realpath(pfile, NULL))){
			ERR_K2HPRN("Could not convert path(%s) to real path or there is no such file(errno:%d).", pfile, errno);
			return -1;
		}
		abspath = rpath;
		K2H_Free(rpath);
	}

	// open file
	ifstream	passstream(abspath.c_str(), ios::in);
	if(!passstream.good()){
		ERR_K2HPRN("Could not open(read only) file(%s:%s)", pfile, abspath.c_str());
		return -1;
	}

	// clear all
	K2hAttrBuiltin::ClearEncryptPassMap(pPack);

	// load pass
	string		line;
	K2HENCPASS	encpass;
	for(bool is_first_set = false; passstream.good() && getline(passstream, line); ){
		encpass.strPass = trim(line);
		if(encpass.strPass.empty()){
			continue;
		}
		if(K2HATTR_ENCFILE_COMMENT_CHAR == encpass.strPass.at(0)){
			// comment line
			continue;
		}
		// make md5
		encpass.strMD5 = to_md5_string(encpass.strPass.c_str());
		// set map
		pPack->EncPassMap[encpass.strMD5] = encpass;

		// first pass is default encrypt pass
		if(!is_first_set){
			pPack->DefaultPassMD5 = encpass.strMD5;
			is_first_set = true;
		}
	}
	passstream.close();

	return pPack->EncPassMap.size();
}

bool K2hAttrBuiltin::RawInitializeEnv(PK2HBATTRPACK pPack)
{
	if(!pPack){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	string	value;

	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_MTIME, value)){
		if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_NO) || 0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_OFF)){
			pPack->IsAttrMTime = false;
		}else if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_YES) || 0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_ON)){
			pPack->IsAttrMTime = true;
		}else{
			WAN_K2HPRN("environment %s has unknown value, but continue...", K2hAttrBuiltin::ATTR_ENV_MTIME);
		}
	}
	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_HISTORY, value)){
		if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_NO) || 0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_OFF)){
			pPack->IsAttrHistory = false;
		}else if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_YES) || 0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_ON)){
			pPack->IsAttrHistory = true;
		}else{
			WAN_K2HPRN("environment %s has unknown value, but continue...", K2hAttrBuiltin::ATTR_ENV_HISTORY);
		}
	}
	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_EXPIRE_SEC, value)){
		pPack->AttrExpireSec = static_cast<time_t>(atoll(value.c_str()));
		if(pPack->AttrExpireSec <= 0){
			WAN_K2HPRN("environment %s value is wrong(%zd), so set 'not expire'.", K2hAttrBuiltin::ATTR_ENV_EXPIRE_SEC, pPack->AttrExpireSec);
			pPack->AttrExpireSec = K2hAttrBuiltin::NOT_EXPIRE;
		}
	}
	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_DEFAULT_ENC, value)){
		if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_NO) || 0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_OFF)){
			pPack->IsDefaultEncrypt = false;
		}else if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_YES) || 0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_ON)){
			pPack->IsDefaultEncrypt = true;
		}else{
			WAN_K2HPRN("environment %s has unknown value, but continue...", K2hAttrBuiltin::ATTR_ENV_DEFAULT_ENC);
		}
	}
	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_ENCFILE, value)){
		if(-1 == K2hAttrBuiltin::LoadEncryptPassMap(pPack, value.c_str())){
			WAN_K2HPRN("Failed to load encrypt pass list from file %s, but continue...", value.c_str());
			K2hAttrBuiltin::ClearEncryptPassMap(pPack);
		}
	}
	// check default encrypt
	if(pPack->IsDefaultEncrypt){
		// If there is encrypt pass list, DefaultPassMD5 is not empty.
		if(pPack->DefaultPassMD5.empty()){
			ERR_K2HPRN("Default encrypt is ON, but there is no encrypt pass list.");
			return false;
		}
	}
	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_ENC_TYPE, value)){
		if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_ENCTYPE_AES256_PBKDF1)){
			pPack->EncType		= K2H_ENC_AES256_PBKDF1;
			pPack->IterCount	= 1;						// This case is always 1 for compatibility
		}else if(0 == strcasecmp(value.c_str(), K2HATTR_ENV_VAL_ENCTYPE_AES256_PBKDF2)){
			pPack->EncType = K2H_ENC_AES256_PBKDF2;
		}else{
			WAN_K2HPRN("environment %s has unknown value(%s), but continue...", K2hAttrBuiltin::ATTR_ENV_ENC_TYPE, value.c_str());
		}
	}
	if(k2h_getenv(K2hAttrBuiltin::ATTR_ENV_ENC_ITER, value)){
		if(K2H_ENC_AES256_PBKDF1 == pPack->EncType){
			WAN_K2HPRN("environment %s is specified, but encrypt type is %s. this type is always iteration count = 1.", K2hAttrBuiltin::ATTR_ENV_ENC_ITER, K2HATTR_ENV_VAL_ENCTYPE_AES256_PBKDF1);
		}else{
			int	count = atoi(value.c_str());
			if(count < 1){
				WAN_K2HPRN("environment %s is wrong value(%d), iteration count must be 1 - 0x7fffffff", K2hAttrBuiltin::ATTR_ENV_ENC_ITER, count);
			}else{
				pPack->IterCount = count;
			}
		}
	}
	return true;
}

bool K2hAttrBuiltin::RawInitialize(PK2HBATTRPACK pPack, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire)
{
	if(is_mtime){
		pPack->IsAttrMTime = *is_mtime;
	}
	if(is_history){
		pPack->IsAttrHistory = *is_history;
	}
	if(expire){
		if(*expire <= 0){
			ERR_K2HPRN("Wrong expire second(%zd).", *expire);
			return false;
		}
		pPack->AttrExpireSec = *expire;
	}
	if(is_defenc){
		pPack->IsDefaultEncrypt = *is_defenc;
	}
	if(!ISEMPTYSTR(passfile)){
		if(-1 == K2hAttrBuiltin::LoadEncryptPassMap(pPack, passfile)){
			ERR_K2HPRN("Failed to load encrypt pass list from file %s", passfile);
			K2hAttrBuiltin::ClearEncryptPassMap(pPack);
			return false;
		}
	}
	// check default encrypt
	if(pPack->IsDefaultEncrypt){
		// If there is encrypt pass list, DefaultPassMD5 is not empty.
		if(pPack->DefaultPassMD5.empty()){
			ERR_K2HPRN("Default encrypt is ON, but there is no encrypt pass list.");
			return false;
		}
	}
	return true;
}

bool K2hAttrBuiltin::Initialize(const K2HShm* pshm, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire)
{
	if(!pshm){
		ERR_K2HPRN("Parameter is wrong");
		return false;
	}
	PK2HBATTRPACK	pPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm);
	if(!pPack){
		pPack = new K2HBATTRPACK;
		K2hAttrBuiltin::GetAttrPackMap()[pshm] = pPack;
	}
	if(!K2hAttrBuiltin::RawInitializeEnv(pPack)){
		ERR_K2HPRN("Failed to load preset attributes from environment.");
		K2hAttrBuiltin::CleanAttrBuiltin(pshm);
		return false;
	}
	if(!K2hAttrBuiltin::RawInitialize(pPack, is_mtime, is_defenc, passfile, is_history, expire)){
		ERR_K2HPRN("Failed to set preset attributes.");
		K2hAttrBuiltin::CleanAttrBuiltin(pshm);
		return false;
	}
	return true;
}

bool K2hAttrBuiltin::IsMarkHistory(const K2HShm* pshm)
{
	PK2HBATTRPACK	pPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm);
	if(!pPack){
		MSG_K2HPRN("Could not find builtin attribute pack structure for shm, maybe does not initialize builtin attribute. So try to initialize.");

		if(!K2hAttrBuiltin::Initialize(pshm)){
			ERR_K2HPRN("Could not initialize builtin attribute object.");
			return false;
		}
		if(NULL == (pPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm))){
			ERR_K2HPRN("Could not get builtin attribute object.");
			return false;
		}
	}
	return pPack->IsAttrHistory;
}

bool K2hAttrBuiltin::AddCryptPass(const K2HShm* pshm, const char* pPass, bool is_default_encrypt)
{
	if(!pshm || ISEMPTYSTR(pPass)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}

	PK2HBATTRPACK	pPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm);
	if(!pPack){
		MSG_K2HPRN("Could not find builtin attribute pack structure for shm, maybe does not initialize builtin attribute. So try to initialize.");

		if(!K2hAttrBuiltin::Initialize(pshm)){
			ERR_K2HPRN("Could not initialize builtin attribute object.");
			return false;
		}
		if(NULL == (pPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm))){
			ERR_K2HPRN("Could not get builtin attribute object.");
			return false;
		}
	}

	// set data
	K2HENCPASS	encpass;
	encpass.strPass	= pPass;
	encpass.strMD5	= to_md5_string(encpass.strPass.c_str());
	pPack->EncPassMap[encpass.strMD5] = encpass;

	if(is_default_encrypt){
		// over write
		pPack->IsDefaultEncrypt	= true;
		pPack->DefaultPassMD5	= encpass.strMD5;
	}
	return true;
}

//---------------------------------------------------------
// K2hAttrBuiltin Methods
//---------------------------------------------------------
K2hAttrBuiltin::K2hAttrBuiltin(void) : K2hAttrOpsBase(), pBuiltinAttrPack(NULL), EncPass(""), ExpireSec(K2hAttrBuiltin::NOT_EXPIRE), OldUniqID(""), AttrMask(K2hAttrBuiltin::ATTR_MASK_NO)
{
	(void)K2hAttrBuiltin::GetCryptLibContext();		// for initialize singleton if not initializing
	VerInfo = K2hAttrBuiltin::ATTR_BUILTIN_VERSION;
}

K2hAttrBuiltin::~K2hAttrBuiltin(void)
{
	K2hAttrBuiltin::Clear();
}

void K2hAttrBuiltin::Clear(void)
{
	K2hAttrOpsBase::Clear();

	EncPass.clear();
	OldUniqID.clear();
	pBuiltinAttrPack= NULL;
	ExpireSec		= K2hAttrBuiltin::NOT_EXPIRE;
}

bool K2hAttrBuiltin::Set(const K2HShm* pshm, const char* encpass, const time_t* expire, int mask)
{
	if(NULL == (pBuiltinAttrPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm))){
		MSG_K2HPRN("Could not find builtin attribute pack structure for shm, maybe does not initialize builtin attribute. So try to initialize.");

		if(!K2hAttrBuiltin::Initialize(pshm)){
			MSG_K2HPRN("Could not initialize builtin attribute object, but continue...");
			return true;
		}
		if(NULL == (pBuiltinAttrPack = K2hAttrBuiltin::GetBuiltinAttrPack(pshm))){
			MSG_K2HPRN("Could not get builtin attribute object, but continue...");
			return true;
		}
	}
	if(!ISEMPTYSTR(encpass)){
		EncPass = encpass;
	}
	if(expire){
		// [NOTE]
		// It is allowed to set the expire value directly to 0.
		// This is useful if you want to create keys as placeholders and so on.
		// However, the expire value can be set 0 only when this method is
		// called directly.
		//
		if(*expire < 0){
			ERR_K2HPRN("expire(%zd) must be 0 or over 0.", *expire);
			Clear();
			return false;
		}
		if(*expire == 0){
			MSG_K2HPRN("expire(%zd) is allowed, it means placeholder.", *expire);
		}
		ExpireSec = *expire;
	}

	AttrMask = ATTR_MASK_FILTER(mask);

	return true;
}

bool K2hAttrBuiltin::IsHandleAttr(const unsigned char* key, size_t keylen) const
{
	if(!key || 0 == keylen){
		MSG_K2HPRN("target key name for attribute is empty.");
		return false;
	}
	if(	((strlen(K2hAttrBuiltin::ATTR_MTIME)		+ 1) == keylen && 0 == memcmp(K2hAttrBuiltin::ATTR_MTIME,			key, keylen))	||
		((strlen(K2hAttrBuiltin::ATTR_EXPIRE)		+ 1) == keylen && 0 == memcmp(K2hAttrBuiltin::ATTR_EXPIRE,			key, keylen))	||
		((strlen(K2hAttrBuiltin::ATTR_UNIQID)		+ 1) == keylen && 0 == memcmp(K2hAttrBuiltin::ATTR_UNIQID,			key, keylen))	||
		((strlen(K2hAttrBuiltin::ATTR_PUNIQID)		+ 1) == keylen && 0 == memcmp(K2hAttrBuiltin::ATTR_PUNIQID,			key, keylen))	||
		((strlen(K2hAttrBuiltin::ATTR_AES256_PBKDF2)+ 1) == keylen && 0 == memcmp(K2hAttrBuiltin::ATTR_AES256_PBKDF2,	key, keylen))	||
		((strlen(K2hAttrBuiltin::ATTR_AES256_MD5)	+ 1) == keylen && 0 == memcmp(K2hAttrBuiltin::ATTR_AES256_MD5,		key, keylen))	)
	{
		return true;
	}
	return false;
}

bool K2hAttrBuiltin::UpdateAttr(K2HAttrs& attrs)
{
	if(!pBuiltinAttrPack){
		//MSG_K2HPRN("builtin attribute object is not initialized, thus nothing to do.");
		return true;
	}
	bool	IsCahnged = false;				// for history

	// make realtime if needed.
	struct timespec	rtime = {0, 0};
	if(	(!IS_ATTR_MASK_MTIME(AttrMask) && pBuiltinAttrPack->IsAttrMTime)	||
		(!IS_ATTR_MASK_EXPIRE(AttrMask) && (K2hAttrBuiltin::NOT_EXPIRE != ExpireSec || K2hAttrBuiltin::NOT_EXPIRE != pBuiltinAttrPack->AttrExpireSec)) ||
		IS_ATTR_MASK_EXPIRE_KP(AttrMask)	)
	{
		if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &rtime)){
			WAN_K2HPRN("Failed to get clock time by errno(%d)", errno);
			rtime.tv_sec	= time(NULL);
			rtime.tv_nsec	= 0L;
		}
	}

	// mtime
	if(!IS_ATTR_MASK_MTIME(AttrMask) && pBuiltinAttrPack->IsAttrMTime){
		if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_MTIME, reinterpret_cast<unsigned char*>(&rtime), sizeof(struct timespec))){
			ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_MTIME);
			return false;
		}
	}else{
		RemoveAttr(attrs, K2hAttrBuiltin::ATTR_MTIME);
	}

	// expire
	if(IS_ATTR_MASK_EXPIRE_KP(AttrMask)){
		// keep expire time if exists
		if(attrs.end() == attrs.find(K2hAttrBuiltin::ATTR_EXPIRE)){
			// no expire attribute
			if(K2hAttrBuiltin::NOT_EXPIRE != ExpireSec){
				// set custom expire time.
				struct timespec	tmptime	= rtime;
				tmptime.tv_sec			+= ExpireSec;
				if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_EXPIRE, reinterpret_cast<unsigned char*>(&tmptime), sizeof(struct timespec))){
					ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_EXPIRE);
					return false;
				}
			}else{
				// there is no custom expire time, so do not set expire.
			}
		}else{
			// expire exists, keep it
		}
	}else{
		if(!IS_ATTR_MASK_EXPIRE(AttrMask) && (K2hAttrBuiltin::NOT_EXPIRE != ExpireSec || K2hAttrBuiltin::NOT_EXPIRE != pBuiltinAttrPack->AttrExpireSec)){
			// update new expire time
			struct timespec	tmptime	= rtime;
			tmptime.tv_sec			+= (K2hAttrBuiltin::NOT_EXPIRE != ExpireSec ? ExpireSec : pBuiltinAttrPack->AttrExpireSec);
			if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_EXPIRE, reinterpret_cast<unsigned char*>(&tmptime), sizeof(struct timespec))){
				ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_EXPIRE);
				return false;
			}
		}else{
			// no expire
			RemoveAttr(attrs, K2hAttrBuiltin::ATTR_EXPIRE);
		}
	}

	// encrypt
	if(!IS_ATTR_MASK_ENCRYPT(AttrMask) && (!EncPass.empty() || pBuiltinAttrPack->IsDefaultEncrypt) && byValue && 0 < ValLen){
		// get pass
		string	pass = EncPass;
		string	strmd5;
		if(pass.empty()){
			// set default encrypt pass
			if(pBuiltinAttrPack->EncPassMap.end() == pBuiltinAttrPack->EncPassMap.find(pBuiltinAttrPack->DefaultPassMD5)){
				ERR_K2HPRN("There is no default encrypt pass.");
				return false;
			}
			pass	= pBuiltinAttrPack->EncPassMap[pBuiltinAttrPack->DefaultPassMD5].strPass;
			strmd5	= pBuiltinAttrPack->DefaultPassMD5;
		}else{
			strmd5	= to_md5_string(pass.c_str());
		}

		// encrypt
		unsigned char*	encValue;
		size_t			encValLen = 0;
		if(K2H_ENC_AES256_PBKDF1 == pBuiltinAttrPack->EncType){
			encValue = k2h_encrypt_aes256_cbc(pass.c_str(), byValue, ValLen, encValLen);
		}else{		// K2H_ENC_AES256_PBKDF2 == pBuiltinAttrPack->EncType
			encValue = k2h_encrypt_aes256_cbc_pbkdf2(pass.c_str(), pBuiltinAttrPack->IterCount, byValue, ValLen, encValLen);
		}
		if(!encValue){
			ERR_K2HPRN("Could not encrypt value by AES256 CBC PAD(PBKDF1 or PBKDF2).");
			return false;
		}

		// replace value pointer in base class.
		K2H_Free(byAllocValue);
		byAllocValue= encValue;
		byValue		= byAllocValue;
		ValLen		= encValLen;

		// set flag for history
		IsCahnged = true;

		// set md5
		if(!SetAttr(attrs, (K2H_ENC_AES256_PBKDF1 == pBuiltinAttrPack->EncType ? K2hAttrBuiltin::ATTR_AES256_MD5 : K2hAttrBuiltin::ATTR_AES256_PBKDF2), strmd5.c_str())){
			ERR_K2HPRN("Could not set %s attribute.", (K2H_ENC_AES256_PBKDF1 == pBuiltinAttrPack->EncType ? K2hAttrBuiltin::ATTR_AES256_MD5 : K2hAttrBuiltin::ATTR_AES256_PBKDF2));
			return false;
		}
	}else{
		RemoveAttr(attrs, (K2H_ENC_AES256_PBKDF1 == pBuiltinAttrPack->EncType ? K2hAttrBuiltin::ATTR_AES256_MD5 : K2hAttrBuiltin::ATTR_AES256_PBKDF2));
	}

	// history
	if(!IS_ATTR_MASK_HISTORY(AttrMask) && pBuiltinAttrPack->IsAttrHistory){
		K2HAttrs::iterator	iter = attrs.find(K2hAttrBuiltin::ATTR_UNIQID);
		if(IsCahnged){
			// get original uniqid
			OldUniqID = "";
			if(iter != attrs.end()){
				if(iter->pval){
					OldUniqID = reinterpret_cast<const char*>(iter->pval);
				}else{
					WAN_K2HPRN("The attribute %s is exists, but it value is empty. but continue...", K2HATTR_COMMON_UNIQ_ID);
				}
			}

			// generate new uniqid
			string	newuniqid = k2h_get_uniqid_for_history(rtime);
			// set new uniqid
			if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_UNIQID, newuniqid.c_str())){
				ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_UNIQID);
				return false;
			}

			// set parent uniqid(=original uniqid)
			if(!OldUniqID.empty()){
				if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_PUNIQID, OldUniqID.c_str())){
					ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_PUNIQID);
					return false;
				}
			}
		}else{
			if(iter == attrs.end()){
				// there is no uniqid, so make uniqid
				string	newuniqid = k2h_get_uniqid_for_history(rtime);
				// set new uniqid
				if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_UNIQID, newuniqid.c_str())){
					ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_UNIQID);
					return false;
				}
			}else{
				// no changed value, so nothing to do.
			}
		}
	}else{
		// [NOTE]
		// If attrs has history(uniqid and parent uniqid) when history is OFF,
		// we do nothing. Because history is ON means that k2hash ONLY stamps
		// uniqid(parent uniqid) for history.
		//
		OldUniqID = "";
	}
	return true;
}

void K2hAttrBuiltin::GetInfo(stringstream& ss) const
{
	K2hAttrOpsBase::GetInfo(ss);

	if(pBuiltinAttrPack){
		ss << "  Attribute mtime:                      " << (pBuiltinAttrPack->IsAttrMTime ? "yes" : "no")		<< endl;
		ss << "  Attribute history:                    " << (pBuiltinAttrPack->IsAttrHistory ? "yes" : "no")	<< endl;
		if(K2hAttrBuiltin::NOT_EXPIRE == pBuiltinAttrPack->AttrExpireSec){
			ss << "  Attribute expire:                     no expire" << endl;
		}else{
			ss << "  Attribute expire:                     " << pBuiltinAttrPack->AttrExpireSec << endl;
		}
		ss << "  Attribute default encrypt:            " << (pBuiltinAttrPack->IsDefaultEncrypt ? "yes" : "no")	<< endl;
		if(pBuiltinAttrPack->DefaultPassMD5.empty()){
			ss << "  Attribute default encrypt pass MD5    no" << endl;
		}else{
			ss << "  Attribute default encrypt pass MD5    " << pBuiltinAttrPack->DefaultPassMD5					<< endl;
		}
		for(k2hepmap_t::const_iterator iter = pBuiltinAttrPack->EncPassMap.begin(); iter != pBuiltinAttrPack->EncPassMap.end(); ++iter){
			ss << "    encrypt pass(\"MD5\":\"pass\")          \"" << iter->second.strMD5 << "\":\"" << iter->second.strPass << "\"" << endl;
		}
	}else{
		ss << "  Attribute mtime:                      n/a" << endl;
		ss << "  Attribute history:                    n/a" << endl;
		ss << "  Attribute expire:                     n/a" << endl;
		ss << "  Attribute default encrypt:            n/a" << endl;
		ss << "  Attribute default enc pass MD5        n/a" << endl;
	}
}

bool K2hAttrBuiltin::DirectSetUniqId(K2HAttrs& attrs, const char* olduniqid)
{
	if(!pBuiltinAttrPack){
		//MSG_K2HPRN("builtin attribute object is not initialized, thus nothing to do.");
		return true;
	}
	if(IS_ATTR_MASK_HISTORY(AttrMask) || !pBuiltinAttrPack->IsAttrHistory){
		MSG_K2HPRN("Could not update uniqid, because history mode is OFF.");
		return true;
	}

	// make old uniqid
	string	strOldUid;
	if(ISEMPTYSTR(olduniqid)){
		K2HAttrs::iterator	iter = attrs.find(K2hAttrBuiltin::ATTR_UNIQID);
		if(iter != attrs.end()){
			if(iter->pval){
				strOldUid = reinterpret_cast<const char*>(iter->pval);
			}else{
				WAN_K2HPRN("The attribute %s is exists, but it value is empty. but continue...", K2HATTR_COMMON_UNIQ_ID);
			}
		}
	}else{
		strOldUid = olduniqid;
	}

	// generate new uniqid
	struct timespec	rtime = {0, 0};
	if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &rtime)){
		WAN_K2HPRN("Failed to get clock time by errno(%d)", errno);
		rtime.tv_sec	= time(NULL);
		rtime.tv_nsec	= 0L;
	}
	string	newuniqid = k2h_get_uniqid_for_history(rtime);

	// set new uniqid
	if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_UNIQID, newuniqid.c_str())){
		ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_UNIQID);
		return false;
	}

	// set parent uniqid
	if(!strOldUid.empty()){
		if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_PUNIQID, strOldUid.c_str())){
			ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_PUNIQID);
			return false;
		}
	}
	return true;
}

bool K2hAttrBuiltin::MarkHistoryEx(K2HAttrs& attrs, bool is_mark) const
{
	if(!pBuiltinAttrPack){
		//MSG_K2HPRN("builtin attribute object is not initialized, thus nothing to do.");
		return true;
	}
	// set history marker
	if(is_mark){
		bool	hismark = true;
		if(!SetAttr(attrs, K2hAttrBuiltin::ATTR_HISMARK, reinterpret_cast<const unsigned char*>(&hismark), sizeof(bool))){
			ERR_K2HPRN("Could not set %s attribute.", K2hAttrBuiltin::ATTR_HISMARK);
			return false;
		}
	}else{
		if(!RemoveAttr(attrs, K2hAttrBuiltin::ATTR_HISMARK)){
			ERR_K2HPRN("Could not remove %s attribute.", K2hAttrBuiltin::ATTR_HISMARK);
			return false;
		}
	}
	return true;
}

bool K2hAttrBuiltin::IsExpire(K2HAttrs& attrs) const
{
	const unsigned char*	pval	= NULL;
	size_t					vallen	= 0;
	if(!GetAttr(attrs, K2hAttrBuiltin::ATTR_EXPIRE, &pval, vallen) || !pval){
		// attr does not have expire key
		return false;
	}
	if(sizeof(struct timespec) != vallen){
		ERR_K2HPRN("%s attribute value must be %zu byte, but value is %zu byte.", K2hAttrBuiltin::ATTR_EXPIRE, sizeof(struct timespec), vallen);
		return false;
	}

	// expire and now time
	const struct timespec*	expire	= reinterpret_cast<const struct timespec*>(pval);
	struct timespec			nowtime	= {0, 0};
	if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &nowtime)){
		WAN_K2HPRN("Failed to get clock time by errno(%d), but continue with time() result value...", errno);
		nowtime.tv_sec	= time(NULL);
		nowtime.tv_nsec	= 0L;
	}

	// check
	bool	is_expire = false;
	if(expire->tv_sec < nowtime.tv_sec){
		is_expire = true;
	}else if(expire->tv_sec == nowtime.tv_sec){
		if(expire->tv_nsec < nowtime.tv_nsec){
			is_expire = true;
		}
	}
	return is_expire;
}

bool K2hAttrBuiltin::GetTime(K2HAttrs& attrs, const char* key, struct timespec& time) const
{
	const unsigned char*	pval	= NULL;
	size_t					vallen	= 0;
	if(!GetAttr(attrs, key, &pval, vallen) || !pval){
		// attr does not have key
		return false;
	}
	if(sizeof(struct timespec) != vallen){
		ERR_K2HPRN("%s attribute value must be %zu byte, but value is %zu byte.", key, sizeof(struct timespec), vallen);
		return false;
	}

	// copy
	const struct timespec*	vtime	= reinterpret_cast<const struct timespec*>(pval);
	time.tv_sec						= vtime->tv_sec;
	time.tv_nsec					= vtime->tv_nsec;

	return true;
}

bool K2hAttrBuiltin::GetUniqId(K2HAttrs& attrs, bool is_parent, string& uniqid) const
{
	const char*	key		= is_parent ? K2hAttrBuiltin::ATTR_PUNIQID : K2hAttrBuiltin::ATTR_UNIQID;
	const char*	pval	= NULL;
	if(!GetAttr(attrs, key, &pval) || !pval){
		// attr does not have key
		return false;
	}
	uniqid = pval;
	return true;
}

bool K2hAttrBuiltin::GetEncryptKeyMd5(K2HAttrs& attrs, string* enckeymd5) const
{
	const char*	pval = NULL;
	if(!GetAttr(attrs, K2hAttrBuiltin::ATTR_AES256_PBKDF2, &pval) || !pval){		// check AES256 CBC PBKDF2 type
		if(!GetAttr(attrs, K2hAttrBuiltin::ATTR_AES256_MD5, &pval) || !pval){		// check AES256 CBC PBKDF1 type
			// attr does not have key
			return false;
		}
	}
	if(enckeymd5){
		*enckeymd5 = pval;
	}
	return true;
}

bool K2hAttrBuiltin::GetEncryptType(K2HAttrs& attrs, K2HATTR_ENC_TYPE& enctype) const
{
	const char*	pval	= NULL;
	bool		result	= false;
	if(GetAttr(attrs, K2hAttrBuiltin::ATTR_AES256_PBKDF2, &pval) && pval){			// AES256 CBC PBKDF2 type
		enctype	= K2H_ENC_AES256_PBKDF2;
		result	= true;
	}else if(GetAttr(attrs, K2hAttrBuiltin::ATTR_AES256_MD5, &pval) && pval){		// AES256 CBC PBKDF1 type
		enctype	= K2H_ENC_AES256_PBKDF1;
		result	= true;
	}
	return result;
}

// 
// This method returns copied the Value if the value is not encrypted.
// If the value is encrypted, there is following pattern.
// 1) if specify encpass, compare encpass's md5 and md5 in attrs.
//    if the result of comparison is correct, do decrypt value.
// 2) if not specify encpass, search attrs's md5 in loaded pass list.
//    if found it, do decrypt value.
// 3) if not found attrs's md5 in loaded pass list, check this object's
//    EncPass. if it has correct md5, do decrypt value.
//
unsigned char* K2hAttrBuiltin::GetDecryptValue(K2HAttrs& attrs, const char* encpass, size_t& declen) const
{
	if(!pBuiltinAttrPack){
		ERR_K2HPRN("object is not initialized.");
		return NULL;
	}
	unsigned char*	presult = NULL;
	declen					= 0;
	if(!byValue || 0 == ValLen){
		return presult;
	}

	K2HATTR_ENC_TYPE	enctype = K2H_ENC_AES256_PBKDF2;
	string				enckeymd5;
	if(GetEncryptType(attrs, enctype) && GetEncryptKeyMd5(attrs, enckeymd5)){
		// value is encrypted
		string	strPass;
		if(ISEMPTYSTR(encpass)){
			// not specify encrypt pass, so search pass from system loaded pass by md5
			if(pBuiltinAttrPack->EncPassMap.end() == pBuiltinAttrPack->EncPassMap.find(enckeymd5)){
				// not found in list, so compare object's EncPass
				if(EncPass.empty() || enckeymd5 != to_md5_string(EncPass.c_str())){
					ERR_K2HPRN("There is no match encrypt pass in list.");
					return presult;
				}
				strPass = EncPass;
			}else{
				strPass = pBuiltinAttrPack->EncPassMap[enckeymd5].strPass;
			}
		}else{
			// specified encrypt pass, so check it's md5
			if(enckeymd5 != to_md5_string(encpass)){
				ERR_K2HPRN("Specified pass is not as same as encrypted value's pass.");
				return presult;
			}
			strPass = encpass;
		}

		// do decrypt
		if(K2H_ENC_AES256_PBKDF1 == enctype){
			presult = k2h_decrypt_aes256_cbc(strPass.c_str(), byValue, ValLen, declen);
		}else{	// K2H_ENC_AES256_PBKDF2 == enctype
			presult = k2h_decrypt_aes256_cbc_pbkdf2(strPass.c_str(), byValue, ValLen, declen);
		}
		if(!presult){
			ERR_K2HPRN("Failed to decrypt value by pass.");
			return presult;
		}
	}else{
		// value is not encrypted, so copy value to result
		if(NULL == (presult = reinterpret_cast<unsigned char*>(malloc(ValLen)))){
			ERR_K2HPRN("Could not allocation memory.");
			return presult;
		}
		memcpy(presult, byValue, ValLen);
		declen = ValLen;
	}
	return presult;
}

bool K2hAttrBuiltin::IsHistory(K2HAttrs& attrs) const
{
	const unsigned char*	pval	= NULL;
	size_t					vallen	= 0;
	if(!GetAttr(attrs, K2hAttrBuiltin::ATTR_HISMARK, &pval, vallen) || !pval){
		// attr does not have history marker key
		return false;
	}
	if(sizeof(bool) != vallen){
		ERR_K2HPRN("%s attribute value must be %zu byte, but value is %zu byte.", K2hAttrBuiltin::ATTR_HISMARK, sizeof(bool), vallen);
		return false;
	}
	const bool*	is_hismark	= reinterpret_cast<const bool*>(pval);
	return *is_hismark;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
