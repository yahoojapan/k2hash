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
 * CREATE:   Mon Feb 10 2014
 * REVISION:
 *
 */
#ifndef	K2HTRANS_H
#define	K2HTRANS_H

#include <pthread.h>
#include <map>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hcommand.h"

//---------------------------------------------------------
// K2HTransaction Class
//---------------------------------------------------------
class K2HTransaction : public K2HCommandArchive
{
	protected:
		const K2HShm*	pShm;
		bool			IsStackMode;

	public:
		K2HTransaction(const K2HShm* pk2hshm = NULL, bool isstack = false);
		virtual ~K2HTransaction();

		bool Put(void);
		bool IsEnable(void) const;

	protected:
		virtual bool Put(long type, const unsigned char* byKey, size_t keylength, const unsigned char* byVal, size_t vallength, const unsigned char* bySKey, size_t skeylength, const unsigned char* byAttrs, size_t attrlength, const unsigned char* byExdata, size_t exdatalength);
};

//---------------------------------------------------------
// Structure
//---------------------------------------------------------
// File information for each K2HShm object
//
typedef struct transaction_file_info{
	std::string		filepath;
	int				arfd;
	struct stat		statbuf;
	time_t			last_update;

	transaction_file_info() : arfd(-1), last_update(0L) {}

}TRFILEINFO, *PTRFILEINFO;

typedef std::map<const K2HShm*, PTRFILEINFO>	trfilemap_t;

//
// The structure is for each k2handle.
//
typedef struct transaction_k2h_prefix{
	unsigned char*	pprefix;
	size_t			prefixlen;
	bool			is_set_expire;
	time_t			expire;

	transaction_k2h_prefix() : pprefix(NULL), prefixlen(0L), is_set_expire(false), expire(0) {}
	transaction_k2h_prefix(const unsigned char* ppref, size_t preflen, const time_t* pexpire) : pprefix(NULL), prefixlen(0L), is_set_expire(false), expire(0)
	{
		if(ppref && 0 < preflen){
			pprefix		= k2hbindup(ppref, preflen);
			prefixlen	= preflen;
		}
		if(pexpire && 0 < *pexpire){
			is_set_expire	= true;
			expire			= *pexpire;
		}
	}
	~transaction_k2h_prefix()
	{
		K2H_Free(pprefix);
	}

}TRK2HPREF, *PTRK2HPREF;

typedef std::map<const K2HShm*, PTRK2HPREF>		trprefmap_t;

//
// The structure is for each thread because we do not use locking.
//
typedef struct transaction_thread_param{
	k2h_h				handle;
	pthread_mutex_t*	ptrmutex;
	pthread_cond_t*		ptrcond;
	unsigned char*		pprefix;
	size_t				prefixlen;
	volatile bool		is_exit;

	transaction_thread_param() : handle(K2H_INVALID_HANDLE), ptrmutex(NULL), ptrcond(NULL), pprefix(NULL), prefixlen(0L), is_exit(false) {}
	transaction_thread_param(k2h_h hk2h, pthread_mutex_t* pmutex, pthread_cond_t* pcond, const unsigned char* ppref, size_t preflen) : handle(hk2h), ptrmutex(pmutex), ptrcond(pcond), pprefix(NULL), prefixlen(0L), is_exit(false)
	{
		if(ppref && 0 < preflen){
			pprefix		= k2hbindup(ppref, preflen);
			prefixlen	= preflen;
		}
	}

	~transaction_thread_param()
	{
		K2H_Free(pprefix);
	}
}TRTHPARAM, *PTRTHPARAM;

typedef std::map<pthread_t, PTRTHPARAM>		trparammap_t;

//
// Thread pool for each k2handle
//
typedef struct transaction_thread_pool{
	trparammap_t	trparammap;
	pthread_mutex_t	trmutex;
	pthread_cond_t	trcond;
}TRTHPOOL, *PTRTHPOOL;

typedef std::map<const K2HShm*, PTRTHPOOL>	trpoolmap_t;

//---------------------------------------------------------
// K2HTransManager Class
//---------------------------------------------------------
class K2HTransManager
{
	public:
		static const int			NO_THREAD_POOL			= 0;
		static const int			DEFAULT_THREAD_POOL		= 1;
		static const int			MAX_THREAD_POOL			= 64;
		static const time_t			DEFAULT_TREAD_COND_WAIT	= 5L;	// second(default 5s)
		static const long			MINIMUM_WAIT_SEEP		= 100L;	// default minimum sleep for waiting(100ms)
		static const long			FINISH_WAIT_BLOCK		= -1;	// for blocking

