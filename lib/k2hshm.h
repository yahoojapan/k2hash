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
 * CREATE:   Fri Dec 2 2013
 * REVISION:
 *
 */
#ifndef	K2HSHM_H
#define	K2HSHM_H

#include <string>

#include "k2hstructure.h"
#include "k2hmmapinfo.h"
#include "k2hpage.h"
#include "k2hutil.h"
#include "k2hlock.h"
#include "k2hfind.h"
#include "k2hdaccess.h"
#include "k2hfilemonitor.h"
#include "k2hqueue.h"
#include "k2hshmdirect.h"

//---------------------------------------------------------
// Templates
//---------------------------------------------------------
// initial key index structure count
template<typename T> inline long INIT_KINDEX_CNT(const T& bitcnt)
{
	return (1L << (bitcnt));
}

// collision key index structure count by each key index
template<typename T> inline long CKINDEX_BYKINDEX_CNT(const T& bitcnt)
{
	return (1L << (bitcnt));
}

// initial collision key index structure count
template<typename T> inline long INIT_CKINDEX_CNT(const T& maskbitcnt, const T& cmaskbitcnt)
{
	return (INIT_KINDEX_CNT(maskbitcnt) * CKINDEX_BYKINDEX_CNT(cmaskbitcnt));
}

//
// these function is implemented in fullock.h
//
#if	0
	// For alignment
	template<typename T1, typename T2> inline T1 ALIGNMENT(const T1& length, const T2& pagesize)
	{
		return ((((length) / (pagesize)) + (0 == (length) % (pagesize) ? 0 : 1)) * (pagesize));
	}

	// Addtional/Subtruction Pointer
	template <typename T> inline T* SUBPTR(T* pointer, off_t offset)
	{
		return reinterpret_cast<T*>(reinterpret_cast<off_t>(pointer) - offset);
	}

	template <typename T> inline T* ADDPTR(T* pointer, off_t offset)
	{
		return reinterpret_cast<T*>(reinterpret_cast<off_t>(pointer) + offset);
	}
#endif

//---------------------------------------------------------
// Extern
//---------------------------------------------------------
class K2HPage;
class K2HAttrs;
class K2HIterator;
class K2HDAccess;
class K2HDALock;

//---------------------------------------------------------
// Class K2HShm
//---------------------------------------------------------
class K2HShm
{
		friend class K2HIterator;
		friend class K2HDAccess;
		friend class K2HDALock;

	public:
		typedef K2HIterator	iterator;

		enum{														// Dump mode bits
			DUMP_HEAD				= 0,
			DUMP_RAW_KINDEX_ARRAY	= 1,
			DUMP_KINDEX_ARRAY		= 1,							// OR
			DUMP_RAW_CKINDEX_ARRAY	= 2,
			DUMP_CKINDEX_ARRAY		= 3,							// OR
			DUMP_RAW_ELEMENT_LIST	= 4,
			DUMP_ELEMENT_LIST		= 7,							// OR
			DUMP_RAW_PAGE_LIST		= 8,
			DUMP_PAGE_LIST			= 15							// OR
		};

		enum{
			PAGEOBJ_KEY,
			PAGEOBJ_VALUE,
			PAGEOBJ_SUBKEYS,
			PAGEOBJ_ATTRS
		};

		static const int	MIN_SYSPAGE_SIZE				= 4096;	// default system page size
		static const int	MIN_PAGE_SIZE					= 128;	// minimum page size
		static const int	MAX_MASK_BITCOUNT				= 28;	// maximum mask(this value + collision mask = 32bit)
		static const int	MIN_MASK_BITCOUNT				= 2;	// minimum key mask bit count(0x03)
		static const int	DEFAULT_MASK_BITCOUNT			= 8;	// default key mask bit count(0xFF)
		static const int	DEFAULT_COLLISION_MASK_BITCOUNT	= 4;	// default Collision mask
		static const int	DEFAULT_MAX_ELEMENT_CNT			= 32;	// default maximum element count by each collision key
		static const int	ELEMENT_CNT_RATIO				= 4;	// initial elemnt coefficient by each collision key index
		static const int	PAGE_CNT_RATIO					= 2;	// initial page coefficient by each element
		static const int	MAX_EXPAND_ELEMENT_CNT			= (1024 * 1024);	// maximum element count for expanding
		static const int	MAX_EXPAND_PAGE_CNT				= (1024 * 1024);	// maximum page count for expanding
		static const long	DETACH_NO_WAIT					= 0;	// no wait finishing transaction at detaching
		static const long	DETACH_BLOCK_WAIT				= -1;	// wait blocking by finishing transaction at detaching

