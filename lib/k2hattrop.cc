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

#include "k2hcommon.h"
#include "k2hattrop.h"
#include "k2hattrbuiltin.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2hAttrOpsBase Class Variable
//---------------------------------------------------------
const int			K2hAttrOpsBase::TYPE_ATTROP;

//---------------------------------------------------------
// K2hAttrOpsBase Class Methods
//---------------------------------------------------------
//
// Add always to lastest in list.
//
bool K2hAttrOpsBase::AddAttrOpArray(k2hattroplist_t& attroplist, K2hAttrOpsBase* pattrop)
{
	if(!pattrop){
		ERR_K2HPRN("parameter is wrong.");
		return false;
	}
	for(k2hattroplist_t::const_iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		if((*iter)->VerInfo == pattrop->VerInfo){
			// found same attrop in list
			ERR_K2HPRN("found same attribute operation object(%s) in list.", pattrop->VerInfo.c_str());
			return false;
		}
	}
	attroplist.push_back(pattrop);

	return true;
}

//---------------------------------------------------------
// K2hAttrOpsBase Methods
//---------------------------------------------------------
K2hAttrOpsBase::K2hAttrOpsBase(void) : VerInfo(""), byKey(NULL), KeyLen(0), byValue(NULL), ValLen(0), byAllocValue(NULL)
{
}

K2hAttrOpsBase::~K2hAttrOpsBase(void)
{
	Clear();
}

void K2hAttrOpsBase::Clear(void)
{
	K2H_Free(byAllocValue);
	byKey	= NULL;
	KeyLen	= 0;
	byValue	= NULL;
	ValLen	= 0;
}

bool K2hAttrOpsBase::Set(const unsigned char* pkey, size_t key_len, const unsigned char* pvalue, size_t value_len)
{
	// [NOTE]
	// Clear only this class member.
	//
	Clear();

	if(pkey && 0 < key_len){
		byKey	= pkey;
		KeyLen	= key_len;
	}
	if(pvalue && 0 < value_len){
		byValue	= pvalue;
		ValLen	= value_len;
	}
	return true;
}

bool K2hAttrOpsBase::IsHandleAttr(const char* key) const
{
	return IsHandleAttr(reinterpret_cast<const unsigned char*>(key), (key ? strlen(key) + 1 : 0));
}

bool K2hAttrOpsBase::GetAttr(K2HAttrs& attrs, const unsigned char* key, size_t keylen, const unsigned char** ppval, size_t& vallen) const
{
	if(!key || 0 == keylen){
		ERR_K2HPRN("attribute key is empty.");
		return false;
	}
	K2HAttrs::iterator	iter = attrs.find(key, keylen);
	if(attrs.end() == iter){
		// attr does not have key
		return false;
	}
	if(ppval){
		*ppval = iter->pval;
		vallen = iter->vallength;
	}
	return true;
}

bool K2hAttrOpsBase::SetAttr(K2HAttrs& attrs, const unsigned char* key, size_t keylen, const unsigned char* val, size_t vallen) const
{
	return (attrs.end() != attrs.insert(key, keylen, val, vallen));
}

bool K2hAttrOpsBase::RemoveAttr(K2HAttrs& attrs, const unsigned char* key, size_t keylen) const
{
	K2HAttrs::iterator	iter = attrs.find(key, keylen);
	if(attrs.end() == iter){
		//MSG_K2HPRN("not found key in attribute list.");
		return false;
	}
	attrs.erase(iter);
	return true;
}

bool K2hAttrOpsBase::compare(const K2hAttrOpsBase& other) const
{
	return (GetType() == other.GetType());
}

void K2hAttrOpsBase::GetInfo(stringstream& ss) const
{
	ss << "Attribute library Version:              " << VerInfo << endl;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
