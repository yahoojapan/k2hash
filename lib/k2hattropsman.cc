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
 * CREATE:   Tue Dec 22 2015
 * REVISION:
 *
 */

#include "k2hcommon.h"
#include "k2hattropsman.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Utility
//---------------------------------------------------------
inline int GetBuiltinMaskValue(K2hAttrOpsMan::ATTRINITTYPE type)
{
	if(K2hAttrOpsMan::OPSMAN_MASK_ALL == type){
		return (K2hAttrBuiltin::ATTR_MASK_MTIME | K2hAttrBuiltin::ATTR_MASK_ENCRYPT | K2hAttrBuiltin::ATTR_MASK_HISTORY | K2hAttrBuiltin::ATTR_MASK_EXPIRE);
	}else if(K2hAttrOpsMan::OPSMAN_MASK_HIS_EXPIREKP == type){
		return (K2hAttrBuiltin::ATTR_MASK_HISTORY | K2hAttrBuiltin::ATTR_MASK_EXPIRE_KP);
	}else if(K2hAttrOpsMan::OPSMAN_MASK_TRANSQUEUEKEY == type){
		return (K2hAttrBuiltin::ATTR_MASK_MTIME | K2hAttrBuiltin::ATTR_MASK_ENCRYPT | K2hAttrBuiltin::ATTR_MASK_HISTORY | K2hAttrBuiltin::ATTR_MASK_EXPIRE_KP);
	}else if(K2hAttrOpsMan::OPSMAN_MASK_NORMAL == type){
		return K2hAttrBuiltin::ATTR_MASK_NO;
	}
	return K2hAttrBuiltin::ATTR_MASK_NO;
}

//---------------------------------------------------------
// K2hAttrOpsMan Class Methods
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
k2hattrlibmap_t& K2hAttrOpsMan::GetLibMap(void)
{
	static k2hattrlibmap_t	PluginLibs;					// singleton
	return PluginLibs;
}

bool K2hAttrOpsMan::InitializeCommonAttr(const K2HShm* pshm, const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire, const strarr_t* pluginlibs)
{
	if(!K2hAttrBuiltin::Initialize(pshm, is_mtime, is_defenc, passfile, is_history, expire)){
		ERR_K2HPRN("Failed to initialize builtin attribute by shm.");
		return false;
	}
	if(pluginlibs){
		for(strarr_t::const_iterator iter = pluginlibs->begin(); iter != pluginlibs->end(); ++iter){
			if(!K2hAttrOpsMan::AddPluginLib(pshm, iter->c_str())){
				ERR_K2HPRN("Failed to add attribute plugin library(%s), but continue...", iter->c_str());
			}
		}
	}
	return true;
}

bool K2hAttrOpsMan::RemoveBuiltinAttr(const K2HShm* pshm)
{
	return K2hAttrBuiltin::CleanAttrBuiltin(pshm);
}

bool K2hAttrOpsMan::AddPluginLib(const K2HShm* pshm, const char* path)
{
	if(!pshm || ISEMPTYSTR(path)){
		ERR_K2HPRN("parameters are wong.");
		return false;
	}

	K2hAttrPluginLib*	pLib = new K2hAttrPluginLib();
	if(!pLib->Load(path)){
		ERR_K2HPRN("Failed to load library(%s)", path);
		K2H_Delete(pLib);
		return false;
	}

	k2hattrlibmap_t::iterator	miter = K2hAttrOpsMan::GetLibMap().find(pshm);
	if(K2hAttrOpsMan::GetLibMap().end() == miter){
		// need to new list
		k2hattrliblist_t*	plist = new k2hattrliblist_t;

		plist->push_back(pLib);
		K2hAttrOpsMan::GetLibMap()[pshm] = plist;
	}else{
		k2hattrliblist_t*	plist = miter->second;
		if(!plist){
			// WHY?
			plist			= new k2hattrliblist_t;
			miter->second	= plist;
		}
		// check same library
		for(k2hattrliblist_t::const_iterator liter = plist->begin(); liter != plist->end(); ++liter){
			K2hAttrPluginLib*	ploaded = *liter;
			if(*ploaded == *pLib){
				// found same lib in list
				ERR_K2HPRN("found same attribute operation object(%s) in list.", ploaded->GetVersionInfo());
				K2H_Delete(pLib);
				return false;
			}
		}
		// append library
		plist->push_back(pLib);
	}
	return true;
}

bool K2hAttrOpsMan::RemovePluginLib(k2hattrliblist_t* plist)
{
	if(!plist){
		ERR_K2HPRN("parameter is wong.");
		return false;
	}
	for(k2hattrliblist_t::iterator liter = plist->begin(); liter != plist->end(); liter = plist->erase(liter)){
		K2hAttrPluginLib*	ploaded = *liter;
		K2H_Delete(ploaded);
	}
	return true;
}