	private:
		static size_t	SystemPageSize;			// System page size, used this for initializing, extending area

		int				ShmFd;
		bool			isAnonMem;				// Anonymous memory mapping(for thread only)
		bool			isFullMapping;			// Mapping is full area
		bool			isTemporary;			// Removing file after closing it
		bool			isReadMode;
		bool			isSync;					// whichever doing msync
		std::string		ShmPath;
		PK2H			pHead;
		K2HMmapInfo		MmapInfos;
		K2HFileMonitor	FileMon;

	public:
		static size_t GetSystemPageSize(void);
		static int GetMaskBitCount(k2h_hash_t mask);
		static int GetTransThreadPool(void);
		static bool SetTransThreadPool(int count);
		static bool UnsetTransThreadPool(void);

		K2HShm();
		virtual ~K2HShm();

		// Attach
		bool Create(const char* file, bool isfullmapping = true, int mask_bitcnt = DEFAULT_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE);
		bool Attach(const char* file, bool isReadOnly, bool isCreate = true, bool isTempFile = false, bool isfullmapping = true, int mask_bitcnt = DEFAULT_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE);
		bool AttachMem(int mask_bitcnt = DEFAULT_MASK_BITCOUNT, int cmask_bitcnt = DEFAULT_COLLISION_MASK_BITCOUNT, int max_element_cnt = DEFAULT_MAX_ELEMENT_CNT, size_t pagesize = MIN_PAGE_SIZE) { return Attach(NULL, false, true, false, true, mask_bitcnt, cmask_bitcnt, max_element_cnt, pagesize); }
		bool IsAttached(void) const { return (NULL != pHead); }
		bool Detach(long waitms = DETACH_NO_WAIT);

		// Convert
		off_t Rel(void* pAddress) const { return MmapInfos.CvtRel(pAddress); }
		void* Abs(void* pAddress) const { return MmapInfos.CvtAbs(reinterpret_cast<off_t>(pAddress)); }

		// Accessing data
		bool PutBackPages(PPAGEHEAD pRelTopPage, PPAGEHEAD pRelLastPage, unsigned long pagecount) const;
		bool AddPages(K2HPage* pLastPage, size_t length) const { return (const_cast<K2HShm*>(this))->ExpandPages(pLastPage, length); }

		// Header information
		size_t GetPageSize(void) const { return (IsAttached() ? pHead->page_size : 0UL); }
		k2h_hash_t GetCurrentMask(void) const { return (IsAttached() ? pHead->cur_mask : 0UL); }
		k2h_hash_t GetCollisionMask(void) const { return (IsAttached() ? pHead->collision_mask : 0UL); }
		unsigned long GetMaxElementCount(void) const { return (IsAttached() ? pHead->max_element_count : 0UL); }
		const char* GetK2hashFilePath(void) const { return (IsAttached() ? ShmPath.c_str() : NULL); }
		const char* GetRawK2hashFilePath(void) const { return ShmPath.c_str(); }
		int GetRawK2hashFd(void) const { return ShmFd; }
		bool GetRawK2hashReadMode(void) const { return isReadMode; }

