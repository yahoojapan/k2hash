/*
 * K2HASH
 *
 * Copyright 2013 Yahoo! JAPAN corporation.
 *
 * K2HASH is key-valuew store base libraries.
 * K2HASH is made for the purpose of the construction of
 * original KVS system and the offer of the library.
 * The characteristic is this KVS library which Key can
 * layer. And can support multi-processing and multi-thread,
 * and is provided safely as available KVS.
 *
 * For the full copyright and license information, please view
 * the LICENSE file that was distributed with this source code.
 *
 * AUTHOR:   Takeshi Nakatani
 * CREATE:   Tue Dec 22 2015
 * REVISION:
 *
 */

#ifndef	K2HATTROPSMAN_H
#define	K2HATTROPSMAN_H

#include "k2hattrop.h"
#include "k2hattrbuiltin.h"
#include "k2hattrplugin.h"

//---------------------------------------------------------
// Class K2hAttrOpsMan
//---------------------------------------------------------
class K2hAttrOpsMan
{
	protected:
		static k2hattrlibmap_t		PluginLibs;				// all libraries list map by each shm

		k2hattroplist_t				attroplist;				// libraries list
		const unsigned char*		byKey;
		size_t						KeyLen;
		const unsigned char*		byValue;
		size_t						ValLen;
		const unsigned char*		byUpdateValue;
		size_t						UpdateValLen;

	public:
		// [NOTE]
		// Queue and KeyQueue need to mask attributes,
		// thus we use following enum value to Initialize() method from K2HShm class.
		//
		typedef enum attr_init_type{
			OPSMAN_MASK_NORMAL			= 0,
			OPSMAN_MASK_HIS_EXPIREKP,
			OPSMAN_MASK_ALL_NOT_EXPIREKP,
			OPSMAN_MASK_ALL,
			OPSMAN_MASK_QUEUEKEY		= OPSMAN_MASK_HIS_EXPIREKP,		// For normal K2HQueue's key
			OPSMAN_MASK_TRANSQUEUEKEY	= OPSMAN_MASK_ALL_NOT_EXPIREKP,	// For Transaction K2HQueue's key
			OPSMAN_MASK_KEYQUEUEKEY		= OPSMAN_MASK_ALL,				// For K2HKeyQueue's key
			OPSMAN_MASK_QUEUEMARKER		= OPSMAN_MASK_ALL,				// For Queue's marker
		}ATTRINITTYPE;

	protected:
		static bool RemoveBuiltinAttr(const K2HShm* pshm);
		static bool RemovePluginLib(k2hattrliblist_t* plist);
		static bool RemovePluginLib(const K2HShm* pshm);

		bool InitializeEx(const K2HShm* pshm, const unsigned char* pkey, size_t key_len, const unsigned char* pvalue, size_t value_len, const char* encpass, const time_t* expire, ATTRINITTYPE type);
		const K2hAttrBuiltin* GetAttrBuiltin(void) const;
		bool MarkHistoryEx(K2HAttrs& attrs, bool is_mark);

	public:
		static bool AddPluginLib(const K2HShm* pshm, const char* path);
		static bool InitializeCommonAttr(const K2HShm* pshm, const bool* is_mtime = NULL, const bool* is_defenc = NULL, const char* passfile = NULL, const bool* is_history = NULL, const time_t* expire = NULL, const strarr_t* pluginlibs = NULL);
		static bool CleanCommonAttr(const K2HShm* pshm);
		static bool GetVersionInfos(const K2HShm* pshm, strarr_t& verinfos);
		static bool IsMarkHistory(const K2HShm* pshm) { return K2hAttrBuiltin::IsMarkHistory(pshm); }
		static bool AddCryptPass(const K2HShm* pshm, const char* pPass, bool is_default_encrypt = false) { return K2hAttrBuiltin::AddCryptPass(pshm, pPass, is_default_encrypt); }

		K2hAttrOpsMan(void);
		virtual ~K2hAttrOpsMan(void);

		bool Clean(void);
		bool Initialize(const K2HShm* pshm, const unsigned char* pkey, size_t key_len, const unsigned char* pvalue, size_t value_len, const char* encpass = NULL, const time_t* expire = NULL, ATTRINITTYPE type = OPSMAN_MASK_NORMAL);
		bool GetVersionInfos(strarr_t& verinfos) const;
		void GetInfos(std::stringstream& ss) const;

		// after update
		bool UpdateAttr(K2HAttrs& attrs);
		bool MarkHistory(K2HAttrs& attrs) { return MarkHistoryEx(attrs, true); }
		bool UnmarkHistory(K2HAttrs& attrs) { return MarkHistoryEx(attrs, true); }
		bool DirectSetUniqId(K2HAttrs& attrs, const char* olduniqid = NULL);
		const unsigned char* GetValue(size_t& vallen) const;
		bool IsUpdateValue(void) const;
		const char* IsUpdateUniqID(void) const;

		// after load
		bool IsExpire(K2HAttrs& attrs) const;
		bool GetMTime(K2HAttrs& attrs, struct timespec& time) const;
		bool GetExpireTime(K2HAttrs& attrs, struct timespec& time) const;
		bool GetUniqId(K2HAttrs& attrs, std::string& uniqid) const;
		bool GetParentUniqId(K2HAttrs& attrs, std::string& uniqid) const;
		bool IsValueEncrypted(K2HAttrs& attrs) const;
		bool GetEncryptKeyMd5(K2HAttrs& attrs, std::string& enckeymd5) const;
		unsigned char* GetDecryptValue(K2HAttrs& attrs, const char* encpass, size_t& declen) const;
		bool IsHistory(K2HAttrs& attrs) const;
};

#endif	// K2HATTROPSMAN_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
