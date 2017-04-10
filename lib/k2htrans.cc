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
 * CREATE:   Mon Feb 10 2014
 * REVISION:
 *
 */
#include <assert.h>
#include <string>

#include <fullock/flckstructure.h>
#include <fullock/flckbaselist.tcc>

#include "k2hash.h"
#include "k2htrans.h"
#include "k2htransfunc.h"
#include "k2hcommand.h"
#include "k2hqueue.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// K2HTransaction Class
//---------------------------------------------------------
K2HTransaction::K2HTransaction(const K2HShm* pk2hshm, bool isstack) : K2HCommandArchive(), pShm(pk2hshm), IsStackMode(isstack)
{
}

K2HTransaction::~K2HTransaction()
{
}

bool K2HTransaction::Put(long type, const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, const unsigned char* bySKey, size_t skeylength, const unsigned char* byAttrs, size_t attrlength, const unsigned char* byExdata, size_t exdatalength)
{
	if(!pShm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsEnable()){
		// do not need transaction.
		return true;
	}

	//
	// Check which byKey is transaction name or not.
	//
	// [NOTICE]
	// This function is called from K2HTransManager::Put(), too.
	// That case is that transaction data (pBinCom) is queued in k2hash.
	// So k2hash cuts transaction data on this case.
	//
	if(K2HTransManager::Get()->CompareTransactionKeyPrefix(pShm, byKey, keylength)){
		// byKey is transaction key, so do nothing.
		return true;
	}

	if(!K2HCommandArchive::Put(type, byKey, keylength, byVal, vallength, bySKey, skeylength, byAttrs, attrlength, byExdata, exdatalength)){
		return false;
	}

	if(IsStackMode){
		// only stacking, not put
		return true;
	}
	return K2HTransManager::Get()->Put(pShm, pBinCom);
}

//
// Only push stacked data
//
bool K2HTransaction::Put(void)
{
	if(!pShm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsStackMode || !pBinCom){
		// do not need transaction.
		return true;
	}
	if(!IsEnable()){
		// do not need transaction.
		return true;
	}
	return K2HTransManager::Get()->Put(pShm, pBinCom);
}

