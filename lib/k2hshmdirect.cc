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
 * CREATE:   Fri Jul 01 2016
 * REVISION:
 *
 */

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// For direct get/set methods
//---------------------------------------------------------
//
// Make element binary data
//
PBALLEDATA K2HShm::GetElementToBinary(PELEMENT pAbsElement) const
{
	if(!pAbsElement){
		ERR_K2HPRN("Paramter is wrong.");
		return NULL;
	}

	// Get pages
	unsigned char*	byKey		= NULL;
	unsigned char*	byValue		= NULL;
	unsigned char*	bySubKey	= NULL;
	unsigned char*	byAttr		= NULL;
	ssize_t			keylen		= Get(pAbsElement, &byKey, PAGEOBJ_KEY);
	ssize_t			vallen		= Get(pAbsElement, &byValue, PAGEOBJ_VALUE);
	ssize_t			subkeylen	= Get(pAbsElement, &bySubKey, PAGEOBJ_SUBKEYS);
	ssize_t			attrlen		= Get(pAbsElement, &byAttr, PAGEOBJ_ATTRS);

	if(!byKey && !byValue && !bySubKey && !byAttr){
		ERR_K2HPRN("pAbsElement(%p) does not have any page data.", pAbsElement);
		return NULL;
	}

	// make binary
	PBALLEDATA	pBinAll;
	if(NULL == (pBinAll = reinterpret_cast<PBALLEDATA>(malloc(sizeof(RALLEDATA) + (keylen < 0 ? 0 : static_cast<size_t>(keylen)) + (vallen < 0 ? 0 : static_cast<size_t>(vallen)) + (subkeylen < 0 ? 0 : static_cast<size_t>(subkeylen)) + (attrlen < 0 ? 0 : static_cast<size_t>(attrlen)))))){
		ERR_K2HPRN("Could not allocate memory.");
		return NULL;
	}

	// set
	pBinAll->rawdata.hash			= pAbsElement->hash;
	pBinAll->rawdata.subhash		= pAbsElement->subhash;
	pBinAll->rawdata.key_length		= keylen < 0	? 0 : static_cast<size_t>(keylen);
	pBinAll->rawdata.val_length		= vallen < 0	? 0 : static_cast<size_t>(vallen);
	pBinAll->rawdata.skey_length	= subkeylen < 0	? 0 : static_cast<size_t>(subkeylen);
	pBinAll->rawdata.attrs_length	= attrlen < 0	? 0 : static_cast<size_t>(attrlen);
	pBinAll->rawdata.key_pos		=								static_cast<off_t>(sizeof(RALLEDATA));
	pBinAll->rawdata.val_pos		= pBinAll->rawdata.key_pos +	static_cast<off_t>(pBinAll->rawdata.key_length);
	pBinAll->rawdata.skey_pos		= pBinAll->rawdata.val_pos +	static_cast<off_t>(pBinAll->rawdata.val_length);
	pBinAll->rawdata.attrs_pos		= pBinAll->rawdata.skey_pos +	static_cast<off_t>(pBinAll->rawdata.skey_length);

	// copy binary data after structure extended area
	unsigned char*	pbintop = reinterpret_cast<unsigned char*>(&(pBinAll->rawdata));
	if(0 < pBinAll->rawdata.key_length){
		memcpy((pbintop + pBinAll->rawdata.key_pos), byKey, pBinAll->rawdata.key_length);
	}
	if(0 < pBinAll->rawdata.val_length){
		memcpy((pbintop + pBinAll->rawdata.val_pos), byValue, pBinAll->rawdata.val_length);
	}
	if(0 < pBinAll->rawdata.skey_length){
		memcpy((pbintop + pBinAll->rawdata.skey_pos), bySubKey, pBinAll->rawdata.skey_length);
	}
	if(0 < pBinAll->rawdata.attrs_length){
		memcpy((pbintop + pBinAll->rawdata.attrs_pos), byAttr, pBinAll->rawdata.attrs_length);
	}

	// free
	K2H_Free(byKey);
	K2H_Free(byValue);
	K2H_Free(bySubKey);
	K2H_Free(byAttr);

	return pBinAll;
}

