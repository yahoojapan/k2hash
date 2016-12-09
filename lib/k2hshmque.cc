/*
 * K2HASH
 *
 * Copyright 2013-2015 Yahoo! JAPAN corporation.
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
 * CREATE:   Mon Feb 2 2015
 * REVISION:
 *
 */

#include <string.h>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hutil.h"
#include "k2hashfunc.h"
#include "k2hdbg.h"

using namespace std;

// [NOTE]
// About attribute in Queue
//
// (Key)Queue's marker does NOT set ANY attribute.
// KeyQueus's key does NOT set ANY attribute too.
// Queus's key can be set attributes EXCEPT history attribute, allowed attributes are set by builtin/plugin attribute object.
// Queus's key can set expire attribute overwrite old expire attribute if specifies k2hattrs.
//
// Marker(key)                  - not allow any attribute
//  -> Key for K2HQueue         - allow mtime, encrypt, expire(*), plugin's attributes, deny history attribute
//  -> Key for K2HKeyQueue      - not allow any attribute
//     -> Value for K2HKeyQueue - this means normal key-value's key, so this normal key allows any attribute like normal key.
//
// *) expire in queue key is not normal expire attribute, but it uses same attribute name.
//    here, exipre inherits value if specified old attributes(which has expire attribute).
//    if there is no exipre attribute in old attribute list, exipre is set new value when the builtin attribute object has expire seconds.
//    if there is no expire and not set expire seconds in builtin object, exipre value is not set.
//
//---------------------------------------------------------
// Class methods
//---------------------------------------------------------
PBK2HMARKER K2HShm::InitK2HMarker(size_t& marklen, const unsigned char* bystart, size_t startlen, const unsigned char* byend, size_t endlen)
{
	if(((NULL == bystart) != (0 == startlen)) || ((NULL == byend) != (0 == endlen))){
		ERR_K2HPRN("Some parameters are wrong.");
		return NULL;
	}
	if(!byend){
		byend	= bystart;
		endlen	= startlen;
	}

	PBK2HMARKER	pmarker;

	// allocation
	marklen = sizeof(K2HMARKER) + startlen + endlen;
	if(NULL == (pmarker = reinterpret_cast<PBK2HMARKER>(malloc(marklen)))){
		ERR_K2HPRN("Could not allocation memory.");
		return NULL;
	}

	// set data
	// [NOTICE]
	// If startlen and endlen is 0, the offset pointers point over this structure area.
	// At first, must check length for accessing this structure.
	//
	pmarker->marker.startlen	= startlen;
	pmarker->marker.startoff	= static_cast<off_t>(sizeof(K2HMARKER));
	pmarker->marker.endlen		= endlen;
	pmarker->marker.endoff		= static_cast<off_t>(sizeof(K2HMARKER) + startlen);

	if(0 < startlen){
		memcpy((&(pmarker->byData[0]) + pmarker->marker.startoff), bystart, startlen);
	}
	if(0 < endlen){
		memcpy((&(pmarker->byData[0]) + pmarker->marker.endoff), byend, endlen);
	}

	return pmarker;
}

