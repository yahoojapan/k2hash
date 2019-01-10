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
 * CREATE:   Fri Jan 30 2015
 * REVISION:
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <time.h>
#include <errno.h>

#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hqueue.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#define	K2HQUEUE_MAKER		"MARKER"
#define	K2HQUEUE_KEY_FORM	"%016X_%016zX_%016lX"

//---------------------------------------------------------
// K2HUniqTimespec Class
//---------------------------------------------------------
// This class is uniq timespec generator for FIFO/LIFO key name(transaction etc).
// Increasing timespec structure with mutex is faster than clock_gettime with
// CLOCK_MONOTONIC(etc) at every time.
// If this logic is low performance, should change assap.
//
class K2HUniqTimespec
{
	protected:
		struct timespec	uniq_count;					// uses for building uniq key name
		int				LockVal;					// like mutex for variables

	public:
		static K2HUniqTimespec* Get(void);

		void GetUniqTimespec(struct timespec& ts);

	protected:
		K2HUniqTimespec();
		virtual ~K2HUniqTimespec();
};

//---------------------------------------------------------
// K2HUniqTimespec : Methods
//---------------------------------------------------------
// [NOTE]
// To avoid static object initialization order problem(SIOF)
//
K2HUniqTimespec* K2HUniqTimespec::Get(void)
{
	static K2HUniqTimespec	uniqts;					// singleton
	return &uniqts;
}

K2HUniqTimespec::K2HUniqTimespec() : LockVal(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED)
{
	if(-1 == clock_gettime(CLOCK_REALTIME, &uniq_count)){
		WAN_K2HPRN("Could not get timespec(errno=%d), but continue with other initial value...", errno);
		uniq_count.tv_sec	= time(NULL);
		uniq_count.tv_nsec	= 0L;
	}
}

K2HUniqTimespec::~K2HUniqTimespec()
{
}

void K2HUniqTimespec::GetUniqTimespec(struct timespec& ts)
{
	// count up
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()

	uniq_count.tv_nsec++;
	if((1000 * 1000 * 1000) <= uniq_count.tv_nsec){
		uniq_count.tv_sec++;
		uniq_count.tv_nsec = 0;
	}
	ts.tv_sec	= uniq_count.tv_sec;
	ts.tv_nsec	= uniq_count.tv_nsec;

	fullock::flck_unlock_noshared_mutex(&LockVal);
}

//---------------------------------------------------------
// K2HQueue Class : Variables
//---------------------------------------------------------
// The builtin prefix is "\0K2HQUEUE_PREFIX_".
//
const unsigned char	K2HQueue::builtin_pref[] = {'\0', 'K', '2', 'H', 'Q', 'U', 'E', 'U', 'E', '_', 'P', 'R', 'E', 'F', 'I', 'X', '_'};

//---------------------------------------------------------
// K2HQueue Class : Class Method
//---------------------------------------------------------
unsigned char* K2HQueue::GetMarkerName(const unsigned char* pref, size_t preflen, size_t& markerlength)
{
	if(!pref || 0 == preflen){
		ERR_K2HPRN("Parameters are wrong.");
		return NULL;
	}
	// make marker key
	unsigned char*	pmarker;
	markerlength = 0;
	if(NULL == (pmarker = k2hbinappendstr(pref, preflen, K2HQUEUE_MAKER, markerlength))){
		ERR_K2HPRN("Could not make marker.");
	}
	return pmarker;
}

unsigned char* K2HQueue::GetUniqKeyName(const unsigned char* pref, size_t preflen, pid_t tid, size_t& keylength)
{
	if(!pref || 0 == preflen){
		ERR_K2HPRN("Parameters are wrong.");
		return NULL;
	}
	// get counter
	struct timespec	uniq_count = {0, 0};
	K2HUniqTimespec::Get()->GetUniqTimespec(uniq_count);

	// uniq key base string
	char	buff[(16 + 1) * 3];			// ((8 + 16 + 8)->(16 * 3)) byte + separator + nil
	sprintf(buff, K2HQUEUE_KEY_FORM, tid, uniq_count.tv_sec, uniq_count.tv_nsec);

	// make uniq key
	unsigned char*	bykey;
	keylength = 0;
	if(NULL == (bykey = k2hbinappendstr(pref, preflen, buff, keylength))){
		ERR_K2HPRN("Could not make marker.");
	}
	return bykey;
}