bool K2HTransaction::IsEnable(void) const
{
	if(!pShm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	return K2HTransManager::Get()->isEnableWithoutLock(pShm);	// without locking
}

//---------------------------------------------------------
// K2HTransManager : Class valiables
//---------------------------------------------------------
const int			K2HTransManager::NO_THREAD_POOL;
const int			K2HTransManager::DEFAULT_THREAD_POOL;
const int			K2HTransManager::MAX_THREAD_POOL;
const time_t		K2HTransManager::DEFAULT_TREAD_COND_WAIT;
const long			K2HTransManager::MINIMUM_WAIT_SEEP;
const long			K2HTransManager::FINISH_WAIT_BLOCK;
const unsigned char	K2HTransManager::default_prefix[] = {'\0', 'K', '2', 'H', 'T', 'R', 'A', 'N', 'S', '_', 'P', 'R', 'E', 'F', 'I', 'X', '_'};
const time_t		K2HTransManager::DEFAULT_INTERVAL;
K2HTransManager		K2HTransManager::singleton;

//---------------------------------------------------------
// K2HTransManager : Class Methods
//---------------------------------------------------------
void* K2HTransManager::WorkerProc(void* param)
{
	PTRTHPARAM	ptrparam = reinterpret_cast<PTRTHPARAM>(param);
	if(!ptrparam){
		ERR_K2HPRN("The parameter pointer is NULL.");
		pthread_exit(NULL);
	}

	// Loop
	struct timespec	timeout;
	int				result;
	K2HQueue		queue;
	while(!ptrparam->is_exit){
		// reset timespec
		timeout.tv_sec	= time(NULL) + K2HTransManager::DEFAULT_TREAD_COND_WAIT;
		timeout.tv_nsec	= 0L;

		// wait cond
		pthread_mutex_lock(ptrparam->ptrmutex);

		if(0 != (result = pthread_cond_timedwait(ptrparam->ptrcond, ptrparam->ptrmutex, &timeout))){
			if(ETIMEDOUT == result){
				// Timeouted, on this case should check queue.
				//MSG_K2HPRN("Timeouted waiting cond.");
			}else if(EINTR == result){
				//MSG_K2HPRN("Caught signal.");
				pthread_mutex_unlock(ptrparam->ptrmutex);
				continue;
			}else{
				ERR_K2HPRN("Something error occurred for waiting cond, return code(error) = %d", result);
				pthread_mutex_unlock(ptrparam->ptrmutex);
				break;
			}
		}
		pthread_mutex_unlock(ptrparam->ptrmutex);

		// check break
		if(ptrparam->is_exit){
			break;
		}

		// attach
		if(!queue.Init(reinterpret_cast<K2HShm*>(ptrparam->handle), true, ptrparam->pprefix, ptrparam->prefixlen, K2hAttrOpsMan::OPSMAN_MASK_TRANSQUEUEKEY)){
			ERR_K2HPRN("Could not attach k2hqueue.");
			break;
		}

		// loop - pop queue
		unsigned char*	pval	= NULL;
		size_t			vallen	= 0;
		K2HAttrs*		pAttrs	= NULL;								// for keep exipre time
		while(queue.Pop(&pval, vallen, &pAttrs) && pval){
			// call handler
			PBCOM	pBinCom = reinterpret_cast<PBCOM>(pval);
			if(!K2H_TRANS_FUNC(ptrparam->handle, pBinCom)){
				ERR_K2HPRN("Something error occurred in transaction function(handler), try to recover(repush the data to queue)");

				// make new queue as LIFO(not FIFO) for pushing as recovering.
				K2HQueue	recover_queue(reinterpret_cast<K2HShm*>(ptrparam->handle), false, ptrparam->pprefix, ptrparam->prefixlen, K2hAttrOpsMan::OPSMAN_MASK_TRANSQUEUEKEY);
				if(!recover_queue.Push(pval, vallen, pAttrs)){		// keep expire time(K2HQueue for transaction does not set attributes except expire time)
					ERR_K2HPRN("Something error occurred during recover(repush the data to queue), so this value is lost.");
				}
				K2H_Free(pval);
				break;
			}
			K2H_Free(pval);
			K2H_Delete(pAttrs);
			vallen = 0;
		}
		K2H_Delete(pAttrs);
	}
	return NULL;
}

//
// The handle is const K2HShm object pointer.
// If you need to access this object without const, you can do it carefully.
//
bool K2HTransManager::Do(k2h_h handle, PBCOM pBinCom)
{
	return K2H_TRANS_FUNC(handle, pBinCom);
}

//---------------------------------------------------------
// K2HTransManager : Constructor / Destructor
//---------------------------------------------------------
K2HTransManager::K2HTransManager() : LockVal(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), LockPool(FLCK_NOSHARED_MUTEX_VAL_UNLOCKED), threadcnt(0), interval(K2HTransManager::DEFAULT_INTERVAL)
{
	if(this == K2HTransManager::Get()){
		trfilemap.clear();
		trprefmap.clear();
		trpoolmap.clear();
	}else{
		assert(false);
	}
}

K2HTransManager::~K2HTransManager()
{
	if(this == K2HTransManager::Get()){
		for(trprefmap_t::iterator iter = trprefmap.begin(); iter != trprefmap.end(); trprefmap.erase(iter++)){
			Stop(iter->first, false);
			K2H_Delete(iter->second);
		}
	}else{
		assert(false);
	}
}

//---------------------------------------------------------
// K2HTransManager : Methods
//---------------------------------------------------------
bool K2HTransManager::SetTransactionKeyPrefix(const K2HShm* pk2hshm, const unsigned char* pprefix, size_t prefixlen, const time_t* expire)
{
	if(!pk2hshm || (NULL == pprefix) != (0 == prefixlen)){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()

	trprefmap_t::iterator	iter;
	if(trprefmap.end() != (iter = trprefmap.find(pk2hshm))){
		K2H_Delete(iter->second);
		trprefmap.erase(iter);
	}

	PTRK2HPREF	ptrpref;
	if(pprefix){
		ptrpref = new TRK2HPREF(pprefix, prefixlen, expire);
	}else{
		ptrpref = new TRK2HPREF(K2HTransManager::default_prefix, sizeof(K2HTransManager::default_prefix), expire);
	}
	trprefmap[pk2hshm] = ptrpref;

	fullock::flck_unlock_noshared_mutex(&LockVal);

	return true;
}

bool K2HTransManager::RemoveTransactionKeyPrefix(const K2HShm* pk2hshm)
{
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()

	trprefmap_t::iterator	iter;
	if(trprefmap.end() != (iter = trprefmap.find(pk2hshm))){
		K2H_Delete(iter->second);
		trprefmap.erase(iter);
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);

	return true;
}

bool K2HTransManager::RemoveAllTransactionKeyPrefix(void)
{
	for(trprefmap_t::iterator iter = trprefmap.begin(); iter != trprefmap.end(); trprefmap.erase(iter++)){
		K2H_Delete(iter->second);
	}
	return true;
}

bool K2HTransManager::CompareTransactionKeyPrefix(const K2HShm* pk2hshm, const unsigned char* pkey, size_t keylen)
{
	if(!pk2hshm || !pkey || 0 == keylen){
		return false;
	}
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()

	trprefmap_t::const_iterator	iter;
	if(trprefmap.end() == (iter = trprefmap.find(pk2hshm))){
		fullock::flck_unlock_noshared_mutex(&LockVal);
		return false;
	}
	if(keylen < iter->second->prefixlen){
		fullock::flck_unlock_noshared_mutex(&LockVal);
		return false;
	}
	bool	bResult = (0 == memcmp(pkey, iter->second->pprefix, iter->second->prefixlen));

	fullock::flck_unlock_noshared_mutex(&LockVal);

	return bResult;
}

const unsigned char* K2HTransManager::GetTransactionKeyPrefix(const K2HShm* pk2hshm, size_t& prefixlen)
{
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()

	trprefmap_t::const_iterator	iter;
	if(trprefmap.end() == (iter = trprefmap.find(pk2hshm))){
		// This case, the prefix does not have been initialized yet.
		fullock::flck_unlock_noshared_mutex(&LockVal);

		if(!ResetTransactionKeyPrefix(pk2hshm)){
			ERR_K2HPRN("Could not initialize prefix for target.");
			return NULL;
		}
		while(!fullock::flck_trylock_noshared_mutex(&LockVal));	// no call sched_yield()
	}
	const unsigned char*	pReturn;
	pReturn		= iter->second->pprefix;
	prefixlen	= iter->second->prefixlen;

	fullock::flck_unlock_noshared_mutex(&LockVal);

	return pReturn;
}

PTRFILEINFO K2HTransManager::GetFileInfo(const K2HShm* pk2hshm)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameters are wrong.");
		return NULL;
	}

	trfilemap_t::iterator iter = trfilemap.find(pk2hshm);
	if(trfilemap.end() == iter){
		return NULL;
	}

	return iter->second;
}

