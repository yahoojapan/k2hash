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

#ifndef	K2HATTRBUILTIN_H
#define	K2HATTRBUILTIN_H

#include <map>
#include <string>
#include "k2hattrop.h"

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
#define	K2H_PPBKDF2_KEY_ITERATION_COUNT			1000	// default iteration count

typedef enum _k2hattr_enc_type{							// k2hattr encrypt type
	K2H_ENC_AES256_PBKDF1	= 0,						// AES256 CBC PAD with PBKDF1
	K2H_ENC_AES256_PBKDF2	= 1							// AES256 CBC PAD with PBKDF2
}K2HATTR_ENC_TYPE;

// [NOTE]
// In the future, k2hash could also support encryption of not AES256.
// In that case, it would be better to add the type of encryption to
// this structure.
// Now, k2hash sets "aes256md5" attribute which means AES256 type.
// Thus if k2hash need to support another encryption, should use
// another attribute key name for it(ex. sha256md5).
//
typedef struct k2h_encrypt_pass{
	std::string		strMD5;
	std::string		strPass;
}K2HENCPASS, *PK2HENCPASS;

typedef std::map<std::string, K2HENCPASS>		k2hepmap_t;

typedef struct k2h_builtin_attr_pack{
	bool				IsAttrMTime;			// whether stamp last modified time
	bool				IsAttrHistory;			// whether build versioning
	time_t				AttrExpireSec;			// expire second, -1 means not set
	bool				IsDefaultEncrypt;		// whether encode as default
	std::string			DefaultPassMD5;			// md5 for default encrypt pass
	int					IterCount;				// iteration count for PBKDF2 key
	K2HATTR_ENC_TYPE	EncType;				// encrypt type(default AES256 CBC with PBKDF2)
	k2hepmap_t			EncPassMap;				// map for encrypt pass

	k2h_builtin_attr_pack(void) : IsAttrMTime(false), IsAttrHistory(false), AttrExpireSec(-1), IsDefaultEncrypt(false), DefaultPassMD5(""), IterCount(K2H_PPBKDF2_KEY_ITERATION_COUNT), EncType(K2H_ENC_AES256_PBKDF2) { }	// -1 = K2hAttrBuiltin::NOT_EXPIRE
	~k2h_builtin_attr_pack(void) { }
}K2HBATTRPACK, *PK2HBATTRPACK;

typedef std::map<const K2HShm*, PK2HBATTRPACK>	k2hbapackmap_t;

//---------------------------------------------------------
// Class K2hAttrBuiltin
//---------------------------------------------------------
class K2hCryptContext;

class K2hAttrBuiltin : public K2hAttrOpsBase
{
	protected:
		// attr environment(const)
		static const char*		ATTR_ENV_MTIME;
		static const char*		ATTR_ENV_HISTORY;
		static const char*		ATTR_ENV_EXPIRE_SEC;
		static const char*		ATTR_ENV_DEFAULT_ENC;
		static const char*		ATTR_ENV_ENCFILE;
		static const char*		ATTR_ENV_ENC_TYPE;
		static const char*		ATTR_ENV_ENC_ITER;

		static const time_t		NOT_EXPIRE = -1;

	public:
		// const
		static const int		TYPE_ATTRBUILTIN = 1;
		static const char*		ATTR_BUILTIN_VERSION;

		// attr names
		static const char*		ATTR_MTIME;
		static const char*		ATTR_EXPIRE;
		static const char*		ATTR_UNIQID;
		static const char*		ATTR_PUNIQID;
		static const char*		ATTR_AES256_MD5;
		static const char*		ATTR_AES256_PBKDF2;
		static const char*		ATTR_HISMARK;

		// Mask values
		//
		// The mask is set, we do not set attribute for mask.
		// 
		// [NOTE]
		// ATTR_MASK_EXPIRE_KP means keeping expire time if k2hattrs is specified.
		// If k2hattrs is not specified, set new expire time from ExpireSec(object's) value.
		// Thus if ATTR_MASK_EXPIRE_KP is set, you must set ExpireSec value to object.
		//
		static const int		ATTR_MASK_NO		= 0;
		static const int		ATTR_MASK_MTIME		= 1;
		static const int		ATTR_MASK_ENCRYPT	= 1 << 1;
		static const int		ATTR_MASK_HISTORY	= 1 << 2;
		static const int		ATTR_MASK_EXPIRE	= 1 << 3;
		static const int		ATTR_MASK_EXPIRE_KP	= 1 << 4;		// Special mask

