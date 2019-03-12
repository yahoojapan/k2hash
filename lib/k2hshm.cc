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

#include <string.h>
#include <sys/time.h>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hpagefile.h"
#include "k2hpagemem.h"
#include "k2hashfunc.h"
#include "k2htrans.h"
#include "k2htransfunc.h"
#include "k2hshmupdater.h"
#include "k2hattrs.h"
#include "k2hattropsman.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Symbols
//---------------------------------------------------------
#ifndef CLOCK_REALTIME_COARSE
#define	CLOCK_REALTIME_COARSE		CLOCK_REALTIME
#endif

//---------------------------------------------------------
// Utility macros
//---------------------------------------------------------
#define	CVT_ABS_PKINDEX(kindex_array, kiptrarraypos, kiarraypos)	(&(static_cast<PKINDEX>(Abs(kindex_array[kiptrarraypos])))[kiarraypos])
#define	CVT_REL_PKINDEX(kindex_array, kiptrarraypos, kiarraypos)	(reinterpret_cast<PKINDEX>(Rel(&(static_cast<PKINDEX>(Abs(kindex_array[kiptrarraypos])))[kiarraypos])))

//---------------------------------------------------------
// Const Class Member
//---------------------------------------------------------
const int	K2HShm::MIN_SYSPAGE_SIZE;
const int	K2HShm::MIN_PAGE_SIZE;
const int	K2HShm::MAX_MASK_BITCOUNT;
const int	K2HShm::MIN_MASK_BITCOUNT;
const int	K2HShm::DEFAULT_MASK_BITCOUNT;
const int	K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT;
const int	K2HShm::DEFAULT_MAX_ELEMENT_CNT;
const int	K2HShm::ELEMENT_CNT_RATIO;
const int	K2HShm::PAGE_CNT_RATIO;
const int	K2HShm::MAX_EXPAND_ELEMENT_CNT;
const int	K2HShm::MAX_EXPAND_PAGE_CNT;
const long	K2HShm::DETACH_NO_WAIT;
const long	K2HShm::DETACH_BLOCK_WAIT;

//---------------------------------------------------------
// Class Member
//---------------------------------------------------------
size_t	K2HShm::SystemPageSize = K2HShm::MIN_SYSPAGE_SIZE;

//---------------------------------------------------------
// Class Methods
//---------------------------------------------------------
k2h_hash_t K2HShm::MakeMask(int bitcnt)
{
	k2h_hash_t	mask;
	for(mask = 0UL; 0 < bitcnt; mask = ((mask << 1) | 1UL), bitcnt--);
	return mask;
}

int K2HShm::GetMaskBitCount(k2h_hash_t mask)
{
	int	bitcnt;
	for(bitcnt = 0; 0 != mask; bitcnt++, mask = (mask >> 1));
	return bitcnt;
}

bool K2HShm::SetAreasArray(PK2H pHead, long type, off_t file_offset, size_t length)
{
	if(!pHead){
		ERR_K2HPRN("PK2H is null");
		return false;
	}

	int			nCnt;
	PK2HAREA	pK2hArea;
	for(nCnt = 0, pK2hArea = &(pHead->areas[0]); nCnt < MAX_K2HAREA_COUNT && K2H_AREA_UNKNOWN != pK2hArea->type; pK2hArea++, nCnt++);
	if(nCnt >= MAX_K2HAREA_COUNT){
		ERR_K2HPRN("Could not set Area information by over array size,");
		return false;
	}
	pK2hArea->type			= type;
	pK2hArea->file_offset	= file_offset;
	pK2hArea->length		= length;

	return true;
}

size_t K2HShm::GetSystemPageSize(void)
{
	return K2HShm::SystemPageSize;
}

// History key is
//	<original key name> + 0x00 + <uniqid string>
//
unsigned char* K2HShm::MakeHistoryKey(const unsigned char* byBaseKey, size_t basekeylen, const char* pUniqid, size_t& hiskeylen)
{
	if(!byBaseKey || 0 == basekeylen || ISEMPTYSTR(pUniqid)){
		ERR_K2HPRN("Some parameters are wrong.");
		return NULL;
	}
	hiskeylen = basekeylen + strlen(pUniqid) + 2;	// 2 = two 0x00('\0')

	unsigned char*	presult;
	if(NULL == (presult = reinterpret_cast<unsigned char*>(malloc(hiskeylen)))){
		ERR_K2HPRN("Could not allocate memory.");
		return NULL;
	}
	memcpy(presult, byBaseKey, basekeylen);
	presult[basekeylen] = 0x00;
	strcpy(reinterpret_cast<char*>(&presult[basekeylen + 1]), pUniqid);

	return presult;
}

bool K2HShm::ParseHistoryKey(const unsigned char* byHisKey, size_t hiskeylen, unsigned char** ppBaseKey, size_t& basekeylen, const char** ppUniqid)
{
	if(!byHisKey || 0 == hiskeylen || !ppBaseKey || !ppUniqid){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	// check terminator
	if(0x00 != byHisKey[hiskeylen - 1]){
		ERR_K2HPRN("The history key is wrong format.");
		return false;
	}
	// check separator
	const unsigned char* byTmp;
	for(byTmp = &byHisKey[hiskeylen - 2]; byHisKey < byTmp; --byTmp){
		if(0x00 == *byTmp){
			break;
		}
	}
	if(byHisKey >= byTmp){
		ERR_K2HPRN("The history key is wrong format.");
		return false;
	}

	// make key
	basekeylen = static_cast<size_t>(byTmp - byHisKey);
	if(NULL == (*ppBaseKey = reinterpret_cast<unsigned char*>(malloc(basekeylen)))){
		ERR_K2HPRN("Could not allocate memory.");
		return false;
	}
	memcpy(*ppBaseKey, byHisKey, basekeylen);

	// make uniqid
	byTmp++;			// skip 0x00
	*ppUniqid = strdup(reinterpret_cast<const char*>(byTmp));

	return true;
}

//
// Utility for timeval
//
void K2HShm::GetRealTimeval(struct timeval& tv)
{
	struct timespec	rtime = {0, 0};
	if(-1 == clock_gettime(CLOCK_REALTIME_COARSE, &rtime)){
		WAN_K2HPRN("Failed to get clock time by errno(%d), thus use time function.", errno);
		rtime.tv_sec	= time(NULL);
		rtime.tv_nsec	= 0L;
	}
	tv.tv_sec	= rtime.tv_sec;
	tv.tv_usec	= static_cast<suseconds_t>(rtime.tv_nsec / 1000);
}

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HShm::K2HShm() : ShmFd(-1), isAnonMem(false), isFullMapping(true), isTemporary(false), isReadMode(false), isSync(true), ShmPath(""), pHead(NULL), MmapInfos(this)
{
}

K2HShm::~K2HShm()
{
	Clean();
}

//---------------------------------------------------------
// Methods (loading / cleaning)
//---------------------------------------------------------
// **** NOTICE ****
// 
// [ No Core ]
// On linux, the bit1(anonymous shared mappings) and bit3(file-backed shared mappings) in 
// "/proc/self/coredump_filter" procfs file must set off.
// Then if program is crushed and put core file, but mmap area did not push into core file.
// see: http://man7.org/linux/man-pages/man5/core.5.html
// This likes "MAP_NOCORE"(maybe 0x20000) flag on FreeBSD's mmap.
// 
// [Truncate]
// If using ftruncate for expanding mmap file, At First you should initialize expanding area 
// by like fwrite function before using mmap area. If you did not, it was fatal error.
// 
// [ No Sync ]
// On FreeBSD mmap allows "MAP_NOSYNC"(maybe 0x800) flags, but on linux does not have it.
// If you would use shared memory which mapped a file without syncing, you can do below case.
// * use "MAP_ANONYMOUS"(or "MAP_ANON") flag.
// * map "/dev/zero"
// * map file and unlink it.(maybe this does not work)
// 

bool K2HShm::Clean(bool isRemoveFile)
{
	// stop transaction
	DisableTransaction();

	// clean attribute plugins from common
	CleanCommonAttribute();

	MmapInfos.UnmapAll();
	if(pHead){
		pHead = NULL;
	}

	if(isRemoveFile && 0 < ShmPath.length()){
		if(-1 == unlink(ShmPath.c_str())){
			ERR_K2HPRN("Could not remove file(%s), errno = %d.", ShmPath.c_str(), errno);
			return false;
		}else{
			ShmPath = "";
		}
	}
	if(isTemporary && 0 < ShmPath.length()){
		if(-1 == unlink(ShmPath.c_str()) && EBUSY != errno){
			ERR_K2HPRN("Could not remove file(%s), errno = %d.", ShmPath.c_str(), errno);
			return false;
		}
	}
	FileMon.Close();

	ShmPath			= "";
	isAnonMem		= false;
	isFullMapping	= true;
	isTemporary		= false;
	isReadMode		= false;
	isSync			= true;

	return true;
}

bool K2HShm::Detach(long waitms)
{
	if(waitms < K2HShm::DETACH_BLOCK_WAIT){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsAttached()){
		return false;
	}

	// check transaction
	if(K2HShm::DETACH_NO_WAIT != waitms){
		// wait for finishing transaction
		if(K2HShm::DETACH_BLOCK_WAIT == waitms){
			// wait blocking
			waitms = K2HTransManager::FINISH_WAIT_BLOCK;
		}
		if(!K2HTransManager::Get()->WaitFinish(this, waitms)){
			WAN_K2HPRN("Wait for finishing transaction(%ld ms), but timeouted or error. NOW REMAINING TRANSACTION DATA!", waitms);
		}
	}
	return Clean(false);
}

bool K2HShm::Create(const char* file, bool isfullmapping, int mask_bitcnt, int cmask_bitcnt, int max_element_cnt, size_t pagesize)
{
	if(ISEMPTYSTR(file)){
		ERR_K2HPRN("File name is NULL.");
		return false;
	}
	// check file
	struct stat	st;
	if(0 == stat(file, &st)){
		if(0 != unlink(file)){
			ERR_K2HPRN("File(%s) exists, but remove(initiate) it. errno(%d)", file, errno);
			return false;
		}
	}
	return Attach(file, false, true, false, isfullmapping, mask_bitcnt, cmask_bitcnt, max_element_cnt, pagesize);
}

bool K2HShm::Attach(const char* file, bool isReadOnly, bool isCreate, bool isTempFile, bool isfullmapping, int mask_bitcnt, int cmask_bitcnt, int max_element_cnt, size_t pagesize)
{
	Clean(false);

	if(-1 == K2HShm::InitialSystemPageSize()){
		ERR_K2HPRN("Something error occured in getting page size.");
		return false;
	}
	if(!K2HShm::CheckSystemLimit()){
		ERR_K2HPRN("Something error occured in checking(setting) system limit.");
		return false;
	}

	struct stat	st;
	if(!ISEMPTYSTR(file) && 0 == stat(file, &st)){
		// check file size
		if(st.st_size <= static_cast<off_t>(sizeof(K2H))){
			ERR_K2HPRN("file(%s) is too short as k2hash file. it must be at least %zu bytes", file, sizeof(K2H));
			return false;
		}

		// Specify file & exists
		isTemporary	= isTempFile;

		if(!AttachFile(file, isReadOnly, isfullmapping)){
			ERR_K2HPRN("Could not attach(mmap) file(%s).", file);
			return false;
		}
	}else{
		if(!ISEMPTYSTR(file)){
			// Specify file & does not exist
			if(!isCreate){
				ERR_K2HPRN("File(%s) is not found.", file);
				return false;
			}
			// New file
			if(isReadOnly){
				ERR_K2HPRN("File(%s) is not found, but the request is read mode to open, so error.", file);
				return false;
			}
			isTemporary	= isTempFile;
		}else{
			isTemporary	= false;
		}
		// check mask values
		if(mask_bitcnt < K2HShm::MIN_MASK_BITCOUNT || K2HShm::MAX_MASK_BITCOUNT < mask_bitcnt){
			ERR_K2HPRN("Initializing mask bit is wrong, it must be from %d to %d.", K2HShm::MIN_MASK_BITCOUNT, K2HShm::MAX_MASK_BITCOUNT);
			return false;
		}
		// Initial file(shm)
		if(!InitializeFile(file, isfullmapping, mask_bitcnt, cmask_bitcnt, max_element_cnt, pagesize)){
			ERR_K2HPRN("Could not create file(%s) or mmap.", ISEMPTYSTR(file) ? "anonymous memory" : file);
			return false;
		}
	}
	// default is msync ON when shm file
	SetMsyncMode(true);

	return true;
}

//---------------------------------------------------------
// Expand Methods
//---------------------------------------------------------
bool K2HShm::CheckExpandingKeyArea(PCKINDEX pCKIndex)
{
	if(!pCKIndex){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// For performance
	// First locks as RDLOCK, and checks. If need to expanding, locks as RWLOCK.
	// 
	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RDLOCK);		// LOCK
	K2HLock	ALObjCKI(ShmFd, Rel(pCKIndex), K2HLock::RDLOCK);					// LOCK

	if(pCKIndex->element_count <= pHead->max_element_count){
		// Nothing to do
		MSG_K2HPRN("Does not need to expand key/ckey area. max element count(%lu) is larger than now(%lu)", pHead->max_element_count, pCKIndex->element_count);
		return true;
	}
	if(pHead->max_mask <= pHead->cur_mask){
		// Over maximum mask value
		WAN_K2HPRN("Current mask(%p) could not increase because of limiting by max_mask(%p)", reinterpret_cast<void*>(pHead->cur_mask), reinterpret_cast<void*>(pHead->max_mask));
		MSG_K2HPRN("You need to increase max_mask. This hash has many collision value.");
		return true;
	}
	ALObjCKI.Unlock();
	ALObjCMask.Unlock();

	ALObjCMask.Lock(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RWLOCK);		// LOCK
	ALObjCKI.Lock(ShmFd, Rel(pCKIndex), K2HLock::RDLOCK);					// LOCK

	// re-check
	if(pCKIndex->element_count <= pHead->max_element_count || pHead->max_mask <= pHead->cur_mask){
		// Nothing to do
		return true;
	}
	ALObjCKI.Unlock();

	// Expand key/ckey area(automatically increasing cur_mask)
	{
		if(!ExpandKIndexArea(ALObjCMask)){
			ERR_K2HPRN("Failed increasing Current mask(%p) by error, after setting new key-value.", reinterpret_cast<void*>(pHead->cur_mask));
			return false;
		}
	}
	return true;
}

void* K2HShm::ExpandArea(long type, size_t area_length, off_t& new_area_start)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HLock	ALObjUnArea(ShmFd, Rel(&(pHead->unassign_area)), K2HLock::RWLOCK);	// LOCK
	void*	pNewArea;

	// Get start offset by alignment
	new_area_start	= ALIGNMENT(pHead->unassign_area, static_cast<off_t>(K2HShm::SystemPageSize));

	if(isAnonMem){
		// mapping
		if(MAP_FAILED == (pNewArea = mmap(NULL, area_length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
			ERR_K2HPRN("Could not mmap file, errno = %d", errno);
			return NULL;
		}
	}else{
		// Initialize expanded file area
		if(!K2HShm::InitializeFileZero(ShmFd, pHead->unassign_area, area_length + (new_area_start - pHead->unassign_area))){
			ERR_K2HPRN("Could not initialize new area in file for element.");
			return NULL;
		}
		// mapping
		if(!isFullMapping && K2H_AREA_PAGE == type){
			// [NOTICE]
			// If type is K2H_AREA_PAGE and ShmFile is not full mapping, so this case does not need to mmapping.
			// Set especially value which is not NULL.
			pNewArea = reinterpret_cast<void*>(-1);
		}else{
			if(MAP_FAILED == (pNewArea = mmap(NULL, area_length, PROT_READ | PROT_WRITE, MAP_SHARED, ShmFd, new_area_start))){
				ERR_K2HPRN("Could not mmap file, errno = %d", errno);
				return NULL;
			}
		}
	}
	pHead->unassign_area = new_area_start + area_length;

	// Add mmap area
	if(K2H_AREA_PAGE != type || reinterpret_cast<void*>(-1) != pNewArea){
		MmapInfos.AddArea(type, new_area_start, pNewArea, area_length);
	}

	// Set Area Array
	if(!K2HShm::SetAreasArray(pHead, type, new_area_start, area_length)){
		ERR_K2HPRN("Failed to set New Area(%ld type) information into Areas array.", type);
		return NULL;
	}

	if(!UpdateTimeval(true)){
		WAN_K2HPRN("Failed to update timeval for area update.");
	}

	// msync
	if(!Msync(NULL, true)){
		WAN_K2HPRN("Failed to sync area.");
	}

	// area update
	bool	is_need_check = false;
	if(!isAnonMem && !isTemporary && !FileMon.UpdateArea(is_need_check)){
		ERR_K2HPRN("Could not update area in monitor file, but continue...");
	}
	ALObjUnArea.Unlock();				// UNLOCK

	// need to check area update
	if(is_need_check){
		if(!DoAreaUpdate()){
			ERR_K2HPRN("Something error occurred during updating area information, but continue...");
		}
	}
	return pNewArea;
}

bool K2HShm::ExpandKIndexArea(K2HLock& ALObjCMask)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	ALObjCMask.Lock(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RWLOCK);	// LOCK

	// Key Index
	PCKINDEX	pCKIndex;
	void*		pCKIndexShmBase;

	// Collision Key Index
	{
		// Get start offset by alignment
		off_t	new_area_start	= 0L;
		int		ckindex_count	= INIT_CKINDEX_CNT(K2HShm::GetMaskBitCount(pHead->cur_mask), K2HShm::GetMaskBitCount(pHead->collision_mask));	// (new cur_mask) - 1 = old cur_mask
		size_t	area_length		= sizeof(CKINDEX) * ckindex_count;

		if(NULL == (pCKIndex = static_cast<PCKINDEX>(ExpandArea(K2H_AREA_CKINDEX, area_length, new_area_start)))){
			ERR_K2HPRN("Could not expand area for collision key index.");
			return false;
		}

		if(!K2HShm::InitializeCollisionKeyIndexArray(pCKIndex, ckindex_count)){
			ERR_K2HPRN("Failed to initialize new Collision Key Index Area.");
			return false;
		}
		pCKIndexShmBase = SUBPTR(pCKIndex, new_area_start);
	}

	// Key Index
	{
		// Get start offset by alignment
		PKINDEX	pKIndex;
		off_t	new_area_start	= 0L;
		int		kindex_count	= INIT_KINDEX_CNT(K2HShm::GetMaskBitCount(pHead->cur_mask));	// (new cur_mask) - 1 = old cur_mask
		size_t	area_length		= sizeof(KINDEX) * kindex_count;

		if(NULL == (pKIndex = static_cast<PKINDEX>(ExpandArea(K2H_AREA_KINDEX, area_length, new_area_start)))){
			ERR_K2HPRN("Could not expand area for key index.");
			return false;
		}

		// Initialize KIndex value & set CKIndex into KIndex
		// NOTICE: start_hash is ((real hash value for Key Index) >> collision mask bit count).
		//
		k2h_hash_t	start_hash	= 1UL << K2HShm::GetMaskBitCount(pHead->cur_mask);
		k2h_hash_t	new_cur_mask= (pHead->cur_mask << 1) + 1UL;
		if(!K2HShm::InitializeKeyIndexArray(pCKIndexShmBase, pKIndex, start_hash, new_cur_mask, K2HShm::GetMaskBitCount(pHead->collision_mask), pCKIndex, kindex_count, KINDEX_NOTASSIGNED)){
			ERR_K2HPRN("Failed to initialize new Key Index Area.");
			return false;
		}

		// Set new KIndex array
		pHead->key_index_area[K2HShm::GetMaskBitCount(new_cur_mask)] = reinterpret_cast<PKINDEX>(Rel(pKIndex));
		pHead->cur_mask = new_cur_mask;
	}
	return true;
}

// [NOTICE]
// ALObjFEC must be RWLOCK or UNLOCK before calling this method
//
bool K2HShm::ExpandElementArea(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HLock		ALObjFEC(ShmFd, Rel(&(pHead->free_element_count)), K2HLock::RWLOCK);	// LOCK
	PELEMENT	pElement;

	// Get start offset by alignment
	off_t	new_area_start	= 0L;
	int		element_count	= INIT_CKINDEX_CNT(K2HShm::GetMaskBitCount(pHead->cur_mask), K2HShm::GetMaskBitCount(pHead->collision_mask)) * K2HShm::ELEMENT_CNT_RATIO;
			element_count	= min(element_count, K2HShm::MAX_EXPAND_ELEMENT_CNT);
	size_t	area_length		= sizeof(ELEMENT) * element_count;

	if(NULL == (pElement = static_cast<PELEMENT>(ExpandArea(K2H_AREA_PAGELIST, area_length, new_area_start)))){
		ERR_K2HPRN("Could not expand area for element.");
		return false;
	}

	// initialize for elements( free_elements -> new area -> old free elements)
	if(!K2HShm::InitializeElementArray(SUBPTR(pElement, new_area_start), pElement, element_count, pHead->pfree_elements)){
		ERR_K2HPRN("Failed to initialize new element.");
		return false;
	}

	PELEMENT	pElementTop = static_cast<PELEMENT>(Abs(pHead->pfree_elements));
	if(pElementTop){
		// old free element top parent --> lastest new area
		pElementTop->parent = reinterpret_cast<PELEMENT>(Rel( ADDPTR(pElement, static_cast<off_t>(sizeof(ELEMENT) * (element_count - 1))) ));
	}
	pHead->pfree_elements		= reinterpret_cast<PELEMENT>(Rel(pElement));
	pHead->free_element_count	+= element_count;

	return true;
}

// [NOTICE]
// ALObjFPC must be RWLOCK or UNLOCK before calling this method
//
bool K2HShm::ExpandPageArea(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HLock		ALObjFPC(ShmFd, Rel(&(pHead->free_page_count)), K2HLock::RWLOCK);	// LOCK
	PPAGEHEAD	pPage;

	// Get start offset by alignment
	off_t	new_area_start	= 0L;
	int		page_count		= (INIT_CKINDEX_CNT(K2HShm::GetMaskBitCount(pHead->cur_mask), K2HShm::GetMaskBitCount(pHead->collision_mask)) * K2HShm::ELEMENT_CNT_RATIO) * K2HShm::PAGE_CNT_RATIO;
			page_count		= min(page_count, K2HShm::MAX_EXPAND_PAGE_CNT);
	size_t	area_length		= pHead->page_size * page_count;

	if(NULL == (pPage = static_cast<PPAGEHEAD>(ExpandArea(K2H_AREA_PAGE, area_length, new_area_start)))){
		ERR_K2HPRN("Could not expand area for element.");
		return false;
	}

	if(isFullMapping){
		// initialize for page( free_pages -> new area -> old free pages)
		if(!K2HShm::InitializePageArray(SUBPTR(pPage, new_area_start), pPage, pHead->page_size, page_count, pHead->pfree_pages)){
			ERR_K2HPRN("Failed to initialize new pages.");
			return false;
		}

		PPAGEHEAD	pPageTop = static_cast<PPAGEHEAD>(Abs(pHead->pfree_pages));
		if(pPageTop){
			// old free page top prev --> lastest new area
			pPageTop->prev = reinterpret_cast<PPAGEHEAD>(Rel( ADDPTR(pPage, static_cast<off_t>(pHead->page_size * (page_count - 1))) ));
		}
		pHead->pfree_pages		= reinterpret_cast<PPAGEHEAD>(Rel(pPage));
		pHead->free_page_count	+= page_count;
	}else{
		// initialize for page( free_pages -> new area -> old free pages)
		if(!K2HShm::InitializePageArray(ShmFd, new_area_start, pHead->page_size, page_count, pHead->pfree_pages)){
			ERR_K2HPRN("Failed to initialize new pages.");
			return false;
		}

		if(pHead->pfree_pages){
			// old free page top prev --> lastest new area
			// can not use Rel() because before setting new area in mmapinfos.
			if(!ReplacePageHead(pHead->pfree_pages, reinterpret_cast<PPAGEHEAD>(new_area_start + (pHead->page_size * (page_count - 1))), true)){
				ERR_K2HPRN("Failed to set pointer into old top free pagehead pointer.");
				return false;
			}
		}
		pHead->pfree_pages		= reinterpret_cast<PPAGEHEAD>(new_area_start);
		pHead->free_page_count	+= page_count;
	}
	return true;
}

bool K2HShm::ReplacePageHead(PPAGEHEAD pLastRelPage, PPAGEHEAD pRelPtr, bool isSetPrevPtr) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	PAGEWRAP	PageWrap;
	{
		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress unreadVariable
		PageWrap.pagehead.prev		= isSetPrevPtr ? pRelPtr : NULL;

		// cppcheck-suppress unmatchedSuppression
		// cppcheck-suppress unreadVariable
		PageWrap.pagehead.next		= isSetPrevPtr ? NULL : pRelPtr;
	}
	unsigned char*	pbyData	= &(PageWrap.barray[isSetPrevPtr ? PAGEHEAD_PREV_OFFSET : PAGEHEAD_NEXT_OFFSET]);
	off_t			offset	= reinterpret_cast<off_t>(pLastRelPage) + (isSetPrevPtr ? PAGEHEAD_PREV_OFFSET : PAGEHEAD_NEXT_OFFSET);

	if(-1 == k2h_pwrite(ShmFd, pbyData, sizeof(PPAGEHEAD), offset)){
		ERR_K2HPRN("Could not write from fd(%d:%jd:%zu).", ShmFd, static_cast<intmax_t>(offset), sizeof(PPAGEHEAD));
		return false;
	}
	return true;
}