		// Get(value/subkeys)
		char* Get(PELEMENT pElement, int type) const;												// Any          by Element
		ssize_t Get(PELEMENT pElement, unsigned char** byData, int type) const;						// Value        by Element
		char* Get(const char* pKey, bool checkattr = true, const char* encpass = NULL) const;		// Value        by Key
																									// Value        by Key
		ssize_t Get(const unsigned char* byKey, size_t length, unsigned char** byValue, bool checkattr = true, const char* encpass = NULL) const;
																									// Value        by Key with callback function
		ssize_t Get(const unsigned char* byKey, size_t keylen, unsigned char** byValue, k2h_get_trial_callback fp, void* pExtData, bool checkattr, const char* encpass);
																									// Value(array) by Key
		strarr_t::size_type Get(const char* pKey, strarr_t& strarr, bool checkattr = true, const char* encpass = NULL) const;
																									// Value(array) by Key
		strarr_t::size_type Get(const unsigned char* byKey, size_t length, strarr_t& strarr, bool checkattr = true, const char* encpass = NULL) const;
																									// Value(array) by Element
		strarr_t::size_type Get(PELEMENT pElement, strarr_t& strarr, bool checkattr = true, const char* encpass = NULL) const;
		K2HSubKeys* GetSubKeys(const char* pKey, bool checkattr = true) const;						// Subkeys      by Key
																									// Subkeys      by Key
		K2HSubKeys* GetSubKeys(const unsigned char* byKey, size_t length, bool checkattr = true) const;
		K2HSubKeys* GetSubKeys(PELEMENT pElement, bool checkattr = true) const;						// Subkeys      by Element
		K2HAttrs* GetAttrs(const char* pKey) const;													// Attributes	by Key
		K2HAttrs* GetAttrs(const unsigned char* byKey, size_t length) const;						// Attributes	by Key
		K2HAttrs* GetAttrs(PELEMENT pElement) const;												// Attributes	by Element

		// Set/Add
		bool Set(const char* pKey, const char* pValue, const char* encpass = NULL, const time_t* expire = NULL);															// Keep subkey
		bool Set(const char* pKey, const char* pValue, K2HSubKeys* pSubKeys, bool isRemoveSubKeys = true, const char* encpass = NULL, const time_t* expire = NULL);			// Overwrite subkey
		bool Set(const char* pKey, const char* pValue, K2HAttrs* pAttrs, const char* encpass = NULL, const time_t* expire = NULL);											// Keep subkey
		bool Set(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const char* encpass = NULL, const time_t* expire = NULL);	// Keep subkey
																																											// Overwrite subkey
		bool Set(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2HSubKeys* pSubKeys, bool isRemoveSubKeys = true, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL, K2hAttrOpsMan::ATTRINITTYPE attrtype = K2hAttrOpsMan::OPSMAN_MASK_NORMAL);
		bool AddSubkey(const char* pKey, const char* pSubkey, const char* pValue, const char* encpass = NULL, const time_t* expire = NULL);
		bool AddSubkey(const char* pKey, const char* pSubkey, const unsigned char* byValue, size_t vallength, const char* encpass = NULL, const time_t* expire = NULL);
		bool AddSubkey(const unsigned char* byKey, size_t keylength, const unsigned char* bySubkey, size_t skeylength, const unsigned char* byValue, size_t vallength, const char* encpass = NULL, const time_t* expire = NULL);
		bool AddAttr(const char* pKey, const char* pattrkey, const char* pattrval);
		bool AddAttr(const char* pKey, const unsigned char* pattrkey, size_t attrkeylen, const unsigned char* pattrval, size_t attrvallen);
		bool AddAttr(const unsigned char* byKey, size_t keylength, const unsigned char* pattrkey, size_t attrkeylen, const unsigned char* pattrval, size_t attrvallen);