bool K2hAttrOpsMan::RemovePluginLib(const K2HShm* pshm)
{
	if(!pshm){
		for(k2hattrlibmap_t::iterator miter = K2hAttrOpsMan::GetLibMap().begin(); miter != K2hAttrOpsMan::GetLibMap().end(); K2hAttrOpsMan::GetLibMap().erase(miter++)){
			k2hattrliblist_t*	plist	= miter->second;
			miter->second				= NULL;

			// remove all libraries in list
			if(!K2hAttrOpsMan::RemovePluginLib(plist)){
				ERR_K2HPRN("Could not remove libraries in list.");
				return false;
			}
			// delete list
			K2H_Delete(plist);
		}
	}else{
		// search
		k2hattrlibmap_t::iterator	miter = K2hAttrOpsMan::GetLibMap().find(pshm);
		if(K2hAttrOpsMan::GetLibMap().end() == miter){
			WAN_K2HPRN("There is no library list for shm.");
			return true;		// OK
		}
		k2hattrliblist_t*	plist	= miter->second;
		miter->second				= NULL;

		// remove all libraries in list
		if(!K2hAttrOpsMan::RemovePluginLib(plist)){
			ERR_K2HPRN("Could not remove libraries in list.");
			return false;
		}
		// delete list
		K2H_Delete(plist);

		// retrieve list from map
		K2hAttrOpsMan::GetLibMap().erase(miter);
	}
	return true;
}

bool K2hAttrOpsMan::CleanCommonAttr(const K2HShm* pshm)
{
	bool	result = true;
	if(!K2hAttrOpsMan::RemoveBuiltinAttr(pshm)){
		ERR_K2HPRN("Failed to clean builtin attribute by shm.");
		result = false;
	}
	if(!K2hAttrOpsMan::RemovePluginLib(pshm)){
		ERR_K2HPRN("Failed to clean plugin attribute libs by shm.");
		result = false;
	}
	return result;
}

bool K2hAttrOpsMan::GetVersionInfos(const K2HShm* pshm, strarr_t& verinfos)
{
	if(!pshm){
		ERR_K2HPRN("parameter is wong.");
		return false;
	}

	// builtin
	if(!K2hAttrBuiltin::IsInitialized(pshm)){
		MSG_K2HPRN("builtin attribute is not initialized yet.");
	}else{
		K2hAttrBuiltin	Builtin;
		verinfos.push_back(string(Builtin.GetVersionInfo()));
	}

	// plugin libs
	k2hattrlibmap_t::const_iterator	miter = K2hAttrOpsMan::GetLibMap().find(pshm);
	if(K2hAttrOpsMan::GetLibMap().end() != miter){
		const k2hattrliblist_t*	plist = miter->second;
		if(plist){
			for(k2hattrliblist_t::const_iterator liter = plist->begin(); liter != plist->end(); ++liter){
				const K2hAttrPluginLib*	ploaded = *liter;
				verinfos.push_back(string(ploaded->GetVersionInfo()));
			}
		}
	}
	return true;
}

//---------------------------------------------------------
// K2hAttrOpsMan Methods
//---------------------------------------------------------
K2hAttrOpsMan::K2hAttrOpsMan(void) : byKey(NULL), KeyLen(0), byValue(NULL), ValLen(0), byUpdateValue(NULL), UpdateValLen(0)
{
}

K2hAttrOpsMan::~K2hAttrOpsMan(void)
{
	Clean();
}

bool K2hAttrOpsMan::Clean(void)
{
	for(k2hattroplist_t::iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		K2hAttrOpsBase*	pAttrOp = *iter;
		K2H_Delete(pAttrOp);
	}
	attroplist.clear();
	return true;
}