// [NOTICE]
// ALObjFPC must be RWLOCK or UNLOCK before calling this method
//
bool K2HShm::PutBackPages(PPAGEHEAD pRelTopPage, PPAGEHEAD pRelLastPage, unsigned long pagecount) const
{
	if(!pRelTopPage || !pRelLastPage){
		ERR_K2HPRN("PPAGEHEAD is NULL.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HLock	ALObjFPC(ShmFd, Rel(&(pHead->free_page_count)), K2HLock::RWLOCK);		// LOCK

	// old free page top prev --> pRelLastPage
	// pRelLastPage next      --> old free page top
	if(isFullMapping){
		PPAGEHEAD	pOldPageTop	= static_cast<PPAGEHEAD>(Abs(pHead->pfree_pages));
		PPAGEHEAD	pAbsLastPage= static_cast<PPAGEHEAD>(Abs(pRelLastPage));

		pAbsLastPage->next = pHead->pfree_pages;

		if(pOldPageTop){
			pOldPageTop->prev = pRelLastPage;
		}
	}else{
		if(!ReplacePageHead(pRelLastPage, pHead->pfree_pages, false)){
			ERR_K2HPRN("Failed to set pointer into new free pagehead pointer.");
			return false;
		}
		if(pHead->pfree_pages){
			if(!ReplacePageHead(pHead->pfree_pages, pRelLastPage, true)){
				ERR_K2HPRN("Failed to set pointer into old top free pagehead pointer.");
				return false;
			}
		}
	}
	pHead->pfree_pages = pRelTopPage;
	if(pagecount <= static_cast<unsigned long>(pHead->free_page_count)){
		pHead->free_page_count += pagecount;
	}else{
		pHead->free_page_count = 0UL;
	}

	return true;
}

//
// Expanding(adding) pages
//
bool K2HShm::ExpandPages(K2HPage* pLastPage, size_t length)
{
	if(!pLastPage || 0UL == length){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	PPAGEHEAD	pLastHead = pLastPage->GetPageHeadRelAddress();
	if(!pLastHead){
		ERR_K2HPRN("PPAGEHEAD is null or the page is not last page.");
		return false;
	}

	K2HPage*	pNewPages;
	if(NULL == (pNewPages = ReservePages(length))){
		ERR_K2HPRN("Failed to add pages( +%zu bytes)", length);
		return false;
	}

	if(!pNewPages->SetPageHead(K2HPage::SETHEAD_PREV, pLastHead)){
		ERR_K2HPRN("Could not set prev pointer for new top page object");
		pNewPages->Free();
		K2H_Delete(pNewPages);
		return false;
	}

	if(!pLastPage->SetPageHead(K2HPage::SETHEAD_NEXT, NULL, pNewPages->GetPageHeadRelAddress())){
		ERR_K2HPRN("Could not set next pointer for reserving page object.");
		pNewPages->Free();
		K2H_Delete(pNewPages);
		return false;
	}
	K2H_Delete(pNewPages);

	return true;
}

//---------------------------------------------------------
// Get Methods
//---------------------------------------------------------
K2HPage* K2HShm::GetPageObject(PPAGEHEAD pRelPageHead, bool need_load) const
{
	if(!pRelPageHead){
		MSG_K2HPRN("RelPageHead is NULL.");
		return NULL;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HPage*	pPage;
	if(isFullMapping){
		pPage = new K2HPageMem(this, pRelPageHead);										// K2HPageMem class needs relative address
	}else{
		pPage = new K2HPageFile(this, ShmFd, reinterpret_cast<off_t>(pRelPageHead));	// Notice: offset is pRelPageHead
	}
	if(pPage && need_load){
		if(!pPage->LoadData()){
			ERR_K2HPRN("Failed to load page data.");
			K2H_Delete(pPage);
			return NULL;
		}
	}
	return pPage;
}

//
// <cur_mask>  <KIPtrArrayPos>      <KIArrayCount>
// 0x00000000  key_index_area[0]  -> PKINDEX[1]
// 0x00000001  key_index_area[1]  -> PKINDEX[1]
// 0x00000003  key_index_area[2]  -> PKINDEX[2]
// 0x00000007  key_index_area[3]  -> PKINDEX[4]
//    ...           ...                ...
// 0x7FFFFFFF  key_index_area[30] -> PKINDEX[2^29]
// 0xFFFFFFFF  key_index_area[31] -> PKINDEX[2^30]
//
bool K2HShm::GetKIndexPos(k2h_hash_t hash, k2h_arrpos_t& KIPtrArrayPos, k2h_arrpos_t& KIArrayPos, k2h_hash_t* pCurMask) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RDLOCK);	// LOCK

	if(pCurMask && (pHead->cur_mask < *pCurMask)){
		ERR_K2HPRN("Specified current mask value(%p: 0x%" PRIx64 ") is over current mask value(%p: 0x%" PRIx64 ").", reinterpret_cast<void*>(*pCurMask), *pCurMask, reinterpret_cast<void*>(pHead->cur_mask), pHead->cur_mask);
		return false;
	}
	k2h_hash_t	shifted_hash = (hash >> K2HShm::GetMaskBitCount(pHead->collision_mask));
	k2h_hash_t	bitmask;
	k2h_hash_t	tmphash;

	// Get position of Key Index Pointers Array
	for(tmphash = shifted_hash & (pCurMask ? *pCurMask : pHead->cur_mask), KIPtrArrayPos = 0UL, bitmask = 0UL; 0 != (tmphash & ~bitmask); KIPtrArrayPos++, bitmask = ((bitmask << 1) | 1UL));

	// Make position of Key Index Array which is pointed by position of Key Index Pointers Array
	KIArrayPos = shifted_hash & K2HShm::MakeMask(0 < KIPtrArrayPos ? KIPtrArrayPos - 1 : 0);

	return true;
}

//
// This function does not check whichever the Key Index is assigned.
// If *pCurMak is over cur_mask, this means there is not assigned Key Index area.
// So this function returns NULL.
//
PKINDEX K2HShm::GetReservedKIndex(k2h_hash_t hash, bool isAbsolute, k2h_hash_t* pCurMask) const
{
	k2h_arrpos_t	KIPtrArrayPos;
	k2h_arrpos_t	KIArrayPos;

	K2HLock	ALObjCMask(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RDLOCK);	// LOCK

	if(!GetKIndexPos(hash, KIPtrArrayPos, KIArrayPos, pCurMask)){
		return NULL;
	}
	if(!isAbsolute){
		return CVT_REL_PKINDEX(pHead->key_index_area, KIPtrArrayPos, KIArrayPos);
	}else{
		return CVT_ABS_PKINDEX(pHead->key_index_area, KIPtrArrayPos, KIArrayPos);
	}
}

//
// This function returns Assigned Key Index.
// If finding assigned key index, do to arrange elements/ckey index in key indexes.
// This case is after increasing cur_mask value.( and expanding key index area )
//
PKINDEX K2HShm::GetKIndex(k2h_hash_t hash, bool isMergeCurmask, bool is_need_lock) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}

	K2HLock	ALObjCMask;
	if(is_need_lock){
		ALObjCMask.Lock(ShmFd, Rel(&(pHead->cur_mask)), K2HLock::RDLOCK);	// LOCK
	}

	// [need to update area]
	// When accessing with k2hash iterator, that logic only calls update file(area) at first.
	// If looping for dumping all key, it is very wrong because of not updating area.
	// So here, updating mmap area.(only adding)
	//
	K2HFILE_UPDATE_AREA(const_cast<K2HShm*>(this));

	PKINDEX	pKindex = NULL;
	for(k2h_hash_t cur_mask = pHead->cur_mask; 0 < cur_mask; cur_mask = cur_mask >> 1){
		k2h_arrpos_t	KIPtrArrayPos;
		k2h_arrpos_t	KIArrayPos;
		if(!GetKIndexPos(hash, KIPtrArrayPos, KIArrayPos, &cur_mask)){
			return NULL;
		}

		pKindex = CVT_ABS_PKINDEX(pHead->key_index_area, KIPtrArrayPos, KIArrayPos);
		if(pKindex && KINDEX_ASSIGNED == pKindex->assign){
			break;
		}else if(pKindex && KINDEX_ASSIGNED != pKindex->assign && isMergeCurmask){
			// Need to merge position for elements, because cur_mask is changed.
			MSG_K2HPRN("Need to check and move hash(%" PRIu64 ") element.", hash);

			if(!ArrangeToUpperKIndex(hash, pHead->cur_mask)){
				ERR_K2HPRN("Failed to arrange key index.");
				return NULL;
			}
			// retry...
			return GetKIndex(hash, false, false);
		}
	}
	return pKindex;
}

// 
// After increasing cur_mask bits, new Key Index is not assigned for elements in ckey
// index list. So this function merges elements in old key indexes to new key indexes.
// Be careful, this function calls own.
// 
bool K2HShm::ArrangeToUpperKIndex(k2h_hash_t hash, k2h_hash_t mask) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(mask < 3L){
		// nothing to do
		return true;
	}
	if(pHead->cur_mask < mask){
		// nothing to do
		return true;
	}

	// Checking Lower
	if(!ArrangeToUpperKIndex(hash, (mask >> 1))){
		ERR_K2HPRN("Failed to arrange ckey index(with elements) to Upper key index.");
		return false;
	}

	// Key Indexes
	k2h_hash_t	lower_mask	= (mask >> 1);
	PKINDEX		pUpperKIndex= GetReservedKIndex(hash, true, &mask);				// not using cur_mask
	PKINDEX		pLowerKIndex= GetReservedKIndex(hash, true, &lower_mask);
	if(!pUpperKIndex || !pLowerKIndex){
		ERR_K2HPRN("Upper KeyIndex(mask:%p) is %p or Lower KeyIndex(mask:%p) is %p", reinterpret_cast<void*>(mask), (pUpperKIndex ? pUpperKIndex : NULL), reinterpret_cast<void*>(lower_mask), (pLowerKIndex ? pLowerKIndex : NULL));
		return false;
	}else if(pUpperKIndex == pLowerKIndex){
		MSG_K2HPRN("Upper KeyIndex((mask:%p) %p) is same as Lower KeyIndex((mask:%p) %p), so not need to arrange..", reinterpret_cast<void*>(mask), (pUpperKIndex ? pUpperKIndex : NULL), reinterpret_cast<void*>(lower_mask), (pLowerKIndex ? pLowerKIndex : NULL));
		return true;
	}

	// CKey Indexes
																	// cppcheck-suppress unmatchedSuppression
																	// cppcheck-suppress nullPointerRedundantCheck
	PCKINDEX	pUpperCKIndex	= static_cast<PCKINDEX>(Abs(pUpperKIndex->ckey_list));
																	// cppcheck-suppress unmatchedSuppression
																	// cppcheck-suppress nullPointerRedundantCheck
	PCKINDEX	pLowerCKIndex	= static_cast<PCKINDEX>(Abs(pLowerKIndex->ckey_list));
	if(!pUpperCKIndex || !pLowerCKIndex){
		ERR_K2HPRN("FATAL: Upper CKeyIndex is %p or Lower CKeyIndex is %p", (pUpperKIndex ? pUpperKIndex : NULL), (pLowerKIndex ? pLowerKIndex : NULL));
		return false;
	}

	// Move lower child elements to upper
	K2HLock			ALObjCKI1(K2HLock::RWLOCK);
	K2HLock			ALObjCKI2(K2HLock::RWLOCK);
	k2h_arrpos_t	CKIndexCount = pHead->collision_mask + 1;		// Array count
	for(k2h_arrpos_t CKIndexPos = 0L; CKIndexPos < CKIndexCount; CKIndexPos++){
		ALObjCKI1.Lock(ShmFd, Rel(&pLowerCKIndex[CKIndexPos]));		// LOCK(Important order)
		ALObjCKI2.Lock(ShmFd, Rel(&pUpperCKIndex[CKIndexPos]));		// LOCK

		if(!(pLowerCKIndex[CKIndexPos].element_list)){
			ALObjCKI2.Unlock();
			ALObjCKI1.Unlock();
			continue;
		}
		// NOTICE: CKIndexPos is same for both CKeyIndex.
		k2h_hash_t	shifted_hash = hash >> K2HShm::GetMaskBitCount(pHead->collision_mask);
		if(!MoveElementToUpperMask(static_cast<PELEMENT>(Abs(pLowerCKIndex[CKIndexPos].element_list)), &pLowerCKIndex[CKIndexPos], &pUpperCKIndex[CKIndexPos], mask, (shifted_hash & mask))){
			ERR_K2HPRN("Failed to move elements in lower ckey index to upper ckey index.");
			return false;
		}
		ALObjCKI2.Unlock();
		ALObjCKI1.Unlock();
	}

	// Assigned
	if(KINDEX_ASSIGNED != pUpperKIndex->assign){
		pUpperKIndex->assign = KINDEX_ASSIGNED;
	}
	return true;
}