	protected:
		static const unsigned char	default_prefix[];				// default transaction key prefix
		static const time_t			DEFAULT_INTERVAL	= 10;		// 10s

		volatile int				LockVal;						// like mutex for variables(transaction file etc)
		volatile int				LockPool;						// like mutex for thread and thread pool
		trfilemap_t					trfilemap;						// for output file :			K2HShm object <-> PTRFILEINFO
		trprefmap_t					trprefmap;						// for transaction key prefix :	K2HShm object <-> PTRK2HPREF
		trpoolmap_t					trpoolmap;						// for thread pool :			K2HShm object <-> PTRTHPOOL( thread id <-> PTRTHPARAM )
		int							threadcnt;						// count of thread pool for each K2HShm
		time_t						interval;						// interval for checking transaction file

	public:
		static K2HTransManager* Get(void);

		bool CompareTransactionKeyPrefix(const K2HShm* pk2hshm, const unsigned char* pkey, size_t keylen);

		bool Stop(const K2HShm* pk2hshm = NULL);
		bool Stop(k2h_h handle = K2H_INVALID_HANDLE) { return Stop(reinterpret_cast<const K2HShm*>(handle)); }
		bool Start(const K2HShm* pk2hshm, const char* pFile, const unsigned char* pprefix = NULL, size_t prefixlen = 0, const time_t* expire = NULL);
		bool Start(k2h_h handle, const char* pFile, const unsigned char* pprefix = NULL, size_t prefixlen = 0, const time_t* expire = NULL) { return Start(reinterpret_cast<const K2HShm*>(handle), pFile, pprefix, prefixlen, expire); }

		bool isEnableWithoutLock(const K2HShm* pk2hshm) { return (NULL != GetFileInfo(pk2hshm)); }	// without locking
		bool isEnable(const K2HShm* pk2hshm) { return isEnable(pk2hshm, true); }
		bool isEnable(k2h_h handle) { return isEnable(reinterpret_cast<const K2HShm*>(handle), true); }

		int GetArchiveFd(const K2HShm* pk2hshm);
		int GetArchiveFd(k2h_h handle) { return GetArchiveFd(reinterpret_cast<const K2HShm*>(handle)); }
		bool Put(const K2HShm* pk2hshm, PBCOM pBinCom);
		bool Put(k2h_h handle, PBCOM pBinCom) { return Put(reinterpret_cast<const K2HShm*>(handle), pBinCom); }

		int GetThreadPool(void) const { return threadcnt; }
		bool SetThreadPool(int count = DEFAULT_THREAD_POOL);
		bool UnsetThreadPool(void) { return SetThreadPool(NO_THREAD_POOL); }

		bool IsRemainingQueue(const K2HShm* pk2hshm) { return WaitFinish(pk2hshm, 0L); }
		bool WaitFinish(const K2HShm* pk2hshm, long ms);

	protected:
		static void* WorkerProc(void* param);
		static bool Do(k2h_h handle, PBCOM pBinCom);

		K2HTransManager();
		virtual ~K2HTransManager();

		bool SetTransactionKeyPrefix(const K2HShm* pk2hshm, const unsigned char* pprefix, size_t prefixlen, const time_t* expire = NULL);
		bool ResetTransactionKeyPrefix(const K2HShm* pk2hshm) { return SetTransactionKeyPrefix(pk2hshm, NULL, 0, NULL); }
		bool RemoveTransactionKeyPrefix(const K2HShm* pk2hshm);
		bool RemoveAllTransactionKeyPrefix(void);
		const unsigned char* GetTransactionKeyPrefix(const K2HShm* pk2hshm, size_t& prefixlen);

		bool isEnable(const K2HShm* pk2hshm, bool with_lock);
		bool Stop(const K2HShm* pk2hshm, bool is_remove_prefix);
		PTRFILEINFO GetFileInfo(const K2HShm* pk2hshm);
		bool CheckFile(const K2HShm* pk2hshm);

		bool CreateThreads(const K2HShm* pk2hshm);
		bool ExitThreads(const K2HShm* pk2hshm);
		bool ExitAllThreads(void);
};

#endif	// K2HTRANS_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