		// Remove/Replace
		bool Remove(const char* pKey, bool isSubKeys);
		bool Remove(const unsigned char* byKey, size_t keylength, bool isSubKeys = true) { return Remove(byKey, keylength, isSubKeys, NULL, false); }
		bool Remove(const char* pKey, const char* pSubKey);
		bool Remove(const unsigned char* byKey, size_t keylength, const unsigned char* bySubKey, size_t sklength);
		bool ReplaceAll(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const unsigned char* bySubkeys, size_t sklength, const unsigned char* byAttrs, size_t attrlength);
		bool ReplaceValue(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength);
		bool ReplaceSubkeys(const unsigned char* byKey, size_t keylength, const unsigned char* bySubkeys, size_t sklength);
		bool ReplaceAttrs(const unsigned char* byKey, size_t keylength, const unsigned char* byAttrs, size_t attrlength);

		// Rename
		bool Rename(const char* pOldKey, const char* pNewKey);
		bool Rename(const unsigned char* byOldKey, size_t oldkeylen, const unsigned char* byNewKey, size_t newkeylen);
		// Rename(for transaction)
		bool Rename(const unsigned char* byOldKey, size_t oldkeylen, const unsigned char* byNewKey, size_t newkeylen, const unsigned char* byAttrs, size_t attrlen);

		// Direct access
		K2HDAccess* GetDAccessObj(const char* pKey, K2HDAccess::ACSMODE acsmode = K2HDAccess::READ_ACCESS, off_t offset = 0L);
		K2HDAccess* GetDAccessObj(const unsigned char* byKey, size_t keylength, K2HDAccess::ACSMODE acsmode = K2HDAccess::READ_ACCESS, off_t offset = 0L);

		// Direct get/set
		bool GetElementsByHash(const k2h_hash_t starthash, const struct timespec startts, const struct timespec endts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check, k2h_hash_t* pnexthash, PK2HBIN* ppbindatas, size_t* pdatacnt) const;
		bool SetElementByBinArray(const PRALLEDATA prawdata, const struct timespec* pts);

		// search
		K2HShm::iterator begin(const char* pKey);
		K2HShm::iterator begin(const unsigned char* byKey, size_t length);
		K2HShm::iterator begin(void);
		K2HShm::iterator end(bool isSubKey = false);