//---------------------------------------------------------
// K2HQueue Class : Methods
//---------------------------------------------------------
K2HQueue::K2HQueue(K2HShm* pk2h, bool is_fifo, const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type) : pK2HShm(pk2h), isFIFO(is_fifo), prefix(NULL), prefix_len(0UL), marker(NULL), marker_len(0UL), tid(0), attrtype(type)
{
	if(pK2HShm){
		Init(pref, preflen);
	}
	tid = gettid();		// for uniq key name
}

K2HQueue::~K2HQueue()
{
	Clear();
}

bool K2HQueue::Clear(void)
{
	prefix_len	= 0UL;
	marker_len	= 0UL;
	K2H_Free(prefix);
	K2H_Free(marker);

	return true;
}

bool K2HQueue::Init(K2HShm* pk2h, bool is_fifo, const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
{
	if(!pk2h){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	pK2HShm = pk2h;
	isFIFO	= is_fifo;

	return Init(pref, preflen, type);
}

bool K2HQueue::Init(const unsigned char* pref, size_t preflen, K2hAttrOpsMan::ATTRINITTYPE type)
{
	if((NULL == pref) != (0L == preflen)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsInit()){
		ERR_K2HPRN("This object is not initialized.");
		return false;
	}
	Clear();

	// keep prefix
	if(!pref){
		prefix		= k2hbindup(builtin_pref, sizeof(builtin_pref));
		prefix_len	= sizeof(builtin_pref);
	}else{
		prefix		= k2hbindup(pref, preflen);
		prefix_len	= preflen;
	}

	// make marker key
	if(NULL == (marker = K2HQueue::GetMarkerName(prefix, prefix_len, marker_len))){
		ERR_K2HPRN("Could not make marker.");
		Clear();
		return false;
	}
	attrtype = type;

	return true;
}

unsigned char* K2HQueue::GetUniqKey(size_t& keylen)
{
	if(!IsInit()){
		ERR_K2HPRN("This object is not initialized.");
		return NULL;
	}
	unsigned char*	bykey;
	if(NULL == (bykey = K2HQueue::GetUniqKeyName(prefix, prefix_len, tid, keylen))){
		ERR_K2HPRN("Could not make uniq key name for queue.");
	}
	return bykey;
}

bool K2HQueue::IsEmpty(void) const
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	return pK2HShm->IsEmptyQueue(marker, marker_len);
}

int K2HQueue::GetCount(void) const
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return 0;
	}
	return pK2HShm->GetCountQueue(marker, marker_len);
}

bool K2HQueue::Read(unsigned char** ppdata, size_t& datalen, int pos, const char* encpass) const
{
	if(!ppdata){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}

	unsigned char*	pKey	= NULL;
	size_t			keylen	= 0;
	*ppdata					= NULL;
	datalen					= 0;
	if(!pK2HShm->ReadQueue(marker, marker_len, &pKey, keylen, ppdata, datalen, pos, encpass)){
		ERR_K2HPRN("Something error occurred during popping.");
		K2H_Free(pKey);
		K2H_Free(*ppdata);
		return false;
	}
	K2H_Free(pKey);

	return true;
}

bool K2HQueue::Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}

	// make key automatically
	unsigned char*	bykey;
	size_t			keylen = 0;
	if(NULL == (bykey = GetUniqKey(keylen))){
		ERR_K2HPRN("Could not get uniq key.");
		return false;
	}

	// Push
	bool	bResult;
	if(isFIFO){
		bResult = pK2HShm->PushFifoQueue(marker, marker_len, bykey, keylen, bydata, datalen, attrtype, pAttrs, encpass, expire);
	}else{
		bResult = pK2HShm->PushLifoQueue(marker, marker_len, bykey, keylen, bydata, datalen, attrtype, pAttrs, encpass, expire);
	}
	K2H_Free(bykey);

	return bResult;
}