//
// Make elements binary data under relative element pointer
//
PK2HBIN K2HShm::GetElementListToBinary(PELEMENT pRelElement, size_t* pdatacnt, const struct timespec* pstartts, const struct timespec* pendts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check) const
{
	if(!pRelElement || !pdatacnt){
		ERR_K2HPRN("Paramters are wrong.");
		return NULL;
	}
	*pdatacnt = 0;

	PELEMENT	pElement = static_cast<PELEMENT>(Abs(pRelElement));
	if(!pElement){
		ERR_K2HPRN("Absolute pElement converted from pRelElement(%p) is NULL.", pRelElement);
		return NULL;
	}

	// build end of target(old) hash value
	k2h_hash_t	end_target_hash	= (target_hash	+ ((0 < target_hash_range) ? (target_hash_range - 1) : 0)) % target_max_hash;
	k2h_hash_t	end_old_hash	= (static_cast<k2h_hash_t>(-1) == old_hash) ? static_cast<k2h_hash_t>(-1) : ((old_hash + ((0 < target_hash_range) ? (target_hash_range - 1) : 0)) % old_max_hash);

	// check whether current element is target.
	bool	is_target = true;
	if(target_hash <= end_target_hash){
		if((pElement->hash % target_max_hash) < target_hash || end_target_hash < (pElement->hash % target_max_hash)){
			is_target = false;	// not target hash range
		}
	}else{
		if(end_target_hash < (pElement->hash % target_max_hash) && (pElement->hash % target_max_hash) < target_hash){
			is_target = false;	// not target hash range
		}
	}

	// own element to binary
	PBALLEDATA	pElementBin	= is_target ? GetElementToBinary(pElement) : NULL;

	// check own mtime and expire
	if(pElementBin && (pstartts || pendts || is_expire_check)){
		K2HAttrs*	pAttr;
		if(NULL != (pAttr = GetAttrs(pElement))){
			// [NOTE]
			// We need only to check expire flag, then we do not need to fill all parameters for initialize().
			//
			K2hAttrOpsMan	attrman;
			if(attrman.Initialize(this, NULL, 0UL, NULL, 0)){
				// check expire
				if(is_expire_check && attrman.IsExpire(*pAttr)){
					// element is expired.
					MSG_K2HPRN("the key is expired, so we do not wirte this element.");
					K2H_Free(pElementBin);

				}
				// get mtime and check it
				if(pElementBin && (pstartts || pendts)){
					struct timespec	mtime;
					if(!attrman.GetMTime(*pAttr, mtime)){
						// element does not have mtime attribute.
						//MSG_K2HPRN("probabry, the key dose not have mtime, so we write this element.");

					}else{
						// [NOTE]
						// If test_hash is in old hash range, it means the caller already has this element.
						// On this case, we check start time and element's mtime.
						// The other hand, we do not check start time, thus we always check only end time.
						//
						bool	need_check_start = true;
						if(old_hash <= end_old_hash){
							if((pElement->hash % old_max_hash) < old_hash || end_old_hash < (pElement->hash % old_max_hash)){
								// pElement->hash is not in old hash range, so we need to get all these elements without checking start time.
								need_check_start = false;
							}
						}else{
							if(end_old_hash < (pElement->hash % old_max_hash) && (pElement->hash % old_max_hash) < old_hash){
								// pElement->hash is not in old hash range, so we need to get all these elements without checking start time.
								need_check_start = false;
							}
						}

						// check mtime
						if(need_check_start && pstartts && (mtime.tv_sec < pstartts->tv_sec || (mtime.tv_sec == pstartts->tv_sec && (mtime.tv_nsec < pstartts->tv_nsec)))){
							// older than start time
							MSG_K2HPRN("the element mtime is older then start time, so we do not write this element.");
							K2H_Free(pElementBin);
						}else if(pendts && (pendts->tv_sec < mtime.tv_sec || (pendts->tv_sec == mtime.tv_sec && (pendts->tv_nsec < mtime.tv_nsec)))){
							// newer than end time
							MSG_K2HPRN("the element mtime is newer then end time, so we do not write this element.");
							K2H_Free(pElementBin);
						}
					}
				}
			}else{
				MSG_K2HPRN("Something error occurred during initializing attributes manager class, but continue to write this element.");
			}
			K2H_Delete(pAttr);
		}
	}

	// other(same/small/big) elements to binary array.(reentrant)
	size_t		SameCnt	= 0;
	PK2HBIN		pSameBin	= pElement->same ? GetElementListToBinary(pElement->same, &SameCnt, pstartts, pendts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check) : NULL;
	size_t		SmallCnt	= 0;
	PK2HBIN		pSmallBin	= pElement->small ? GetElementListToBinary(pElement->small, &SmallCnt, pstartts, pendts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check) : NULL;
	size_t		BigCnt		= 0;
	PK2HBIN		pBigBin		= pElement->big ? GetElementListToBinary(pElement->big, &BigCnt, pstartts, pendts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check) : NULL;

	// check
	if(!pElementBin && 0 == SameCnt && 0 == SmallCnt && 0 == BigCnt){
		// there is no binary data.
		ERR_K2HPRN("There is no element under pRelElement(%p), this case is something wrong.", pRelElement);
		return NULL;
	}

	// make binary array
	PK2HBIN	pResultBin;
	if(NULL == (pResultBin = reinterpret_cast<PK2HBIN>(malloc(sizeof(K2HBIN) * ((pElementBin ? 1 : 0) + SameCnt + SmallCnt + BigCnt))))){
		ERR_K2HPRN("Could not allocate memory.");
		K2H_Free(pElementBin);
		if(pSameBin){
			free_k2hbins(pSameBin, SameCnt);
		}
		if(pSmallBin){
			free_k2hbins(pSmallBin, SmallCnt);
		}
		if(pBigBin){
			free_k2hbins(pBigBin, BigCnt);
		}
		return NULL;
	}

	// copy
	*pdatacnt = 0;
	if(pElementBin){
		pResultBin[*pdatacnt].byptr		= reinterpret_cast<unsigned char*>(pElementBin);
		pResultBin[*pdatacnt].length	= calc_ralledata_length(pElementBin->rawdata);
		++(*pdatacnt);
	}
	for(size_t cnt = 0; cnt < SameCnt; ++cnt, ++(*pdatacnt)){
		pResultBin[*pdatacnt].byptr		= pSameBin[cnt].byptr;
		pResultBin[*pdatacnt].length	= pSameBin[cnt].length;
	}
	for(size_t cnt = 0; cnt < SmallCnt; ++cnt, ++(*pdatacnt)){
		pResultBin[*pdatacnt].byptr		= pSmallBin[cnt].byptr;
		pResultBin[*pdatacnt].length	= pSmallBin[cnt].length;
	}
	for(size_t cnt = 0; cnt < BigCnt; ++cnt, ++(*pdatacnt)){
		pResultBin[*pdatacnt].byptr		= pBigBin[cnt].byptr;
		pResultBin[*pdatacnt].length	= pBigBin[cnt].length;
	}

	// free
	K2H_Free(pSameBin);
	K2H_Free(pSmallBin);
	K2H_Free(pBigBin);

	return pResultBin;
}