//
// This function checks and moves a element ckey index in lower key index to ckey in upper.
// Be careful, this function calls own.
// 
// [NOTICE]
// Must lock source and destination ckindex before calling this method.
//
bool K2HShm::MoveElementToUpperMask(PELEMENT pElement, PCKINDEX pSrcCKIndex, PCKINDEX pDstCKIndex, k2h_hash_t target_mask, k2h_hash_t target_masked_val) const
{
	if(!pElement || !pSrcCKIndex || !pDstCKIndex){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(pSrcCKIndex == pDstCKIndex){
		ERR_K2HPRN("Source and Destination CKIndex address(%p) is same, why?", pSrcCKIndex);
		return true;		// return as success
	}

	// Does the element have children?
	if(pElement->same){
		if(pElement->same == pElement){
			WAN_K2HPRN("Found element->same is as same as itself, thus try to repair it.");
			pElement->same = NULL;
		}else{
			if(!MoveElementToUpperMask(static_cast<PELEMENT>(Abs(pElement->same)), pSrcCKIndex, pDstCKIndex, target_mask, target_masked_val)){
				ERR_K2HPRN("Failed to move same children elements.");
				return false;
			}
		}
	}
	if(pElement->small){
		if(pElement->small == pElement){
			WAN_K2HPRN("Found element->small is as same as itself, thus try to repair it.");
			pElement->small = NULL;
		}else{
			if(!MoveElementToUpperMask(static_cast<PELEMENT>(Abs(pElement->small)), pSrcCKIndex, pDstCKIndex, target_mask, target_masked_val)){
				ERR_K2HPRN("Failed to move small children elements.");
				return false;
			}
		}
	}
	if(pElement->big){
		if(pElement->big == pElement){
			WAN_K2HPRN("Found element->big is as same as itself, thus try to repair it.");
			pElement->big = NULL;
		}else{
			if(!MoveElementToUpperMask(static_cast<PELEMENT>(Abs(pElement->big)), pSrcCKIndex, pDstCKIndex, target_mask, target_masked_val)){
				ERR_K2HPRN("Failed to move big children elements.");
				return false;
			}
		}
	}

	// own
	k2h_hash_t	shifted_hash= (pElement->hash >> K2HShm::GetMaskBitCount(pHead->collision_mask));
	if(target_masked_val == (shifted_hash & target_mask)){
		// remove this element from source ckey index.
		if(!TakeOffElement(pSrcCKIndex, pElement)){
			ERR_K2HPRN("Failed to take off a element.");
			return false;
		}
		// insert this element from source ckey index.
		if(NULL == pDstCKIndex->element_list){
			pDstCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pElement));
		}else{
			if(!InsertElement(static_cast<PELEMENT>(Abs(pDstCKIndex->element_list)), pElement)){
				ERR_K2HPRN("FATAL: Failed to insert element that is removed element's small element, so elements are leaked!!!");
				return false;
			}
		}
		pDstCKIndex->element_count += 1UL;

		MSG_K2HPRN("pElement->hash(%" PRIu64 ") is moved.", pElement->hash);
	}
	return true;
}

//
// This function returns Assigned Collision Key Index.
//
PCKINDEX K2HShm::GetCKIndex(PKINDEX pKindex, k2h_hash_t hash, K2HLock& ALObjCKI) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	if(!pKindex){
		ERR_K2HPRN("PKINDEX is null.");
		return NULL;
	}
	// Get list pointer in KeyIndex
	PCKINDEX	pCKindex = static_cast<PCKINDEX>(Abs(pKindex->ckey_list));
	if(!pCKindex){
		ERR_K2HPRN("PCKINDEX list is null.");
		return NULL;
	}
	if(ALObjCKI.IsLocked()){
		ALObjCKI.Unlock();
	}
	// Get target CKIndex pointer
	ALObjCKI.Lock(ShmFd, Rel(&pCKindex[hash & pHead->collision_mask]));				// LOCK

	return &pCKindex[hash & pHead->collision_mask];
}

PCKINDEX K2HShm::GetCKIndex(k2h_hash_t hash, K2HLock& ALObjCKI, bool isMergeCurmask) const
{
	PKINDEX	pKindex;
	if(NULL == (pKindex = GetKIndex(hash, isMergeCurmask ? !ALObjCKI.IsLocked() : isMergeCurmask))){	// if already locked ALObjCKI, must not merge in GetKIndex
		return NULL;
	}
	return GetCKIndex(pKindex, hash, ALObjCKI);
}

//
// Get same has value Element list.
//
PELEMENT K2HShm::GetElementList(PELEMENT pRelElement, k2h_hash_t hash, k2h_hash_t subhash) const
{
	PELEMENT pElement = static_cast<PELEMENT>(Abs(pRelElement));
	if(!pElement){
		return NULL;
	}
	if(subhash == pElement->subhash){
		if(hash != pElement->hash){
			// why?
			WAN_K2HPRN("Element(%p) is same subhash(%" PRIu64 "), but not same hash(%" PRIu64 ") expected hash(%" PRIu64 ")", pElement, subhash, pElement->hash, hash);
			return NULL;
		}else{
			// found
			MSG_K2HPRN("Found element(%p) is same subhash(%" PRIu64 ") and hash(%" PRIu64 ")", pElement, subhash, hash);
		}
	}else if(subhash < pElement->subhash){
		if(pElement->small){
			pElement = GetElementList(pElement->small, hash, subhash);
		}else{
			pElement = NULL;
		}
	}else{	// pElement->subhash < subhash
		if(pElement->big){
			pElement = GetElementList(pElement->big, hash, subhash);
		}else{
			pElement = NULL;
		}
	}
	return pElement;
}

PELEMENT K2HShm::GetElementList(PCKINDEX pCKindex, k2h_hash_t hash, k2h_hash_t subhash) const
{
	if(!pCKindex){
		ERR_K2HPRN("PCKINDEX is null.");
		return NULL;
	}
	if(0UL == pCKindex->element_count){
		return NULL;
	}
	return GetElementList(pCKindex->element_list, hash, subhash);
}

PELEMENT K2HShm::GetElementList(PKINDEX pKindex, k2h_hash_t hash, k2h_hash_t subhash, K2HLock& ALObjCKI) const
{
	if(!pKindex){
		ERR_K2HPRN("PKINDEX is null.");
		return NULL;
	}
	PCKINDEX	pCKindex;
	if(NULL == (pCKindex = GetCKIndex(pKindex, hash, ALObjCKI))){
		return NULL;
	}
	return GetElementList(pCKindex, hash, subhash);
}

PELEMENT K2HShm::GetElementList(k2h_hash_t hash, k2h_hash_t subhash, K2HLock& ALObjCKI) const
{
	PKINDEX	pKindex;
	if(NULL == (pKindex = GetKIndex(hash, !ALObjCKI.IsLocked()))){		// if already locked ALObjCKI, must not merge in GetKIndex
		return NULL;
	}
	return GetElementList(pKindex, hash, subhash, ALObjCKI);
}

unsigned long K2HShm::GetElementListUpCount(PELEMENT pElementList) const
{
	if(!pElementList){
		WAN_K2HPRN("PELEMENT is null.");
		return 0UL;
	}
	PELEMENT		pTmpElement;
	unsigned long	count = 1UL;		// own

	if(NULL != (pTmpElement = static_cast<PELEMENT>(Abs(pElementList->small)))){
		count += GetElementListUpCount(pTmpElement);
	}
	if(NULL != (pTmpElement = static_cast<PELEMENT>(Abs(pElementList->big)))){
		count += GetElementListUpCount(pTmpElement);
	}
	if(NULL != (pTmpElement = static_cast<PELEMENT>(Abs(pElementList->same)))){
		count += GetElementListUpCount(pTmpElement);
	}
	return count;
}

PELEMENT K2HShm::GetElement(PELEMENT pElementList, const unsigned char* byKey, size_t length) const
{
	PELEMENT	pElement;
	for(pElement = pElementList ; pElement; pElement = static_cast<PELEMENT>(Abs(pElement->same))){
		K2HPage*	pPage;
		if(NULL == (pPage = GetPageObject(pElement->key))){
			WAN_K2HPRN("Failed to make(get) Page Object");
			continue;
		}
		if(pPage->Compare(byKey, length)){
			K2H_Delete(pPage);
			return pElement;
		}
		K2H_Delete(pPage);
	}
	return NULL;
}

PELEMENT K2HShm::GetElement(const unsigned char* byKey, size_t length, k2h_hash_t hash, k2h_hash_t subhash, K2HLock& ALObjCKI) const
{
	PELEMENT	pElement;
	if(NULL == (pElement = GetElementList(hash, subhash, ALObjCKI))){
		return NULL;
	}
	return GetElement(pElement, byKey, length);
}

PELEMENT K2HShm::GetElement(const unsigned char* byKey, size_t length, K2HLock& ALObjCKI) const
{
	if(!byKey || 0UL == length){
		ERR_K2HPRN("Parameters is wrong.");
		return NULL;
	}
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byKey), length);
	k2h_hash_t	subhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byKey), length);

	return GetElement(byKey, length, hash, subhash, ALObjCKI);
}

PELEMENT K2HShm::GetElement(const char* pKey, K2HLock& ALObjCKI) const
{
	return GetElement(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL, ALObjCKI);
}

// [NOTICE]
// ALObjFEC must be RWLOCK or UNLOCK before calling this method
//
PELEMENT K2HShm::ReserveElement(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HLock		ALObjFEC(ShmFd, Rel(&(pHead->free_element_count)), K2HLock::RWLOCK);	// LOCK
	PELEMENT	pElement;
	if(NULL == (pElement = static_cast<PELEMENT>(Abs(pHead->pfree_elements)))){
		if(!ExpandElementArea()){
			ERR_K2HPRN("Failed to expand ELEMENT area.");
			return NULL;
		}
		if(NULL == (pElement = static_cast<PELEMENT>(Abs(pHead->pfree_elements)))){
			ERR_K2HPRN("There is not free ELEMENT area.");
			return NULL;
		}
	}

	PELEMENT	pNextElement = static_cast<PELEMENT>(Abs(pElement->same));
	if(pNextElement){
		pNextElement->parent = NULL;
	}
	pHead->pfree_elements	= pElement->same;
	pElement->parent		= NULL;
	pElement->same			= NULL;
	pElement->small			= NULL;
	pElement->big			= NULL;

	if(0 < pHead->free_element_count){
		pHead->free_element_count -= 1UL;
	}
	return pElement;
}

bool K2HShm::InsertElement(PELEMENT pTopElement, PELEMENT pElement) const
{
	// NOTICE: This function is not care for same key value.
	// 
	// If you use this function, you must check same value and remove it.
	// This function adding a element into same element list by only subhash.
	// 
	if(!pTopElement || !pElement){
		ERR_K2HPRN("Top Element or Element is NULL");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	if(pTopElement->subhash < pElement->subhash){
		if(pTopElement->big){
			return InsertElement(static_cast<PELEMENT>(Abs(pTopElement->big)), pElement);
		}else{
			pTopElement->big = reinterpret_cast<PELEMENT>(Rel(pElement));
			pElement->parent = reinterpret_cast<PELEMENT>(Rel(pTopElement));
		}
	}else if(pTopElement->subhash > pElement->subhash){
		if(pTopElement->small){
			return InsertElement(static_cast<PELEMENT>(Abs(pTopElement->small)), pElement);
		}else{
			pTopElement->small = reinterpret_cast<PELEMENT>(Rel(pElement));
			pElement->parent = reinterpret_cast<PELEMENT>(Rel(pTopElement));
		}
	}else{	// pTopElement->subhash == pElement->subhash
		PELEMENT	pSameElement;
		for(pSameElement = pTopElement; pSameElement->same; pSameElement = static_cast<PELEMENT>(Abs(pSameElement->same)));
		pSameElement->same = reinterpret_cast<PELEMENT>(Rel(pElement));
		pElement->parent = reinterpret_cast<PELEMENT>(Rel(pSameElement));
	}
	return true;
}

// [NOTICE]
// Must lock ALObjCKI(pCKIndex) before calling this method.
//
bool K2HShm::TakeOffElement(PCKINDEX pCKIndex, PELEMENT pElement) const
{
	if(!pCKIndex || !pElement){
		ERR_K2HPRN("Collision Key Index or Element is NULL");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	PELEMENT	pParent	= static_cast<PELEMENT>(Abs(pElement->parent));
	if(!pParent && (pElement != static_cast<PELEMENT>(Abs(pCKIndex->element_list)))){
		ERR_K2HPRN("Something wrong, this element is not top element.");
		return false;
	}

	PELEMENT	pInsertSmall= NULL;
	PELEMENT	pInsertBig	= NULL;
	bool		isCountDown	= true;
	if(pElement->same){
		// there are same hash values.
		// --> switch same value
		PELEMENT pChild	= static_cast<PELEMENT>(Abs(pElement->same));
		if(!pChild){
			ERR_K2HPRN("Something wrong, this element->same is not safe address.");
			return false;
		}
		pChild->parent	= pElement->parent;
		pChild->small	= pElement->small;
		pChild->big		= pElement->big;

		if(pParent){
			if(pElement == static_cast<PELEMENT>(Abs(pParent->small))){
				pParent->small = pElement->same;
			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->big))){
				pParent->big = pElement->same;
			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->same))){
				pParent->same = pElement->same;
			}else{
				// why?
				WAN_K2HPRN("Parent does not have this element pointer.");
				isCountDown = false;
			}
		}else{
			// This element is top
			pCKIndex->element_list = pElement->same;
		}

	}else if(pElement->big && pElement->small){
		// there are big and small hash link.
		PELEMENT	pBig	= static_cast<PELEMENT>(Abs(pElement->big));
		PELEMENT	pSmall	= static_cast<PELEMENT>(Abs(pElement->small));

		if(pParent){
			if(pElement == static_cast<PELEMENT>(Abs(pParent->small))){
				// this element is parent's small
				// --> replace this element's big
				pBig->parent	= pElement->parent;
				pParent->small	= pElement->big;
				pInsertSmall	= static_cast<PELEMENT>(Abs(pElement->small));	// For inserting after

			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->big))){
				// this element is parent's big
				// --> replace this element's small
				pSmall->parent	= pElement->parent;
				pParent->big	= pElement->small;
				pInsertBig		= static_cast<PELEMENT>(Abs(pElement->big));	// For inserting after

			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->same))){
				// why?
				WAN_K2HPRN("Parent\'s same is this element pointer, it is something wrong.");
				pParent->same	= NULL;
				pInsertSmall	= static_cast<PELEMENT>(Abs(pElement->small));	// For inserting after
				pInsertBig		= static_cast<PELEMENT>(Abs(pElement->big));	// For inserting after

			}else{
				// why?
				WAN_K2HPRN("Parent does not have this element pointer.");
				isCountDown = false;
			}
		}else{
			// This element is top
			pBig->parent			= NULL;
			pCKIndex->element_list	= pElement->big;
			pInsertSmall			= static_cast<PELEMENT>(Abs(pElement->small));	// For inserting after
		}

	}else if(pElement->big){
		// there is big hash link.
		// --> switch small value
		PELEMENT	pBig = static_cast<PELEMENT>(Abs(pElement->big));

		pBig->parent = pElement->parent;

		if(pParent){
			if(pElement == static_cast<PELEMENT>(Abs(pParent->small))){
				pParent->small = pElement->big;

			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->big))){
				pParent->big = pElement->big;

			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->same))){
				// why?
				WAN_K2HPRN("Parent\'s same is this element pointer, it is something wrong.");
				pParent->same	= NULL;
				pInsertBig		= static_cast<PELEMENT>(Abs(pElement->big));	// For inserting after

			}else{
				// why?
				WAN_K2HPRN("Parent does not have this element pointer.");
				isCountDown = false;
			}
		}else{
			// This element is top
			pCKIndex->element_list = pElement->big;
		}

	}else if(pElement->small){
		// there is small hash link.
		// --> switch small value
		PELEMENT	pSmall = static_cast<PELEMENT>(Abs(pElement->small));

		pSmall->parent = pElement->parent;

		if(pParent){
			if(pElement == static_cast<PELEMENT>(Abs(pParent->small))){
				pParent->small = pElement->small;

			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->big))){
				pParent->big = pElement->small;

			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->same))){
				// why?
				WAN_K2HPRN("Parent\'s same is this element pointer, it is something wrong.");
				pParent->same	= NULL;
				pInsertSmall	= static_cast<PELEMENT>(Abs(pElement->small));	// For inserting after

			}else{
				// why?
				WAN_K2HPRN("Parent does not have this element pointer.");
				isCountDown = false;
			}
		}else{
			// This element is top
			pCKIndex->element_list = pElement->small;
		}

	}else{
		// no children
		// --> only remove this pointer
		if(pParent){
			if(pElement == static_cast<PELEMENT>(Abs(pParent->small))){
				pParent->small = NULL;
			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->big))){
				pParent->big = NULL;
			}else if(pElement == static_cast<PELEMENT>(Abs(pParent->same))){
				// why?
				WAN_K2HPRN("Parent\'s same is this element pointer, it is something wrong.");
				pParent->same = NULL;
			}else{
				// why?
				WAN_K2HPRN("Parent does not have this element pointer.");
				isCountDown = false;
			}
		}else{
			// This element is top
			pCKIndex->element_list = NULL;
		}
	}

	// element links
	pElement->small		= NULL;
	pElement->big		= NULL;
	pElement->parent	= NULL;
	pElement->same		= NULL;

	// Insert element's children into tree
	if(pInsertSmall){
		if(NULL == pCKIndex->element_list){
			// why?
			pCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pInsertSmall));
		}else{
			if(!InsertElement(static_cast<PELEMENT>(Abs(pCKIndex->element_list)), pInsertSmall)){
				ERR_K2HPRN("FATAL: Failed to insert element that is removed element's small element, so elements are leaked!!!");
				return false;
			}
		}
	}
	if(pInsertBig){
		if(NULL == pCKIndex->element_list){
			// why?
			pCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pInsertBig));
		}else{
			if(!InsertElement(static_cast<PELEMENT>(Abs(pCKIndex->element_list)), pInsertBig)){
				ERR_K2HPRN("Failed to insert element that is removed element's big element, so elements are leaked!!!");
				return false;
			}
		}
	}

	// count
	if(isCountDown){
		if(0UL < pCKIndex->element_count){
			pCKIndex->element_count -= 1UL;
		}
		// check & repair
		if(NULL == pCKIndex->element_list && 0UL != pCKIndex->element_count){
			WAN_K2HPRN("Element count in collision Key Index is wrong, so repair it.");
			pCKIndex->element_count = 0UL;
		}else if(NULL != pCKIndex->element_list && 0UL == pCKIndex->element_count){
			WAN_K2HPRN("Element count in collision Key Index is wrong, so repair it.");
			pCKIndex->element_count = K2HShm::GetElementListUpCount(static_cast<PELEMENT>(Abs(pCKIndex->element_list)));
		}
	}
	return true;
}