bool K2HTransManager::Stop(const K2HShm* pk2hshm)
{
	return Stop(pk2hshm, true);
}

bool K2HTransManager::Stop(const K2HShm* pk2hshm, bool is_remove_prefix)
{
	if(pk2hshm){
		while(!fullock::flck_trylock_noshared_mutex(&LockVal));	// no call sched_yield()

		trfilemap_t::iterator iter;
		if(trfilemap.end() != (iter = trfilemap.find(pk2hshm))){
			if(!iter->second){
				WAN_K2HPRN("Transaction File info is NULL.");
			}else{
				K2H_CLOSE(iter->second->arfd);
			}
			K2H_Delete(iter->second);
			trfilemap.erase(iter);
		}
		fullock::flck_unlock_noshared_mutex(&LockVal);

		// clear prefix
		if(is_remove_prefix && !RemoveTransactionKeyPrefix(pk2hshm)){
			ERR_K2HPRN("Could not remove prefix for target.");
		}

		// stop thread for target
		if(!ExitThreads(pk2hshm)){
			ERR_K2HPRN("Could not stop threads for target.");
		}

	}else{
		while(!fullock::flck_trylock_noshared_mutex(&LockVal));	// no call sched_yield()

		// All stop
		for(trfilemap_t::iterator iter = trfilemap.begin(); iter != trfilemap.end(); trfilemap.erase(iter++)){
			if(!iter->second){
				WAN_K2HPRN("Transaction File info is NULL.");
			}else{
				K2H_CLOSE(iter->second->arfd);
			}
			K2H_Delete(iter->second);
		}
		fullock::flck_unlock_noshared_mutex(&LockVal);

		// remove all prefix
		if(!RemoveAllTransactionKeyPrefix()){
			ERR_K2HPRN("Could not remove prefix for all.");
		}

		// stop all threads.
		if(!ExitAllThreads()){
			ERR_K2HPRN("Could not stop threads for target.");
		}
	}
	return true;
}