PBK2HMARKER K2HShm::UpdateK2HMarker(PBK2HMARKER pmarker, size_t& marklen, const unsigned char* byKey, size_t keylength, bool is_end)
{
	if(!pmarker || (NULL == byKey) != (0 == keylength)){
		ERR_K2HPRN("Some parameters are wrong.");
		return NULL;
	}

	PBK2HMARKER	pnewmarker;

	if(0 == pmarker->marker.startlen && 0 == pmarker->marker.endlen){
		// marker is empty

		// allocation
		marklen = sizeof(K2HMARKER) + (keylength * 2);
		if(NULL == (pnewmarker = reinterpret_cast<PBK2HMARKER>(malloc(marklen)))){
			ERR_K2HPRN("Could not allocation memory.");
			return NULL;
		}

		// set data
		pnewmarker->marker.startlen	= keylength;
		pnewmarker->marker.startoff	= static_cast<off_t>(sizeof(K2HMARKER));
		pnewmarker->marker.endlen	= keylength;
		pnewmarker->marker.endoff	= static_cast<off_t>(sizeof(K2HMARKER) + keylength);

		memcpy((&(pnewmarker->byData[0]) + pnewmarker->marker.startoff),	byKey,	pnewmarker->marker.startlen);
		memcpy((&(pnewmarker->byData[0]) + pnewmarker->marker.endoff),		byKey,	pnewmarker->marker.endlen);
	}else{

		if((is_end && 0 == pmarker->marker.startlen) || (!is_end && 0 == pmarker->marker.endlen)){
			// When setting start(end) key, the pmarker does not have end(start) key.
			// Then the end(start) key must be set as same as start(end) key.
			// So it means initializing.
			//
			K2H_Free(pmarker);
			return K2HShm::InitK2HMarker(marklen, byKey, keylength);
		}

		// allocation
		marklen = sizeof(K2HMARKER) + (is_end ? (pmarker->marker.startlen + keylength) : (keylength + pmarker->marker.endlen));
		if(NULL == (pnewmarker = reinterpret_cast<PBK2HMARKER>(malloc(marklen)))){
			ERR_K2HPRN("Could not allocation memory.");
			return NULL;
		}

		// set data
		pnewmarker->marker.startlen	= is_end ? pmarker->marker.startlen : keylength;
		pnewmarker->marker.startoff	= static_cast<off_t>(sizeof(K2HMARKER));
		pnewmarker->marker.endlen	= is_end ? keylength : pmarker->marker.endlen;
		pnewmarker->marker.endoff	= static_cast<off_t>(sizeof(K2HMARKER) + pnewmarker->marker.startlen);

		memcpy((&(pnewmarker->byData[0]) + pnewmarker->marker.startoff),	(is_end ? (&(pmarker->byData[0]) + pmarker->marker.startoff) : byKey),	pnewmarker->marker.startlen);
		memcpy((&(pnewmarker->byData[0]) + pnewmarker->marker.endoff),		(is_end ? byKey : (&(pmarker->byData[0]) + pmarker->marker.endoff)),	pnewmarker->marker.endlen);
	}

	K2H_Free(pmarker);

	return pnewmarker;
}

bool K2HShm::IsEmptyK2HMarker(PBK2HMARKER pmarker)
{
	if(!pmarker){
		MSG_K2HPRN("Some parameter is wrong.");
		return true;
	}
	return (0 == pmarker->marker.startlen);
}

//---------------------------------------------------------
// Methods for Queue
//---------------------------------------------------------
//
// Retrieving key from K2HMaker always is top of list.
//
// [NOTE]
// This method find not expired key in queue marker list.
// If the key is expired, the key is removed in k2hash and this method continues to find next key.
// After finding the key, return next marker and set attributes if ppAttrs is not NULL.
//
PBK2HMARKER K2HShm::PopK2HMarker(PBK2HMARKER pmarker, size_t& marklen, unsigned char** ppKey, size_t& keylength, K2HAttrs** ppAttrs)
{
	if(!pmarker || !ppKey){
		ERR_K2HPRN("Some parameters are wrong.");
		K2H_Free(pmarker);
		return NULL;
	}

	bool		is_expire	= false;
	K2HAttrs*	pKeyAttrs	= NULL;
	*ppKey					= NULL;
	keylength				= 0;
	if(ppAttrs){
		*ppAttrs			= NULL;
	}

	// loop until finding not expired key in queue.
	do{
		//
		// clean up and remove expired key in queue.
		//
		K2H_Delete(pKeyAttrs);
		if(*ppKey){
			if(!Remove(*ppKey, keylength, false, NULL, true)){			// Remove key without subkey & history
				WAN_K2HPRN("Failed to remove key in expired queue, but continue...");
			}
			*ppKey		= NULL;
			keylength	= 0;
		}

		// check marker
		if(0 == pmarker->marker.startlen){
			K2H_Free(pmarker);
			return K2HShm::InitK2HMarker(marklen);
		}

		// copy current start key
		*ppKey		= k2hbindup((&(pmarker->byData[0]) + pmarker->marker.startoff), pmarker->marker.startlen);
		keylength	= pmarker->marker.startlen;

		// Get subkeys in current start key.(without checking attribute)
		K2HSubKeys*	psubkeys;
		if(NULL == (psubkeys = GetSubKeys(*ppKey, keylength, false))){
			// There is no key nor subkeys
			K2H_Free(pmarker);
			return K2HShm::InitK2HMarker(marklen);
		}

		// Check subkeys
		K2HSubKeys::iterator iter = psubkeys->begin();
		if(iter == psubkeys->end()){
			// Current key does not have any subkey.
			K2H_Delete(psubkeys);
			K2H_Free(pmarker);
			return K2HShm::InitK2HMarker(marklen);
		}

		// Get attribute and check expire
		is_expire = false;
		if(NULL != (pKeyAttrs = GetAttrs(*ppKey, keylength))){
			K2hAttrOpsMan	attrman;
			if(attrman.Initialize(this, *ppKey, keylength, NULL, 0UL, NULL)){
				if(attrman.IsExpire(*pKeyAttrs)){
					// key is expired
					is_expire = true;
				}
			}else{
				WAN_K2HPRN("Something error occurred during initializing attributes manager class, but continue...");
			}
		}

		// Make new next K2HMaker
		PBK2HMARKER	pnewmarker;
		if(NULL == (pnewmarker = K2HShm::InitK2HMarker(marklen, iter->pSubKey, iter->length, (0 == pmarker->marker.endlen ? NULL : (&(pmarker->byData[0]) + pmarker->marker.endoff)), pmarker->marker.endlen))){
			ERR_K2HPRN("Something error is occurred.");
			K2H_Delete(psubkeys);
			K2H_Delete(pKeyAttrs);
			K2H_Free(pmarker);
			return NULL;
		}
		K2H_Delete(psubkeys);
		K2H_Free(pmarker);

		// set new next marker
		pmarker = pnewmarker;

	}while(is_expire);

	if(ppAttrs){
		*ppAttrs = pKeyAttrs;
	}
	return pmarker;
}