// [NOTICE]
// MUST pElement take off from CKIndex before calling this method.
//
bool K2HShm::FreeElement(PELEMENT pElement)
{
	// Free pages in this element
	if(!PutBackElementPages(pElement)){
		ERR_K2HPRN("Something error occurred for free pages in element, so elements and pages are leaked!!!");
		return false;
	}
	// Free Element
	if(!PutBackElement(pElement)){
		ERR_K2HPRN("Something error occurred for free element, so elements are leaked!!!");
		return false;
	}
	return true;
}

// [NOTICE]
// ALObjFEC must be RWLOCK or UNLOCK before calling this method
//
bool K2HShm::PutBackElement(PELEMENT pElement)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HLock	ALObjFEC(ShmFd, Rel(&(pHead->free_element_count)), K2HLock::RWLOCK);	// LOCK

	PELEMENT	pOldTopElement	= static_cast<PELEMENT>(Abs(pHead->pfree_elements));
	if(pOldTopElement){
		pOldTopElement->parent	= reinterpret_cast<PELEMENT>(Rel(pElement));
	}
	pElement->small				= NULL;
	pElement->big				= NULL;
	pElement->parent			= NULL;
	pElement->same				= pHead->pfree_elements;
	pElement->hash				= 0UL;
	pElement->subhash			= 0UL;
	pElement->key				= NULL;
	pElement->value				= NULL;
	pElement->subkeys			= NULL;

	pHead->pfree_elements		= reinterpret_cast<PELEMENT>(Rel(pElement));
	pHead->free_element_count	+= 1UL;

	return true;
}

bool K2HShm::PutBackElementPages(PELEMENT pElement)
{
	if(!pElement){
		ERR_K2HPRN("Element is NULL");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HPage*	pPage;
	bool		Result = true;

	// Attrs
	if(NULL != (pPage = GetPage(pElement, PAGEOBJ_ATTRS, false))){
		if(!pPage->Free()){
			ERR_K2HPRN("Failed to free pages for attrs.");
			Result = false;
		}
		K2H_Delete(pPage);
	}
	// SubKeys
	if(NULL != (pPage = GetPage(pElement, PAGEOBJ_SUBKEYS, false))){
		if(!pPage->Free()){
			ERR_K2HPRN("Failed to free pages for subkey.");
			Result = false;
		}
		K2H_Delete(pPage);
	}
	// Value
	if(NULL != (pPage = GetPage(pElement, PAGEOBJ_VALUE, false))){
		if(!pPage->Free()){
			ERR_K2HPRN("Failed to free pages for subkey.");
			Result = false;
		}
		K2H_Delete(pPage);
	}
	// Key
	if(NULL != (pPage = GetPage(pElement, PAGEOBJ_KEY, false))){
		if(!pPage->Free()){
			ERR_K2HPRN("Failed to free pages for subkey.");
			Result = false;
		}
		K2H_Delete(pPage);
	}
	return Result;
}

K2HPage* K2HShm::GetPage(PELEMENT pElement, int type, bool need_load) const
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is null.");
		return NULL;
	}
	return GetPageObject(type == PAGEOBJ_VALUE ? pElement->value : type == PAGEOBJ_SUBKEYS ? pElement->subkeys : type == PAGEOBJ_ATTRS ? pElement->attrs : pElement->key, need_load);
}

// [NOTICE]
// ALObjFPC must be RWLOCK or UNLOCK before calling this method
//
K2HPage* K2HShm::ReservePages(size_t length)
{
	K2HLock	ALObjFPC(ShmFd, Rel(&(pHead->free_page_count)), K2HLock::RWLOCK);		// LOCK

	// Check enough page for length
	while(((GetPageSize() - PAGEHEAD_SIZE) * (pHead->free_page_count)) < length){
		// Need to expand
		if(!ExpandPageArea()){
			ERR_K2HPRN("Could not expand page area");
			return NULL;
		}
		// [NOTICE]
		// checking dead loop...
	}

	// Get page head and tail
	PAGEHEAD		LastPageHead;
	K2HPage*		pLastPage;
	K2HPage*		pStartPage = NULL;
	unsigned long	rpage_count;
	for(pLastPage = GetPageObject(pHead->pfree_pages, false), rpage_count = 1; pLastPage; pLastPage = GetPageObject(LastPageHead.next, false), rpage_count++){
		if(!pLastPage->GetPageHead(&LastPageHead)){
			ERR_K2HPRN("Could not load(get) PAGEHEAD data");
			return NULL;
		}
		if(length <= ((GetPageSize() - PAGEHEAD_SIZE) * rpage_count)){
			if(!pStartPage){
				pStartPage = pLastPage;
			}
			break;
		}
		if(pStartPage){
			K2H_Delete(pLastPage);
		}else{
			pStartPage = pLastPage;
		}
	}

	// set prev/next pointer for reserving/new top page object.
	K2HPage*	pNewFreeTopPage;
	if(NULL != (pNewFreeTopPage = GetPageObject(LastPageHead.next, false))){
		if(!pNewFreeTopPage->SetPageHead(K2HPage::SETHEAD_PREV, NULL)){
			ERR_K2HPRN("Could not set prev pointer for new top page object");
			if(pStartPage != pLastPage){
				K2H_Delete(pStartPage);
			}
			K2H_Delete(pLastPage);
			K2H_Delete(pNewFreeTopPage);
			return NULL;
		}
		K2H_Delete(pNewFreeTopPage);

		pHead->pfree_pages = LastPageHead.next;
		if(rpage_count <= static_cast<unsigned long>(pHead->free_page_count)){
			pHead->free_page_count -= rpage_count;
		}else{
			pHead->free_page_count = 0UL;
		}

		if(!pLastPage->SetPageHead(K2HPage::SETHEAD_NEXT, NULL, NULL)){
			ERR_K2HPRN("FATAL: Could not set next pointer for reserving page object, this case can not recover, so pages area is leaked!!!!");
			if(pStartPage != pLastPage){
				K2H_Delete(pStartPage);
			}
			K2H_Delete(pLastPage);
			return NULL;
		}
	}else{
		MSG_K2HPRN("Maybe after this operation, there is not the new top page.");
		pHead->pfree_pages		= NULL;
		pHead->free_page_count	= 0UL;
	}

	if(pStartPage != pLastPage){
		K2H_Delete(pLastPage);
	}
	return pStartPage;
}

bool K2HShm::ReservePages(const unsigned char* byData, size_t length, K2HPage** ppPage)
{
	if((!byData && 0UL != length) || (byData && 0UL == length) || !ppPage){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	*ppPage = NULL;

	if(0UL < length){
		if(NULL == (*ppPage = ReservePages(length))){
			ERR_K2HPRN("Could not reserve pages.");
			return false;
		}
		if(!(*ppPage)->SetData(byData, length)){
			ERR_K2HPRN("Failed to set page data.");
			K2H_Delete(*ppPage);
			return false;
		}
	}
	return true;
}

//---------------------------------------------------------
// Get Methods
//---------------------------------------------------------
char* K2HShm::Get(const char* pKey, bool checkattr, const char* encpass) const
{
	char*	pValue = NULL;
	if(0 > Get(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), reinterpret_cast<unsigned char**>(&pValue), checkattr, encpass)){
		return NULL;
	}
	return pValue;
}

strarr_t::size_type K2HShm::Get(const char* pKey, strarr_t& strarr, bool checkattr, const char* encpass) const
{
	return Get(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL, strarr, checkattr, encpass);
}

strarr_t::size_type K2HShm::Get(const unsigned char* byKey, size_t length, strarr_t& strarr, bool checkattr, const char* encpass) const
{
	unsigned char*	byValue = NULL;
	ssize_t			vallen;
	if(0 > (vallen = Get(byKey, length, &byValue, checkattr, encpass))){
		return -1;
	}
	strarr_t::size_type	lcnt = ::ParseStringArray(reinterpret_cast<const char*>(byValue), static_cast<size_t>(vallen), strarr);
	K2H_Free(byValue);

	return lcnt;
}

strarr_t::size_type K2HShm::Get(PELEMENT pElement, strarr_t& strarr, bool checkattr, const char* encpass) const
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return -1;
	}
	// reverse element to key
	ssize_t			keylen;
	unsigned char*	byKey = NULL;
	if(0 > (keylen = Get(pElement, &byKey, PAGEOBJ_KEY))){
		MSG_K2HPRN("Could not get key from element.");
		return -1;
	}
	strarr_t::size_type	lcnt = Get(byKey, static_cast<size_t>(keylen), strarr, checkattr, encpass);
	K2H_Free(byKey);

	return lcnt;
}

ssize_t K2HShm::Get(const unsigned char* byKey, size_t keylen, unsigned char** byValue, k2h_get_trial_callback fp, void* pExtData, bool checkattr, const char* encpass)
{
	if(!byKey || 0 == keylen || !byValue || !fp){
		ERR_K2HPRN("Parameters are wrong.");
		return -1;
	}

	// get value
	unsigned char*	byNowValue	= NULL;
	ssize_t			nowvallen	= 0;
	if(0 > (nowvallen = Get(byKey, keylen, &byNowValue, checkattr, encpass))){
		return nowvallen;
	}

	// get attribute as structure array
	PK2HATTRPCK		pattrspck	= NULL;
	int				attrspckcnt	= 0;
	k2h_get_attrs(reinterpret_cast<k2h_h>(this), byKey, keylen, &pattrspck, &attrspckcnt);

	// Call callback function and check value.
	unsigned char*	byNewValue	= NULL;
	size_t			newvallen	= 0;
	K2HGETCBRES		cbres		= fp(byKey, keylen, byNowValue, static_cast<size_t>(nowvallen), &byNewValue, &newvallen, pattrspck, attrspckcnt, pExtData);

	// free attributes
	k2h_free_attrpack(pattrspck, attrspckcnt);

	if(K2HGETCB_RES_OVERWRITE == cbres){
		K2H_Free(byNowValue);

		// Need to over write new value
		if(!Set(byKey, keylen, byNewValue, newvallen, encpass)){
			ERR_K2HPRN("Failed to over write value.");
			K2H_Free(byNewValue);
			return -1;
		}
		byNowValue	= byNewValue;
		nowvallen	= static_cast<ssize_t>(newvallen);

	}else if(K2HGETCB_RES_NOTHING == cbres){
		// Nothing to do
		K2H_Free(byNewValue);

	}else{	// K2HGETCB_RES_ERROR == cbres
		ERR_K2HPRN("Something error is occurred in callback function.");
		K2H_Free(byNewValue);
		K2H_Free(byNowValue);
		return -1;
	}

	// result
	*byValue = byNowValue;
	return static_cast<ssize_t>(nowvallen);
}

ssize_t K2HShm::Get(const unsigned char* byKey, size_t length, unsigned char** byValue, bool checkattr, const char* encpass) const
{
	K2HLock		ALObjCKI(K2HLock::RDLOCK);
	PELEMENT	pElement;

	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	if(NULL == (pElement = GetElement(byKey, length, ALObjCKI))){
		MSG_K2HPRN("Key(%s) is not found", reinterpret_cast<const char*>(byKey));
		return -1;
	}

	// at first, check attributes
	K2HAttrs*	pAttrs		= NULL;
	bool		IsEncrypted	= false;
	if(checkattr){
		if(NULL != (pAttrs = GetAttrs(pElement))){
			// check only expire and history marker.
			K2hAttrOpsMan	attrman;
			if(!attrman.Initialize(this, byKey, length, NULL, 0UL, NULL)){
				ERR_K2HPRN("Something error occurred during initializing attributes manager class.");
				K2H_Delete(pAttrs);
				return -1;
			}
			// check expire
			if(attrman.IsExpire(*pAttrs)){
				MSG_K2HPRN("the key is expired.");
				K2H_Delete(pAttrs);
				return -1;
			}
			// check history marker
			if(attrman.IsHistory(*pAttrs)){
				MSG_K2HPRN("the key is marked history.");
				K2H_Delete(pAttrs);
				return -1;
			}
			// check encrypted value
			if(attrman.IsValueEncrypted(*pAttrs)){
				IsEncrypted = true;
			}
		}
	}

	// get value
	ssize_t	vallen = Get(pElement, byValue, PAGEOBJ_VALUE);

	// decrypt
	if(0 < vallen && pAttrs && IsEncrypted){
		K2hAttrOpsMan	attrman;
		if(!attrman.Initialize(this, byKey, length, *byValue, static_cast<size_t>(vallen), encpass)){
			ERR_K2HPRN("Something error occurred during initializing attributes manager class.");
			K2H_Delete(pAttrs);
			K2H_Free(*byValue);
			return -1;
		}

		// try to decrypt
		unsigned char*	pDecryptValue;
		size_t			DecryptLength = 0;
		if(NULL == (pDecryptValue = attrman.GetDecryptValue(*pAttrs, encpass, DecryptLength))){
			ERR_K2HPRN("Something error occurred during decrypting value.");
			K2H_Delete(pAttrs);
			K2H_Free(*byValue);
			return -1;
		}
		K2H_Free(*byValue);

		// copy decrypt data
		if(NULL == (*byValue = reinterpret_cast<unsigned char*>(malloc(DecryptLength)))){
			MSG_K2HPRN("Could not allocate memory.");
			K2H_Delete(pAttrs);
			return -1;
		}
		memcpy(*byValue, pDecryptValue, DecryptLength);
		vallen = static_cast<ssize_t>(DecryptLength);
	}
	K2H_Delete(pAttrs);

	return vallen;
}

char* K2HShm::Get(PELEMENT pElement, int type) const
{
	unsigned char*	byData = NULL;
	if(-1 == Get(pElement, &byData, type) || !byData){
		ERR_K2HPRN("Could not get page data as type(%d) from element.", type);
		return NULL;
	}
	return reinterpret_cast<char*>(byData);
}

ssize_t K2HShm::Get(PELEMENT pElement, unsigned char** byData, int type) const
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return -1;
	}
	K2HPage*	pPage;
	if(NULL == (pPage = GetPage(pElement, type))){
		MSG_K2HPRN("Could not get page object from element for type(%d).", type);
		return -1;
	}
	ssize_t	Length	= -1;
	*byData = NULL;
	if(!pPage->CopyData(byData, reinterpret_cast<size_t*>(&Length)) || 0 >= Length){
		ERR_K2HPRN("Could not get page data as type(%d) from element.", type);
		K2H_Delete(pPage);
		K2H_Free(*byData);
		return -1;
	}
	K2H_Delete(pPage);

	return Length;
}

K2HSubKeys* K2HShm::GetSubKeys(const char* pKey, bool checkattr) const
{
	return GetSubKeys(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL, checkattr);
}

K2HSubKeys* K2HShm::GetSubKeys(const unsigned char* byKey, size_t length, bool checkattr) const
{
	K2HLock		ALObjCKI(K2HLock::RDLOCK);
	PELEMENT	pElement;

	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	if(NULL == (pElement = GetElement(byKey, length, ALObjCKI))){
		MSG_K2HPRN("Key(%s) is not found", reinterpret_cast<const char*>(byKey));
		return NULL;
	}
	return GetSubKeys(pElement, checkattr);
}

K2HSubKeys* K2HShm::GetSubKeys(PELEMENT pElement, bool checkattr) const
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return NULL;
	}
	if(checkattr){
		// [Note]
		K2HAttrs*	pAttrs;
		if(NULL != (pAttrs = GetAttrs(pElement))){
			// [NOTE]
			// We need only to check expire flag, then we do not need to fill all parameters for initialize().
			//
			K2hAttrOpsMan	attrman;
			if(!attrman.Initialize(this, NULL, 0UL, NULL, 0)){
				ERR_K2HPRN("Something error occurred during initializing attributes manager class.");
				K2H_Delete(pAttrs);
				return NULL;
			}
			// check expire
			if(attrman.IsExpire(*pAttrs)){
				MSG_K2HPRN("the key is expired.");
				K2H_Delete(pAttrs);
				return NULL;
			}
			// check history marker
			if(attrman.IsHistory(*pAttrs)){
				MSG_K2HPRN("the key is marked history.");
				K2H_Delete(pAttrs);
				return NULL;
			}
			K2H_Delete(pAttrs);

		}else{
			// key does not have attribute
		}
	}
	K2HPage*	pPage;
	if(NULL == (pPage = GetPage(pElement, PAGEOBJ_SUBKEYS))){
		MSG_K2HPRN("Could not get subkeys from element.");
		return NULL;
	}
	K2HSubKeys*	pSubKeys = pPage->GetSubKeys();
	K2H_Delete(pPage);
	return pSubKeys;
}

K2HAttrs* K2HShm::GetAttrs(const char* pKey) const
{
	return GetAttrs(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL);
}

K2HAttrs* K2HShm::GetAttrs(const unsigned char* byKey, size_t length) const
{
	K2HLock		ALObjCKI(K2HLock::RDLOCK);
	PELEMENT	pElement;

	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	if(NULL == (pElement = GetElement(byKey, length, ALObjCKI))){
		MSG_K2HPRN("Key(%s) is not found", reinterpret_cast<const char*>(byKey));
		return NULL;
	}
	return GetAttrs(pElement);
}