bool K2HTransManager::Start(const K2HShm* pk2hshm, const char* pFile, const unsigned char* pprefix, size_t prefixlen, const time_t* expire)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm or file path is empty.");
		return false;
	}
	if(ISEMPTYSTR(pFile)){
		MSG_K2HPRN("Transaction file path is empty.");
	}

	// stop if running
	Stop(pk2hshm);

	// set prefix
	if(!SetTransactionKeyPrefix(pk2hshm, pprefix, prefixlen, expire)){
		ERR_K2HPRN("Something error occurred during setting prefix.");
		return false;
	}

	// make file information
	PTRFILEINFO	pFileInfo = new TRFILEINFO;
	if(ISEMPTYSTR(pFile)){
		while(!fullock::flck_trylock_noshared_mutex(&LockVal));	// no call sched_yield()

		pFileInfo->filepath.erase();
		pFileInfo->arfd			= -1;
		pFileInfo->last_update	= time(NULL);
		memset(&pFileInfo->statbuf, 0, sizeof(struct stat));
		trfilemap[pk2hshm]		= pFileInfo;

		fullock::flck_unlock_noshared_mutex(&LockVal);

	}else{
		if(-1 == (pFileInfo->arfd = open(pFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
			ERR_K2HPRN("Could not open/create file: errno(%d)", errno);
			K2H_Delete(pFileInfo);
			return false;
		}
		if(-1 == fstat(pFileInfo->arfd, &(pFileInfo->statbuf))){
			ERR_K2HPRN("Could not get file stat: errno(%d)", errno);
			K2H_CLOSE(pFileInfo->arfd);
			K2H_Delete(pFileInfo);
			return false;
		}
		while(!fullock::flck_trylock_noshared_mutex(&LockVal));	// no call sched_yield()

		pFileInfo->filepath		= pFile;
		pFileInfo->last_update	= time(NULL);
		trfilemap[pk2hshm]		= pFileInfo;

		fullock::flck_unlock_noshared_mutex(&LockVal);
	}

	// start thread pool
	if(!CreateThreads(pk2hshm)){
		ERR_K2HPRN("Could not start thread pool.");
		return false;
	}
	return true;
}

bool K2HTransManager::isEnable(const K2HShm* pk2hshm, bool with_lock)
{
	if(!pk2hshm){
		ERR_K2HPRN("pk2hshm is NULL.");
		return false;
	}
	if(with_lock){
		while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()
	}

	PTRFILEINFO	pFileInfo;
	if(NULL == (pFileInfo = GetFileInfo(pk2hshm))){
		if(with_lock){
			fullock::flck_unlock_noshared_mutex(&LockVal);
		}
		return false;
	}
	if(-1 == pFileInfo->arfd && !pFileInfo->filepath.empty()){
		if(with_lock){
			fullock::flck_unlock_noshared_mutex(&LockVal);
		}
		return false;
	}
	if(with_lock){
		fullock::flck_unlock_noshared_mutex(&LockVal);
	}
	return true;
}

// [NOTE]
// This method does not lock LockVal, MUST lock it before call this method.
//
bool K2HTransManager::CheckFile(const K2HShm* pk2hshm)
{
	if(!isEnable(pk2hshm, false)){
		return true;
	}

	PTRFILEINFO	pFileInfo;
	if(NULL == (pFileInfo = GetFileInfo(pk2hshm))){
		return true;
	}

	time_t	now = time(NULL);
	if(now < (pFileInfo->last_update + interval)){
		// no update
		return true;
	}

	if(-1 == pFileInfo->arfd && pFileInfo->filepath.empty()){
		// This file infomation means that transaction loadable module does not use a file.
		return true;
	}

	struct stat	stattmp;
	if(-1 != stat(pFileInfo->filepath.c_str(), &stattmp) && stattmp.st_dev == pFileInfo->statbuf.st_dev && stattmp.st_ino == pFileInfo->statbuf.st_ino){
		// update
		pFileInfo->last_update = now;
		return true;
	}

	// reopen file
	MSG_K2HPRN("Need to reopen file.");
	K2H_CLOSE(pFileInfo->arfd);

	if(-1 == (pFileInfo->arfd = open(pFileInfo->filepath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
		ERR_K2HPRN("Could not reopen/create file: errno(%d)", errno);
		return false;
	}
	if(-1 == fstat(pFileInfo->arfd, &(pFileInfo->statbuf))){
		ERR_K2HPRN("Could not get reopened file stat: errno(%d)", errno);
		K2H_CLOSE(pFileInfo->arfd);
		return false;
	}
	return true;
}

//
// [NOTICE]
// The archive file discripter(arfd) is not locked as value.
// So when using arfd, it has already been closed.
// Thus the process which calls this method and uses arfd will
// get some error for reading/writing to it.
//
int K2HTransManager::GetArchiveFd(const K2HShm* pk2hshm)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameters are wrong.");
		return -1;
	}
	while(!fullock::flck_trylock_noshared_mutex(&LockVal));		// no call sched_yield()

	if(!isEnable(pk2hshm, false)){
		fullock::flck_unlock_noshared_mutex(&LockVal);
		return -1;
	}
	if(!CheckFile(pk2hshm)){
		fullock::flck_unlock_noshared_mutex(&LockVal);
		return -1;
	}

	PTRFILEINFO	pFileInfo;
	if(NULL == (pFileInfo = GetFileInfo(pk2hshm))){
		fullock::flck_unlock_noshared_mutex(&LockVal);
		return -1;
	}

	if(-1 == pFileInfo->arfd && pFileInfo->filepath.empty()){
		// This file infomation means that transaction loadable module does not use a file.
		WAN_K2HPRN("Transaction is enabled without transaction file path, so this case is for custom loadable module. WHY come here?");
	}
	fullock::flck_unlock_noshared_mutex(&LockVal);

	return pFileInfo->arfd;
}

bool K2HTransManager::Put(const K2HShm* pk2hshm, PBCOM pBinCom)
{
	if(!pk2hshm || !pBinCom){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!isEnable(pk2hshm)){
		return true;
	}

	bool						bResult;
	trpoolmap_t::const_iterator	iter;
	if(trpoolmap.end() == (iter = trpoolmap.find(pk2hshm))){
		// have no thread pool
		if(false == (bResult = K2HTransManager::Do(reinterpret_cast<k2h_h>(pk2hshm), pBinCom))){
			ERR_K2HPRN("Something error occurred in transaction function.");
		}
	}else{
		// have thread pool

		// get prefix
		// [NOTICE]
		// without locking mutex...
		//
		const unsigned char*		pprefix		= NULL;
		size_t						prefixlen	= 0;
		time_t*						expire		= NULL;
		trprefmap_t::const_iterator	iter2;
		if(trprefmap.end() != (iter2 = trprefmap.find(pk2hshm))){
			pprefix		= iter2->second->pprefix;
			prefixlen	= iter2->second->prefixlen;
			expire		= iter2->second->is_set_expire ? &(iter2->second->expire) : NULL;
		}

		// put to queue
		K2HQueue	k2hq(const_cast<K2HShm*>(pk2hshm), true, pprefix, prefixlen, K2hAttrOpsMan::OPSMAN_MASK_TRANSQUEUEKEY);
		if(false == (bResult = k2hq.Push(pBinCom->byData, scom_total_length(pBinCom->scom), NULL, NULL, expire))){
			ERR_K2HPRN("Something error occurred during transaction queuing.");
		}else{
			// wakeup one of thread
			int	result;
			pthread_mutex_lock(&(iter->second->trmutex));
			if(0 != (result = pthread_cond_signal(&(iter->second->trcond)))){
				ERR_K2HPRN("Could not signal to cond, return code(error) = %d", result);
				bResult = false;
			}
			pthread_mutex_unlock(&(iter->second->trmutex));
		}
	}
	return bResult;
}

bool K2HTransManager::CreateThreads(const K2HShm* pk2hshm)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(0 == threadcnt){
		MSG_K2HPRN("Not need to create thread pool, why come here?");
		return true;
	}

	// get prefix
	size_t					prefixlen	= 0;
	const unsigned char*	pprefix		= GetTransactionKeyPrefix(pk2hshm, prefixlen);
	if(!pprefix){
		ERR_K2HPRN("There is no prefix information.");
		return false;
	}
	while(!fullock::flck_trylock_noshared_mutex(&LockPool));	// no call sched_yield()

	if(trpoolmap.end() != trpoolmap.find(pk2hshm)){
		MSG_K2HPRN("Already threads running.");
		fullock::flck_unlock_noshared_mutex(&LockPool);
		return true;
	}

	PTRTHPOOL	pthpool = new TRTHPOOL;

	// initialize mutex
	pthread_mutex_init(&(pthpool->trmutex), NULL);

	// initialize cond
	int	result;
	if(0 != (result = pthread_cond_init(&(pthpool->trcond), NULL))){
		ERR_K2HPRN("Could not initialize cond, return code(error) = %d", result);
		pthread_mutex_destroy(&(pthpool->trmutex));
		fullock::flck_unlock_noshared_mutex(&LockPool);
		K2H_Delete(pthpool);
		return false;
	}

	// create threads
	PTRTHPARAM	pparam;
	pthread_t	tid;
	for(int cnt = 0; cnt < threadcnt; cnt++){
		// make param
		pparam	= new TRTHPARAM(reinterpret_cast<k2h_h>(pk2hshm), &(pthpool->trmutex), &(pthpool->trcond), pprefix, prefixlen);
		tid		= 0;
		if(0 != (result = pthread_create(&tid, NULL, K2HTransManager::WorkerProc, pparam))){
			ERR_K2HPRN("Failed to create thread(return code = %d), but continue...", result);
			K2H_Delete(pparam);
			break;
		}
		pthpool->trparammap[tid] = pparam;
	}
	if(0 == pthpool->trparammap.size()){
		ERR_K2HPRN("Could not create any thread.");
		pthread_cond_destroy(&(pthpool->trcond));
		pthread_mutex_destroy(&(pthpool->trmutex));
		K2H_Delete(pthpool);
		return false;
	}

	// add map
	trpoolmap[pk2hshm] = pthpool;

	fullock::flck_unlock_noshared_mutex(&LockPool);

	return true;
}