//
// Queue and KeyQueue need to mask attributes because their keys does not need history etc.
// The marker key does not need any attribute, and the key for KeyQueue is as same as marker.
// Queue's Key does not need only history attribute.
// So we control these case by type parameter in this method.
//
bool K2hAttrOpsMan::Initialize(const K2HShm* pshm, const unsigned char* pkey, size_t key_len, const unsigned char* pvalue, size_t value_len, const char* encpass, const time_t* expire, K2hAttrOpsMan::ATTRINITTYPE type)
{
	if(!pshm){
		ERR_K2HPRN("parameter is wong.");
		return false;
	}

	Clean();

	if(K2hAttrOpsMan::OPSMAN_MASK_ALL != type){
		// if has lib list in map, make attr plugin objects from plugin library list, and push object list.
		k2hattrlibmap_t::const_iterator	miter = K2hAttrOpsMan::GetLibMap().find(pshm);
		if(K2hAttrOpsMan::GetLibMap().end() != miter){
			const k2hattrliblist_t*	plist = miter->second;

			if(plist){
				for(k2hattrliblist_t::const_iterator liter = plist->begin(); liter != plist->end(); ++liter){
					const K2hAttrPluginLib*	ploaded = *liter;
					K2hAttrPlugin*			pPlugin = new K2hAttrPlugin(ploaded);

					if(!pPlugin->Set(pkey, key_len, pvalue, value_len)){
						ERR_K2HPRN("Failed to set key and value pointer to attr plugin(%s).", ploaded->GetVersionInfo());
						K2H_Delete(pPlugin);
						Clean();
						return false;
					}
					if(!K2hAttrOpsBase::AddAttrOpArray(attroplist, pPlugin)){
						ERR_K2HPRN("Failed to adding attr plugin(%s).", ploaded->GetVersionInfo());
						K2H_Delete(pPlugin);
						Clean();
						return false;
					}
				}
			}
		}
	}

	// check builtin initialized
	if(!K2hAttrBuiltin::IsInitialized(pshm)){
		MSG_K2HPRN("builtin attribute is not initialized yet, so initialize it here.");

		if(!K2hAttrBuiltin::Initialize(pshm)){
			ERR_K2HPRN("Could not initialize to builtin attribute.");
			Clean();
			return false;
		}
	}

	// make builtin and push it
	//
	// [NOTE]
	// Builtin attribute plugin must be added lastest position in list.
	// Because the plugin changes value for encrypting when the flag is enabled.
	//
	K2hAttrBuiltin*	pBuiltin = new K2hAttrBuiltin();

	if(pkey && 0 != key_len){
		K2hAttrOpsBase*	pBaseOps = pBuiltin;
		if(!pBaseOps->Set(pkey, key_len, pvalue, value_len)){
			ERR_K2HPRN("Failed to set key and value pointer to builtin attr plugin(%s).", pBuiltin->GetVersionInfo());
			K2H_Delete(pBuiltin);
			Clean();
			return false;
		}
	}
	int	BuiltinAttrMask = GetBuiltinMaskValue(type);
	if(!pBuiltin->Set(pshm, encpass, expire, BuiltinAttrMask)){
		ERR_K2HPRN("Could not initialize to builtin attribute object.");
		K2H_Delete(pBuiltin);
		Clean();
		return false;
	}
	if(!K2hAttrOpsBase::AddAttrOpArray(attroplist, pBuiltin)){
		ERR_K2HPRN("Failed to adding builtin attr plugin(%s).", pBuiltin->GetVersionInfo());
		K2H_Delete(pBuiltin);
		Clean();
		return false;
	}

	// set key & value
	byKey	= pkey;
	KeyLen	= key_len;
	byValue	= pvalue;
	ValLen	= value_len;

	return true;
}

bool K2hAttrOpsMan::GetVersionInfos(strarr_t& verinfos) const
{
	for(k2hattroplist_t::const_iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		const K2hAttrOpsBase*	pAttrOp 	= *iter;
		const char*				pVerInfo	= pAttrOp ? pAttrOp->GetVersionInfo() : NULL;
		if(!ISEMPTYSTR(pVerInfo)){
			verinfos.push_back(string(pVerInfo));
		}
	}
	return true;
}

void K2hAttrOpsMan::GetInfos(stringstream& ss) const
{
	for(k2hattroplist_t::const_iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		const K2hAttrOpsBase*	pAttrOp = *iter;
		if(pAttrOp){
			pAttrOp->GetInfo(ss);
		}else{
			ss << "Attribute library Version:              Unknown(something wrong in program)" << endl;
		}
	}
}

bool K2hAttrOpsMan::UpdateAttr(K2HAttrs& attrs)
{
	byUpdateValue	= byValue;
	UpdateValLen	= ValLen;
	for(k2hattroplist_t::iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		K2hAttrOpsBase*	pAttrOp = *iter;

		// set basical data
		if(!pAttrOp || !pAttrOp->Set(byKey, KeyLen, byUpdateValue, UpdateValLen)){
			ERR_K2HPRN("Could not set basical data to attribute operation object(%s).", pAttrOp ? pAttrOp->GetVersionInfo() : "unknown");
			return false;
		}

		// update
		if(!pAttrOp->UpdateAttr(attrs)){
			ERR_K2HPRN("Could not update attribute by attribute operation object(%s).", pAttrOp->GetVersionInfo());
			return false;
		}

		// get updated value
		byUpdateValue = pAttrOp->GetValue(UpdateValLen);
	}
	return true;
}