K2HAttrs* K2HShm::GetAttrs(PELEMENT pElement) const
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return NULL;
	}
	K2HPage*	pPage;
	if(NULL == (pPage = GetPage(pElement, PAGEOBJ_ATTRS))){
		MSG_K2HPRN("Could not get attributes from element.");
		return NULL;
	}
	K2HAttrs*	pAttrs = pPage->GetAttrs();
	K2H_Delete(pPage);
	return pAttrs;
}

//---------------------------------------------------------
// Set/Add Methods
//---------------------------------------------------------
//
// Set - overwrite any data.
//
bool K2HShm::Set(const char* pKey, const char* pValue, const char* encpass, const time_t* expire)
{
	return Set(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), (pValue ? reinterpret_cast<const unsigned char*>(pValue) : NULL), (pValue ? (strlen(pValue) + 1) : 0UL), encpass, expire);
}

bool K2HShm::Set(const char* pKey, const char* pValue, K2HSubKeys* pSubKeys, bool isRemoveSubKeys, const char* encpass, const time_t* expire)
{
	K2HAttrs*	pAttrs	= GetAttrs(pKey);
	bool		result	= Set(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), (pValue ? reinterpret_cast<const unsigned char*>(pValue) : NULL), (pValue ? (strlen(pValue) + 1) : 0UL), pSubKeys, isRemoveSubKeys, pAttrs, encpass, expire);

	K2H_Delete(pAttrs);
	return result;
}

bool K2HShm::Set(const char* pKey, const char* pValue, K2HAttrs* pAttrs, const char* encpass, const time_t* expire)
{
	K2HSubKeys*	pSubKeys= GetSubKeys(pKey);
	bool		result	= Set(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), (pValue ? reinterpret_cast<const unsigned char*>(pValue) : NULL), (pValue ? (strlen(pValue) + 1) : 0UL), pSubKeys, false, pAttrs, encpass, expire);

	K2H_Delete(pSubKeys);
	return result;
}

bool K2HShm::Set(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const char* encpass, const time_t* expire)
{
	K2HSubKeys*	pSubKeys= GetSubKeys(byKey, keylength);
	K2HAttrs*	pAttrs	= GetAttrs(byKey, keylength);
	bool		result	= Set(byKey, keylength, byValue, vallength, pSubKeys, false, pAttrs, encpass, expire);

	K2H_Delete(pSubKeys);
	K2H_Delete(pAttrs);
	return result;
}

// [NOTE]
// For Queue and KeyQueue, this method has attrtype parameter.
// The attrtype parameter controls attribute mask. Queue's and KeyQueue's marker and KeyQueue's key do not need to
// set any attribute, and Queue's key do not need to set history. This parameter controls attribute for these case.
//
bool K2HShm::Set(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, K2HSubKeys* pSubKeys, bool isRemoveSubKeys, K2HAttrs* pAttrs, const char* encpass, const time_t* expire, K2hAttrOpsMan::ATTRINITTYPE attrtype)
{
	if(!byKey || 0 == keylength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// [NOTE]
	// We remove key at first, and free lock to reget lock.
	// Then there is a possibility that the same key may be created by another process
	// in a very short period of time after deleting the key and creating a new key.
	// In the case of this conflict, redo from the key deletion.
	//
	k2htransobjlist_t	translist;
	bool				is_check_updated = true;
	while(true){
		// remove old key if existed.(we need uniqid before make attr binary data)
		//
		// [NOTE]
		// "K2HFILE_UPDATE_CHECK(this)" is called in Remove() method.
		// Thus we do not need call it here. 
		//
		K2HSubKeys*		pRmSubKeys = NULL;
		string			parent_uid;
		{
			char*	pUniqId = NULL;
			if(!RemoveEx(byKey, keylength, isRemoveSubKeys, pRmSubKeys, &pUniqId, false, &translist, is_check_updated)){
				ERR_K2HPRN("Could not remove(or rename for history) key.");
				return false;
			}
			if(pUniqId){
				parent_uid = pUniqId;
				// cppcheck-suppress unmatchedSuppression
				// cppcheck-suppress identicalInnerCondition
				K2H_Free(pUniqId);
			}
		}

		// make subkeys to buffer
		unsigned char*	bySubKeys = NULL;			// Do not change this value for transaction in this function end.
		size_t			sublength = 0UL;			// Do not change this value for transaction in this function end.
		if(isRemoveSubKeys && pRmSubKeys && pSubKeys){
			// If new subkeys list has subkey which is listed for removing,
			// that subkey retrieve from removing list.
			for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); ++iter){
				if(pRmSubKeys->end() != pRmSubKeys->find(iter->pSubKey, iter->length)){
					pRmSubKeys->erase(iter->pSubKey, iter->length);
				}
			}
			if(0 == pRmSubKeys->size()){
				K2H_Delete(pRmSubKeys);
			}
		}
		if(pSubKeys && !pSubKeys->Serialize(&bySubKeys, sublength)){
			ERR_K2HPRN("Could not set subkeys binary data from subkeys object.");
			K2H_Delete(pRmSubKeys);
			return false;
		}

		// make hash
		k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylength);
		k2h_hash_t	subhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylength);

		// Lock CKIndex before remove key
		//
		// For attribute, we have to remove existed key for making history key name.
		// Thus we must lock during the period from removing key to remaking it.
		//
		K2HLock		ALObjCKI(K2HLock::RWLOCK);		// LOCK
		PCKINDEX	pCKIndex;
		if(NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI))){
			ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
			K2H_Free(bySubKeys);
			K2H_Delete(pRmSubKeys);
			return false;
		}else{
			// check removed key existing.
			PELEMENT	pElementList;
			if(NULL != (pElementList = GetElementList(pCKIndex, hash, subhash)) && NULL != GetElement(pElementList, byKey, keylength)){
				// [NOTE]
				// found removed key which probably create for unlocking time.
				// thus retry.
				// Notice about translist is kept.
				//
				MSG_K2HPRN("Created same key during removing it to locking.");
				ALObjCKI.Unlock();					// Unlock
				parent_uid.clear();
				K2H_Delete(pRmSubKeys);
				K2H_Free(bySubKeys);
				sublength = 0UL;
				continue;
			}
		}

		// make attribute
		K2hAttrOpsMan	attrman;					// Do not destroy this object until set value to page.
		unsigned char*	byAttrs		= NULL;			// Do not change this value for transaction in this function end.
		size_t			attrlength	= 0UL;			// Do not change this value for transaction in this function end.
		{
			K2HAttrs		attrs;
			if(!pAttrs){
				pAttrs = &attrs;
			}
			if(!attrman.Initialize(this, byKey, keylength, byValue, vallength, encpass, expire, attrtype)){
				ERR_K2HPRN("Could not initialize attribute manager object.");
				K2H_Free(bySubKeys);
				K2H_Delete(pRmSubKeys);
				return false;
			}
			if(!attrman.UpdateAttr(*pAttrs)){
				ERR_K2HPRN("Failed to update attributes.");
				K2H_Free(bySubKeys);
				K2H_Delete(pRmSubKeys);
				return false;
			}

			const char*	pOldUid = attrman.IsUpdateUniqID();
			if(ISEMPTYSTR(pOldUid)){
				if(!parent_uid.empty()){
					// [NOTE]
					// there is no parent uniqid in attrs after updating, but we have it after removing.
					// so force to set parent uniqid and remake uniqid.
					//
					if(!attrman.DirectSetUniqId(*pAttrs, parent_uid.c_str())){
						ERR_K2HPRN("Could not update uniqid and set old uniqid to attrs object.");
						K2H_Free(bySubKeys);
						K2H_Delete(pRmSubKeys);
						return false;
					}
				}else{
					// [NOTE]
					// If history is OFF, both parent_uid and pOldUid are empty.
					// Thus come here, and nothing to do.
				}
			}else{
				if(parent_uid != pOldUid){
					// [NOTE]
					// there is parent uniqid in attrs after updating, but that uniqid is different from the uniqid after removing.
					// so force to set parent uniqid and remake uniqid.
					//
					if(!attrman.DirectSetUniqId(*pAttrs, parent_uid.c_str())){
						ERR_K2HPRN("Could not update uniqid and set old uniqid to attrs object.");
						K2H_Free(bySubKeys);
						K2H_Delete(pRmSubKeys);
						return false;
					}
				}else{
					// parent uniqid is same, so nothing to do because of already setting it in attrs.
				}
			}

			if(!pAttrs->Serialize(&byAttrs, attrlength)){
				ERR_K2HPRN("Could not set binary array from attribute list.");
				K2H_Free(bySubKeys);
				K2H_Delete(pRmSubKeys);
				return false;
			}
			byValue = attrman.GetValue(vallength);		// get (new) value through attribute manager.
		}

		// make new element
		PELEMENT	pNewElement;
		if(NULL == (pNewElement = AllocateElement(hash, subhash, byKey, keylength, byValue, vallength, bySubKeys, sublength, byAttrs, attrlength))){
			ERR_K2HPRN("Failed to allocate new element and to set datas to it.");
			K2H_Free(bySubKeys);
			K2H_Free(byAttrs);
			K2H_Delete(pRmSubKeys);
			return false;
		}

		// Insert new element
		if(pCKIndex->element_list){
			if(!InsertElement(static_cast<PELEMENT>(Abs(pCKIndex->element_list)), pNewElement)){
				ERR_K2HPRN("Failed to insert element");
				K2H_Free(bySubKeys);
				K2H_Free(byAttrs);
				FreeElement(pNewElement);
				K2H_Delete(pRmSubKeys);
				return false;
			}
		}else{
			pCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pNewElement));
		}
		pCKIndex->element_count	+= 1UL;

		ALObjCKI.Unlock();								// Unlock

		// transaction for setting new key
		K2HTransaction*	ptransobj = new K2HTransaction(this, true);		// stacking mode
		if(ptransobj->IsEnable()){
			if(!ptransobj->SetAll(byKey, keylength, byValue, vallength, bySubKeys, sublength, byAttrs, attrlength)){
				WAN_K2HPRN("Failed to put setting transaction.");
				K2H_Delete(ptransobj);
			}else{
				translist.push_back(ptransobj);							// stacked
			}
		}else{
			K2H_Delete(ptransobj);
		}
		K2H_Free(bySubKeys);
		K2H_Free(byAttrs);

		// remove subkeys and put transaction
		if(!RemoveSubkeys(pRmSubKeys, &translist, is_check_updated)){
			WAN_K2HPRN("Failed to remove subkeys or put transaction for removing subkeys.");
		}
		K2H_Delete(pRmSubKeys);

		if(!UpdateTimeval()){
			WAN_K2HPRN("Failed to update timeval for data update.");
		}

		// check element count in ckey for increasing cur_mask(expanding key/ckey area)
		if(!CheckExpandingKeyArea(pCKIndex)){					// Do not care for locking
			ERR_K2HPRN("Something error occurred by checking/expanding key/ckey area.");
			return false;
		}
		break;
	}
	return true;
}

//
// Make new element from free element list and set all data to it.
//
PELEMENT K2HShm::AllocateElement(k2h_hash_t hash, k2h_hash_t subhash, const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const unsigned char* bySubKeys, size_t sublength, const unsigned char* byAttrs, size_t attrlength)
{
	if(!byKey || 0 == keylength){
		ERR_K2HPRN("Some parameters are wrong.");
		return NULL;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}

	// make new element
	PELEMENT	pNewElement;
	if(NULL == (pNewElement = ReserveElement())){
		ERR_K2HPRN("Failed to get free element.");
		return NULL;
	}

	// make Page object
	K2HPage*	pKeyPage = NULL;
	K2HPage*	pValPage = NULL;
	K2HPage*	pSubPage = NULL;
	K2HPage*	pAttrPage= NULL;
	if(	!ReservePages(byKey, keylength, &pKeyPage)		||
		!ReservePages(byValue, vallength, &pValPage)	||
		!ReservePages(bySubKeys, sublength, &pSubPage)	||
		!ReservePages(byAttrs, attrlength, &pAttrPage)	)
	{
		if(pKeyPage && !pKeyPage->Free()){
			ERR_K2HPRN("FATAL: In error recovery logic, failed to free pages.");
		}
		if(pValPage && !pValPage->Free()){
			ERR_K2HPRN("FATAL: In error recovery logic, failed to free pages.");
		}
		if(pSubPage && !pSubPage->Free()){
			ERR_K2HPRN("FATAL: In error recovery logic, failed to free pages.");
		}
		if(pAttrPage && !pAttrPage->Free()){
			ERR_K2HPRN("FATAL: In error recovery logic, failed to free pages.");
		}
		K2H_Delete(pKeyPage);
		K2H_Delete(pValPage);
		K2H_Delete(pSubPage);
		K2H_Delete(pAttrPage);
		PutBackElement(pNewElement);

		ERR_K2HPRN("Failed to set page data for key/value/subkeys/attrs.");
		return NULL;
	}

	// set data into element
	pNewElement->hash		= hash;
	pNewElement->subhash	= subhash;
	pNewElement->key		= pKeyPage ? pKeyPage->GetPageHeadRelAddress() : NULL;
	pNewElement->value		= pValPage ? pValPage->GetPageHeadRelAddress() : NULL;
	pNewElement->subkeys	= pSubPage ? pSubPage->GetPageHeadRelAddress() : NULL;
	pNewElement->attrs		= pAttrPage ? pAttrPage->GetPageHeadRelAddress() : NULL;
	pNewElement->keylength	= pKeyPage ? keylength : 0UL;
	pNewElement->vallength	= pValPage ? vallength : 0UL;
	pNewElement->skeylength	= pSubPage ? sublength : 0UL;
	pNewElement->attrlength	= pAttrPage ? attrlength : 0UL;

	K2H_Delete(pKeyPage);
	K2H_Delete(pValPage);
	K2H_Delete(pSubPage);
	K2H_Delete(pAttrPage);

	return pNewElement;
}

bool K2HShm::AddSubkey(const char* pKey, const char* pSubkey, const char* pValue, const char* encpass, const time_t* expire)
{
	return AddSubkey(pKey, pSubkey, reinterpret_cast<const unsigned char*>(pValue), (pValue ? strlen(pValue) + 1 : 0UL), encpass, expire);
}

bool K2HShm::AddSubkey(const char* pKey, const char* pSubkey, const unsigned char* byValue, size_t vallength, const char* encpass, const time_t* expire)
{
	return AddSubkey(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), reinterpret_cast<const unsigned char*>(pSubkey), (pSubkey ? strlen(pSubkey) + 1 : 0UL), byValue, vallength, encpass, expire);
}

bool K2HShm::AddSubkey(const unsigned char* byKey, size_t keylength, const unsigned char* bySubkey, size_t skeylength, const unsigned char* byValue, size_t vallength, const char* encpass, const time_t* expire)
{
	K2HFILE_UPDATE_CHECK(this);

	// make subkey
	if(!Set(bySubkey, skeylength, byValue, vallength, NULL, false, NULL, encpass, expire)){
		ERR_K2HPRN("Could not set subkey(%s) value.", reinterpret_cast<const char*>(bySubkey));
		return false;
	}

	// get parent subkeys
	K2HSubKeys	k2hsubkeys;
	K2HSubKeys*	pk2hsubkeys;
	bool		isDelete = true;
	if(NULL == (pk2hsubkeys = GetSubKeys(byKey, keylength))){
		pk2hsubkeys	= &k2hsubkeys;
		isDelete	= false;
	}

	// prepare parent's new subkeys
	unsigned char*	bySubkeys	= NULL;
	size_t			skeyslength	= 0UL;
	pk2hsubkeys->insert(bySubkey, skeylength);
	if(!pk2hsubkeys->Serialize(&bySubkeys, skeyslength)){
		ERR_K2HPRN("Could not build subkeys for parent.");
		if(isDelete){
			K2H_Delete(pk2hsubkeys);
		}
		return false;
	}

	// update parent subkeys.
	bool result;
	if(false == (result = ReplaceSubkeys(byKey, keylength, bySubkeys, skeyslength))){
		ERR_K2HPRN("Could not replace subkeys in parent.");
	}
	if(isDelete){
		K2H_Delete(pk2hsubkeys);
	}
	K2H_Free(bySubkeys);

	return result;
}

bool K2HShm::AddAttr(const char* pKey, const char* pattrkey, const char* pattrval)
{
	return AddAttr(pKey, reinterpret_cast<const unsigned char*>(pattrkey), pattrkey ? strlen(pattrkey) + 1 : 0UL, reinterpret_cast<const unsigned char*>(pattrval), pattrval ? strlen(pattrval) + 1 : 0UL);
}

bool K2HShm::AddAttr(const char* pKey, const unsigned char* pattrkey, size_t attrkeylen, const unsigned char* pattrval, size_t attrvallen)
{
	return AddAttr(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), pattrkey, attrkeylen, pattrval, attrvallen);
}

bool K2HShm::AddAttr(const unsigned char* byKey, size_t keylength, const unsigned char* pattrkey, size_t attrkeylen, const unsigned char* pattrval, size_t attrvallen)
{
	if(!pattrkey || 0 == attrkeylen){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// get current attrs
	K2HAttrs	Attrs;
	K2HAttrs*	pAttrs;
	bool		isDelete = true;
	if(NULL == (pAttrs = GetAttrs(byKey, keylength))){
		pAttrs		= &Attrs;
		isDelete	= false;
	}

	// add attr
	pAttrs->insert(pattrkey, attrkeylen, pattrval, attrvallen);

	// make binary data
	unsigned char*	byAttrs		= NULL;
	size_t			attrlength	= 0UL;
	if(!pAttrs->Serialize(&byAttrs, attrlength)){
		ERR_K2HPRN("Could not make attribute binary data from object.");
		if(isDelete){
			K2H_Delete(pAttrs);
		}
		return false;
	}

	// update attrs.
	bool result;
	if(false == (result = ReplaceAttrs(byKey, keylength, byAttrs, attrlength))){
		ERR_K2HPRN("Could not replace attributes.");
	}
	if(isDelete){
		K2H_Delete(pAttrs);
	}
	K2H_Free(byAttrs);

	return result;
}

//---------------------------------------------------------
// Remove Methods
//---------------------------------------------------------
bool K2HShm::Remove(const char* pKey, bool isSubKeys)
{
	return Remove(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL), isSubKeys);
}

bool K2HShm::Remove(const unsigned char* byKey, size_t keylength, bool isSubKeys)
{
	return Remove(byKey, keylength, isSubKeys, NULL, false);
}

//
// [NOTE]
// hismask parameter is specified, this method does not make history for removing key.
//
bool K2HShm::Remove(const unsigned char* byKey, size_t keylength, bool isSubKeys, char** ppUniqid, bool hismask)
{
	// remove keys
	k2htransobjlist_t	toptranslist;
	K2HSubKeys*			pSubKeys		= NULL;
	bool				is_check_updated = false;
	if(!RemoveEx(byKey, keylength, isSubKeys, pSubKeys, ppUniqid, hismask, &toptranslist, is_check_updated)){
		return false;
	}
	// remove subkeys and put transaction
	bool	result = RemoveSubkeys(pSubKeys, &toptranslist, is_check_updated);

	K2H_Delete(pSubKeys);
	toptranslist.clear();

	return result;
}