//
// Update only start marker without key modifying.
//
// [NOTICE] Be careful for using this method.
//
bool K2HShm::UpdateStartK2HMarker(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength)
{
	if(!byMark || 0 == marklength || ((NULL == byKey) != (0 == keylength))){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// [NOTE]
	// This method keeps locking CKIndex area during some operation.
	// So we need to lock current mask area for escaping deadlock.
	//
	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RWLOCK);		// LOCK

	// Lock cindex for writing marker.
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byMark), marklength);

	K2HLock		ALObjCKI(K2HLock::RWLOCK);										// LOCK
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}

	// Read current marker.(without checking expire)
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval, false))){
		// There is no marker
		return false;
	}
	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);

	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return false;
	}
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		// There is no data in queue
		K2H_Free(pmkval);
		return true;															// [NOTE] returns success
	}

	// set new marker
	if(byKey){
		if(0 != k2hbincmp(byKey, keylength, (&(pmarker->byData[0]) + pmarker->marker.startoff), pmarker->marker.startlen)){
			// Update marker data
			pmarker = K2HShm::UpdateK2HMarker(pmarker, marklen, byKey, keylength, false);
		}else{
			// same queue key is already set, so nothing to do.
		}
	}else{
		// There is no start queue key, it means the queue is empty.
		pmarker = K2HShm::InitK2HMarker(marklen);
	}

	// Set new marker
	//
	// marker does not have any attribute.
	//
	if(!Set(byMark, marklength, &(pmarker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
		ERR_K2HPRN("Could not set new marker.");
		K2H_Free(pmarker);
		return false;
	}
	K2H_Free(pmarker);

	// Unlock
	ALObjCKI.Unlock();
	ALObjCMask.Unlock();

	return true;
}

bool K2HShm::IsEmptyQueue(const unsigned char* byMark, size_t marklength) const
{
	if(!byMark || 0 == marklength){
		ERR_K2HPRN("Some parameters are wrong.");
		return true;		// wrong marker means as same as empty
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return true;		// wrong parameter means as same as empty
	}

	// Read current marker.
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval)) || !pmkval){
		return true;		// no marker
	}

	// Check empty marker
	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		K2H_Free(pmkval);
		return true;		// marker is empty
	}

	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return true;		// marker is wrong size
	}
	K2H_Free(pmkval);

	// there are one or more key in queue.
	return false;
}

//
// Returns queue count which is included expired keys.
//
int K2HShm::GetCountQueue(const unsigned char* byMark, size_t marklength) const
{
	if(!byMark || 0 == marklength){
		ERR_K2HPRN("Some parameters are wrong.");
		return 0;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return 0;
	}

	// Read current marker.
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval)) || !pmkval){
		// There is no marker
		return 0;
	}

	// Check empty marker
	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		K2H_Free(pmkval);
		return 0;
	}

	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return 0;
	}

	// loop for counting
	unsigned char*	pKey		= k2hbindup(&(pmarker->byData[pmarker->marker.startoff]), pmarker->marker.startlen);
	size_t			keylength	= pmarker->marker.startlen;
	int				count;
	for(count = 0; pKey; count++){
		// check end of queue
		if(0 == k2hbincmp(pKey, keylength, &(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen)){
			K2H_Free(pKey);
			count++;
			break;
		}

		// get subkeys
		K2HSubKeys*	psubkeys;
		if(NULL == (psubkeys = GetSubKeys(pKey, keylength))){
			// There is no key nor subkeys
			K2H_Free(pKey);
			count++;
			break;
		}
		K2HSubKeys::iterator iter = psubkeys->begin();
		if(iter == psubkeys->end()){
			// Current key does not have any subkey.
			K2H_Free(pKey);
			K2H_Delete(psubkeys);
			count++;
			break;
		}

		// set next key
		K2H_Free(pKey);
		pKey		= k2hbindup(iter->pSubKey, iter->length);
		keylength	= iter->length;

		K2H_Delete(psubkeys);
	}
	K2H_Free(pmkval);

	return count;
}