//
// [NOTICE]
// If there is no popping data in k2hshm, this method returns true with *ppdata=NULL.
// Return false means something error is occurred.
//
bool K2HQueue::Pop(unsigned char** ppdata, size_t& datalen, K2HAttrs** ppAttrs, const char* encpass)
{
	if(!ppdata){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}

	unsigned char*	pKey	= NULL;
	size_t			keylen	= 0;
	*ppdata					= NULL;
	datalen					= 0;
	if(!pK2HShm->PopQueue(marker, marker_len, &pKey, keylen, ppdata, datalen, ppAttrs, encpass)){
		ERR_K2HPRN("Something error occurred during popping.");
		K2H_Free(pKey);
		K2H_Free(*ppdata);
		return false;
	}
	K2H_Free(pKey);

	return true;
}

int K2HQueue::Remove(int count, k2h_q_remove_trial_callback fp, void* pExtData, const char* encpass)
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	return pK2HShm->RemoveQueue(marker, marker_len, count, false, fp, pExtData, encpass);
}

bool K2HQueue::Dump(FILE* stream)
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	if(!pK2HShm->DumpQueue(stream, marker, marker_len)){
		ERR_K2HPRN("Something error occurred during dumping.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// K2HKeyQueue Class : Methods
//---------------------------------------------------------
bool K2HKeyQueue::Read(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, int pos, const char* encpass) const
{
	if(!ppkey || !ppval){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppval	= NULL;
	vallen	= 0;

	// Pop key name(= queued data)
	*ppkey	= NULL;
	keylen	= 0;
	if(!K2HQueue::Read(ppkey, keylen, pos, encpass)){
		ERR_K2HPRN("Something error occurred during popping.");
		return false;
	}

	// Get value by key.(with checking attribute)
	*ppval 	= NULL;
	ssize_t	valuelength;
	if(-1 == (valuelength = pK2HShm->Get(*ppkey, keylen, ppval, true, encpass))){
		ERR_K2HPRN("There is no key or failed to getting value by key.");
		K2H_Free(*ppkey);
		return false;
	}
	vallen = static_cast<size_t>(valuelength);

	return true;
}

bool K2HKeyQueue::Read(unsigned char** ppdata, size_t& datalen, int pos, const char* encpass) const
{
	unsigned char*	pkey	= NULL;
	size_t			keylen	= 0;
	bool			result	= Read(&pkey, keylen, ppdata, datalen, pos, encpass);

	K2H_Free(pkey);
	return result;
}

bool K2HKeyQueue::Push(const unsigned char* pkey, size_t keylen, const unsigned char* byval, size_t vallen, const char* encpass, const time_t* expire)
{
	if(!pkey || 0 == keylen){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}

	// write key and value
	if(!pK2HShm->Set(pkey, keylen, byval, vallen, encpass, expire)){
		ERR_K2HPRN("Something error occurred during writing key-value.");
		return false;
	}

	// set queue
	return K2HQueue::Push(pkey, keylen, NULL, encpass, expire);
}

bool K2HKeyQueue::Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	WAN_K2HPRN("K2HKeyQueue::Push should not use directly.");

	return Push(bydata, datalen, NULL, 0UL, encpass, expire);
}

bool K2HKeyQueue::Pop(unsigned char** ppkey, size_t& keylen, unsigned char** ppval, size_t& vallen, const char* encpass)
{
	if(!ppkey || !ppval){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	*ppval	= NULL;
	vallen	= 0;

	// Pop key name(= queued data)
	*ppkey	= NULL;
	keylen	= 0;
	if(!K2HQueue::Pop(ppkey, keylen, NULL, encpass)){
		ERR_K2HPRN("Something error occurred during popping.");
		return false;
	}

	// Get value by key.(with checking attribute)
	unsigned char*	pvalue = NULL;
	ssize_t			valuelength;
	if(-1 == (valuelength = pK2HShm->Get(*ppkey, keylen, &pvalue, true, encpass))){
		ERR_K2HPRN("There is no key or failed to getting value by key.");
		K2H_Free(*ppkey);
		return false;
	}
	*ppval	= pvalue;
	vallen	= static_cast<size_t>(valuelength);

	// Remove key and value from k2hash
	if(!pK2HShm->Remove(*ppkey, keylen, true)){
		ERR_K2HPRN("Could not remove key from k2hash, bu continue...");
	}
	return true;
}

// This method returns the value which is pointed by "Key".
// The "Key" is stored as value in Queue.
//
bool K2HKeyQueue::Pop(unsigned char** ppdata, size_t& datalen, K2HAttrs** ppAttrs, const char* encpass)
{
	// K2HKeyQueue's key in queue list always does not have attribute.
	//
	if(ppAttrs){
		*ppAttrs = NULL;
	}
	WAN_K2HPRN("K2HKeyQueue::Pop should not use directly.");

	unsigned char*	pkey	= NULL;
	size_t			keylen	= 0;
	bool			result	= Pop(&pkey, keylen, ppdata, datalen, encpass);

	K2H_Free(pkey);
	return result;
}

//
// This method removes stored data("Key" name)s from queue.
// And removes the "Value"s which is pointed by "Key".
//
int K2HKeyQueue::Remove(int count, k2h_q_remove_trial_callback fp, void* pExtData, const char* encpass)
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	return pK2HShm->RemoveQueue(marker, marker_len, count, true, fp, pExtData, encpass);
}

//---------------------------------------------------------
// K2HLowOpsQueue Class : Methods
//---------------------------------------------------------
//
// Read next pop queue key name
// ppdata	- next queue key binary array
// datalen	- next queue key length
//
bool K2HLowOpsQueue::Read(unsigned char** ppdata, size_t& datalen, int pos, const char* encpass) const
{
	if(!ppdata || (0 != pos && -1 != pos) || encpass){				// [NOTE] do not use encpass and pos must be 0 or -1
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	return pK2HShm->ReadQueue(marker, marker_len, ppdata, datalen, pos);
}

//
// Push queue key name to marker
// bydata	- pushing queue key binary array
// datalen	- pushing queue key length
//
bool K2HLowOpsQueue::Push(const unsigned char* bydata, size_t datalen, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	if(!bydata || 0 == datalen || pAttrs || encpass || expire){		// [NOTE] do not use pAttrs, encpass and expire.
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	return pK2HShm->AddQueue(marker, marker_len, bydata, datalen, isFIFO);
}

//
// Remove next queue key name from marker and set new next queue key
//
bool K2HLowOpsQueue::ReplaceTopQueueKey(const unsigned char* preplacekey, size_t replacekeylen)
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return false;
	}
	return pK2HShm->UpdateStartK2HMarker(marker, marker_len, preplacekey, replacekeylen);
}

//
// Get queue key name which is edge(top or bottom) key in queue list.
//
unsigned char* K2HLowOpsQueue::GetEdgeQueueKey(size_t& keylen, bool is_top) const
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return NULL;
	}
	unsigned char*	pkey = NULL;
	if(!Read(&pkey, keylen, (is_top ? 0 : -1), NULL)){		// [NOTE] -1 means lastest position, always encpass is NULL
		ERR_K2HPRN("Failed to read %s queue key name for %s.", (is_top ? "Top" : "Bottom"), (isFIFO ? "FIFO" : "LIFO"));
		return NULL;
	}
	return pkey;
}

//
// Get next popped queue key name.
//
unsigned char* K2HLowOpsQueue::GetPlannedPopQueueKey(size_t& keylen) const
{
	if(!IsSafe()){
		ERR_K2HPRN("This object is not safe.");
		return NULL;
	}
	unsigned char*	pkey = NULL;
	if(!Read(&pkey, keylen, 0, NULL)){						// [NOTE] always get from top of queue and encpass is NULL
		ERR_K2HPRN("Failed to read top queue key name for %s.", (isFIFO ? "FIFO" : "LIFO"));
		return NULL;
	}
	return pkey;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