bool K2HShm::Remove(const char* pKey, const char* pSubKey)
{
	return Remove(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL, reinterpret_cast<const unsigned char*>(pSubKey), pSubKey ? strlen(pSubKey) + 1 : 0UL);
}

bool K2HShm::Remove(const unsigned char* byKey, size_t keylength, const unsigned char* bySubKey, size_t sklength)
{
	if(!byKey || 0UL == keylength || !bySubKey || 0UL == sklength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);
	bool	is_check_updated = true;

	// make hash
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylength);
	k2h_hash_t	subhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylength);

	// get element
	K2HLock		ALObjCKI(K2HLock::RWLOCK);					// LOCK
	PCKINDEX	pCKIndex;
	PELEMENT	pElementList;
	PELEMENT	pElement;
	if(	NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI)) ||
		NULL == (pElementList = GetElementList(pCKIndex, hash, subhash)) ||
		NULL == (pElement = GetElement(pElementList, byKey, keylength)) )
	{
		// Not found
		MSG_K2HPRN("Not found Key in k2hash.");
		return true;
	}
	return RemoveEx(pElement, bySubKey, sklength, ALObjCKI, is_check_updated);
}

bool K2HShm::RemoveEx(PELEMENT pElement, const unsigned char* bySubKey, size_t length, K2HLock& ALObjCKI, bool& is_check_updated)
{
	if(!pElement || !bySubKey || 0L == length){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!is_check_updated){
		K2HFILE_UPDATE_CHECK(this);
		is_check_updated = true;
	}

	// get subkey page/array
	K2HPage*	pSKeyPage = NULL;
	K2HSubKeys*	pSubKeys;
	if(!pElement->subkeys || NULL == (pSKeyPage = GetPageObject(pElement->subkeys)) || NULL == (pSubKeys = pSKeyPage->GetSubKeys())){
		WAN_K2HPRN("Element does not have subkeys.");
		K2H_Delete(pSKeyPage);
		return true;
	}
	K2H_Delete(pSKeyPage);

	// search subkey and remove it
	if(!pSubKeys->erase(bySubKey, length)){
		WAN_K2HPRN("Element does not have subkeys.");
		K2H_Delete(pSubKeys);
		return true;
	}

	// Serialize
	unsigned char*	bySubkeys = NULL;
	size_t			SKLength = 0UL;
	if(!pSubKeys->Serialize(&bySubkeys, SKLength)){
		ERR_K2HPRN("Failed serializing subkeys.");
		K2H_Delete(pSubKeys);
		return false;
	}
	K2H_Delete(pSubKeys);		// NOTE : pSubKeys = NULL;

	// Replace
	if(!ReplacePage(pElement, bySubkeys, SKLength, PAGEOBJ_SUBKEYS)){
		ERR_K2HPRN("Could not replace subkeys.");
		K2H_Free(bySubkeys);
		return false;
	}
	K2H_Free(bySubkeys);

	// Unlock
	if(ALObjCKI.IsLocked()){
		ALObjCKI.Unlock();
	}

	// Remove subkey
	k2htransobjlist_t	translist;
	if(!RemoveEx(bySubKey, length, true, pSubKeys, NULL, false, &translist, is_check_updated)){
		ERR_K2HPRN("Failed to remove subkey");
		return false;
	}

	// Remove subkeys in subkey's list and put transaction
	bool	result = RemoveSubkeys(pSubKeys, &translist, is_check_updated);
	K2H_Delete(pSubKeys);

	return result;
}

bool K2HShm::RemoveEx(PELEMENT pElement, k2htransobjlist_t* ptranslist, bool& is_check_updated)
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!is_check_updated){
		K2HFILE_UPDATE_CHECK(this);
		is_check_updated = true;
	}

	// for transaction
	K2HTransaction*			ptransobj	= new K2HTransaction(this, (NULL != ptranslist));
	K2HPage*				pPage		= NULL;
	const unsigned char*	byKey		= NULL;
	size_t					keylength	= 0UL;
	if(ptransobj->IsEnable()){
		if(NULL == (pPage = GetPage(pElement, PAGEOBJ_KEY)) || !pPage->GetData(&byKey, &keylength)){
			WAN_K2HPRN("Failed to get key & length for removing transaction.");
		}
		// [NOTICE]
		// Do not access pPage object after FreeElement.
		//
	}

	// remove element
	if(!FreeElement(pElement)){
		ERR_K2HPRN("Failed to free element.");
		return false;
	}

	// transaction
	if(ptransobj->IsEnable()){
		if(byKey){
			// [TODO]
			// At first, checking transaction enable for cost of getting key value now.
			// If can, calling transaction before converting key value to element pointer.
			//
			if(!ptransobj->DelKey(byKey, keylength)){
				WAN_K2HPRN("Failed to put removing transaction.");
			}else{
				if(ptranslist){
					ptranslist->push_back(ptransobj);								// stacking
					ptransobj = NULL;
				}
			}
		}
		K2H_Delete(pPage);
	}
	K2H_Delete(ptransobj);

	// update time
	if(!UpdateTimeval()){
		WAN_K2HPRN("Failed to update timeval for data update.");
	}
	return true;
}

//
// [NOTE]
// Remove key and subkeys, this method runs reentrant.
//
bool K2HShm::RemoveEx(const unsigned char* byKey, size_t keylength, k2htransobjlist_t* ptranslist, bool& is_check_updated)
{
	if(!byKey || 0L == keylength){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!is_check_updated){
		K2HFILE_UPDATE_CHECK(this);
		is_check_updated = true;
	}

	// transaction stack if no stack
	k2htransobjlist_t	toptranslist;
	if(!ptranslist){
		// this is top level
		ptranslist = &toptranslist;
	}

	// Remove top key
	K2HSubKeys*		pSubKeys = NULL;
	if(!RemoveEx(byKey, keylength, true, pSubKeys, NULL, false, ptranslist, is_check_updated)){
		ERR_K2HPRN("Failed to remove key");
		return false;
	}

	// Remove subkeys
	if(pSubKeys){
		for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); iter = pSubKeys->erase(iter)){
			// reentrant
			if(!RemoveEx(iter->pSubKey, iter->length, ptranslist, is_check_updated)){
				WAN_K2HPRN("Failed to remove one subkey from subkey list, but continue...");
			}
		}
		K2H_Delete(pSubKeys);
	}

	// Put transaction
	if(0 < toptranslist.size()){
		for(k2htransobjlist_t::iterator iter = toptranslist.begin(); toptranslist.end() != iter; ++iter){
			K2HTransaction*	ptransobj = *iter;
			if(!ptransobj->Put()){
				WAN_K2HPRN("Failed to put one transaction data in stacking, but continue...");
			}
			K2H_Delete(ptransobj);
		}
		toptranslist.clear();
	}
	return true;
}

bool K2HShm::RemoveEx(const unsigned char* byKey, size_t keylength, bool isSubKeys, K2HSubKeys*& pSubKeys, char** ppUniqid, bool hismask, k2htransobjlist_t* ptranslist, bool& is_check_updated)
{
	if(!byKey || 0 == keylength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!is_check_updated){
		K2HFILE_UPDATE_CHECK(this);
		is_check_updated = true;
	}

	// make hash
	k2h_hash_t	hash	= K2H_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylength);
	k2h_hash_t	subhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylength);

	// get element
	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pCKIndex;
	PELEMENT	pElementList;
	PELEMENT	pElement;
	if(	NULL == (pCKIndex = GetCKIndex(hash, ALObjCKI)) ||
		NULL == (pElementList = GetElementList(pCKIndex, hash, subhash)) ||
		NULL == (pElement = GetElement(pElementList, byKey, keylength)) )
	{
		// Not found
		MSG_K2HPRN("Not found Key in k2hash.");
		return true;
	}

	// get(keep) subkeys if needs
	if(isSubKeys){
		if(!GetSubKeys(pElement, pSubKeys)){
			MSG_K2HPRN("Failed to get subkeys, but continue...");
		}
	}

	// remove or make history
	bool	result;
	if(!hismask && K2hAttrOpsMan::IsMarkHistory(this)){
		// make history
		ALObjCKI.Unlock();							// Unlock

		// rename key
		string	uniqid;
		result = RenameForHistory(byKey, keylength, &uniqid, ptranslist);
		if(ppUniqid){
			if(!uniqid.empty()){
				*ppUniqid = strdup(uniqid.c_str());
			}else{
				*ppUniqid = NULL;
			}
		}
	}else{
		// not need to make history

		// At first take off element from ckindex
		if(!TakeOffElement(pCKIndex, pElement)){
			ERR_K2HPRN("Failed to take off element from ckey index.");
			return false;
		}
		ALObjCKI.Unlock();							// Unlock

		// remove key(pelement already take off from ckeyindex, so do not need to lock)
		result = RemoveEx(pElement, ptranslist, is_check_updated);
		if(ppUniqid){
			*ppUniqid = NULL;
		}
	}
	return result;
}

bool K2HShm::RemoveSubkeys(K2HSubKeys* pSubKeys, k2htransobjlist_t* ptranslist, bool& is_check_updated)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!is_check_updated){
		K2HFILE_UPDATE_CHECK(this);
		is_check_updated = true;
	}

	// Remove subkeys in list
	if(pSubKeys){
		for(K2HSubKeys::iterator iter = pSubKeys->begin(); iter != pSubKeys->end(); iter = pSubKeys->erase(iter)){
			if(!RemoveEx(iter->pSubKey, iter->length, ptranslist, is_check_updated)){
				WAN_K2HPRN("Failed to remove one subkey from subkey list, but continue...");
			}
		}
	}

	// Put transaction
	if(0 < ptranslist->size()){
		for(k2htransobjlist_t::iterator iter = ptranslist->begin(); ptranslist->end() != iter; ++iter){
			K2HTransaction*	ptransobj = *iter;
			if(!ptransobj->Put()){
				WAN_K2HPRN("Failed to put one transaction data in stacking, but continue...");
			}
			K2H_Delete(ptransobj);
		}
		ptranslist->clear();
	}
	return true;
}

bool K2HShm::GetSubKeys(PELEMENT pElement, K2HSubKeys*& pSubKeys)
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// get subkey page/array
	K2HPage*	pTgSKeyPage = NULL;
	K2HSubKeys*	pTgSubKeys	= NULL;
	if(!pElement->subkeys || NULL == (pTgSKeyPage = GetPageObject(pElement->subkeys)) || NULL == (pTgSubKeys = pTgSKeyPage->GetSubKeys())){
		MSG_K2HPRN("Element does not have any subkeys.");
		K2H_Delete(pTgSKeyPage);
		return true;
	}
	K2H_Delete(pTgSKeyPage);

	// add subkeys to buffer
	if(pSubKeys){
		// add this element's subkeys to subkey buffer delivered
		for(K2HSubKeys::iterator iter = pTgSubKeys->begin(); iter != pTgSubKeys->end(); iter = pTgSubKeys->erase(iter)){
			if(pSubKeys->end() == pSubKeys->insert(iter->pSubKey, iter->length)){
				WAN_K2HPRN("Failed to add subkey to subkey list stack, but continue...");
			}
		}
	}else{
		// The subkey delivered is empty, so that this element's subkey list to set it.
		pSubKeys	= pTgSubKeys;
		pTgSubKeys	= NULL;
	}
	K2H_Delete(pTgSubKeys);

	return true;
}