		// Queue
		K2HQueue* GetQueueObj(bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L);
		K2HKeyQueue* GetKeyQueueObj(bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L);
		bool IsEmptyQueue(const unsigned char* byMark, size_t marklength) const;
		int GetCountQueue(const unsigned char* byMark, size_t marklength) const;
		bool ReadQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, int pos = 0, const char* encpass = NULL) const;
		bool PushFifoQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL) { return AddQueue(byMark, marklength, byKey, keylength, byValue, vallength, true, attrtype, pAttrs, encpass, expire); }
		bool PushLifoQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs = NULL, const char* encpass = NULL, const time_t* expire = NULL) { return AddQueue(byMark, marklength, byKey, keylength, byValue, vallength, false, attrtype, pAttrs, encpass, expire); }
		bool PopQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, unsigned char** ppValue, size_t& vallength, K2HAttrs** ppAttrs = NULL, const char* encpass = NULL);
		int RemoveQueue(const unsigned char* byMark, size_t marklength, unsigned int count, bool rmkeyval, k2h_q_remove_trial_callback fp = NULL, void* pExtData = NULL, const char* encpass = NULL);

		// Queue( used by only k2hash library family )
		K2HLowOpsQueue* GetLowOpsQueueObj(bool is_fifo = true, const unsigned char* pref = NULL, size_t preflen = 0L);
		bool UpdateStartK2HMarker(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength);
		bool ReadQueue(const unsigned char* byMark, size_t marklength, unsigned char** ppKey, size_t& keylength, int pos = 0) const;
		bool AddQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, bool is_fifo, bool update_chain = false, bool check_update_file = true);

		// Dump
		bool Dump(FILE* stream = stdout, int dumpmask = DUMP_KINDEX_ARRAY) const;
		bool PrintState(FILE* stream = stdout) const;
		bool PrintAreaInfo(void) const;
		bool DumpQueue(FILE* stream, const unsigned char* byMark, size_t marklength);
		PK2HSTATE GetState(void) const;

		// Transaction
		bool EnableTransaction(const char* filepath, const unsigned char* pprefix = NULL, size_t prefixlen = 0, const unsigned char* pparam = NULL, size_t paramlen = 0, const time_t* expire = NULL) const { return Transaction(true, filepath, pprefix, prefixlen, pparam, paramlen, expire); }
		bool DisableTransaction(void) const { return Transaction(false, NULL, NULL, 0, NULL, 0, NULL); }

		// Attributes
		bool SetCommonAttribute(const bool* is_mtime = NULL, const bool* is_defenc = NULL, const char* passfile = NULL, const bool* is_history = NULL, const time_t* expire = NULL, const strarr_t* pluginlibs = NULL);
		bool AddAttrCryptPass(const char* pass, bool is_default_encrypt = false);
		bool AddAttrPluginLib(const char* path);
		bool CleanCommonAttribute(void);
		bool GetAttrVersionInfos(strarr_t& verinfos) const;
		void GetAttrInfos(std::stringstream& ss) const;

		// Area Compress
		bool AreaCompress(bool& isCompressed);

		// Other
		bool GetUpdateTimeval(struct timeval& tv) const;
		bool SetMsyncMode(bool enable);			// default ON
		bool CheckFileUpdate(void);
		bool CheckAreaUpdate(bool& is_change);
		bool DoAreaUpdate(void);

	private:
		// Utilities
		static k2h_hash_t MakeMask(int bitcnt);
		static bool SetAreasArray(PK2H pHead, long type, off_t file_offset, size_t length);
		static void GetRealTimeval(struct timeval& tv);

		// Initializing(static)
		static bool InitializeFileZero(int fd, off_t start, size_t length);
		static bool InitializeKeyIndexArray(void* pCKIndexShmBase, PKINDEX pKeyIndex, k2h_hash_t& start_hash, k2h_hash_t cur_mask, int cmask_bitcnt, PCKINDEX pCKIndex, int count, long default_assign);
		static bool InitializeCollisionKeyIndexArray(PCKINDEX pCKIndex, int count);
		static bool InitializeElementArray(void* pShmBase, PELEMENT pElement, int count, PELEMENT pLastRelElement = NULL);
		static bool InitializePageArray(int fd, off_t start, ssize_t pagesize, int count, PPAGEHEAD pLastRelPage = NULL);
		static bool InitializePageArray(void* pShmBase, PPAGEHEAD pPage, ssize_t pagesize, int count, PPAGEHEAD pLastRelPage = NULL);	// For Anonymous(only on memory)
		static ssize_t InitialSystemPageSize(void);
		static bool CheckSystemLimit(void);

		// Queue
		static PBK2HMARKER InitK2HMarker(size_t& marklen, const unsigned char* bystart = NULL, size_t startlen = 0, const unsigned char* byend = NULL, size_t endlen = 0);
		static PBK2HMARKER UpdateK2HMarker(PBK2HMARKER pmarker, size_t& marklen, const unsigned char* byKey, size_t keylength, bool is_end);
		static bool IsEmptyK2HMarker(PBK2HMARKER pmarker);

		// For history
		static unsigned char* MakeHistoryKey(const unsigned char* byBaseKey, size_t basekeylen, const char* pUniqid, size_t& hiskeylen);
		static bool ParseHistoryKey(const unsigned char* byHisKey, size_t hiskeylen, unsigned char** ppBaseKey, size_t& basekeylen, const char** ppUniqid);

		// Cleaning
		bool Clean(bool isRemoveFile = false);

		// Accessing data
		K2HPage* GetPageObject(PPAGEHEAD pRelPageHead, bool need_load = true) const;

		bool GetKIndexPos(k2h_hash_t hash, k2h_arrpos_t& KIPtrArrayPos, k2h_arrpos_t& KIArrayPos, k2h_hash_t* pCurMask) const;
		PKINDEX GetReservedKIndex(k2h_hash_t hash, bool isAbsolute, k2h_hash_t* pCurMask) const;
		PKINDEX GetKIndex(k2h_hash_t hash, bool isMergeCurmask, bool is_need_lock = true) const;
		bool ArrangeToUpperKIndex(k2h_hash_t hash, k2h_hash_t mask) const;
		bool MoveElementToUpperMask(PELEMENT pElement, PCKINDEX pSrcCKIndex, PCKINDEX pDstCKIndex, k2h_hash_t target_mask, k2h_hash_t target_masked_val) const;

		PCKINDEX GetCKIndex(PKINDEX pKindex, k2h_hash_t hash, K2HLock& ALObjCKI) const;
		PCKINDEX GetCKIndex(k2h_hash_t hash, K2HLock& ALObjCKI, bool isMergeCurmask = true) const;

		PELEMENT GetElementList(PELEMENT pRelElement, k2h_hash_t hash, k2h_hash_t subhash) const;
		PELEMENT GetElementList(PCKINDEX pCKindex, k2h_hash_t hash, k2h_hash_t subhash) const;
		PELEMENT GetElementList(PKINDEX pKindex, k2h_hash_t hash, k2h_hash_t subhash, K2HLock& ALObjCKI) const;
		PELEMENT GetElementList(k2h_hash_t hash, k2h_hash_t subhash, K2HLock& ALObjCKI) const;
		unsigned long GetElementListUpCount(PELEMENT pElementList) const;

		PELEMENT GetElement(PELEMENT pElementList, const unsigned char* byKey, size_t length) const;
		PELEMENT GetElement(const unsigned char* byKey, size_t length, k2h_hash_t hash, k2h_hash_t subhash, K2HLock& ALObjCKI) const;
		PELEMENT GetElement(const unsigned char* byKey, size_t length, K2HLock& ALObjCKI) const;
		PELEMENT GetElement(const char* pKey, K2HLock& ALObjCKI) const;
		PELEMENT ReserveElement(void);
		bool InsertElement(PELEMENT pTopElement, PELEMENT pElement) const;
		bool TakeOffElement(PCKINDEX pCKIndex, PELEMENT pElement) const;
		bool FreeElement(PELEMENT pElement);
		bool PutBackElement(PELEMENT pElement);
		bool PutBackElementPages(PELEMENT pElement);

		K2HPage* GetPage(PELEMENT pElement, int type, bool need_load = true) const;
		K2HPage* ReservePages(size_t length);
		bool ReservePages(const unsigned char* byData, size_t length, K2HPage** ppPage);

		// Initializing
		bool InitializeFile(const char* file, bool isfullmapping, int mask_bitcnt, int cmask_bitcnt, int max_element_cnt, size_t pagesize);
		bool AttachFile(const char* file, bool isReadOnly, bool isfullmapping);
		bool BuildMmapInfo(void);
		bool ExpandMmapInfo(void);
		bool ContractMmapInfo(void);

		// Expanding
		bool CheckExpandingKeyArea(PCKINDEX pCKIndex);
		void* ExpandArea(long type, size_t area_length, off_t& new_area_start);
		bool ExpandKIndexArea(K2HLock& ALObjCMask);
		bool ExpandElementArea(void);
		bool ExpandPageArea(void);
		bool ReplacePageHead(PPAGEHEAD pLastRelPage, PPAGEHEAD pRelPtr, bool isSetPrevPtr) const;
		bool ExpandPages(K2HPage* pLastPage, size_t length);

		// Search
		PELEMENT FindNextElement(PELEMENT pLastElement, K2HLock& ALObjCKI) const;

		// Set
		PELEMENT AllocateElement(k2h_hash_t hash, k2h_hash_t subhash, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const unsigned char* bySubKeys, size_t sublength, const unsigned char* byAttrs, size_t attrlength);

		// Remove/Replace
		bool Remove(const unsigned char* byKey, size_t keylength, bool isSubKeys, char** ppUniqid, bool hismask = false);
		bool Remove(PELEMENT pElement, const char* pSubKey, K2HLock& ALObjCKI);
		bool Remove(PELEMENT pElement, const unsigned char* bySubKey, size_t length, K2HLock& ALObjCKI);
		bool Remove(PELEMENT pElement, bool isSubKeys = true);
		bool RemoveAllSubkeys(PELEMENT pElement, bool isRemoveSubKeyLists = true);
		bool Replace(const unsigned char* byKey, size_t keylength, const unsigned char* byData, size_t dlength, int type);
		bool ReplacePage(PELEMENT pElement, const char* pData, int type);
		bool ReplacePage(PELEMENT pElement, const unsigned char* byData, size_t length, int type);
		bool ReplacePage(PELEMENT pElement, K2HPage* pPage, size_t totallength, int type);
		bool ReplacePage(PELEMENT pElement, PPAGEHEAD pRelPageHead, size_t totallength, int type);

		// Rename(for history)
		bool RenameForHistory(const char* pKey, std::string* puniqid);
		bool RenameForHistory(const unsigned char* byKey, size_t keylen, std::string* puniqid);

		// Direct get/set
		PBALLEDATA GetElementToBinary(PELEMENT pAbsElement) const;
		PK2HBIN GetElementListToBinary(PELEMENT pRelElement, size_t* pdatacnt, const struct timespec* pstartts, const struct timespec* pendts, const k2h_hash_t target_hash, const k2h_hash_t target_max_hash, const k2h_hash_t old_hash, const k2h_hash_t old_max_hash, const long target_hash_range, bool is_expire_check) const;

		// Queue
		PBK2HMARKER PopK2HMarker(PBK2HMARKER pmarker, size_t& marklen, unsigned char** ppKey, size_t& keylength, K2HAttrs** ppAttrs);
		bool AddQueue(const unsigned char* byMark, size_t marklength, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, bool is_fifo, K2hAttrOpsMan::ATTRINITTYPE attrtype, K2HAttrs* pAttrs, const char* encpass, const time_t* expire);

		// Dumping
		bool DumpFreeElements(FILE* stream, int nest, PELEMENT pRelElements, long count) const;
		bool DumpFreePages(FILE* stream, int nest, PPAGEHEAD pRelPages, long count) const;
		bool DumpKeyIndex(FILE* stream, int nest, PKINDEX* ppKeyIndex, int cmask_bitcnt, int dumpmask) const;
		bool DumpCKeyIndex(FILE* stream, int nest, PCKINDEX pRelCKeyIndex, int cmask_bitcnt, int dumpmask) const;
		bool DumpElement(FILE* stream, int nest, PELEMENT pRelElement, int dumpmask, int& element_count, std::string* pstring) const;
		bool DumpPageData(FILE* stream, int nest, PPAGEHEAD pRelPageHead, const char* pName, int dumpmask) const;

		// Transaction
		bool Transaction(bool isEnable, const char* filepath, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire) const;

		// Area Compress( especial methods )
		PK2HAREA GetLastestArea(void) const;
		PELEMENT ReserveElement(void* pRelExpArea, size_t ExpLength);
		bool ReplaceElement(PELEMENT pElement, void* pRelExpArea, size_t ExpLength);
		K2HPage* CutOffPage(PPAGEHEAD pCurPageHead, PPAGEHEAD* ppTopPageHead, PPAGEHEAD pSetPrevPageHead);
		K2HPage* ReservePages(size_t length, void* pRelExpArea, size_t ExpLength);
		bool CopyPageData(K2HPage* pSrcTop, K2HPage* pDestTop);
		bool ReplacePageInElement(PELEMENT pElement, int type, void* pRelExpArea, size_t ExpLength);
		bool ReplacePagesInElement(PELEMENT pElement, void* pRelExpArea, size_t ExpLength);

		// Others
		bool UpdateTimeval(bool isAreaUpdate = false) const;
		bool Msync(void* address = NULL, bool is_update_meta = false) const;
};

#endif	// K2HSHM_H

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
