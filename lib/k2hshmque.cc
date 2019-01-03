/*
 * K2HASH
 *
 * Copyright 2013-2015 Yahoo Japan Corporation.
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

//---------------------------------------------------------
// Utility function
//---------------------------------------------------------
// This function in k2hash.cc
//
extern PK2HATTRPCK k2h_cvt_attrs_to_bin(K2HAttrs* pAttrs, int& attrspckcnt);

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
// Get current marker
//
PBK2HMARKER K2HShm::GetMarker(const unsigned char* byMark, size_t marklength, K2HLock* pALObjCKI) const
{
	if(!byMark || 0 == marklength){
		ERR_K2HPRN("Some parameters are wrong.");
		return NULL;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}

	// lock object
	K2HLock	ALObjCKI(K2HLock::RDLOCK);
	if(!pALObjCKI){
		pALObjCKI = &ALObjCKI;
	}

	// make hash and lock cindex
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byMark), marklength);
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(hash, *pALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return NULL;							// automatically unlock ALObjCKI if it is local
	}

	// get element
	PELEMENT	pMarkerElement;
	if(NULL == (pMarkerElement = GetElement(byMark, marklength, *pALObjCKI))){
		MSG_K2HPRN("Could not get element for marker, probabry it is not existed.");
		return NULL;							// automatically unlock ALObjCKI if it is local
	}

	// get marker's value
	unsigned char*	pmkval	= NULL;
	ssize_t			mkvallen= Get(pMarkerElement, &pmkval, PAGEOBJ_VALUE);
	if(!pmkval || 0 == mkvallen){
		MSG_K2HPRN("Marker does not have value, probabry queue is empty.");
		return NULL;							// automatically unlock ALObjCKI if it is local
	}

	// Check empty marker
	PBK2HMARKER		pmarker = reinterpret_cast<PBK2HMARKER>(pmkval);
	size_t			marklen = static_cast<size_t>(mkvallen);
	if(K2HShm::IsEmptyK2HMarker(pmarker)){
		MSG_K2HPRN("Marker exists, but it is empty.");
		K2H_Free(pmkval);
		return NULL;							// automatically unlock ALObjCKI if it is local
	}
	if(marklen != (sizeof(K2HMARKER) + pmarker->marker.startlen + pmarker->marker.endlen)){
		MSG_K2HPRN("Marker exists, but the marker size is wrong.");
		K2H_Free(pmkval);
		return NULL;							// automatically unlock ALObjCKI if it is local
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
	// copy start key
	unsigned char*	pKey		= k2hbindup(&(pmarker->byData[pmarker->marker.startoff]), pmarker->marker.startlen);
	size_t			keylength	= pmarker->marker.startlen;

	// loop for counting
	int	count;
	for(count = 1; pKey; ++count){
		// check end of queue
		if(0 == k2hbincmp(pKey, keylength, &(pmarker->byData[pmarker->marker.endoff]), pmarker->marker.endlen)){
			break;
		}

		// get subkeys
		K2HSubKeys*	psubkeys;
		if(NULL == (psubkeys = GetSubKeys(pKey, keylength, false))){	// not check expired
			// There is no key nor subkeys
			break;
		}
		K2HSubKeys::iterator iter = psubkeys->begin();
		if(iter == psubkeys->end()){
			// Current key does not have any subkey.
			K2H_Delete(psubkeys);
			break;
		}

		// set next key
		K2H_Free(pKey);
		pKey		= k2hbindup(iter->pSubKey, iter->length);
		keylength	= iter->length;

		K2H_Delete(psubkeys);
	}
	K2H_Free(pmkval);
	K2H_Free(pKey);

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
				keylength = 0;
				break;
			}

			// get subkeys.(do not check attribute for no expire)
			K2HSubKeys*	psubkeys;
			if(NULL == (psubkeys = GetSubKeys(*ppKey, keylength, false))){
				// There is no key nor subkeys
				K2H_Free(*ppKey);
				keylength = 0;
				break;
			}
			K2HSubKeys::iterator iter = psubkeys->begin();
			if(iter == psubkeys->end()){
				// Current key does not have any subkey.
				K2H_Delete(psubkeys);
				K2H_Free(*ppKey);
				keylength = 0;
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
			keylength = 0;
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

	bool	result;
	if(is_fifo){
		result = AddFifoQueue(byMark, marklength, byKey, keylength, byValue, vallength, attrtype, pAttrs, encpass, expire);
	}else{
		result = AddLifoQueue(byMark, marklength, byKey, keylength, byValue, vallength, attrtype, pAttrs, encpass, expire);
	}
	return result;
}

//
// Add data to FIFO queue
//
bool K2HShm::AddFifoQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	//--------------------------------------
	// Make new key in K2hash
	//--------------------------------------
	//
	// attrtype should be allowed OPSMAN_MASK_QUEUEKEY, OPSMAN_MASK_TRANSQUEUEKEY, OPSMAN_MASK_QUEUEMARKER and OPSMAN_MASK_KEYQUEUEKEY for Queue
	//
	if(!Set(byKey, keylength, byValue, vallength, NULL, true, pAttrs, encpass, expire, attrtype)){
		ERR_K2HPRN("Could not make new key.");
		return false;
	}

	// Read marker
	PBK2HMARKER				before_marker	= GetMarker(byMark, marklength);
	const unsigned char*	before_endkey	= before_marker ? &(before_marker->byData[before_marker->marker.endoff]) : NULL;
	size_t					before_endlen	= before_marker ? before_marker->marker.endlen : 0;
	PBK2HMARKER				after_marker	= NULL;
	const unsigned char*	after_endkey	= NULL;
	size_t					after_endlen	= 0;
	PBK2HMARKER				last_marker		= NULL;		// for broken marker checking
	const unsigned char*	last_endkey		= NULL;
	size_t					last_endlen		= 0;
	bool					result			= false;	// result code and for loop flag

	do{
		//--------------------------------------
		// Set new key into end key's subkey
		//--------------------------------------
		if(before_marker && 0 < before_endlen){
			// marker has end of queue key

			// Get endkey's element with WRITE LOCK
			K2HLock		ALObjCKI_Endkey(K2HLock::RWLOCK);		// auto release locking at leaving in this scope.
			PELEMENT	pEndKeyElement;
			if(NULL == (pEndKeyElement = GetElement(before_endkey, before_endlen, ALObjCKI_Endkey))){
				// end key is not found.
				MSG_K2HPRN("Key(%s) is not found, so retry to get marker.", reinterpret_cast<const char*>(before_endkey));

				if(0 == k2hbincmp(before_endkey, before_endlen, last_endkey, last_endlen)){
					// before end key in marker is as same as now, so marker is not updated with wrong end key.
					ERR_K2HPRN("Key(%s) is not found, probabry marker end key is broken.", reinterpret_cast<const char*>(before_endkey));
					break;
				}
				// retry(switch before -> last)
				ALObjCKI_Endkey.Unlock();						// Unlock
				K2H_Free(last_marker);
				last_marker		= before_marker;
				last_endkey		= k2hbindup(before_endkey, before_endlen);
				last_endlen		= before_endlen;

				before_marker	= GetMarker(byMark, marklength);
				before_endkey	= before_marker ? &(before_marker->byData[before_marker->marker.endoff]) : NULL;
				before_endlen	= before_marker ? before_marker->marker.endlen : 0;
				continue;
			}else{
				K2H_Free(last_marker);
				last_endkey	= NULL;
				last_endlen	= 0;
			}

			// Get end of queue key's subkey and check it is empty.
			K2HSubKeys*	endkey_subkeys;
			if(NULL != (endkey_subkeys = GetSubKeys(pEndKeyElement, false)) && 0 < endkey_subkeys->size()){
				// end of queue key has subkeys, retry to loop at reading marker
				K2H_Free(before_marker);
				K2H_Delete(endkey_subkeys);
				before_marker	= GetMarker(byMark, marklength);
				before_endkey	= before_marker ? &(before_marker->byData[before_marker->marker.endoff]) : NULL;
				before_endlen	= before_marker ? before_marker->marker.endlen : 0;
				continue;
			}
			K2H_Delete(endkey_subkeys);

			// Replace subkeys of end by new key in it.
			K2HSubKeys		endkey_newskeys;
			unsigned char*	byNewSubkeys	= NULL;
			size_t			new_skeylen		= 0UL;
			endkey_newskeys.insert(byKey, keylength);
			if(!endkey_newskeys.Serialize(&byNewSubkeys, new_skeylen)){
				ERR_K2HPRN("Failed to serialize subkey for adding queue.");
				break;
			}
			if(!ReplacePage(pEndKeyElement, byNewSubkeys, new_skeylen, PAGEOBJ_SUBKEYS)){
				ERR_K2HPRN("Failed to replace subkey list into end of key.");
				K2H_Free(byNewSubkeys);
				break;
			}
			K2H_Free(byNewSubkeys);
		}else{
			K2H_Free(last_marker);
			last_endkey	= NULL;
			last_endlen	= 0;
		}

		//--------------------------------------
		// Re-Read marker with WRITE LOCK
		//--------------------------------------
		// [NOTE]
		// At first, we lock marker and call Set method which locks marker.
		// Then twice locking for marker, but it does not deadlock because
		// marker does not have subkeys
		//
		K2H_Free(after_marker);
		K2HLock			ALObjCKI_Marker(K2HLock::RWLOCK);					// manually release locking
		after_marker	= GetMarker(byMark, marklength, &ALObjCKI_Marker);
		after_endkey	= after_marker ? &(after_marker->byData[after_marker->marker.endoff]) : NULL;
		after_endlen	= after_marker ? after_marker->marker.endlen : 0;

		//--------------------------------------
		// Update marker
		//--------------------------------------
		if(after_marker && 0 < after_endlen){
			// now marker has end of queue key
			if(!before_marker || 0 == before_endlen){
				//
				// marker's end key is changed since reading before( before end key is not there. )
				// ---> thus retry all processing
				//
				ALObjCKI_Marker.Unlock();									// Unlock

				// switch(after -> before)
				K2H_Free(after_marker);
				K2H_Free(before_marker);
				before_marker	= GetMarker(byMark, marklength);
				before_endkey	= before_marker ? &(before_marker->byData[before_marker->marker.endoff]) : NULL;
				before_endlen	= before_marker ? before_marker->marker.endlen : 0;

			}else if(0 == k2hbincmp(before_endkey, before_endlen, after_endkey, after_endlen)){
				//
				// now marker's end key is as same as before end key.
				// ---> update marker
				//
				size_t	marklen	= 0;
				if(NULL == (after_marker = K2HShm::UpdateK2HMarker(after_marker, marklen, byKey, keylength, true))){		// FIFO
					ERR_K2HPRN("Could not make new marker value.");
					break;													// automatically unlock ALObjCKI_Marker
				}

				// Set new marker(marker does not have any attribute.)
				if(!Set(byMark, marklength, &(after_marker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
					ERR_K2HPRN("Could not set new marker.");
					break;													// automatically unlock ALObjCKI_Marker
				}
				result = true;												// automatically unlock ALObjCKI_Marker

			}else if(0 == k2hbincmp(byKey, keylength, after_endkey, after_endlen)){
				//
				// marker's end key is changed, and is as same as new key
				// ---> thus we do not any processing.
				//
				result = true;												// automatically unlock ALObjCKI_Marker

			}else{
				//
				// marker's end key(yyy) is changed since reading before(xxx)
				// ---> we must check deep!
				//

				// Unlock marker
				ALObjCKI_Marker.Unlock();									// Unlock

				//
				// Lock new key(exist?) and check new key's subkeys
				//
				K2HLock		ALObjCKI_Newkey(K2HLock::RWLOCK);				// auto release locking at leaving in this scope.
				PELEMENT	pNewkeyElement;
				if(NULL == (pNewkeyElement = GetElement(byKey, keylength, ALObjCKI_Newkey))){
					//
					// there is no new key, probabry already popped it.
					// ---> thus we do not any processing.
					//
					result = true;											// automatically unlock ALObjCKI_Newkey

				}else{
					//
					// there is new key yet, check it's subkeys
					//
					K2HSubKeys*	newkey_subkeys;
					if(NULL != (newkey_subkeys = GetSubKeys(pNewkeyElement, false)) && 0 < newkey_subkeys->size()){
						//
						// new key is set subkey in it, this means new key is not end key.
						// ---> thus we do not any processing.
						//
						K2H_Delete(newkey_subkeys);
						result = true;										// automatically unlock ALObjCKI_Newkey

					}else{
						//
						// new key does not have subkey
						// ---> unknown reason, but we should retry all processing.
						//
						K2H_Delete(newkey_subkeys);
						ALObjCKI_Newkey.Unlock();							// Unlock

						// switch(after -> before)
						K2H_Free(before_marker);
						before_marker	= after_marker;						// using marker which is read after.
						before_endkey	= after_endkey;
						before_endlen	= after_endlen;
						after_marker	= NULL;
						after_endkey	= NULL;
						after_endlen	= 0;
					}
				}
			}
		}else{
			//
			// now marker does not exist, or does not have end of key
			//
			if(0 == k2hbincmp(before_endkey, before_endlen, after_endkey, after_endlen)){
				//
				// before marker does not exist, or does not have end of key.
				// ---> we did not add new key into subkey list for end key(=not exist)
				//
				size_t	marklen	= 0;
				if(after_marker){
					//
					// there is now marker, but it does not have end key
					// ---> update marker
					//
					if(NULL == (after_marker = K2HShm::UpdateK2HMarker(after_marker, marklen, byKey, keylength, true))){		// FIFO
						ERR_K2HPRN("Could not make new marker value.");
						break;												// automatically unlock ALObjCKI_Marker
					}
				}else{
					//
					// both before and now marker do not exist.
					// ---> update(create new) marker
					//
					if(NULL == (after_marker = K2HShm::InitK2HMarker(marklen, byKey, keylength))){
						ERR_K2HPRN("Could not create marker.");
						break;												// automatically unlock ALObjCKI_Marker
					}
				}

				// Set new marker(marker does not have any attribute.)
				if(!Set(byMark, marklength, &(after_marker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
					ERR_K2HPRN("Could not set new marker.");
					break;													// automatically unlock ALObjCKI_Marker
				}
			}else{
				//
				// before marker exists, and it has end of key.
				// ---> we already added new key into subkey list for end key(=exists)
				//
				if(after_marker){
					//
					// there is now marker, but it's end key does not exist. probabry aready popped new key.
					// ---> thus we do not any processing.
					//
				}else{
					//
					// there is not now marker. probabry aready popped new key(and all queued keys). thus we had already added new key in it.
					// ---> thus we do not any processing.
					//
				}
			}
			result = true;													// automatically unlock ALObjCKI_Marker
		}
	}while(!result);

	K2H_Free(before_marker);
	K2H_Free(after_marker);
	K2H_Free(last_marker);

	return result;
}

//
// Add data to LIFO queue
//
bool K2HShm::AddLifoQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	// Read marker
	PBK2HMARKER				before_marker	= GetMarker(byMark, marklength);
	const unsigned char*	before_startkey	= before_marker ? &(before_marker->byData[before_marker->marker.startoff]) : NULL;
	size_t					before_startlen	= before_marker ? before_marker->marker.startlen : 0;
	PBK2HMARKER				after_marker	= NULL;
	const unsigned char*	after_startkey	= NULL;
	size_t					after_startlen	= 0;
	bool					result			= false;	// result code and for loop flag

	do{
		//--------------------------------------
		// (Re)Make new key with subkey list which is from marker
		//--------------------------------------
		K2HSubKeys*	newkey_subkeys = NULL;
		if(before_marker && 0 < before_startlen){
			// there is start key
			// ---> make subkey list data
			//
			newkey_subkeys	= new K2HSubKeys();
			newkey_subkeys->insert(before_startkey, before_startlen);
		}
		// Make new key in K2hash
		//
		// attrtype should be allowed OPSMAN_MASK_QUEUEKEY, OPSMAN_MASK_TRANSQUEUEKEY, OPSMAN_MASK_QUEUEMARKER and OPSMAN_MASK_KEYQUEUEKEY for Queue
		//
		if(!Set(byKey, keylength, byValue, vallength, newkey_subkeys, true, pAttrs, encpass, expire, attrtype)){
			ERR_K2HPRN("Could not make new key.");
			K2H_Delete(newkey_subkeys);
			break;
		}
		K2H_Delete(newkey_subkeys);

		//--------------------------------------
		// Re-Read marker with WRITE LOCK
		//--------------------------------------
		// [NOTE]
		// At first, we lock marker and call Set method which locks marker.
		// Then twice locking for marker, but it does not deadlock because
		// marker does not have subkeys
		//
		K2HLock		ALObjCKI_Marker(K2HLock::RWLOCK);						// manually release locking
		after_marker	= GetMarker(byMark, marklength, &ALObjCKI_Marker);
		after_startkey	= after_marker ? &(after_marker->byData[after_marker->marker.startoff]) : NULL;
		after_startlen	= after_marker ? after_marker->marker.startlen : 0;

		//--------------------------------------
		// Add new key into subkeys for start key in marker
		//--------------------------------------
		if(after_marker && 0 < after_startlen){
			// now marker has start of queue key
			if(!before_marker || 0 == before_startlen){
				//
				// marker's start key is changed since reading before( before start key is not there. )
				// ---> thus retry all processing
				//
				ALObjCKI_Marker.Unlock();									// Unlock

				// switch(after -> before)
				K2H_Free(after_marker);
				K2H_Free(before_marker);
				before_marker	= GetMarker(byMark, marklength);
				before_startkey	= before_marker ? &(before_marker->byData[before_marker->marker.startoff]) : NULL;
				before_startlen	= before_marker ? before_marker->marker.startlen : 0;

			}else if(0 == k2hbincmp(before_startkey, before_startlen, after_startkey, after_startlen)){
				//
				// now marker's start key is as same as before start key.
				// ---> update marker
				//
				size_t	marklen	= 0;
				if(NULL == (after_marker = K2HShm::UpdateK2HMarker(after_marker, marklen, byKey, keylength, false))){		// LIFO
					ERR_K2HPRN("Could not make new marker value.");
					break;													// automatically unlock ALObjCKI_Marker
				}

				// Set new marker(marker does not have any attribute.)
				if(!Set(byMark, marklength, &(after_marker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
					ERR_K2HPRN("Could not set new marker.");
					break;													// automatically unlock ALObjCKI_Marker
				}
				result = true;												// automatically unlock ALObjCKI_Marker

			}else if(0 == k2hbincmp(byKey, keylength, after_startkey, after_startlen)){
				//
				// marker's start key is changed, and is as same as new key
				// ---> thus we do not any processing.
				//
				result = true;												// automatically unlock ALObjCKI_Marker

			}else{
				//
				// marker's start key(yyy) is changed since reading before(xxx)
				// ---> thus retry all processing
				//
				ALObjCKI_Marker.Unlock();									// Unlock

				// switch(after -> before)
				K2H_Free(after_marker);
				K2H_Free(before_marker);
				before_marker	= GetMarker(byMark, marklength);
				before_startkey	= before_marker ? &(before_marker->byData[before_marker->marker.startoff]) : NULL;
				before_startlen	= before_marker ? before_marker->marker.startlen : 0;
			}
		}else{
			//
			// now marker does not exist, or does not have start of key
			//
			if(0 == k2hbincmp(before_startkey, before_startlen, after_startkey, after_startlen)){
				//
				// before marker does not exist, or does not have start of key.
				// ---> we did not add new key into subkey list for start key(=not exist)
				//
				size_t	marklen	= 0;
				if(after_marker){
					//
					// there is now marker, but it does not have start key
					// ---> update marker
					//
					if(NULL == (after_marker = K2HShm::UpdateK2HMarker(after_marker, marklen, byKey, keylength, false))){		// LIFO
						ERR_K2HPRN("Could not make new marker value.");
						break;												// automatically unlock ALObjCKI_Marker
					}
				}else{
					//
					// both before and now marker do not exist.
					// ---> update(create new) marker
					//
					if(NULL == (after_marker = K2HShm::InitK2HMarker(marklen, byKey, keylength))){
						ERR_K2HPRN("Could not create marker.");
						break;												// automatically unlock ALObjCKI_Marker
					}
				}

				// Set new marker(marker does not have any attribute.)
				if(!Set(byMark, marklength, &(after_marker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
					ERR_K2HPRN("Could not set new marker.");
					break;													// automatically unlock ALObjCKI_Marker
				}
				result = true;												// automatically unlock ALObjCKI_Marker

			}else{
				//
				// before marker exists, and it has start of key.
				// ---> thus retry all processing
				//
				ALObjCKI_Marker.Unlock();									// Unlock

				// switch(after -> before)
				K2H_Free(after_marker);
				K2H_Free(before_marker);
				before_marker	= GetMarker(byMark, marklength);
				before_startkey	= before_marker ? &(before_marker->byData[before_marker->marker.startoff]) : NULL;
				before_startlen	= before_marker ? before_marker->marker.startlen : 0;
			}
		}
	}while(!result);

	K2H_Free(before_marker);
	K2H_Free(after_marker);

	return result;
}

//
// This method does not update any queued key and queued key's subkey.
// For calling this from K2HLowOpsQueue class.
//
bool K2HShm::AddQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, bool is_fifo)
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

	K2HLock			ALObjCKI(K2HLock::RWLOCK);								// auto release locking at leaving in this scope.
	PBK2HMARKER		pmarker	= GetMarker(byMark, marklength, &ALObjCKI);
	size_t			marklen = 0;
	if(!pmarker){
		// There is no marker, so make new marker
		if(NULL == (pmarker = K2HShm::InitK2HMarker(marklen))){
			ERR_K2HPRN("Could not make new marker value.");
			return false;
		}
	}

	// Update marker data
	if(NULL == (pmarker = K2HShm::UpdateK2HMarker(pmarker, marklen, byKey, keylength, is_fifo))){
		ERR_K2HPRN("Could not make new marker value.");
		K2H_Free(pmarker);
		return false;
	}

	// Set new marker
	//
	// marker does not have any attribute.
	//
	if(!Set(byMark, marklength, &(pmarker->byData[0]), marklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
		ERR_K2HPRN("Could not set new/update marker.");
		K2H_Free(pmarker);
		return false;
	}
	K2H_Free(pmarker);

	return true;
}

//
// Get/Retrieve one top key in queue list.
//
//	[arguments]
//	const unsigned char*		byMark			: marker binary pointer
//	size_t						marklength		: marker length
//	bool&						is_found		: the result of whether or not the queued key existed is stored in this buffer
//	bool&						is_expired		: the result of whether or not the queued key is expired is stored in this buffer
//	unsigned char**				ppKey			: returns popped key binary pointer
//	size_t&						keylength		: returns popped key binary length
//	unsigned char**				ppValue			: returns popped key's value binary pointer
//	size_t&						vallength		: returns popped key's value binary length
//	K2HAttrs**					ppAttrs			: returns popped key's attibutes object pointer if ppAttrs is not NULL.(allowed null to this pointer)
//	const char*					encpass			: encrypt pass phrase
//
//	[return]
//	true										: one key is popped(updated marker) or there is no poped key(including expired key)
//	false										: somthing error occurred.
//
bool K2HShm::PopQueueEx(const unsigned char* byMark, size_t marklength, bool& is_found, bool& is_expired, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, K2HAttrs** ppAttrs, const char* encpass)
{
	if(!byMark || 0 == marklength || !ppKey || !ppValue){
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
	*ppValue	= NULL;
	vallength	= 0;
	if(ppAttrs){
		*ppAttrs= NULL;
	}

	while(true){
		is_found	= false;
		is_expired	= false;

		//--------------------------------------
		// Read marker without WRITE LOCK
		//--------------------------------------
		PBK2HMARKER				before_marker	= GetMarker(byMark, marklength);
		const unsigned char*	before_startkey	= before_marker ? &(before_marker->byData[before_marker->marker.startoff]) : NULL;
		size_t					before_startlen	= before_marker ? before_marker->marker.startlen : 0;

		if(!before_marker || !before_startkey || 0 == before_startlen){
			// there is no marker or no start key of queue, it means no stacked key in queue.
			K2H_Free(before_marker);
			break;
		}
		is_found = true;

		// Get subkeys in current start key for next marker's start key(without checking attribute)
		K2HSubKeys*				psubkeys	= GetSubKeys(before_startkey, before_startlen, false);
		const unsigned char*	pnextstart	= NULL;
		size_t					nextstartLen= 0;
		if(psubkeys){
			K2HSubKeys::iterator	iter = psubkeys->begin();
			if(iter != psubkeys->end()){
				pnextstart	= iter->pSubKey;
				nextstartLen= iter->length;
			}
		}

		// Get attribute and check expire
		K2HAttrs*	pKeyAttrs;
		if(NULL != (pKeyAttrs = GetAttrs(before_startkey, before_startlen))){
			K2hAttrOpsMan	attrman;
			if(attrman.Initialize(this, before_startkey, before_startlen, NULL, 0UL, NULL)){
				if(attrman.IsExpire(*pKeyAttrs)){
					is_expired = true;								// key is expired
				}
			}else{
				WAN_K2HPRN("Something error occurred during initializing attributes manager class, but continue...");
			}
			K2H_Delete(pKeyAttrs);
		}

		// Get value(with checking attribute)
		ssize_t	tmpvallen;
		if(-1 == (tmpvallen = Get(before_startkey, before_startlen, ppValue, true, encpass))){
			MSG_K2HPRN("Could not get popped key value(there is no value or key is expired.)");
		}else{
			vallength = static_cast<size_t>(tmpvallen);
		}

		//--------------------------------------
		// Re-Read marker with WRITE LOCK
		//--------------------------------------
		// [NOTE]
		// At first, we lock marker and call Set method which locks marker.
		// Then twice locking for marker, but it does not deadlock because
		// marker does not have subkeys
		//
		K2HLock					ALObjCKI_Marker(K2HLock::RWLOCK);	// auto release locking at leaving in this scope.
		PBK2HMARKER				after_marker	= GetMarker(byMark, marklength, &ALObjCKI_Marker);
		const unsigned char*	after_startkey	= after_marker ? &(after_marker->byData[after_marker->marker.startoff]) : NULL;
		size_t					after_startlen	= after_marker ? after_marker->marker.startlen : 0;
		if(!after_marker){
			MSG_K2HPRN("After reading marker, the marker is empty or wrong size.");

			K2H_Free(before_marker);
			K2H_Delete(psubkeys);
			K2H_Delete(pKeyAttrs);
			K2H_Free(*ppValue);
			vallength	= 0;
			is_found	= false;
			is_expired	= false;
			return false;											// automatically unlock ALObjCKI_Marker
		}

		// Check popped key name, compare it and before reading key name.
		if(0 != k2hbincmp(before_startkey, before_startlen, after_startkey, after_startlen)){
			MSG_K2HPRN("Different popped key name before and after reading, thus retry from first.");

			K2H_Free(before_marker);
			K2H_Free(after_marker);
			K2H_Delete(psubkeys);
			K2H_Delete(pKeyAttrs);
			K2H_Free(*ppValue);
			vallength = 0;
			ALObjCKI_Marker.Unlock();									// manually unlock
			continue;
		}

		// Make new next K2HMaker
		PBK2HMARKER	pnewmarker;
		size_t		newmarklen	= 0;
		if(NULL == (pnewmarker = K2HShm::InitK2HMarker(newmarklen, pnextstart, nextstartLen, (0 == after_marker->marker.endlen ? NULL : (&(after_marker->byData[0]) + after_marker->marker.endoff)), after_marker->marker.endlen))){
			ERR_K2HPRN("Something error is occurred to make new marker.");

			K2H_Free(before_marker);
			K2H_Free(after_marker);
			K2H_Delete(psubkeys);
			K2H_Delete(pKeyAttrs);
			K2H_Free(*ppValue);
			vallength	= 0;
			is_found	= false;
			is_expired	= false;
			return false;											// automatically unlock ALObjCKI_Marker
		}
		K2H_Delete(psubkeys);

		// Set new marker(marker does not have any attribute)
		if(!Set(byMark, marklength, &(pnewmarker->byData[0]), newmarklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
			ERR_K2HPRN("Could not set new marker.");

			K2H_Free(before_marker);
			K2H_Free(after_marker);
			K2H_Free(pnewmarker);
			K2H_Delete(pKeyAttrs);
			K2H_Free(*ppValue);
			vallength	= 0;
			is_found	= false;
			is_expired	= false;
			return false;											// automatically unlock ALObjCKI_Marker
		}
		K2H_Free(pnewmarker);

		// Unlock marker
		ALObjCKI_Marker.Unlock();									// manually unlock

		// Remove key(without subkey & history)
		if(!Remove(after_startkey, after_startlen, false, NULL, true)){
			ERR_K2HPRN("Could not remove popped key, but continue...");
		}

		// Copy key name
		*ppKey		= k2hbindup(before_startkey, before_startlen);
		keylength	= before_startlen;
		if(ppAttrs){
			*ppAttrs = pKeyAttrs;
		}else{
			K2H_Delete(pKeyAttrs);
		}

		K2H_Free(before_marker);
		K2H_Free(after_marker);

		break;
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

	//
	// Pop from queue until not expired key.
	//
	bool	result		= true;
	bool	is_found	= true;
	bool	is_expired	= false;
	for(result = true, is_found = true, is_expired = false; result && is_found; is_expired = false){
		if(false == (result = PopQueueEx(byMark, marklength, is_found, is_expired, ppKey, keylength, ppValue, vallength, ppAttrs, encpass))){
			ERR_K2HPRN("Something error occurred during poping from queue.");
		}else{
			if(!is_expired && is_found){
				break;
			}
		}
		K2H_Free(*ppKey);
		K2H_Free(*ppValue);
		if(ppAttrs){
			K2H_Delete(*ppAttrs);
		}
	}
	return result;
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

	int				removed_count	= 0;
	unsigned char*	ptopkey			= NULL;
	size_t			topkeylen		= 0;

	for(unsigned int cnt = 0; cnt < count; ++cnt){
		if(!ptopkey){
			// Case of removing from top of queue

			//--------------------------------------
			// Read marker without WRITE LOCK
			//--------------------------------------
			PBK2HMARKER				before_marker	= GetMarker(byMark, marklength);
			const unsigned char*	before_startkey	= before_marker ? &(before_marker->byData[before_marker->marker.startoff]) : NULL;
			size_t					before_startlen	= before_marker ? before_marker->marker.startlen : 0;

			if(!before_marker || !before_startkey || 0 == before_startlen){
				// there is no marker or no start key of queue, it means no stacked key in queue.
				K2H_Free(before_marker);
				break;
			}

			// Get subkeys in current start key for next marker's start key(without checking attribute)
			K2HSubKeys*				psubkeys	= GetSubKeys(before_startkey, before_startlen, false);
			const unsigned char*	pnextstart	= NULL;
			size_t					nextstartLen= 0;
			if(psubkeys){
				K2HSubKeys::iterator	iter = psubkeys->begin();
				if(iter != psubkeys->end()){
					pnextstart	= iter->pSubKey;
					nextstartLen= iter->length;
				}
			}

			// Get attribute and check expire(convert to binary structure for callback)
			PK2HATTRPCK	pattrspck	= NULL;
			int			attrspckcnt	= 0;
			if(fp){
				K2HAttrs*	pKeyAttrs;
				if(NULL != (pKeyAttrs = GetAttrs(before_startkey, before_startlen))){
					pattrspck = k2h_cvt_attrs_to_bin(pKeyAttrs, attrspckcnt);
					K2H_Delete(pKeyAttrs);
				}
			}

			// Get value(with checking attribute)
			unsigned char*	pTmpValue = NULL;
			ssize_t			tmpvallen = 0;
			if(rmkeyval){
				if(-1 == (tmpvallen = Get(before_startkey, before_startlen, &pTmpValue, true, encpass))){
					MSG_K2HPRN("Could not get read key value(there is no value or key is expired.)");
					tmpvallen = 0;
				}
			}

			// check by callback function
			K2HQRMCBRES	res = K2HQRMCB_RES_CON_RM;			// default(if null == fp)
			if(fp && pTmpValue){
				// pTmpValue(Poped key's value) is data key for callback
				if(K2HQRMCB_RES_ERROR == (res = fp(pTmpValue, static_cast<size_t>(tmpvallen), pattrspck, attrspckcnt, pExtData))){
					// Stop loop
					K2H_Free(before_marker);
					k2h_free_attrpack(pattrspck, attrspckcnt);
					K2H_Delete(psubkeys);
					K2H_Free(pTmpValue);
					break;
				}
			}
			k2h_free_attrpack(pattrspck, attrspckcnt);

			//--------------------------------------
			// Removing
			//--------------------------------------
			if(K2HQRMCB_RES_CON_RM == res || K2HQRMCB_RES_FIN_RM == res){
				//--------------------------------------
				// Re-Read marker with WRITE LOCK
				//--------------------------------------
				// [NOTE]
				// At first, we lock marker and call Set method which locks marker.
				// Then twice locking for marker, but it does not deadlock because
				// marker does not have subkeys
				//
				K2HLock					ALObjCKI_Marker(K2HLock::RWLOCK);	// auto release locking at leaving in this scope.
				PBK2HMARKER				after_marker	= GetMarker(byMark, marklength, &ALObjCKI_Marker);
				const unsigned char*	after_startkey	= after_marker ? &(after_marker->byData[after_marker->marker.startoff]) : NULL;
				size_t					after_startlen	= after_marker ? after_marker->marker.startlen : 0;
				if(!after_marker){
					MSG_K2HPRN("After reading marker, the marker is empty or wrong size.");

					K2H_Free(before_marker);
					K2H_Delete(psubkeys);
					K2H_Free(pTmpValue);
					break;													// automatically unlock ALObjCKI_Marker
				}

				// Check read key name, compare it and before reading key name.
				if(0 != k2hbincmp(before_startkey, before_startlen, after_startkey, after_startlen)){
					MSG_K2HPRN("Different read key name before and after reading, thus retry from first.");

					K2H_Free(before_marker);
					K2H_Free(after_marker);
					K2H_Delete(psubkeys);
					K2H_Free(pTmpValue);
					ALObjCKI_Marker.Unlock();								// manually unlock
					continue;
				}

				// Make new next K2HMaker
				PBK2HMARKER	pnewmarker;
				size_t		newmarklen	= 0;
				if(NULL == (pnewmarker = K2HShm::InitK2HMarker(newmarklen, pnextstart, nextstartLen, (0 == after_marker->marker.endlen ? NULL : (&(after_marker->byData[0]) + after_marker->marker.endoff)), after_marker->marker.endlen))){
					ERR_K2HPRN("Something error is occurred to make new marker.");

					K2H_Free(before_marker);
					K2H_Free(after_marker);
					K2H_Delete(psubkeys);
					K2H_Free(pTmpValue);
					break;													// automatically unlock ALObjCKI_Marker
				}

				// Set new marker(marker does not have any attribute)
				if(!Set(byMark, marklength, &(pnewmarker->byData[0]), newmarklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
					ERR_K2HPRN("Could not set new marker.");

					K2H_Free(before_marker);
					K2H_Free(after_marker);
					K2H_Delete(psubkeys);
					K2H_Free(pTmpValue);
					K2H_Free(pnewmarker);
					break;													// automatically unlock ALObjCKI_Marker
				}
				K2H_Free(pnewmarker);

				// Unlock marker
				ALObjCKI_Marker.Unlock();									// manually unlock

				// Remove key(without subkey & history)
				if(!Remove(after_startkey, after_startlen, false, NULL, true)){
					ERR_K2HPRN("Could not remove read key, but continue...");
				}
				K2H_Free(after_marker);

				// Remove value(key) with attributes
				if(rmkeyval && pTmpValue){
					if(!Remove(pTmpValue, static_cast<size_t>(tmpvallen), true)){
						ERR_K2HPRN("Could not remove key from k2hash, but continue...");
					}
				}
				removed_count++;

			}else{
				// Set next key, it is not top of queue
				ptopkey		= k2hbindup(before_startkey, before_startlen);
				topkeylen	= before_startlen;
			}
			K2H_Free(before_marker);
			K2H_Delete(psubkeys);
			K2H_Free(pTmpValue);

		}else{
			// Case of removing from middle of queue

			// check end of queue key
			PBK2HMARKER				current_marker	= GetMarker(byMark, marklength);
			const unsigned char*	current_endkey	= current_marker ? &(current_marker->byData[current_marker->marker.endoff]) : NULL;
			size_t					current_endlen	= current_marker ? current_marker->marker.endlen : 0;
			if(!current_marker || !current_endkey || 0 == current_endlen){
				// there is no marker or no end key of queue, thus we do not check end key.
				K2H_Free(current_marker);
				break;
			}
			if(0 == k2hbincmp(current_endkey, current_endlen, ptopkey, topkeylen)){
				MSG_K2HPRN("top normal key is end of queue key. thus stop removing.");
				K2H_Free(current_marker);
				break;
			}

			//--------------------------------------
			// Read key without WRITE LOCK
			//--------------------------------------
			// Get subkeys in top key for next key(without checking attribute)
			K2HSubKeys*				psubkeys	= GetSubKeys(ptopkey, topkeylen, false);
			if(!psubkeys){
				MSG_K2HPRN("reached end of key in top normal key during removing, so stop removing.");
				K2H_Free(current_marker);
				break;
			}
			K2HSubKeys::iterator	iter		= psubkeys->begin();
			if(iter == psubkeys->end()){
				MSG_K2HPRN("reached end of key in top normal key during removing, so stop removing.");
				K2H_Free(current_marker);
				break;
			}
			const unsigned char*	pnextkey	= iter->pSubKey;
			size_t					nextkeyLen	= iter->length;

			// check marker end key as same as top to next key
			bool	is_update_marker = false;
			if(0 == k2hbincmp(current_endkey, current_endlen, pnextkey, nextkeyLen)){
				// need to update marker key
				is_update_marker = true;
			}

			// Get attribute and check expire(convert to binary structure for callback)
			PK2HATTRPCK	pattrspck	= NULL;
			int			attrspckcnt	= 0;
			if(fp){
				K2HAttrs*	pKeyAttrs;
				if(NULL != (pKeyAttrs = GetAttrs(pnextkey, nextkeyLen))){
					pattrspck = k2h_cvt_attrs_to_bin(pKeyAttrs, attrspckcnt);
					K2H_Delete(pKeyAttrs);
				}
			}

			// Get value(with checking attribute)
			unsigned char*	pTmpValue = NULL;
			ssize_t			tmpvallen = 0;
			if(rmkeyval){
				if(-1 == (tmpvallen = Get(pnextkey, nextkeyLen, &pTmpValue, true, encpass))){
					MSG_K2HPRN("Could not get read key value(there is no value or key is expired.)");
					tmpvallen = 0;
				}
			}

			// check by callback function
			K2HQRMCBRES	res = K2HQRMCB_RES_CON_RM;			// default(if null == fp)
			if(fp && pTmpValue){
				// pTmpValue(Poped key's value) is data key for callback
				if(K2HQRMCB_RES_ERROR == (res = fp(pTmpValue, static_cast<size_t>(tmpvallen), pattrspck, attrspckcnt, pExtData))){
					// Stop loop
					K2H_Free(current_marker);
					k2h_free_attrpack(pattrspck, attrspckcnt);
					K2H_Delete(psubkeys);
					K2H_Free(pTmpValue);
					break;
				}
			}
			k2h_free_attrpack(pattrspck, attrspckcnt);

			//--------------------------------------
			// Removing
			//--------------------------------------
			if(K2HQRMCB_RES_CON_RM == res || K2HQRMCB_RES_FIN_RM == res){
				// get next end key
				K2HSubKeys*		pnextendskeys = GetSubKeys(pnextkey, nextkeyLen, false);

				//--------------------------------------
				// Re-Read Key with WRITE LOCK
				//--------------------------------------
				// [NOTE]
				// At first, we lock key and call GetSubkeys/Set method which locks this key.
				// Then twice locking for key, but it does not deadlock because
				// we only read it, and remove subkeys.
				//
				K2HLock		ALObjCKI_TopKey(K2HLock::RWLOCK);				// auto release locking at leaving in this scope.
				k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(ptopkey), topkeylen);
				PCKINDEX	pCKIndex;
				if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI_TopKey))){
					MSG_K2HPRN("normal top queue key does not exist, probabry removing it.");

					K2H_Free(current_marker);
					K2H_Delete(psubkeys);
					K2H_Delete(pnextendskeys);
					K2H_Free(pTmpValue);
					break;													// automatically unlock ALObjCKI_TopKey
				}

				// re-get subkeys and check it
				K2HSubKeys*				presubkeys	= GetSubKeys(ptopkey, topkeylen, false);
				if(!presubkeys){
					MSG_K2HPRN("reached end of key in top normal key during removing, so stop removing.");
					K2H_Free(current_marker);
					K2H_Delete(psubkeys);
					K2H_Delete(pnextendskeys);
					K2H_Free(pTmpValue);
					break;													// automatically unlock ALObjCKI_TopKey
				}
				K2HSubKeys::iterator	reiter		= presubkeys->begin();
				if(reiter == presubkeys->end()){
					MSG_K2HPRN("reached end of key in top normal key during removing, so stop removing.");
					K2H_Free(current_marker);
					K2H_Delete(psubkeys);
					K2H_Delete(pnextendskeys);
					K2H_Delete(presubkeys);
					K2H_Free(pTmpValue);
					break;													// automatically unlock ALObjCKI_TopKey
				}
				// compare subkey
				if(0 == k2hbincmp(pnextkey, nextkeyLen, reiter->pSubKey, reiter->length)){
					MSG_K2HPRN("top normal key is end of queue key. thus stop removing.");
					K2H_Free(current_marker);
					K2H_Delete(psubkeys);
					K2H_Delete(pnextendskeys);
					K2H_Delete(presubkeys);
					K2H_Free(pTmpValue);
					break;													// automatically unlock ALObjCKI_TopKey
				}
				K2H_Delete(presubkeys);

				// replace subkeys to top key
				unsigned char*	bySubkeys = NULL;
				size_t			skeylength= 0UL;
				pnextendskeys->Serialize(&bySubkeys, skeylength);
				if(!ReplaceSubkeys(ptopkey, topkeylen, bySubkeys, skeylength)){
					ERR_K2HPRN("Failed to insert new subkeys into normal top key in queue.");

					K2H_Free(current_marker);
					K2H_Delete(psubkeys);
					K2H_Delete(pnextendskeys);
					K2H_Free(pTmpValue);
					K2H_Free(bySubkeys);
					break;													// automatically unlock ALObjCKI_TopKey
				}
				K2H_Delete(pnextendskeys);
				K2H_Free(bySubkeys);

				// Unlock marker
				ALObjCKI_TopKey.Unlock();									// manually unlock

				// Remove key(without subkey & history)
				if(!Remove(pnextkey, nextkeyLen, false, NULL, true)){
					ERR_K2HPRN("Could not remove read key, but continue...");
				}

				// Remove value(key) with attributes
				if(rmkeyval && pTmpValue){
					if(!Remove(pTmpValue, static_cast<size_t>(tmpvallen), true)){
						ERR_K2HPRN("Could not remove key from k2hash, but continue...");
					}
				}
				removed_count++;

				// special update marker end key if it's needed.
				if(is_update_marker){
					//--------------------------------------
					// Read marker with WRITE LOCK
					//--------------------------------------
					// [NOTE]
					// At first, we lock marker and call Set method which locks marker.
					// Then twice locking for marker, but it does not deadlock because
					// marker does not have subkeys
					//
					K2HLock					ALObjCKI_Marker(K2HLock::RWLOCK);	// auto release locking at leaving in this scope.
					PBK2HMARKER				after_marker	= GetMarker(byMark, marklength, &ALObjCKI_Marker);
					const unsigned char*	after_endkey	= after_marker ? &(after_marker->byData[after_marker->marker.endoff]) : NULL;
					size_t					after_endlen	= after_marker ? after_marker->marker.endlen : 0;
					if(!after_marker){
						MSG_K2HPRN("After reading marker, the marker is empty or wrong size.");

						K2H_Free(current_marker);
						K2H_Delete(psubkeys);
						K2H_Free(pTmpValue);
						break;													// automatically unlock ALObjCKI_Marker
					}

					// Check read key name, compare it and before reading key name.
					if(0 != k2hbincmp(after_endkey, after_endlen, pnextkey, nextkeyLen)){
						MSG_K2HPRN("Different read end key name before and after reading.");

						K2H_Free(current_marker);
						K2H_Free(after_marker);
						K2H_Delete(psubkeys);
						K2H_Free(pTmpValue);
						break;													// automatically unlock ALObjCKI_Marker
					}
					// Make new next K2HMaker
					PBK2HMARKER	pnewmarker;
					size_t		newmarklen	= 0;
					if(NULL == (pnewmarker = K2HShm::InitK2HMarker(newmarklen, (0 == after_marker->marker.startlen ? NULL : (&(after_marker->byData[0]) + after_marker->marker.startoff)), after_marker->marker.startlen, ptopkey, topkeylen))){
						ERR_K2HPRN("Something error is occurred to make new marker.");

						K2H_Free(current_marker);
						K2H_Free(after_marker);
						K2H_Delete(psubkeys);
						K2H_Free(pTmpValue);
						break;													// automatically unlock ALObjCKI_Marker
					}

					// Set new marker(marker does not have any attribute)
					if(!Set(byMark, marklength, &(pnewmarker->byData[0]), newmarklen, NULL, true, NULL, NULL, NULL, K2hAttrOpsMan::OPSMAN_MASK_ALL)){
						ERR_K2HPRN("Could not set new marker.");

						K2H_Free(current_marker);
						K2H_Free(after_marker);
						K2H_Delete(psubkeys);
						K2H_Free(pTmpValue);
						K2H_Free(pnewmarker);
						break;													// automatically unlock ALObjCKI_Marker
					}
					K2H_Free(pnewmarker);
					K2H_Free(after_marker);

					// Unlock marker
					ALObjCKI_Marker.Unlock();									// manually unlock
				}
			}else{
				// Set next key, it is not top of queue
				K2H_Free(ptopkey);
				ptopkey		= k2hbindup(pnextkey, nextkeyLen);
				topkeylen	= nextkeyLen;
			}
			K2H_Free(current_marker);
			K2H_Delete(psubkeys);
			K2H_Free(pTmpValue);

			// check next top key
			if(!ptopkey){
				break;
			}
		}
	}
	K2H_Free(ptopkey);

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