//---------------------------------------------------------
// Replace Methods
//---------------------------------------------------------
bool K2HShm::ReplaceAll(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength, const unsigned char* bySubkeys, size_t sklength, const unsigned char* byAttrs, size_t attrlength)
{
	if(!byKey || 0 == keylength){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(	!ReplaceValue(byKey, keylength, byValue, vallength)										||
		(bySubkeys && 0 < sklength && !ReplaceSubkeys(byKey, keylength, bySubkeys, sklength))	||
		(byAttrs && 0 < attrlength && !ReplaceAttrs(byKey, keylength, byAttrs, attrlength))		)
	{
		ERR_K2HPRN("Failed to replace value/subkeys/attrs.");
		return false;
	}
	return true;
}

bool K2HShm::ReplaceValue(const unsigned char* byKey, size_t keylength, const unsigned char* byValue, size_t vallength)
{
	return Replace(byKey, keylength, byValue, vallength, PAGEOBJ_VALUE);
}

bool K2HShm::ReplaceSubkeys(const unsigned char* byKey, size_t keylength, const unsigned char* bySubkeys, size_t sklength)
{
	return Replace(byKey, keylength, bySubkeys, sklength, PAGEOBJ_SUBKEYS);
}

bool K2HShm::ReplaceAttrs(const unsigned char* byKey, size_t keylength, const unsigned char* byAttrs, size_t attrlength)
{
	return Replace(byKey, keylength, byAttrs, attrlength, PAGEOBJ_ATTRS);
}

bool K2HShm::Replace(const unsigned char* byKey, size_t keylength, const unsigned char* byData, size_t dlength, int type)
{
	if(!byKey || 0 == keylength || (PAGEOBJ_VALUE != type && PAGEOBJ_SUBKEYS != type && PAGEOBJ_ATTRS != type)){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	K2HLock		ALObjCKI(K2HLock::RWLOCK);									// LOCK
	PELEMENT	pElement;
	if(NULL == (pElement = GetElement(byKey, keylength, ALObjCKI))){
		ALObjCKI.Unlock();													// Unlock

		// not found -> need to make new key
		if(PAGEOBJ_VALUE == type){
			// type value
			if(!Set(byKey, keylength, byData, dlength)){
				ERR_K2HPRN("Failed to make new key with value.");
				return false;
			}
		}else if(PAGEOBJ_SUBKEYS == type){
			// type subkeys
			K2HSubKeys	SubKeys;
			K2HSubKeys*	pSubKeys = NULL;
			if(byData && 0UL != dlength){
				if(!SubKeys.Serialize(byData, dlength)){
					ERR_K2HPRN("Failed to make subkeys object.");
					return false;
				}
				pSubKeys = &SubKeys;
			}
			if(!Set(byKey, keylength, NULL, 0UL, pSubKeys, false)){
				ERR_K2HPRN("Failed to make new key with subkeys.");
				return false;
			}
		}else if(PAGEOBJ_ATTRS == type){
			// type attrs
			K2HAttrs	attrs;
			K2HAttrs*	pattrs = NULL;
			if(byData && 0UL != dlength){
				if(!attrs.Serialize(byData, dlength)){
					ERR_K2HPRN("Failed to make attrs object.");
					return false;
				}
				pattrs = &attrs;
			}
			if(!Set(byKey, keylength, NULL, 0UL, NULL, false, pattrs)){
				ERR_K2HPRN("Failed to make new key with attrs.");
				return false;
			}
		}
	}else{
		// found -> replace
		if(!ReplacePage(pElement, byData, dlength, type)){
			ERR_K2HPRN("Failed to replace key data(or subkeys).");
			return false;
		}
	}
	return true;
}

bool K2HShm::ReplacePage(PELEMENT pElement, const char* pData, int type)
{
	return ReplacePage(pElement, reinterpret_cast<const unsigned char*>(pData), (pData ? strlen(pData) + 1 : 0UL), type);
}

bool K2HShm::ReplacePage(PELEMENT pElement, const unsigned char* byData, size_t length, int type)
{
	if(!pElement){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HPage*	pPage = NULL;

	if(byData && 0UL < length){
		if(!ReservePages(byData, length, &pPage) || !pPage){
			ERR_K2HPRN("Failed to make page for key/value/strarr.");
			return false;
		}
	}

	if(!ReplacePage(pElement, pPage, length, type)){
		ERR_K2HPRN("Failed to replace data(type: %d).", type);
		if(pPage && !pPage->Free()){
			ERR_K2HPRN("FATAL: In error recovery logic, failed to free pages.");
		}
		K2H_Delete(pPage);
		return false;
	}
	K2H_Delete(pPage);

	// transaction
	//
	// [NOTICE]
	// If dead lock in transaction, it probably is effected by ALObjFEC locking.
	// Here, do not unlock ALObjFEC if it is locked.
	// If something problem is occurred, should check this ALObjFEC locking.
	//
	K2HTransaction	transobj(this);
	if(transobj.IsEnable()){
		// [TODO]
		// At first, checking transaction enable for cost of getting key value now.
		// If can, calling transaction before converting key value to element pointer.
		//
		bool			trans_result = false;
		if(PAGEOBJ_KEY == type){
			// [CHECK]
			// This type is not called now(ever), but if called, you must implement
			// the function which switches key values.
			// Now we should not switch key value because of hash value.
			//
			trans_result = transobj.SetAll(byData, length, NULL, 0UL, NULL, 0UL, NULL, 0UL);

		}else if(PAGEOBJ_VALUE == type){
			K2HPage*				pPage2		= NULL;
			const unsigned char*	byKey		= NULL;
			size_t					keylength	= 0UL;
			if(NULL != (pPage2 = GetPage(pElement, PAGEOBJ_KEY)) && pPage2->GetData(&byKey, &keylength)){
				trans_result = transobj.ReplaceVal(byKey, keylength, byData, length);

			}else{
				WAN_K2HPRN("Could not get key value.");
			}
			K2H_Delete(pPage2);

		}else if(PAGEOBJ_SUBKEYS == type){
			K2HPage*				pPage2		= NULL;
			const unsigned char*	byKey		= NULL;
			size_t					keylength	= 0UL;
			if(NULL != (pPage2 = GetPage(pElement, PAGEOBJ_KEY)) && pPage2->GetData(&byKey, &keylength)){
				trans_result = transobj.ReplaceSKey(byKey, keylength, byData, length);

			}else{
				WAN_K2HPRN("Could not get key value.");
			}
			K2H_Delete(pPage2);

		}else{	// PAGEOBJ_ATTRS == type
			K2HPage*				pPage2		= NULL;
			const unsigned char*	byKey		= NULL;
			size_t					keylength	= 0UL;
			if(NULL != (pPage2 = GetPage(pElement, PAGEOBJ_KEY)) && pPage2->GetData(&byKey, &keylength)){
				trans_result = transobj.ReplaceAttrs(byKey, keylength, byData, length);

			}else{
				WAN_K2HPRN("Could not get key value.");
			}
			K2H_Delete(pPage2);
		}
		if(!trans_result){
			WAN_K2HPRN("Failed to put replacing transaction.");
		}
	}

	// update time
	if(!UpdateTimeval()){
		WAN_K2HPRN("Failed to update timeval for data update.");
	}

	return true;
}

// [NOTICE]
// This function does not put transaction and update time.
// If you call this directly, you must implement these function.
//
bool K2HShm::ReplacePage(PELEMENT pElement, K2HPage* pPage, size_t totallength, int type)
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	PPAGEHEAD	pRelPageHead = NULL;
	if(pPage){
		pRelPageHead = pPage->GetPageHeadRelAddress();
	}
	return ReplacePage(pElement, pRelPageHead, totallength, type);
}

// [NOTICE]
// This function does not put transaction and update time.
// If you call this directly, you must implement these function.
//
bool K2HShm::ReplacePage(PELEMENT pElement, PPAGEHEAD pRelPageHead, size_t totallength, int type)
{
	if(!pElement){
		ERR_K2HPRN("PELEMENT is NULL.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// get PPAGEHEAD
	PPAGEHEAD	pOldRelPageHead = NULL;
	if(PAGEOBJ_KEY == type){
		pOldRelPageHead = pElement->key;
	}else if(PAGEOBJ_VALUE == type){
		pOldRelPageHead = pElement->value;
	}else if(PAGEOBJ_SUBKEYS == type){
		pOldRelPageHead = pElement->subkeys;
	}else if(PAGEOBJ_ATTRS == type){
		pOldRelPageHead = pElement->attrs;
	}else{
		ERR_K2HPRN("type(%d) is not defined.", type);
		return false;
	}

	// Remove Old PPAGEHEADs
	if(pOldRelPageHead){
		K2HPage*	pOldPage;
		if(NULL == (pOldPage = GetPageObject(pOldRelPageHead, false))){
			ERR_K2HPRN("Could not make PageObject.");
			return false;
		}
		if(!pOldPage->Free()){
			ERR_K2HPRN("Could not free Page.");
			return false;
		}
		K2H_Delete(pOldPage);
	}

	// set PPAGEHEAD
	if(PAGEOBJ_KEY == type){
		pElement->key		= pRelPageHead;
		pElement->keylength	= totallength;
	}else if(PAGEOBJ_VALUE == type){
		pElement->value		= pRelPageHead;
		pElement->vallength	= totallength;
	}else if(PAGEOBJ_SUBKEYS == type){
		pElement->subkeys	= pRelPageHead;
		pElement->skeylength= totallength;
	}else if(PAGEOBJ_ATTRS == type){
		pElement->attrs		= pRelPageHead;
		pElement->attrlength= totallength;
	}else{
		ERR_K2HPRN("type(%d) is not defined.", type);
		return false;
	}

	return true;
}

//---------------------------------------------------------
// Rename Methods
//---------------------------------------------------------
bool K2HShm::Rename(const char* pOldKey, const char* pNewKey)
{
	return Rename(reinterpret_cast<const unsigned char*>(pOldKey), (pOldKey ? strlen(pOldKey) + 1 : 0UL), reinterpret_cast<const unsigned char*>(pNewKey), (pNewKey ? strlen(pNewKey) + 1 : 0UL));
}

bool K2HShm::Rename(const unsigned char* byOldKey, size_t oldkeylen, const unsigned char* byNewKey, size_t newkeylen)
{
	if(!byOldKey || 0 == oldkeylen || !byNewKey || 0 == newkeylen){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// make hash for old key
	k2h_hash_t	oldhash		= K2H_HASH_FUNC(reinterpret_cast<const void*>(byOldKey), oldkeylen);
	k2h_hash_t	oldsubhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byOldKey), oldkeylen);

	// get old key element
	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pOldCKIndex;
	PELEMENT	pOldElementList	= NULL;
	PELEMENT	pOldElement		= NULL;
	if(NULL == (pOldCKIndex = GetCKIndex(oldhash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}
	if(NULL == (pOldElementList = GetElementList(pOldCKIndex, oldhash, oldsubhash)) || NULL == (pOldElement = GetElement(pOldElementList, byOldKey, oldkeylen))){
		ERR_K2HPRN("Could not find old key in k2hash.");
		return false;
	}

	// make new page for new key
	K2HPage*	pNewKeyPage = NULL;
	if(!ReservePages(byNewKey, newkeylen, &pNewKeyPage) || !pNewKeyPage){
		ERR_K2HPRN("Failed to set page for new key.");
		return false;
	}

	// make new element for new key
	PELEMENT	pNewElement;
	if(NULL == (pNewElement = ReserveElement())){
		ERR_K2HPRN("Failed to get free element.");
		K2H_Delete(pNewKeyPage);
		return false;
	}

	// make hash for new key
	k2h_hash_t	newhash		= K2H_HASH_FUNC(reinterpret_cast<const void*>(byNewKey), newkeylen);
	k2h_hash_t	newsubhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byNewKey), newkeylen);

	// copy old element's member to new element's
	pNewElement->hash		= newhash;
	pNewElement->subhash	= newsubhash;
	pNewElement->key		= pNewKeyPage->GetPageHeadRelAddress();
	pNewElement->value		= pOldElement->value;
	pNewElement->subkeys	= pOldElement->subkeys;
	pNewElement->attrs		= pOldElement->attrs;
	pNewElement->keylength	= newkeylen;
	pNewElement->vallength	= pOldElement->vallength;
	pNewElement->skeylength	= pOldElement->skeylength;
	pNewElement->attrlength	= pOldElement->attrlength;
	K2H_Delete(pNewKeyPage);

	// take off old element and remove it
	if(!TakeOffElement(pOldCKIndex, pOldElement)){
		ERR_K2HPRN("Failed to take off old element from ckey index.");
		pNewElement->value		= NULL;
		pNewElement->subkeys	= NULL;
		pNewElement->attrs		= NULL;
		pNewElement->vallength	= 0UL;
		pNewElement->skeylength	= 0UL;
		pNewElement->attrlength	= 0UL;
		FreeElement(pNewElement);
		return false;
	}
	pOldElement->value		= NULL;
	pOldElement->subkeys	= NULL;
	pOldElement->attrs		= NULL;
	pOldElement->vallength	= 0UL;
	pOldElement->skeylength	= 0UL;
	pOldElement->attrlength	= 0UL;
	if(!FreeElement(pOldElement)){
		ERR_K2HPRN("Failed to remove old element.");
		FreeElement(pNewElement);
		return false;
	}
	ALObjCKI.Unlock();						// UNLOCK

	// insert new element
	PCKINDEX	pNewCKIndex;
	if(NULL == (pNewCKIndex = GetCKIndex(newhash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		FreeElement(pNewElement);
		return false;
	}
	if(pNewCKIndex->element_list){
		if(!InsertElement(static_cast<PELEMENT>(Abs(pNewCKIndex->element_list)), pNewElement)){
			ERR_K2HPRN("Failed to insert new element");
			FreeElement(pNewElement);
			return false;
		}
	}else{
		pNewCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pNewElement));
	}
	pNewCKIndex->element_count	+= 1UL;
	ALObjCKI.Unlock();						// UNLOCK

	// check element count in ckey for increasing cur_mask(expanding key/ckey area)
	if(!CheckExpandingKeyArea(pNewCKIndex)){				// Do not care for locking
		ERR_K2HPRN("Something error occurred by checking/expanding key/ckey area.");
		return false;
	}

	// transaction
	K2HTransaction	transobj(this);
	if(!transobj.Rename(byOldKey, oldkeylen, byNewKey, newkeylen, NULL, 0UL)){
		WAN_K2HPRN("Failed to put setting transaction.");
	}

	if(!UpdateTimeval()){
		WAN_K2HPRN("Failed to update timeval for data update.");
	}
	return true;
}

//
// For transaction(loading archive)
//
bool K2HShm::Rename(const unsigned char* byOldKey, size_t oldkeylen, const unsigned char* byNewKey, size_t newkeylen, const unsigned char* byAttrs, size_t attrlen)
{
	if(!byOldKey || 0 == oldkeylen || !byNewKey || 0 == newkeylen){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// make hash for old key
	k2h_hash_t	oldhash		= K2H_HASH_FUNC(reinterpret_cast<const void*>(byOldKey), oldkeylen);
	k2h_hash_t	oldsubhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byOldKey), oldkeylen);

	// get old key element
	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pOldCKIndex;
	PELEMENT	pOldElementList	= NULL;
	PELEMENT	pOldElement		= NULL;
	if(NULL == (pOldCKIndex = GetCKIndex(oldhash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}
	if(NULL == (pOldElementList = GetElementList(pOldCKIndex, oldhash, oldsubhash)) || NULL == (pOldElement = GetElement(pOldElementList, byOldKey, oldkeylen))){
		ERR_K2HPRN("Could not find old key in k2hash.");
		return false;
	}

	// make new page for new key
	K2HPage*	pNewKeyPage = NULL;
	if(!ReservePages(byNewKey, newkeylen, &pNewKeyPage) || !pNewKeyPage){
		ERR_K2HPRN("Failed to set page for new key.");
		return false;
	}

	// make new page for attrs
	K2HPage*	pNewAttrPage= NULL;
	bool		IsNewAttr	= false;
	if(byAttrs && 0 < attrlen){
		if(!ReservePages(byAttrs, attrlen, &pNewAttrPage) || !pNewAttrPage){
			ERR_K2HPRN("Failed to set page for new attrs.");
			K2H_Delete(pNewKeyPage);
			return false;
		}
		IsNewAttr = true;
	}

	// make new element for new key
	PELEMENT	pNewElement;
	if(NULL == (pNewElement = ReserveElement())){
		ERR_K2HPRN("Failed to get free element.");
		K2H_Delete(pNewKeyPage);
		K2H_Delete(pNewAttrPage);
		return false;
	}

	// make hash for new key
	k2h_hash_t	newhash		= K2H_HASH_FUNC(reinterpret_cast<const void*>(byNewKey), newkeylen);
	k2h_hash_t	newsubhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byNewKey), newkeylen);

	// copy old element's member to new element's
	pNewElement->hash		= newhash;
	pNewElement->subhash	= newsubhash;
	pNewElement->key		= pNewKeyPage->GetPageHeadRelAddress();
	pNewElement->value		= pOldElement->value;
	pNewElement->subkeys	= pOldElement->subkeys;
	pNewElement->attrs		= IsNewAttr ? pNewAttrPage->GetPageHeadRelAddress() : pOldElement->attrs;
	pNewElement->keylength	= newkeylen;
	pNewElement->vallength	= pOldElement->vallength;
	pNewElement->skeylength	= pOldElement->skeylength;
	pNewElement->attrlength	= IsNewAttr ? attrlen : pOldElement->attrlength;
	K2H_Delete(pNewKeyPage);
	K2H_Delete(pNewAttrPage);

	// take off old element and remove it
	if(!TakeOffElement(pOldCKIndex, pOldElement)){
		ERR_K2HPRN("Failed to take off old element from ckey index.");
		pNewElement->value		= NULL;
		pNewElement->subkeys	= NULL;
		pNewElement->attrs		= NULL;
		pNewElement->vallength	= 0UL;
		pNewElement->skeylength	= 0UL;
		pNewElement->attrlength	= 0UL;
		FreeElement(pNewElement);
		return false;
	}
	pOldElement->value		= NULL;
	pOldElement->subkeys	= NULL;
	pOldElement->vallength	= 0UL;
	pOldElement->skeylength	= 0UL;
	if(!IsNewAttr){
		pOldElement->attrs		= NULL;
		pOldElement->attrlength	= 0UL;
	}
	if(!FreeElement(pOldElement)){
		ERR_K2HPRN("Failed to remove old element.");
		FreeElement(pNewElement);
		return false;
	}
	ALObjCKI.Unlock();						// UNLOCK

	// insert new element
	PCKINDEX	pNewCKIndex;
	if(NULL == (pNewCKIndex = GetCKIndex(newhash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		FreeElement(pNewElement);
		return false;
	}
	if(pNewCKIndex->element_list){
		if(!InsertElement(static_cast<PELEMENT>(Abs(pNewCKIndex->element_list)), pNewElement)){
			ERR_K2HPRN("Failed to insert new element");
			FreeElement(pNewElement);
			return false;
		}
	}else{
		pNewCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pNewElement));
	}
	pNewCKIndex->element_count	+= 1UL;
	ALObjCKI.Unlock();						// UNLOCK

	// check element count in ckey for increasing cur_mask(expanding key/ckey area)
	if(!CheckExpandingKeyArea(pNewCKIndex)){				// Do not care for locking
		ERR_K2HPRN("Something error occurred by checking/expanding key/ckey area.");
		return false;
	}

	// transaction
	K2HTransaction	transobj(this);
	if(!transobj.Rename(byOldKey, oldkeylen, byNewKey, newkeylen, byAttrs, attrlen)){
		WAN_K2HPRN("Failed to put setting transaction.");
	}

	if(!UpdateTimeval()){
		WAN_K2HPRN("Failed to update timeval for data update.");
	}
	return true;
}

//
// Rename for history(backup)
//
// Like rename() method, but this method generates new key name automatically and marks history into attributes.
//
bool K2HShm::RenameForHistory(const unsigned char* byKey, size_t keylen, string* puniqid, k2htransobjlist_t* ptranslist)
{
	if(!byKey || 0 == keylen){
		ERR_K2HPRN("Some parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(!K2hAttrOpsMan::IsMarkHistory(this)){
		ERR_K2HPRN("Not make history mode.");
		return false;
	}
	K2HFILE_UPDATE_CHECK(this);

	// make hash for old key
	k2h_hash_t	oldhash		= K2H_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylen);
	k2h_hash_t	oldsubhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byKey), keylen);

	// get old key element
	K2HLock		ALObjCKI(K2HLock::RWLOCK);			// LOCK
	PCKINDEX	pOldCKIndex;
	PELEMENT	pOldElementList	= NULL;
	PELEMENT	pOldElement		= NULL;
	if(NULL == (pOldCKIndex = GetCKIndex(oldhash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		return false;
	}
	if(NULL == (pOldElementList = GetElementList(pOldCKIndex, oldhash, oldsubhash)) || NULL == (pOldElement = GetElement(pOldElementList, byKey, keylen))){
		ERR_K2HPRN("Could not find old key in k2hash.");
		return false;
	}

	// get now attributes and mark history flag into it. And get uniqid
	//
	// [NOTE] do not need to set value at initializing attrman.
	//
	K2HAttrs*		pAttrs;
	K2hAttrOpsMan	attrman;
	string			uniqid;
	if(NULL == (pAttrs = GetAttrs(pOldElement))){
		pAttrs = new K2HAttrs;
	}
	if(!attrman.Initialize(this, byKey, keylen, NULL, 0UL)){
		ERR_K2HPRN("Failed to initialize attribute manager object.");
		K2H_Delete(pAttrs);
		return false;
	}
	if(!attrman.GetUniqId(*pAttrs, uniqid)){
		// there is not uniqid attribute in attrs, so make it.
		if(!attrman.DirectSetUniqId(*pAttrs, NULL) || !attrman.GetUniqId(*pAttrs, uniqid)){
			ERR_K2HPRN("Failed to add uniqid for marking history into attributes.");
			K2H_Delete(pAttrs);
			return false;
		}
	}
	if(!attrman.MarkHistory(*pAttrs)){
		ERR_K2HPRN("Failed to mark history into attributes.");
		K2H_Delete(pAttrs);
		return false;
	}

	// make new attribute page for new key
	unsigned char*	byNewAttrs		= NULL;
	size_t			newattrlen		= 0UL;
	K2HPage*		pNewAttrPage	= NULL;
	if(!pAttrs->Serialize(&byNewAttrs, newattrlen)){
		ERR_K2HPRN("Could not make attribute binary data from object.");
		K2H_Delete(pAttrs);
		return false;
	}
	if(!ReservePages(byNewAttrs, newattrlen, &pNewAttrPage) || !pNewAttrPage){
		ERR_K2HPRN("Failed to set page for new attributes.");
		K2H_Delete(pAttrs);
		K2H_Free(byNewAttrs);
		return false;
	}
	K2H_Delete(pAttrs);

	// make new key
	unsigned char*	byNewKey;
	size_t			newkeylen = 0;
	if(NULL == (byNewKey = K2HShm::MakeHistoryKey(byKey, keylen, uniqid.c_str(), newkeylen))){
		ERR_K2HPRN("Could not make history key name.");
		K2H_Free(byNewAttrs);
		K2H_Delete(pNewAttrPage);
		return false;
	}

	// make new page for new key
	K2HPage*	pNewKeyPage = NULL;
	if(!ReservePages(byNewKey, newkeylen, &pNewKeyPage) || !pNewKeyPage){
		ERR_K2HPRN("Failed to set page for new key.");
		K2H_Free(byNewAttrs);
		K2H_Free(byNewKey);
		K2H_Delete(pNewAttrPage);
		return false;
	}

	// make new element for new key
	PELEMENT	pNewElement;
	if(NULL == (pNewElement = ReserveElement())){
		ERR_K2HPRN("Failed to get free element.");
		K2H_Free(byNewAttrs);
		K2H_Free(byNewKey);
		K2H_Delete(pNewAttrPage);
		K2H_Delete(pNewKeyPage);
		return false;
	}

	// make hash for new key
	k2h_hash_t	newhash		= K2H_HASH_FUNC(reinterpret_cast<const void*>(byNewKey), newkeylen);
	k2h_hash_t	newsubhash	= K2H_2ND_HASH_FUNC(reinterpret_cast<const void*>(byNewKey), newkeylen);

	// copy old element's member to new element's
	pNewElement->hash		= newhash;
	pNewElement->subhash	= newsubhash;
	pNewElement->key		= pNewKeyPage->GetPageHeadRelAddress();
	pNewElement->value		= pOldElement->value;
	pNewElement->subkeys	= pOldElement->subkeys;
	pNewElement->attrs		= pNewAttrPage->GetPageHeadRelAddress();
	pNewElement->keylength	= newkeylen;
	pNewElement->vallength	= pOldElement->vallength;
	pNewElement->skeylength	= pOldElement->skeylength;
	pNewElement->attrlength	= newattrlen;
	K2H_Delete(pNewAttrPage);
	K2H_Delete(pNewKeyPage);

	// take off old element and remove it
	if(!TakeOffElement(pOldCKIndex, pOldElement)){
		ERR_K2HPRN("Failed to take off old element from ckey index.");
		pNewElement->value		= NULL;
		pNewElement->subkeys	= NULL;
		pNewElement->attrs		= NULL;
		pNewElement->vallength	= 0UL;
		pNewElement->skeylength	= 0UL;
		pNewElement->attrlength	= 0UL;
		FreeElement(pNewElement);
		K2H_Free(byNewAttrs);
		K2H_Free(byNewKey);
		return false;
	}
	pOldElement->value		= NULL;
	pOldElement->subkeys	= NULL;
	pOldElement->vallength	= 0UL;
	pOldElement->skeylength	= 0UL;
	if(!FreeElement(pOldElement)){
		ERR_K2HPRN("Failed to remove old element.");
		FreeElement(pNewElement);
		K2H_Free(byNewAttrs);
		K2H_Free(byNewKey);
		return false;
	}
	ALObjCKI.Unlock();						// UNLOCK

	// insert new element
	PCKINDEX	pNewCKIndex;
	if(NULL == (pNewCKIndex = GetCKIndex(newhash, ALObjCKI))){
		ERR_K2HPRN("Something error occurred, pCKIndex must not be NULL.");
		FreeElement(pNewElement);
		K2H_Free(byNewAttrs);
		K2H_Free(byNewKey);
		return false;
	}
	if(pNewCKIndex->element_list){
		if(!InsertElement(static_cast<PELEMENT>(Abs(pNewCKIndex->element_list)), pNewElement)){
			ERR_K2HPRN("Failed to insert new element");
			FreeElement(pNewElement);
			K2H_Free(byNewAttrs);
			K2H_Free(byNewKey);
			return false;
		}
	}else{
		pNewCKIndex->element_list = reinterpret_cast<PELEMENT>(Rel(pNewElement));
	}
	pNewCKIndex->element_count	+= 1UL;
	ALObjCKI.Unlock();						// UNLOCK

	// check element count in ckey for increasing cur_mask(expanding key/ckey area)
	if(!CheckExpandingKeyArea(pNewCKIndex)){				// Do not care for locking
		ERR_K2HPRN("Something error occurred by checking/expanding key/ckey area.");
		K2H_Free(byNewAttrs);
		K2H_Free(byNewKey);
		return false;
	}

	// transaction
	K2HTransaction*	ptransobj = new K2HTransaction(this, (NULL != ptranslist));		// ptranslist for stacking transaction data
	if(!ptransobj->Rename(byKey, keylen, byNewKey, newkeylen, byNewAttrs, newattrlen)){
		WAN_K2HPRN("Failed to put setting transaction.");
	}else{
		if(ptranslist){
			ptranslist->push_back(ptransobj);										// stacking
			ptransobj = NULL;
		}
	}
	K2H_Delete(ptransobj);
	K2H_Free(byNewAttrs);
	K2H_Free(byNewKey);

	if(!UpdateTimeval()){
		WAN_K2HPRN("Failed to update timeval for data update.");
	}

	if(puniqid){
		*puniqid = uniqid;
	}
	return true;
}

//---------------------------------------------------------
// Direct Access
//---------------------------------------------------------
K2HDAccess* K2HShm::GetDAccessObj(const char* pKey, K2HDAccess::ACSMODE acsmode, off_t offset)
{
	return GetDAccessObj(reinterpret_cast<const unsigned char*>(pKey), pKey ? strlen(pKey) + 1 : 0UL, acsmode, offset);
}

K2HDAccess* K2HShm::GetDAccessObj(const unsigned char* byKey, size_t keylength, K2HDAccess::ACSMODE acsmode, off_t offset)
{
	if(!byKey || 0UL == keylength || offset < 0L){
		ERR_K2HPRN("Some parameters are wrong.");
		return NULL;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	if(isReadMode && acsmode != K2HDAccess::READ_ACCESS){
		ERR_K2HPRN("K2HASH is opened READ mode, but parameter mode is with write.");
		return NULL;
	}
	K2HFILE_UPDATE_CHECK(this);

	K2HDAccess*	pAccess = new K2HDAccess(this, acsmode);
	if(!pAccess || !pAccess->IsInitialize()){
		ERR_K2HPRN("Could not initialize internal K2HDAccess object.");
		K2H_Delete(pAccess);
		return NULL;
	}

	// Open key & set offset in K2HDAccess
	if(!pAccess->Open(byKey, keylength)){
		MSG_K2HPRN("Could not open key.");
		K2H_Delete(pAccess);
		return NULL;
	}
	if(acsmode != K2HDAccess::READ_ACCESS){
		if(!pAccess->SetWriteOffset(offset)){
			ERR_K2HPRN("Could not set write offset to 0L.");
			K2H_Delete(pAccess);
			return NULL;
		}
	}
	if(acsmode != K2HDAccess::WRITE_ACCESS){
		if(!pAccess->SetReadOffset(offset)){
			ERR_K2HPRN("Could not set read offset to 0L.");
			K2H_Delete(pAccess);
			return NULL;
		}
	}
	return pAccess;
}

//---------------------------------------------------------
// Search(Find Next)
//---------------------------------------------------------
// [NOTICE] - VERY IMPORTANT
// After K2HShm::iterator object is made, first(current) element is locked.
// If you write the element with keeping K2HShm::iterator, there is possibility
// about dead lock.
//
K2HShm::iterator K2HShm::begin(const char* pKey)
{
	return begin(reinterpret_cast<const unsigned char*>(pKey), (pKey ? strlen(pKey) + 1 : 0UL));
}

K2HShm::iterator K2HShm::begin(const unsigned char* byKey, size_t length)
{
	if(!byKey || 0L == length){
		return K2HIterator(this, true);
	}
	K2HFILE_UPDATE_CHECK(this);

	K2HLock		ALObjCKI(K2HLock::RDLOCK);									// LOCK
	PELEMENT	pParentElement;
	if(NULL == (pParentElement = GetElement(byKey, length, ALObjCKI))){
		return K2HIterator(this, true);
	}
	K2HSubKeys*				pSubKeys;
	if(NULL != (pSubKeys = GetSubKeys(pParentElement))){
		K2HSubKeys::iterator	sk_iter	= pSubKeys->begin();
		PELEMENT				pElement= NULL;
		if(sk_iter != pSubKeys->end()){
			if(NULL == (pElement= GetElement(sk_iter->pSubKey, sk_iter->length, ALObjCKI))){
				ALObjCKI.Unlock();
			}
		}
		return K2HIterator(this, pElement, pSubKeys, sk_iter, ALObjCKI);
	}
	return K2HIterator(this, true);
}

K2HShm::iterator K2HShm::begin(void)
{
	K2HFILE_UPDATE_CHECK(this);

	for(PELEMENT pFirstElement = static_cast<PELEMENT>(MmapInfos.begin(K2H_AREA_PAGELIST)); pFirstElement; pFirstElement = static_cast<PELEMENT>(MmapInfos.next(pFirstElement, sizeof(ELEMENT)))){
		if(pFirstElement->key){
			// For lock object
			K2HLock		ALObjCKI(K2HLock::RDLOCK);					// LOCK
			if(NULL != GetCKIndex(pFirstElement->hash, ALObjCKI)){
				return K2HIterator(this, pFirstElement, ALObjCKI);
			}
		}
	}
	return K2HIterator(this, false);
}

K2HShm::iterator K2HShm::end(bool isSubKey)
{
	return K2HIterator(this, isSubKey);
}

//
// [NOTICE]
// The best way is searching in key_index_area array, but that way returns sometimes
// same value or skips some elements. Because a area of element is expanded and elements
// is arranged mapping in ckindex delay(at the time of next action).
// So for iterator this function searches in area for elements, this is a certain way
// for find-next.
//
PELEMENT K2HShm::FindNextElement(PELEMENT pLastElement, K2HLock& ALObjCKI) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}

	K2HFILE_UPDATE_CHECK(const_cast<K2HShm*>(this));

	for(PELEMENT pNextElement = static_cast<PELEMENT>(MmapInfos.next(pLastElement, sizeof(ELEMENT))); pNextElement; pNextElement = static_cast<PELEMENT>(MmapInfos.next(pNextElement, sizeof(ELEMENT)))){
		if(pNextElement->key){
			if(NULL != GetCKIndex(pNextElement->hash, ALObjCKI)){
				return pNextElement;
			}
		}
	}
	return NULL;
}

//---------------------------------------------------------
// Transaction control
//---------------------------------------------------------
bool K2HShm::Transaction(bool isEnable, const char* filepath, const unsigned char* pprefix, size_t prefixlen, const unsigned char* pparam, size_t paramlen, const time_t* expire) const
{
	if(!IsAttached()){
		MSG_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	TRANSOPT	transopt;
	if(isEnable){
		if(!ISEMPTYSTR(filepath)){
			if(MAX_TRANSACTION_FILEPATH <= strlen(filepath)){
				ERR_K2HPRN("File path is empty or over %d byte", MAX_TRANSACTION_FILEPATH);
				return false;
			}
			strcpy(transopt.szFilePath, filepath);
		}else{
			transopt.szFilePath[0]	= '\0';
		}
		transopt.isEnable = true;

		if(pprefix && 0 < prefixlen){
			transopt.PrefixLength = min(prefixlen, static_cast<size_t>(MAX_TRANSACTION_PREFIX));
			memcpy(transopt.byTransPrefix, pprefix, transopt.PrefixLength);
		}else{
			transopt.PrefixLength = 0;
			memset(transopt.byTransPrefix, 0, MAX_TRANSACTION_PREFIX);
		}

		if(pparam && 0 < paramlen){
			transopt.ParamLength = min(paramlen, static_cast<size_t>(MAX_TRANSACTION_PARAM));
			memcpy(transopt.byTransParam, pparam, transopt.ParamLength);
		}else{
			transopt.ParamLength = 0;
			memset(transopt.byTransParam, 0, MAX_TRANSACTION_PARAM);
		}
	}else{
		transopt.szFilePath[0]	= '\0';
		transopt.isEnable		= false;
		transopt.PrefixLength	= 0;
		memset(transopt.byTransPrefix, 0, MAX_TRANSACTION_PREFIX);
	}

	if(!K2H_TRANS_CNTL_FUNC(reinterpret_cast<k2h_h>(this), &transopt)){
		WAN_K2HPRN("Denied transaction control(%s) by k2h_trans_cntl.", isEnable ? "start" : "stop");
		return false;
	}

	if(isEnable){
		if(!K2HTransManager::Get()->Start(this, transopt.szFilePath, (0 == transopt.PrefixLength ? NULL : transopt.byTransPrefix), transopt.PrefixLength, expire)){
			ERR_K2HPRN("Failed to (re)start transaction.");
			return false;
		}
	}else{
		if(!K2HTransManager::Get()->Stop(this)){
			ERR_K2HPRN("Failed to stop transaction.");
			return false;
		}
	}
	return true;
}

// Class Method
int K2HShm::GetTransThreadPool(void)
{
	return K2HTransManager::Get()->GetThreadPool();
}

// Class Method
bool K2HShm::SetTransThreadPool(int count)
{
	return K2HTransManager::Get()->SetThreadPool(count);
}

// Class Method
bool K2HShm::UnsetTransThreadPool(void)
{
	return K2HTransManager::Get()->UnsetThreadPool();
}

//---------------------------------------------------------
// Attributes control
//---------------------------------------------------------
bool K2HShm::SetCommonAttribute(const bool* is_mtime, const bool* is_defenc, const char* passfile, const bool* is_history, const time_t* expire, const strarr_t* pluginlibs)
{
	return K2hAttrOpsMan::InitializeCommonAttr(this, is_mtime, is_defenc, passfile, is_history, expire, pluginlibs);
}

bool K2HShm::AddAttrCryptPass(const char* pass, bool is_default_encrypt)
{
	return K2hAttrOpsMan::AddCryptPass(this, pass, is_default_encrypt);
}

bool K2HShm::AddAttrPluginLib(const char* path)
{
	return K2hAttrOpsMan::AddPluginLib(this, path);
}

bool K2HShm::CleanCommonAttribute(void)
{
	return K2hAttrOpsMan::CleanCommonAttr(this);
}

bool K2HShm::GetAttrVersionInfos(strarr_t& verinfos) const
{
	K2hAttrOpsMan	attrman;
	if(!attrman.Initialize(this, NULL, 0UL, NULL, 0UL)){	// do not need any arguments
		ERR_K2HPRN("Could not initialize attribute manager object.");
		return false;
	}
	return attrman.GetVersionInfos(verinfos);
}

void K2HShm::GetAttrInfos(stringstream& ss) const
{
	K2hAttrOpsMan	attrman;
	if(!attrman.Initialize(this, NULL, 0UL, NULL, 0UL)){	// do not need any arguments
		ERR_K2HPRN("Could not initialize attribute manager object.");
		return;
	}
	attrman.GetInfos(ss);
}

//---------------------------------------------------------
// Other Methods
//---------------------------------------------------------
bool K2HShm::GetUpdateTimeval(struct timeval& tv) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// [NOTE]
	// we do not need to lock update time area now.
	//
    // K2HLock	ALObjAUpdate(ShmFd, Rel(&(pHead->last_update)), K2HLock::RDLOCK);	// LOCK

	tv.tv_sec	= pHead->last_update.tv_sec;
	tv.tv_usec	= pHead->last_update.tv_usec;

	return true;
}

bool K2HShm::UpdateTimeval(bool isAreaUpdate) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	struct timeval	tv;
	K2HShm::GetRealTimeval(tv);

	// [LOCK]
	// This field is not read usually, if there is a client, it is dumper only now.
	// So that, this function is not locking.
	// If you need to lock, add it here.

	pHead->last_update.tv_sec	= tv.tv_sec;
	pHead->last_update.tv_usec	= tv.tv_usec;
	if(isAreaUpdate){
		pHead->last_area_update.tv_sec	= tv.tv_sec;
		pHead->last_area_update.tv_usec	= tv.tv_usec;
	}
	return true;
}