//
// Read key(only) from queue at position which is based top of queue.
//
// Parameter	pos:	specify position in queue, position 0 means top of queue.
//						if this is set -1, it means end of queue.
// Returns		false:	not found key or something error occurred.
//				true:	found key at position
//
bool K2HShm::ReadQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, int pos) const
{
	if(!byMark || 0 == marklength || !ppKey || pos < -1){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// This method returns true when there is no queue data.
	// So caller is check following value.
	//
	*ppKey		= NULL;
	keylength	= 0;

	// Read current marker.(do not check attribute for no expire)
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval, false)) || !pmkval){
		// There is no marker
		return false;
	}

	// Check empty marker
	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		K2H_Free(pmkval);
		return false;
	}

	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return false;
	}

	// Get key by position
	if(-1 == pos){
		// End(deepest) of queue
		if(0 == pmarker->marker.endlen){
			K2H_Free(pmkval);
			return false;
		}

		// get key
		*ppKey		= k2hbindup(&(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen);
		keylength	= pmarker->marker.endlen;

	}else{
		// loop count for searching
		*ppKey		= k2hbindup(&(pmarker->byData[pmarker->marker.startoff]), pmarker->marker.startlen);
		keylength	= pmarker->marker.startlen;
		for(int cnt = 0; *ppKey && cnt < pos; cnt++){
			// check end of queue
			if(0 == k2hbincmp(*ppKey, keylength, &(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen)){
				K2H_Free(*ppKey);
				break;
			}

			// get subkeys.(do not check attribute for no expire)
			K2HSubKeys*	psubkeys;
			if(NULL == (psubkeys = GetSubKeys(*ppKey, keylength, false))){
				// There is no key nor subkeys
				K2H_Free(*ppKey);
				break;
			}
			K2HSubKeys::iterator iter = psubkeys->begin();
			if(iter == psubkeys->end()){
				// Current key does not have any subkey.
				K2H_Free(*ppKey);
				K2H_Delete(psubkeys);
				break;
			}

			// set next key
			K2H_Free(*ppKey);
			*ppKey		= k2hbindup(iter->pSubKey, iter->length);
			keylength	= iter->length;

			K2H_Delete(psubkeys);
		}

		if(!*ppKey){
			// not found or there is no key by position.
			K2H_Free(pmkval);
			return false;
		}
	}
	K2H_Free(pmkval);

	return true;
}

//
// Read key from queue at position which is based top of queue.
//
// Parameter	pos:	specify position in queue, position 0 means top of queue.
//						if this is set -1, it means end of queue.
// Returns		false:	there is no key at position or key is expired
//				true:	found key at position
//
bool K2HShm::ReadQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, int pos, const char* encpass) const
{
	// get key
	if(!ReadQueue(byMark, marklength, ppKey, keylength, pos)){
		// not found or there is no key by position.
		return false;
	}

	// Get value.(with checking attribute)
	ssize_t	tmpvallen;
	if(-1 == (tmpvallen = Get(*ppKey, keylength, ppValue, true, encpass))){
		ERR_K2HPRN("Could not get value by key.");
		K2H_Free(*ppKey);
		return false;
	}
	vallength = static_cast<size_t>(tmpvallen);

	return true;
}