bool K2HTransManager::ExitThreads(const K2HShm* pk2hshm)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	while(!fullock::flck_trylock_noshared_mutex(&LockPool));	// no call sched_yield()

	trpoolmap_t::iterator	iter;
	if(trpoolmap.end() == (iter = trpoolmap.find(pk2hshm))){
		MSG_K2HPRN("There is no thread running for target.");
		fullock::flck_unlock_noshared_mutex(&LockPool);
		return true;
	}

	// make thread list & set exit flag for all thread
	std::map<pthread_t, bool>		threadstatus;
	for(trparammap_t::const_iterator iter2 = iter->second->trparammap.begin(); iter2 != iter->second->trparammap.end(); ++iter2){
		threadstatus[iter2->first]	= true;
		iter2->second->is_exit		= true;
	}
	fullock::flck_unlock_noshared_mutex(&LockPool);

	// wakeup all thread
	int	result;
	pthread_mutex_lock(&(iter->second->trmutex));
	if(0 != (result = pthread_cond_broadcast(&(iter->second->trcond)))){
		ERR_K2HPRN("Could not broadcast cond(return code = %d), but continue...", result);
	}
	pthread_mutex_unlock(&(iter->second->trmutex));

	// wait for all thread exit
	for(std::map<pthread_t, bool>::iterator iter2 = threadstatus.begin(); iter2 != threadstatus.end(); ++iter2){
		int	result;
		if(0 != (result = pthread_join(iter2->first, NULL))){
			ERR_K2HPRN("Failed to wait exiting thread(return code = %d), but continue...", result);
			iter2->second = false;
		}
	}

	// all data cleanup
	while(!fullock::flck_trylock_noshared_mutex(&LockPool));	// no call sched_yield()
	for(trparammap_t::iterator iter2 = iter->second->trparammap.begin(); iter2 != iter->second->trparammap.end(); ){
		if(false != threadstatus[iter2->first]){
			K2H_Delete(iter2->second);
			iter->second->trparammap.erase(iter2++);
		}else{
			// this thread does not exit, so do not clean param & not remove map.
			++iter2;
		}
	}

	// remove map for target
	bool	bResult = true;
	if(0 == iter->second->trparammap.size()){
		// destroy cond
		if(0 != (result = pthread_cond_destroy(&(iter->second->trcond)))){
			ERR_K2HPRN("Could not destroy cond(return code = %d)", result);
			bResult = false;
		}
		// destroy mutex
		pthread_mutex_destroy(&(iter->second->trmutex));

		// remove all for this target
		K2H_Delete(iter->second);
		trpoolmap.erase(iter);
	}else{
		ERR_K2HPRN("Could not exit all thread for target.");
		bResult = false;
	}
	fullock::flck_unlock_noshared_mutex(&LockPool);

	return bResult;
}