bool K2HShm::SetMsyncMode(bool enable)
{
	if(isAnonMem){
		return (!enable);
	}
	if(isTemporary){
		return (!enable);
	}
	if(isReadMode){
		return (!enable);
	}
	isSync = enable;
	return true;
}

// [NOTE]
// Calling msync function with MS_ASYNC does nothing after kernel 2.5.67(2.6.17).
// This means that kernel completely guarantees to manage dirty flag on memory and
// to update that area. So that, K2HASH does not call this function when it reads/
// writes shared memory. And fsync(fdatasync) is not needed for multi processing.
// Thus, this Msync() method is called only when K2HASH expands shared memory area.
//
bool K2HShm::Msync(void* address, bool is_update_meta) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(isAnonMem){
		// Case of only memory for threads, does not need to sync.
		return true;
	}
	if(isTemporary){
		// Case of tempolary file, does not need to sync.
		// This case almost is one process and multi thread instead of only memory because of not enough memory.
		return true;
	}
	if(isReadMode){
		// Case of only reading, does not need to sync.( why dose this method call? )
		return true;
	}

	// msync(MS_ASYNC)
	bool result = MmapInfos.AreaMsync(address);

	// fsync
	if(is_update_meta){
		if(-1 == fsync(ShmFd)){
			ERR_K2HPRN("Failed to fsync %p(null means all area). errno=%d", address, errno);
			result = false;
		}
	}else{
		if(-1 == fdatasync(ShmFd)){
			ERR_K2HPRN("Failed to fdatasync %p(null means all area). errno=%d", address, errno);
			result = false;
		}
	}
	return result;
}

bool K2HShm::CheckFileUpdate(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(isAnonMem || isTemporary){
		// These mode do not need to check update.
		return true;
	}

	// [NOTE]
	// This lock(offset=0, fd=-1) is only for file update checking.
	//
	K2HLock	ALObjFU(-1, 0, K2HLock::RDLOCK);		// LOCK

	bool	is_change = false;
	if(!FileMon.PreCheckInode(is_change)){
		ERR_K2HPRN("Failed to check update inode.");
	}
	if(is_change){
		// recheck with RWLOCK
		ALObjFU.Unlock();
		ALObjFU.Lock(-1, 0, K2HLock::RWLOCK);		// LOCK

		if(!FileMon.CheckInode(is_change)){
			ERR_K2HPRN("Failed to check update inode.");
		}
		if(is_change){
			// need to reload file
			MSG_K2HPRN("Found file update, do update hole k2hash.");

			// backup all
			string	Bup_ShmPath			= ShmPath;
			bool	Bup_isAnonMem		= isAnonMem;
			bool	Bup_isFullMapping	= isFullMapping;
			bool	Bup_isTemporary		= isTemporary;
			bool	Bup_isReadMode		= isReadMode;
			bool	Bup_isSync			= isSync;

			// clear all
			Clean(false);

			// reset from backup
			isAnonMem	= Bup_isAnonMem;
			isTemporary	= Bup_isTemporary;
			isSync		= Bup_isSync;

			// re-attach new file
			if(!AttachFile(Bup_ShmPath.c_str(), Bup_isReadMode, Bup_isFullMapping)){
				ERR_K2HPRN("[FATAL] Failed to reload file.");
				return false;
			}
		}

	}else if(!isReadMode){		// Not need to update area on read mode.
		ALObjFU.Unlock();

		// check mmap area
		is_change = false;
		if(!CheckAreaUpdate(is_change)){
			return false;
		}
	}
	return true;
}

//
// Only update area information and mmap it.
//
bool K2HShm::CheckAreaUpdate(bool& is_change)
{
	is_change = false;

	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(isAnonMem || isTemporary || isReadMode){
		// These mode do not need to check update.
		return true;
	}

	if(!FileMon.CheckArea(is_change)){
		ERR_K2HPRN("Failed to check update area.");
	}
	if(!is_change){
		// Nothing to do
		return true;
	}
	MSG_K2HPRN("Found k2hash area update, do update k2hash area.");

	return DoAreaUpdate();
}

//
// Doing update area information.
//
bool K2HShm::DoAreaUpdate(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	if(isAnonMem || isTemporary || isReadMode){
		// These mode do not need to check update.
		return true;
	}

	// [NOTICE]
	// Now we do not lock ALObjCMask as RWLOCK.
	// Old version locked ALObjCMask as RWLOCK because of suspending all thread(process).
	// But this locking is not needed, because in most cases the AREA is only increase and
	// HEAD AREA is not replaced.
	// In the first place, this area extension notice is triggered that k2hash library
	// captures the change of monitor file. And already cur_mask in HEAD AREA has been
	// changed when library notices this change. Thus, we do not need to LOCK ALObjCMask.
	//
	// But if AREA count is decreased, we need to LOCK all thread, but it should not be
	// happened.(Be careful)
	//

	// force other thread to wait for loading all mapping
	K2HLock	ALObjUnArea(ShmFd, Rel(&(pHead->unassign_area)), K2HLock::RWLOCK);		// LOCK

	// Do mmap new area
	if(!ExpandMmapInfo()){
		ERR_K2HPRN("[FATAL] Failed to rebuild mmap info.");
		return false;
	}

	// Do munmap old area
	if(!ContractMmapInfo()){
		ERR_K2HPRN("[FATAL] Failed to contract mmap info.");
		return false;
	}
	return true;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