	protected:
		PK2HBATTRPACK			pBuiltinAttrPack;
		std::string				EncPass;
		time_t					ExpireSec;
		std::string				OldUniqID;							// Unique ID for old value after updating, it means to need to make history.
		int						AttrMask;							// attribute mask for temporary on the object

	protected:
		static K2hCryptContext& GetCryptLibContext(void);			// singleton for crypt library(initializer/destructor)
		static k2hbapackmap_t& GetAttrPackMap(void);				// builtin attribute setting map for each shm

		static PK2HBATTRPACK GetBuiltinAttrPack(const K2HShm* pshm);
		static bool ClearEncryptPassMap(PK2HBATTRPACK pPack);
		static int LoadEncryptPassMap(PK2HBATTRPACK pPack, const char* pfile);

		static bool RawInitializeEnv(PK2HBATTRPACK pPack);
		static bool RawInitialize(PK2HBATTRPACK pPack, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire);

		bool MarkHistoryEx(K2HAttrs& attrs, bool is_mark) const;

		bool GetTime(K2HAttrs& attrs, const char* key, struct timespec& time) const;
		bool GetUniqId(K2HAttrs& attrs, bool is_parent, std::string& uniqid) const;
		bool GetEncryptKeyMd5(K2HAttrs& attrs, std::string* enckeymd5) const;
		bool GetEncryptType(K2HAttrs& attrs, K2HATTR_ENC_TYPE& enctype) const;

	public:
		static bool CleanAttrBuiltin(const K2HShm* pshm);
		static bool Initialize(const K2HShm* pshm, const bool* is_mtime = NULL, const bool* is_defenc = NULL, const char* passfile = NULL, const bool* is_history = NULL, const time_t* expire = NULL);
		static bool IsInitialized(const K2HShm* pshm) { return (NULL != K2hAttrBuiltin::GetBuiltinAttrPack(pshm)); }
		static bool IsMarkHistory(const K2HShm* pshm);
		static bool AddCryptPass(const K2HShm* pshm, const char* pPass, bool is_default_encrypt = false);

		K2hAttrBuiltin(void);
		virtual ~K2hAttrBuiltin(void);

		virtual int GetType(void) const { return TYPE_ATTRBUILTIN; }
		virtual void Clear(void);
		virtual bool IsHandleAttr(const unsigned char* key, size_t keylen) const;
		virtual bool UpdateAttr(K2HAttrs& attrs);
		virtual void GetInfo(std::stringstream& ss) const;

		bool DirectSetUniqId(K2HAttrs& attrs, const char* olduniqid);
		bool MarkHistory(K2HAttrs& attrs) const { return MarkHistoryEx(attrs, true); }
		bool UnmarkHistory(K2HAttrs& attrs) const { return MarkHistoryEx(attrs, false); }
		bool Set(const K2HShm* pshm, const char* encpass, const time_t* expire, int mask = ATTR_MASK_NO);

		// methods using after updating attributes
		const char* GetOldUniqID(void) const { return (OldUniqID.empty() ? NULL : OldUniqID.c_str()); }

		// methods using after loading attribute
		bool IsExpire(K2HAttrs& attrs) const;
		bool GetMTime(K2HAttrs& attrs, struct timespec& time) const { return GetTime(attrs, ATTR_MTIME, time); }
		bool GetExpireTime(K2HAttrs& attrs, struct timespec& time) const { return GetTime(attrs, ATTR_EXPIRE, time); }
		bool GetUniqId(K2HAttrs& attrs, std::string& uniqid) const { return GetUniqId(attrs, false, uniqid); }
		bool GetParentUniqId(K2HAttrs& attrs, std::string& uniqid) const { return GetUniqId(attrs, true, uniqid); }
		bool IsValueEncrypted(K2HAttrs& attrs) const { return GetEncryptKeyMd5(attrs, NULL); }
		bool GetEncryptKeyMd5(K2HAttrs& attrs, std::string& enckeymd5) const { return GetEncryptKeyMd5(attrs, &enckeymd5); }
		unsigned char* GetDecryptValue(K2HAttrs& attrs, const char* encpass, size_t& declen) const;
		bool IsHistory(K2HAttrs& attrs) const;
};

#endif	// K2HATTRBUILTIN_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