bool K2HTransManager::ExitAllThreads(void)
{
	while(!fullock::flck_trylock_noshared_mutex(&LockPool));	// no call sched_yield()

	// make target map
	std::map<const K2HShm*, bool>	k2htrmap;
	for(trpoolmap_t::const_iterator iter = trpoolmap.begin(); iter != trpoolmap.end(); ){
		k2htrmap[iter->first] = true;
	}
	fullock::flck_unlock_noshared_mutex(&LockPool);

	// exit threads in thread pool
	bool	bResult = true;
	for(std::map<const K2HShm*, bool>::const_iterator iter = k2htrmap.begin(); iter != k2htrmap.end(); iter++){
		if(!ExitThreads(iter->first)){
			ERR_K2HPRN("Could not exit thread for target, but continue...");
			bResult = false;
		}
	}

	return bResult;
}

bool K2HTransManager::SetThreadPool(int count)
{
	if(count < K2HTransManager::NO_THREAD_POOL || K2HTransManager::MAX_THREAD_POOL < count){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	while(!fullock::flck_trylock_noshared_mutex(&LockPool));	// no call sched_yield()

	if(0 < trpoolmap.size()){
		// Now thread running
		ERR_K2HPRN("Could not set thread pool count, so MUST do after STOP all thread.");
		fullock::flck_unlock_noshared_mutex(&LockPool);
		return false;
	}
	threadcnt = count;

	fullock::flck_unlock_noshared_mutex(&LockPool);

	return true;
}

bool K2HTransManager::WaitFinish(const K2HShm* pk2hshm, long ms)
{
	if(!pk2hshm){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!isEnable(pk2hshm)){
		return false;
	}

	// get prefix
	// [NOTICE]
	// without locking mutex...
	//
	const unsigned char*	pprefix		= NULL;
	size_t					prefixlen	= 0;
	trprefmap_t::const_iterator	iter;
	if(trprefmap.end() != (iter = trprefmap.find(pk2hshm))){
		pprefix		= iter->second->pprefix;
		prefixlen	= iter->second->prefixlen;
	}

	K2HQueue	k2hq(const_cast<K2HShm*>(pk2hshm), true, pprefix, prefixlen, K2hAttrOpsMan::OPSMAN_MASK_TRANSQUEUEKEY);
	long		waitms;
	bool		blocking = (K2HTransManager::FINISH_WAIT_BLOCK == ms);
	bool		result;
	while(false == (result = k2hq.IsEmpty()) && (blocking || 0 < ms)){
		if(blocking){
			ms = K2HTransManager::MINIMUM_WAIT_SEEP;
		}
		waitms	 = min(ms, K2HTransManager::MINIMUM_WAIT_SEEP);		// default 100ms
		ms		-= min(ms, waitms);									// remaining time

		struct timespec	waitts, remts;
		for(waitts.tv_sec = 0, waitts.tv_nsec = (waitms * 1000 * 1000), remts.tv_sec = 0, remts.tv_nsec = 0; -1 == nanosleep(&waitts, &remts) && (0 != remts.tv_sec || 0 != remts.tv_nsec); waitts.tv_sec = remts.tv_sec, waitts.tv_nsec = remts.tv_nsec);
	}
	return result;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
