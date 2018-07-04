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
 * CREATE:   Fri Dec 2 2013
 * REVISION:
 *
 */

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hfind.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
K2HIterator::K2HIterator() : pShmUpdater(NULL)
{
	Initialize(false);
}

K2HIterator::K2HIterator(const K2HShm* pk2hshm, bool isSubKey) : pShmUpdater(NULL)
{
	Reset(pk2hshm, isSubKey);
}

K2HIterator::K2HIterator(const K2HShm* pk2hshm, PELEMENT pElement, const K2HLock& lockobj) : pShmUpdater(NULL)
{
	Reset(pk2hshm, pElement, lockobj);
}

K2HIterator::K2HIterator(const K2HShm* pk2hshm, PELEMENT pElement, const K2HSubKeys* pSKeys, const K2HSubKeys::iterator& iter, const K2HLock& lockobj) : pShmUpdater(NULL)
{
	Reset(pk2hshm, pElement, pSKeys, iter, lockobj);
}

K2HIterator::K2HIterator(const K2HIterator& other) : pShmUpdater(NULL)
{
	if(other.isOnlySubKey){
		Reset(other.pK2HShm, other.pAbsElement, &(other.SubKeys), other.sk_iter, other.ALObjCKI);
	}else{
		Reset(other.pK2HShm, other.pAbsElement, other.ALObjCKI);
	}
}

K2HIterator::~K2HIterator()
{
	ALObjCKI.Unlock();
	K2H_Delete(pShmUpdater);
}

void K2HIterator::Initialize(bool isSubKey)
{
	pK2HShm					= NULL;
	isOnlySubKey			= isSubKey;
	pAbsElement				= NULL;
	SubKeys.clear();
	sk_iter					= SubKeys.end();

	EmptyElement.small		= NULL;
	EmptyElement.big		= NULL;
	EmptyElement.parent		= NULL;
	EmptyElement.same		= NULL;
	EmptyElement.hash		= 0;
	EmptyElement.subhash	= 0;
	EmptyElement.key		= NULL;
	EmptyElement.value		= NULL;
	EmptyElement.subkeys	= NULL;
	EmptyElement.keylength	= 0UL;
	EmptyElement.vallength	= 0UL;
	EmptyElement.skeylength	= 0UL;
	pEmptyElement			= &EmptyElement;

	K2H_Delete(pShmUpdater);
}

bool K2HIterator::Reset(const K2HShm* pk2hshm, bool isSubKey)
{
	ALObjCKI.Unlock();
	Initialize(isSubKey);

	if(pk2hshm){
		pK2HShm = pk2hshm;
	}
	return true;
}

bool K2HIterator::Reset(const K2HShm* pk2hshm, PELEMENT pElement, const K2HLock& lockobj)
{
	if(!pk2hshm){
		ERR_K2HPRN("Not initializing this object.");
		return false;
	}
	Initialize(false);

	pK2HShm			= pk2hshm;
	pAbsElement		= pElement;
	ALObjCKI		= lockobj;							// LOCK
	pShmUpdater		= new K2HShmUpdater();
	return true;
}

bool K2HIterator::Reset(const K2HShm* pk2hshm, PELEMENT pElement, const K2HSubKeys* pSKeys, const K2HSubKeys::iterator& iter, const K2HLock& lockobj)
{
	if(!pk2hshm){
		ERR_K2HPRN("Not initializing this object.");
		return false;
	}
	Initialize(true);

	pK2HShm			= pk2hshm;
	pAbsElement		= pElement;
	ALObjCKI		= lockobj;							// LOCK
	pShmUpdater		= new K2HShmUpdater();

	if(pSKeys){
		SubKeys		= *pSKeys;
		sk_iter		= SubKeys.find(iter->pSubKey, iter->length);
	}else{
		SubKeys.clear();
		sk_iter		= SubKeys.end();
	}
	return true;
}

bool K2HIterator::Next(void)
{
	if(!IsInit()){
		ERR_K2HPRN("Not initializing this object.");
		return false;
	}
	// Get next element
	PELEMENT	pNextElement;

	if(isOnlySubKey){
		if(sk_iter != SubKeys.end()){
			++sk_iter;
		}
		for(pNextElement = NULL; sk_iter != SubKeys.end(); ++sk_iter){
			if(NULL != (pNextElement = pK2HShm->GetElement(sk_iter->pSubKey, sk_iter->length, ALObjCKI))){
				break;
			}
			// Probably, removing subkey
			MSG_K2HPRN("Could not find subkey, probably remove it. continue search next.");
		}
		if(pNextElement){
			pAbsElement = pNextElement;
		}else{
			pAbsElement = NULL;
			ALObjCKI.Unlock();
			K2H_Delete(pShmUpdater);
		}
	}else{
		if(pAbsElement){
			// already end(), so nothing to do.
			if(NULL == (pNextElement = pK2HShm->FindNextElement(pAbsElement, ALObjCKI))){
				Reset(pK2HShm, false);		// Unlocking in reset function.
			}else{
				pAbsElement = pNextElement;
			}
		}
	}
	return true;
}

K2HIterator& K2HIterator::operator++(void)
{
	if(!Next()){
		WAN_K2HPRN("Something error occurred.");
	}
	return *this;
}

K2HIterator K2HIterator::operator++(int)
{
	K2HIterator	result = *this;

	if(!Next()){
		WAN_K2HPRN("Something error occurred.");
	}
	return result;
}

PELEMENT& K2HIterator::operator*(void)
{
	if(!IsInit()){
		return pEmptyElement;
	}
	if(!pAbsElement){	// == end()
		return pEmptyElement;
	}
	return pAbsElement;
}

PELEMENT* K2HIterator::operator->(void)
{
	if(!IsInit()){
		return &pEmptyElement;
	}
	if(!pAbsElement){	// == end()
		return &pEmptyElement;
	}
	return &pAbsElement;
}

bool K2HIterator::operator==(const K2HIterator& other)
{
	return (pK2HShm == other.pK2HShm && isOnlySubKey == other.isOnlySubKey && pAbsElement == other.pAbsElement);
}

bool K2HIterator::operator!=(const K2HIterator& other)
{
	return !(*this == other);
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
