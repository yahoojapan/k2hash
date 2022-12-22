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

#ifndef	K2HATTROP_H
#define	K2HATTROP_H

#include <sstream>
#include "k2hattrs.h"

//---------------------------------------------------------
// Typedefs
//---------------------------------------------------------
class K2hAttrOpsBase;

typedef std::vector<K2hAttrOpsBase*>		k2hattroplist_t;

//---------------------------------------------------------
// Class K2hAttrOpsBase
//---------------------------------------------------------
class K2hAttrOpsBase
{
	protected:
		std::string				VerInfo;
		const unsigned char*	byKey;
		size_t					KeyLen;
		const unsigned char*	byValue;
		size_t					ValLen;
		unsigned char*			byAllocValue;

	public:
		static const int		TYPE_ATTROP = 0;

	protected:
		bool SetAttr(K2HAttrs& attrs, const unsigned char* key, size_t keylen, const unsigned char* val, size_t vallen) const;
		bool SetAttr(K2HAttrs& attrs, const char* key, const unsigned char* val, size_t vallen) const
		{
			return SetAttr(attrs, reinterpret_cast<const unsigned char*>(key), (key ? strlen(key) + 1 : 0), val, vallen);
		}
		bool SetAttr(K2HAttrs& attrs, const char* key, const char* val) const
		{
			return SetAttr(attrs, reinterpret_cast<const unsigned char*>(key), (key ? strlen(key) + 1 : 0), reinterpret_cast<const unsigned char*>(val), (val ? strlen(val) + 1 : 0));
		}
		bool RemoveAttr(K2HAttrs& attrs, const unsigned char* key, size_t keylen) const;
		bool RemoveAttr(K2HAttrs& attrs, const char* key) const
		{
			return RemoveAttr(attrs, reinterpret_cast<const unsigned char*>(key), (key ? strlen(key) + 1 : 0));
		}

	public:
		static bool AddAttrOpArray(k2hattroplist_t& attroplist, K2hAttrOpsBase* pattrop);

		K2hAttrOpsBase(void);
		virtual ~K2hAttrOpsBase(void);

		virtual int GetType(void) const { return TYPE_ATTROP; }
		virtual void Clear(void);
		virtual bool IsHandleAttr(const char* key) const;
		virtual bool IsHandleAttr(const unsigned char* key, size_t keylen) const = 0;
		virtual bool UpdateAttr(K2HAttrs& attrs) = 0;
		virtual void GetInfo(std::stringstream& ss) const;

		bool Set(const unsigned char* pkey, size_t key_len, const unsigned char* pvalue, size_t value_len);

		const char* GetVersionInfo(void) const { return VerInfo.c_str(); }
		const unsigned char* GetValue(size_t& vallen) const { vallen = ValLen; return byValue; }

		bool GetAttr(K2HAttrs& attrs, const unsigned char* key, size_t keylen, const unsigned char** ppval, size_t& vallen) const;
		bool GetAttr(K2HAttrs& attrs, const char* key, const unsigned char** ppval, size_t& vallen) const
		{
			return GetAttr(attrs, reinterpret_cast<const unsigned char*>(key), (key ? strlen(key) + 1 : 0), ppval, vallen);
		}
		bool GetAttr(K2HAttrs& attrs, const char* key, const char** ppval) const
		{
			size_t	vallen = 0;
			return GetAttr(attrs, reinterpret_cast<const unsigned char*>(key), (key ? strlen(key) + 1 : 0), reinterpret_cast<const unsigned char**>(ppval), vallen);
		}

		bool compare(const K2hAttrOpsBase& other) const;
		bool operator==(const K2hAttrOpsBase& other) const { return compare(other); }
};
#endif	// K2HATTROP_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