//
// K2HShm::GetElementsByHash
//
//	starthash			: [IN]	start hash value to search in k2hash
//	startts				: [IN]	limit of update time start range
//	endts				: [IN]	limit of update time end range
//	target_hash			: [IN]	search target hash start value in target_max_hash
//	target_max_hash		: [IN]	maximum count of rounded circle hash value(= maximum hash vallue + 1)
//	old_hash			: [IN]	old target hash in old_max_hash
//	old_max_hash		: [IN]	maximum count of old rounded circle hash value(= maximum hash vallue + 1)
//	target_hash_range	: [IN]	search target hash range(count) from start value(this value must be as same as old)
//  is_expire_check		: [IN]  whether checking expire time
//	pnexthash			: [OUT] start hash value for next search
//	ppbindatas			: [OUT] binary data array of search result
//	pdatacnt			: [OUT] binary data array count of search result
//
//	return				true	finish without error.
//								if there is no result of search, return true with NULL pointer for *ppbindatas and
//								0 for *pnexthash and 0 for *pdatacnt.
//						false	something error occured
//
bool K2HShm::GetElementsByHash(const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t* pnexthash, PK2HBIN* ppbindatas, size_t* pdatacnt) const
{
	if(!pnexthash || !ppbindatas || !pdatacnt){
		ERR_K2HPRN("Paramters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	*pnexthash	= 0;
	*ppbindatas	= NULL;
	*pdatacnt	= 0UL;

	// Current maximum hash value on current k2hash.
	k2h_hash_t	cur_max_hash = (pHead->cur_mask << K2HShm::GetMaskBitCount(pHead->collision_mask)) | pHead->collision_mask;

	//
	// Loop for finding target element(from starthash to cur_max_hash)
	//
	K2HLock		ALObjCKI(K2HLock::RDLOCK);			// LOCK
	for(k2h_hash_t test_hash = starthash; test_hash <= cur_max_hash; ++test_hash){
		// get element(CKIndex) for test_hash
		PCKINDEX	pCKIndex = NULL;
		if(NULL != (pCKIndex = GetCKIndex(test_hash, ALObjCKI)) && pCKIndex->element_list){
			// get elements to binary data
			//
			if(NULL != (*ppbindatas = GetElementListToBinary(pCKIndex->element_list, pdatacnt, &startts, &endts, target_hash, target_max_hash, old_hash, old_max_hash, target_hash_range, is_expire_check))){
				//MSG_K2HPRN("Found and get element data.");
				++test_hash;
				if(test_hash <= cur_max_hash){
					*pnexthash = test_hash;
				}else{
					MSG_K2HPRN("Found and get LASTEST element data, thus this is last data.");
					*pnexthash = 0UL;
				}
				return true;
			}
			MSG_K2HPRN("Found element, but it is not target element(expierd or out of time range), so search next...");
		}
		ALObjCKI.Unlock();							// UNLOCK
	}

	// Not found target element.
	MSG_K2HPRN("There is no element data after target hash.");
	*pnexthash = 0;

	return true;
}

//
// K2HShm::SetElementByBinArray
//
//	prawdata			: [IN]	element raw data
//	pts					: [IN]	expiration time for prawdata
//
//	return				true	success
//						false	something error occured
//
// [NOTE]
// This method does not make history, even if it has been a valid setting.
// And not make transaction too.
//
bool K2HShm::SetElementByBinArray(const PRALLEDATA prawdata, const struct timespec* pts)
{
	if(!prawdata || !pts){
		ERR_K2HPRN("Paramters are wrong.");
		return false;
	}

	// check length
	size_t	dataslen;
	if(0 == (dataslen = calc_ralledata_datas_length(*prawdata))){
		ERR_K2HPRN("prawdata(%p) datas length is 0", prawdata);
		return false;
	}
	if(0 == prawdata->key_length){
		ERR_K2HPRN("the key in prawdata(%p) is empty", prawdata);
		return false;
	}

	// get datas from rawdata
	unsigned char*	byKey		= reinterpret_cast<unsigned char*>(prawdata) + prawdata->key_pos;
	unsigned char*	byValue		= 0 < prawdata->val_length ? (reinterpret_cast<unsigned char*>(prawdata) + prawdata->val_pos) : NULL;
	unsigned char*	bySubkeys	= 0 < prawdata->skey_length ? (reinterpret_cast<unsigned char*>(prawdata) + prawdata->skey_pos) : NULL;
	unsigned char*	byAttrs		= 0 < prawdata->attrs_length ? (reinterpret_cast<unsigned char*>(prawdata) + prawdata->attrs_pos) : NULL;

	// check key exists and check to allow over writing
	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pCKIndex	= NULL;
	PELEMENT	pElementList= NULL;
	PELEMENT	pElement	= NULL;
	if(NULL == (pCKIndex = GetCKIndex(prawdata->hash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}
	if(	NULL != (pElementList = GetElementList(pCKIndex, prawdata->hash, prawdata->subhash))&&
		NULL != (pElement = GetElement(pElementList, byKey, prawdata->key_length)) 			)
	{
		// key exists, so we need to check mtime and expire time
		K2HAttrs*	pExistAttr;
		if(NULL != (pExistAttr = GetAttrs(pElement))){
			K2hAttrOpsMan	attrman;
			if(!attrman.Initialize(this, byKey, prawdata->key_length, NULL, 0UL, NULL)){
				ERR_K2HPRN("Something error occurred during initializing attributes manager class.");
				return false;
			}

			// check expire
			if(attrman.IsExpire(*pExistAttr)){
				MSG_K2HPRN("the key is expired, so we can over write this key.");
			}else{
				// get mtime and check it
				struct timespec	mtime;
				if(!attrman.GetMTime(*pExistAttr, mtime)){
					// [NOTE]
					// If existed key does not have mtime, so we can not compare mtime.
					// Thus we always over write by requested key.
					//
					MSG_K2HPRN("probabry, the key dose not have mtime, so we over write this key.");
				}else{
					if(pts->tv_sec < mtime.tv_sec || (pts->tv_sec == mtime.tv_sec && pts->tv_nsec <= mtime.tv_nsec)){
						MSG_K2HPRN("the existing key mtime is newer than requested key, so we do not over write it.");
						return true;
					}
					MSG_K2HPRN("the existing key mtime is older then merge start time, so we continue to check next.");

					//
					// compare requested key's mtime and existed key's one.
					//
					K2hAttrOpsMan	NewAttrMan;
					K2HAttrs		NewKeyAttr;
					struct timespec	new_mtime = {0, 0};
					if(byAttrs && NewAttrMan.Initialize(this, NULL, 0UL, NULL, 0) && NewKeyAttr.Serialize(byAttrs, prawdata->attrs_length) && NewAttrMan.GetMTime(NewKeyAttr, new_mtime)){
						// new set key has attributes and mtime, so compare existed key mtime.
						if(new_mtime.tv_sec < mtime.tv_sec || (new_mtime.tv_sec == mtime.tv_sec && new_mtime.tv_nsec <= mtime.tv_nsec)){
							MSG_K2HPRN("the existing key mtime is newer than requested key, so we must not over write it.");
							return true;
						}
						MSG_K2HPRN("the existing key mtime is older then requested key, so we over write it.");
					}else{
						// [NOTE]
						// If existed key has mtime but requested key does not have it, we should not
						// over write it.
						//
						MSG_K2HPRN("the existing key has mtime, but requested key does not have it. so we should not over write it.");
						return true;
					}
					MSG_K2HPRN("the existing key mtime is older then requested key, so we over write it.");
				}
			}
			K2H_Delete(pExistAttr);
		}
	}

	// make new element if datas are existed
	PELEMENT	pNewElement = NULL;
	if(byValue || bySubkeys || byAttrs){
		if(NULL == (pNewElement = AllocateElement(prawdata->hash, prawdata->subhash, byKey, prawdata->key_length, byValue, prawdata->val_length, bySubkeys, prawdata->skey_length, byAttrs, prawdata->attrs_length))){
			ERR_K2HPRN("Failed to allocate new element and to set datas to it.");
			return false;
		}
	}

	// if old element exists, remove it
	if(pElement){
		// take off old element from ckindex
		if(!TakeOffElement(pCKIndex, pElement)){
			ERR_K2HPRN("Failed to take off old element from ckey index.");
			FreeElement(pNewElement);
			return false;
		}
		// remove old element
		if(!FreeElement(pElement)){
			ERR_K2HPRN("Failed to free old element.");
			FreeElement(pNewElement);
			return false;
		}
	}

	// Insert new element(if new is not empty)
	if(pNewElement){
		if(pCKIndex->element_list){
			if(!InsertElement(static_cast<PELEMENT>(Abs(pCKIndex->element_list)), pNewElement)){
				ERR_K2HPRN("Failed to insert new element");
				FreeElement(pNewElement);
				return false;
			}
		}else{
			pCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pNewElement));
		}
		pCKIndex->element_count	+= 1UL;
	}

	ALObjCKI.Unlock();								// Unlock

	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