//
// If making FIFO(LIFO) type queue, sets is_fifo flag as true(false).
// So that, adding new key into tail(top) of queue list.
//
// [NOTICE]
// It is also the case that the end key in K2HMaker has subkeys, but it is correct.
// *** Even if end key had subkeys, never the subkeys is used. ***
//
bool K2HShm::AddQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, bool is_fifo, bool update_chain, bool check_update_file)
{
	if(!byMark || 0 == marklength || !byKey || 0 == keylength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(check_update_file){
		K2HFILE_UPDATE_CHECK(this);
	}

	// [NOTE]
	// This method keeps locking CKIndex area during some operation.
	// So we need to lock current mask area for escaping deadlock.
	//
	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RWLOCK);		// LOCK

	// Lock cindex for writing marker.
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byMark), marklength);

	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		if(!Remove(byKey, keylength, true, NULL, true)){					// Remove key without history
			ERR_K2HPRN("Failed to remove key for recovering, so the key has been staying in k2hash.");
		}
		return false;
	}

	// Read marker.(without checking expire)
	PBK2HMARKER		pmarker;
	size_t			marklen = 0;
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	bool			is_need_update_marker = true;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval, false, NULL))){
		// There is no marker, so make new marker
		if(NULL == (pmarker = K2HShm::InitK2HMarker(marklen))){
			ERR_K2HPRN("Could not make new marker value.");
			return false;
		}

	}else{
		// found marker
		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);

		// Check marker size
		if(static_cast<size_t>(mkvallength) != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
			ERR_K2HPRN("The marker is not same size which is calculated.");
			K2H_Free(pmkval);
			return false;
		}

		// [NOTE]
		// if using this method directly not from another K2HShm::AddQueue method
		// (thus do not need change subkey list in key), set update_chain
		// as false.
		// But you must set subkey list in key manually.
		//
		if(update_chain){
			// Update chain
			if(is_fifo){
				// Update subkey in end key
				if(0 < pmarker->marker.endlen){
					if(0 != k2hbincmp(byKey, keylength, (&(pmarker->byData[0]) + pmarker->marker.endoff), pmarker->marker.endlen)){
						// add key to subkey at end of keys.
						K2HSubKeys		k2hsubkeys;
						unsigned char*	bySubkeys = NULL;
						size_t			skeylength= 0UL;

						k2hsubkeys.insert(byKey, keylength);
						if(!k2hsubkeys.Serialize(&bySubkeys, skeylength)){
							ERR_K2HPRN("Failed to serialize subkey.");
							K2H_Free(pmkval);
							return false;
						}

						if(!ReplaceSubkeys((&(pmarker->byData[0]) + pmarker->marker.endoff), pmarker->marker.endlen, bySubkeys, skeylength)){
							ERR_K2HPRN("Failed to insert key as subkey into end of key.");
							K2H_Free(bySubkeys);
							K2H_Free(pmkval);
							return false;
						}
						K2H_Free(bySubkeys);

					}else{
						// end key is as same as key name, so do not need to update merker
						is_need_update_marker = false;
					}
				}
			}else{
				// Update subkey in key
				if(0 < pmarker->marker.startlen){
					if(0 != k2hbincmp((&(pmarker->byData[0]) + pmarker->marker.startoff), pmarker->marker.startlen, byKey, keylength)){
						// add start key to subkey at key.
						K2HSubKeys		k2hsubkeys;
						unsigned char*	bySubkeys = NULL;
						size_t			skeylength= 0UL;

						k2hsubkeys.insert((&(pmarker->byData[0]) + pmarker->marker.startoff), pmarker->marker.startlen);
						if(!k2hsubkeys.Serialize(&bySubkeys, skeylength)){
							ERR_K2HPRN("Failed to serialize subkey.");
							K2H_Free(pmkval);
							return false;
						}

						if(!ReplaceSubkeys(byKey, keylength, bySubkeys, skeylength)){
							ERR_K2HPRN("Failed to insert start key as subkey into key.");
							K2H_Free(bySubkeys);
							K2H_Free(pmkval);
							return false;
						}
						K2H_Free(bySubkeys);

					}else{
						// start key is as same as key name, so do not need update marker
						is_need_update_marker = false;
					}
				}
			}
		}
	}

	if(is_need_update_marker){
		// Update marker data
		if(NULL == (pmarker = K2HShm::UpdateK2HMarker(pmarker, marklen, byKey, keylength, is_fifo))){
			ERR_K2HPRN("Could not make new marker value.");
			//
			// If is_fifo is false, do not recover end key's subkey.
			// Please see top of this method comment.
			//
			K2H_Free(pmkval);
			return false;
		}

		// Set new marker
		//
		// marker does not have any attribute.
		//
		if(!Set(byMark, marklength, &(pmarker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
			if(0 == mkvallength){
				ERR_K2HPRN("Could not set new marker.");
			}else{
				// on this case, give up fore recovering...
				ERR_K2HPRN("Could not set new marker, we could not recover data...");
			}
			K2H_Free(pmarker);
			return false;
		}
	}
	K2H_Free(pmarker);

	return true;
}

//
// If making FIFO(LIFO) type queue, sets is_fifo flag as true(false).
// So that, adding new key into tail(top) of queue list.
//
// [NOTICE]
// It is also the case that the end key in K2HMaker has subkeys, but it is correct.
// *** Even if end key had subkeys, never the subkeys is used. ***
//
bool K2HShm::AddQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, bool is_fifo, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	if(!byMark || 0 == marklength || !byKey || 0 == keylength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// Make new key in K2hash
	//
	// attrtype should be allowed OPSMAN_MASK_QUEUEKEY, OPSMAN_MASK_TRANSQUEUEKEY, OPSMAN_MASK_QUEUEMARKER
	// and OPSMAN_MASK_KEYQUEUEKEY for Queue
	//
	if(!Set(byKey, keylength, byValue, vallength, NULL, true, pAttrs, encpass, expire, attrtype)){
		ERR_K2HPRN("Could not make new key.");
		return false;
	}

	// set key to marker
	if(!AddQueue(byMark, marklength, byKey, keylength, is_fifo, true, false)){	// [NOTE] update chain, not check update file
		ERR_K2HPRN("Something error occurred during set key to marker.");
		if(!Remove(byKey, keylength, true, NULL, true)){						// Remove key without history
			ERR_K2HPRN("Failed to remove key for recovering, so the key has been staying in k2hash.");
		}
		return false;
	}
	return true;
}

//
// Always remove key from queue at top.
// Controling FIFO or LIFO is decided at adding key into queue.
//
// Returns	false:	Something error occurred.
//			true:	Succeed
//					if there is no popping data, returns true but ppValue is NULL.
//
// [NOTICE]
// It is also the case that new end key in K2HMaker has subkeys after retrieving the key, but it is correct.
// *** Even if end key had subkeys, never the subkeys is used. ***
//
// [NOTE]
// If the key in queue list is expired, skip it.
//
bool K2HShm::PopQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, K2HAttrs** ppAttrs, const char* encpass)
{
	if(!byMark || 0 == marklength || !ppKey || !ppValue){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// This method returns true when there is no queue data.
	// So caller is check following value.
	//
	*ppKey		= NULL;
	keylength	= 0;
	*ppValue	= NULL;
	vallength	= 0;
	if(ppAttrs){
		*ppAttrs= NULL;
	}

	// [NOTE]
	// This method keeps locking CKIndex area during some operation.
	// So we need to lock current mask area for escaping deadlock.
	//
	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RWLOCK);		// LOCK

	// Lock cindex for writing marker.
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byMark), marklength);

	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}

	// Read current marker.(without checking expire)
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval, false)) || !pmkval){
		// There is no marker
		return true;
	}

	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);

	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return false;
	}
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		// There is no queued key in marker
		K2H_Free(pmkval);
		return true;
	}

	// Pop top of key & make new marker.(with checking expired key)
	if(NULL == (pmarker = PopK2HMarker(pmarker, marklen, ppKey, keylength, ppAttrs))){
		ERR_K2HPRN("Coult not pop key from marker.");
		K2H_Free(pmkval);
		K2H_Free(*ppKey);		// if set it
		return false;
	}

	// Set new marker
	//
	// marker does not have any attribute.
	//
	if(!Set(byMark, marklength, &(pmarker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
		ERR_K2HPRN("Could not set new marker.");
		K2H_Free(*ppKey);		// if set it
		keylength = 0;

		K2H_Free(pmarker);
		return false;
	}
	K2H_Free(pmarker);

	// Unlock
	ALObjCKI.Unlock();
	ALObjCMask.Unlock();

	if(!(*ppKey)){
		// There is no pop key.
		return true;
	}

	// Get value.(with checking attribute)
	ssize_t	tmpvallen;
	if(-1 == (tmpvallen = Get(*ppKey, keylength, ppValue, true, encpass))){
		ERR_K2HPRN("Could not get poped key value.");

		// Remove key.(without subkey & history)
		if(!Remove(*ppKey, keylength, false, NULL, true)){
			ERR_K2HPRN("Could not remove poped key, but continue...");
		}
		K2H_Free(*ppKey);
		keylength = 0;
		return false;
	}
	vallength = static_cast<size_t>(tmpvallen);

	// Remove key.(without subkey & history)
	if(!Remove(*ppKey, keylength, false, NULL, true)){
		ERR_K2HPRN("Could not remove poped key, but continue...");
	}
	return true;
}

int K2HShm::RemoveQueue(const unsigned char* byMark, size_t marklength, unsigned int count, bool rmkeyval, k2h_q_remove_trial_callback fp, void* pExtData, const char* encpass)
{
	if(!byMark || 0 == marklength){
		ERR_K2HPRN("Some parameters are wrong.");
		return -1;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return -1;
	}
	K2HFILE_UPDATE_CHECK(this);

	int	removed_count = 0;
	if(0 == count){
		return removed_count;
	}

	// [NOTE]
	// This method keeps locking CKIndex area during some operation.
	// So we need to lock current mask area for escaping deadlock.
	//
	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RWLOCK);		// LOCK

	// Lock cindex for writing marker.
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byMark), marklength);

	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return -1;
	}

	// Read current marker.(without checking expire)
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallength;
	if(-1 == (mkvallength = Get(byMark, marklength, &pmkval, false))){
		// There is no marker
		return removed_count;
	}

	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallength);

	// Check current marker size
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		ERR_K2HPRN("The marker is not same size which is calculated.");
		K2H_Free(pmkval);
		return -1;
	}
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		// There is no data in queue
		K2H_Free(pmkval);
		return removed_count;
	}

	// Get start queue key
	unsigned char*	pQueueKey	= k2hbindup((&(pmarker->byData[0]) + pmarker->marker.startoff), pmarker->marker.startlen);
	size_t			qkeylen		= pmarker->marker.startlen;

	// Remove loop
	unsigned char*	pStartQKey	= NULL;
    size_t			startqkeylen= 0;
	unsigned char*	pLastQKey	= NULL;
    size_t			lastqkeylen	= 0;
	unsigned char*	pDataKey	= NULL;
	ssize_t			datakeylen	= -1;
	bool			is_rm_last	= false;
	PK2HATTRPCK		pattrspck	= NULL;
	int				attrspckcnt	= 0;
	K2HQRMCBRES		res			= K2HQRMCB_RES_CON_RM;

	for(unsigned int cnt = 0; cnt < count; cnt++){
		pattrspck	= NULL;
		attrspckcnt	= 0;

		// Get data key( is queued by queue key ) with checking attribute
		if(rmkeyval || fp){
			if(-1 == (datakeylen = Get(pQueueKey, qkeylen, &pDataKey, true, encpass))){
				// not found data key
				WAN_K2HPRN("Could not get poped key value(one of case is unabling decrypt), something is wrong.");
			}
			// get attribute as structure array
			k2h_get_attrs(reinterpret_cast<k2h_h>(this), pQueueKey, qkeylen, &pattrspck, &attrspckcnt);
		}

		// check by callback function
		if(!fp){
			res	= K2HQRMCB_RES_CON_RM;
		}else{
			if(!pQueueKey || -1 == datakeylen){
				// not found data key
				WAN_K2HPRN("There is no poped key value, so remove this key is automatically.");
				res = K2HQRMCB_RES_CON_RM;

			}else{
				// found data key -> check by callback
				if(K2HQRMCB_RES_ERROR == (res = fp(pDataKey, static_cast<size_t>(datakeylen), pattrspck, attrspckcnt, pExtData))){
					// Stop loop ASSAP.
					if(pLastQKey){
						// replace last key's subkey as now queue key.
						if(0 < removed_count){
							K2HSubKeys		k2hsubkeys;
							unsigned char*	bySubkeys = NULL;
							size_t			skeylength= 0UL;

							k2hsubkeys.insert(pQueueKey, qkeylen);
							if(!k2hsubkeys.Serialize(&bySubkeys, skeylength)){
								// error...
								ERR_K2HPRN("Failed to serialize subkey in closing, thus broken queue and leaked...");
							}else{
								// replace
								if(!ReplaceSubkeys(pLastQKey, lastqkeylen, bySubkeys, skeylength)){
									// error...
									ERR_K2HPRN("Failed to insert now queue key into last queue key's subkey in closing, thus broken queue and leaked...");
								}
							}
							K2H_Free(bySubkeys);
						}
					}
					K2H_Free(pLastQKey);
					k2h_free_attrpack(pattrspck, attrspckcnt);

					if(!pStartQKey){
						pStartQKey	= pQueueKey;
						startqkeylen= qkeylen;
						pQueueKey	= NULL;
						qkeylen		= 0;
					}
					K2H_Free(pDataKey);
					datakeylen = -1;
					break;
				}
			}
		}
		k2h_free_attrpack(pattrspck, attrspckcnt);

		// Get next queue key in queued key's subkey.(do not check attribute for no expire)
		K2HSubKeys*		psubkeys;
		unsigned char*	pNextQKey	= NULL;
		ssize_t			nextqkeylen	= 0;
		if(NULL != (psubkeys = GetSubKeys(pQueueKey, qkeylen, false))){
			// Check subkeys
			K2HSubKeys::iterator iter = psubkeys->begin();
			if(iter != psubkeys->end()){
				pNextQKey	= k2hbindup(iter->pSubKey, iter->length);
				nextqkeylen	= iter->length;
			}else{
				// Current key does not have any subkey.
			}
		}else{
			// There is no key nor subkeys
		}
		K2H_Delete(psubkeys);

		// Remove or Set last/start queue key
		if(K2HQRMCB_RES_CON_RM == res || K2HQRMCB_RES_FIN_RM == res){
			// Remove data key and it's value from k2hash
			if(rmkeyval){
				if(pDataKey){
					if(!Remove(pDataKey, static_cast<size_t>(datakeylen), true)){			// with attributes
						ERR_K2HPRN("Could not remove key from k2hash, but continue...");
					}
				}
			}
			// Remove queue key
			if(!Remove(pQueueKey, qkeylen, false, NULL, true)){								// without history
				ERR_K2HPRN("Could not remove poped key, but continue...");
			}

			// check last key
			if(0 == k2hbincmp(pQueueKey, qkeylen, &(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen)){
				is_rm_last = true;
			}
			removed_count++;

		}else{
			if(pLastQKey){
				if(0 < removed_count){
					// replace last key's subkey as now queue key.
					K2HSubKeys		k2hsubkeys;
					unsigned char*	bySubkeys = NULL;
					size_t			skeylength= 0UL;

					k2hsubkeys.insert(pQueueKey, qkeylen);
					if(!k2hsubkeys.Serialize(&bySubkeys, skeylength)){
						// error...
						ERR_K2HPRN("Failed to serialize subkey in closing, thus broken queue and leaked. but continue...");
					}else{
						// replace
						if(!ReplaceSubkeys(pLastQKey, lastqkeylen, bySubkeys, skeylength)){
							// error...
							ERR_K2HPRN("Failed to insert now queue key into last queue key's subkey in closing, thus broken queue and leaked. but continue...");
						}
					}
					K2H_Free(bySubkeys);
				}
			}
			K2H_Free(pLastQKey);

			if(!pStartQKey){
				pStartQKey	= k2hbindup(pQueueKey, qkeylen);
				startqkeylen= qkeylen;
			}
			pLastQKey	= pQueueKey;
			lastqkeylen	= qkeylen;
			pQueueKey	= NULL;
			qkeylen		= 0;
		}

		// Set next key
		K2H_Free(pQueueKey);
		pQueueKey	= pNextQKey;
		qkeylen		= nextqkeylen;
		pNextQKey	= NULL;
		nextqkeylen	= 0;

		// check finish
		if(K2HQRMCB_RES_FIN_RM == res || K2HQRMCB_RES_FIN_NOTRM == res || !pQueueKey){
			break;
		}
	}
	if(!pStartQKey){
		pStartQKey	= pQueueKey;
		startqkeylen= qkeylen;
		pQueueKey	= NULL;
		qkeylen		= 0;
	}
	K2H_Free(pQueueKey);

	// Reset marker
	if(0 < removed_count){
		if(pStartQKey){
			// Update marker data
			pmarker = K2HShm::UpdateK2HMarker(pmarker, marklen, pStartQKey, startqkeylen, false);
		}else{
			// There is no start queue key, it means the queue is empty.
			K2H_Free(pmkval);
			pmarker = K2HShm::InitK2HMarker(marklen);
		}

		if(is_rm_last && pLastQKey){
			// end key is changed.
			if(!ReplaceSubkeys(pLastQKey, lastqkeylen, NULL, 0)){
				ERR_K2HPRN("Failed to remove subkey for end of queue key, but continue...");
			}
			// Update marker data
			pmarker = K2HShm::UpdateK2HMarker(pmarker, marklen, pLastQKey, lastqkeylen, true);
		}

		if(pmarker){
			// Set new marker
			//
			// marker does not have any attribute.
			//
			if(!Set(byMark, marklength, &(pmarker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
				ERR_K2HPRN("Could not set new marker...");
				removed_count = -1;
			}
		}else{
			// Why...
			ERR_K2HPRN("Somthing wrong about updating marker...");
			removed_count = -1;
		}
		K2H_Free(pmarker);
	}
	K2H_Free(pStartQKey);
	K2H_Free(pLastQKey);

	return removed_count;
}

K2HQueue* K2HShm::GetQueueObj(bool is_fifo, const unsigned char* pref, size_t preflen)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HQueue*	queue = new K2HQueue();
	if(!queue->Init(this, is_fifo, pref, preflen)){
		K2H_Delete(queue);
		return NULL;
	}
	return queue;
}

K2HKeyQueue* K2HShm::GetKeyQueueObj(bool is_fifo, const unsigned char* pref, size_t preflen)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HKeyQueue*	queue = new K2HKeyQueue();
	if(!queue->Init(this, is_fifo, pref, preflen)){
		K2H_Delete(queue);
		return NULL;
	}
	return queue;
}

K2HLowOpsQueue* K2HShm::GetLowOpsQueueObj(bool is_fifo, const unsigned char* pref, size_t preflen)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HLowOpsQueue*	queue = new K2HLowOpsQueue();
	if(!queue->Init(this, is_fifo, pref, preflen)){
		K2H_Delete(queue);
		return NULL;
	}
	return queue;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