bool K2hAttrOpsMan::MarkHistoryEx(K2HAttrs& attrs, bool is_mark)
{
	for(k2hattroplist_t::iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		// search builtin attribute object
		K2hAttrOpsBase*	pAttrOp = *iter;

		if(pAttrOp && K2hAttrBuiltin::TYPE_ATTRBUILTIN == pAttrOp->GetType()){
			// found
			K2hAttrBuiltin*	pAttrBuilt = dynamic_cast<K2hAttrBuiltin*>(pAttrOp);
			if(pAttrBuilt){
				if(is_mark){
					return pAttrBuilt->MarkHistory(attrs);
				}else{
					return pAttrBuilt->UnmarkHistory(attrs);
				}
			}
			ERR_K2HPRN("Could not down cast base to builtin attribute object, why...");
		}
	}
	return false;
}

bool K2hAttrOpsMan::DirectSetUniqId(K2HAttrs& attrs, const char* olduniqid)
{
	for(k2hattroplist_t::iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		// search builtin attribute object
		K2hAttrOpsBase*	pAttrOp = *iter;

		if(pAttrOp && K2hAttrBuiltin::TYPE_ATTRBUILTIN == pAttrOp->GetType()){
			// found
			K2hAttrBuiltin*	pAttrBuilt = dynamic_cast<K2hAttrBuiltin*>(pAttrOp);
			if(pAttrBuilt){
				return pAttrBuilt->DirectSetUniqId(attrs, olduniqid);
			}
			ERR_K2HPRN("Could not down cast base to builtin attribute object, why...");
		}
	}
	return false;
}

const K2hAttrBuiltin* K2hAttrOpsMan::GetAttrBuiltin(void) const
{
	for(k2hattroplist_t::const_iterator iter = attroplist.begin(); iter != attroplist.end(); ++iter){
		// search builtin attribute object
		const K2hAttrOpsBase*	pAttrOp = *iter;

		if(pAttrOp && K2hAttrBuiltin::TYPE_ATTRBUILTIN == pAttrOp->GetType()){
			// found
			const K2hAttrBuiltin*	pAttrBuilt = dynamic_cast<const K2hAttrBuiltin*>(pAttrOp);
			if(!pAttrBuilt){
				ERR_K2HPRN("Could not down cast base to builtin attribute object, why...");
			}
			return pAttrBuilt;
		}
	}
	return NULL;
}

bool K2hAttrOpsMan::IsUpdateValue(void) const
{
	return (byUpdateValue != byValue || UpdateValLen != ValLen);
}

const unsigned char* K2hAttrOpsMan::GetValue(size_t& vallen) const
{
	const unsigned char*	pValue;
	if(IsUpdateValue()){
		pValue	= byUpdateValue;
		vallen	= UpdateValLen;
	}else{
		pValue	= byValue;
		vallen	= ValLen;
	}
	return pValue;
}

//
// If you have to make history, this method returns old unique id which must be a part of history key.
//
const char* K2hAttrOpsMan::IsUpdateUniqID(void) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return NULL;
	}
	return pBuiltin->GetOldUniqID();
}

bool K2hAttrOpsMan::IsExpire(K2HAttrs& attrs) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->IsExpire(attrs);
}

bool K2hAttrOpsMan::GetMTime(K2HAttrs& attrs, struct timespec& time) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->GetMTime(attrs, time);
}

bool K2hAttrOpsMan::GetExpireTime(K2HAttrs& attrs, struct timespec& time) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->GetExpireTime(attrs, time);
}

bool K2hAttrOpsMan::GetUniqId(K2HAttrs& attrs, std::string& uniqid) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->GetUniqId(attrs, uniqid);
}

bool K2hAttrOpsMan::GetParentUniqId(K2HAttrs& attrs, std::string& uniqid) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->GetParentUniqId(attrs, uniqid);
}

bool K2hAttrOpsMan::IsValueEncrypted(K2HAttrs& attrs) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->IsValueEncrypted(attrs);
}

bool K2hAttrOpsMan::GetEncryptKeyMd5(K2HAttrs& attrs, std::string& enckeymd5) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->GetEncryptKeyMd5(attrs, enckeymd5);
}

unsigned char* K2hAttrOpsMan::GetDecryptValue(K2HAttrs& attrs, const char* encpass, size_t& declen) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return NULL;
	}
	return pBuiltin->GetDecryptValue(attrs, encpass, declen);
}

bool K2hAttrOpsMan::IsHistory(K2HAttrs& attrs) const
{
	const K2hAttrBuiltin*	pBuiltin = GetAttrBuiltin();
	if(!pBuiltin){
		ERR_K2HPRN("Could not get a builtin attribute object.");
		return false;
	}
	return pBuiltin->IsHistory(attrs);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
