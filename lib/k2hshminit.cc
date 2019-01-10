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
#include <sys/resource.h>
#include <errno.h>
#include <algorithm>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hashfunc.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Structure / Symbols
//---------------------------------------------------------
//
// For initializing only
//
typedef	struct initialize_area_map{
	off_t		file_offset;
	size_t		length;
	void*		pmmap;
}INITAREAMMAP, *PINITAREAMMAP;

#define	INITAREAMMAP_POS_KINDEX		0
#define	INITAREAMMAP_POS_CKINDEX	1
#define	INITAREAMMAP_POS_ELEMENT	2
#define	INITAREAMMAP_POS_PAGE		3
#define	INITAREAMMAP_SIZE			4

//---------------------------------------------------------
// Class Methods
//---------------------------------------------------------
bool K2HShm::InitializeFileZero(int fd, off_t start, size_t length)
{
	struct stat		st;
	unsigned char	szBuff[K2HShm::SystemPageSize];

	if(-1 == fstat(fd, &st)){
		ERR_K2HPRN("Could not get file stats, errno = %d", errno);
		return false;
	}
	// truncate
	if(st.st_size < static_cast<off_t>(start + length)){
		// Need to truncate
		if(0 != ftruncate(fd, (start + length))){
			ERR_K2HPRN("Could not truncate file, errno = %d", errno);
			return false;
		}
	}
	// initialize(fill zero)
	memset(szBuff, 0, K2HShm::SystemPageSize);
	for(ssize_t wrote = 0, onewrote = 0; static_cast<size_t>(wrote) < length; wrote += onewrote){
		onewrote = min(static_cast<size_t>(sizeof(unsigned char) * K2HShm::SystemPageSize), (length - static_cast<size_t>(wrote)));
		if(-1 == k2h_pwrite(fd, szBuff, onewrote, (start + wrote))){
			ERR_K2HPRN("Failed to write initializing file, errno = %d", errno);
			return false;
		}
	}
	return true;
}

bool K2HShm::InitializeKeyIndexArray(void* pCKIndexShmBase, PKINDEX pKeyIndex, k2h_hash_t& start_hash, k2h_hash_t cur_mask, int cmask_bitcnt, PCKINDEX pCKIndex, int count, long default_assign)
{
	if(!pCKIndexShmBase || !pKeyIndex || !pCKIndex){
		ERR_K2HPRN("pSHmBase or PKINDEX or PCKINDEX is null");
		return false;
	}
	for(int pos = 0; pos < count; pos++, start_hash++){
		pKeyIndex[pos].assign		= default_assign;
		pKeyIndex[pos].shifted_mask	= (cur_mask << cmask_bitcnt);
		pKeyIndex[pos].masked_hash	= ((start_hash << cmask_bitcnt) & (cur_mask << cmask_bitcnt));
		pKeyIndex[pos].ckey_list	= CVT_REL(pCKIndexShmBase, &pCKIndex[pos * CKINDEX_BYKINDEX_CNT(cmask_bitcnt)], PCKINDEX);
	}
	return true;
}

bool K2HShm::InitializeCollisionKeyIndexArray(PCKINDEX pCKIndex, int count)
{
	if(!pCKIndex){
		ERR_K2HPRN("PCKINDEX is null");
		return false;
	}
	for(int pos = 0; pos < count; pos++){
		pCKIndex[pos].element_count	= 0UL;
		pCKIndex[pos].element_list	= NULL;
	}
	return true;
}

bool K2HShm::InitializeElementArray(void* pShmBase, PELEMENT pElement, int count, PELEMENT pLastRelElement)
{
	if(!pShmBase || !pElement){
		ERR_K2HPRN("pShmBase or PELEMENT is null");
		return false;
	}
	for(int pos = 0; pos < count; pos++){
		if(0 == pos){
			pElement[pos].parent= NULL;
		}else{
			pElement[pos].parent= CVT_REL(pShmBase, &pElement[pos - 1], PELEMENT);
		}
		if(pos + 1 < count){
			pElement[pos].same	= CVT_REL(pShmBase, &pElement[pos + 1], PELEMENT);
		}else{
			pElement[pos].same	= pLastRelElement;
		}
		pElement[pos].big		= NULL;
		pElement[pos].small		= NULL;
		pElement[pos].hash		= 0UL;
		pElement[pos].subhash	= 0UL;
		pElement[pos].key		= NULL;
		pElement[pos].value		= NULL;
		pElement[pos].subkeys	= NULL;
		pElement[pos].keylength	= 0UL;
		pElement[pos].vallength	= 0UL;
		pElement[pos].skeylength= 0UL;
	}
	return true;
}

bool K2HShm::InitializePageArray(int fd, off_t start, ssize_t pagesize, int count, PPAGEHEAD pLastRelPage)
{
	if(-1 == fd){
		ERR_K2HPRN("file descriptor is wrong");
		return false;
	}
	//
	// [TODO] Locking
	//

	PAGEWRAP	wrap;
	int			pos;
	off_t		cur;
	for(pos = 0, cur = start; pos < count; pos++, cur += pagesize){
		if(0 == pos){
			wrap.pagehead.prev	= NULL;
		}else{
			wrap.pagehead.prev	= reinterpret_cast<PPAGEHEAD>(cur - pagesize);
		}
		if(pos + 1 < count){
			wrap.pagehead.next	= reinterpret_cast<PPAGEHEAD>(cur + pagesize);
		}else{
			wrap.pagehead.next	= pLastRelPage;
		}
		wrap.pagehead.length	= 0UL;
		wrap.pagehead.data[0]	= 0;

		if(-1 == k2h_pwrite(fd, wrap.barray, sizeof(PAGEHEAD), cur)){
			ERR_K2HPRN("Could not initialize one of page head, errno = %d", errno);
			return false;
		}
	}
	fsync(fd);

	return true;
}

bool K2HShm::InitializePageArray(void* pShmBase, PPAGEHEAD pPage, ssize_t pagesize, int count, PPAGEHEAD pLastRelPage)
{
	if(!pShmBase || !pPage){
		ERR_K2HPRN("pShmBase or PPAGEHEAD is null");
		return false;
	}
	//
	// [TODO] Locking
	//
	for(int nCnt = 0; nCnt < count; nCnt++){
		if(0 == nCnt){
			pPage->prev	= NULL;
		}else{
			pPage->prev	= CVT_REL(pShmBase, SUBPTR(pPage, static_cast<off_t>(pagesize)), PPAGEHEAD);
		}
		if(nCnt + 1 < count){
			pPage->next	= CVT_REL(pShmBase, ADDPTR(pPage, static_cast<off_t>(pagesize)), PPAGEHEAD);
		}else{
			pPage->next	= pLastRelPage;
		}
		pPage->length	= 0UL;
		pPage->data[0]	= 0;
		pPage			= ADDPTR(pPage, static_cast<off_t>(pagesize));
	}
	return true;
}

ssize_t K2HShm::InitialSystemPageSize(void)
{
	long psize = sysconf(_SC_PAGE_SIZE);
	if(-1 == psize){
		ERR_K2HPRN("Could not get page size. errno = %d", errno);
		return -1;
	}
	// how many bytes should we use for page size?
	// (maybe on linux, page size is 4096, but if another case, should we do?)
	// Page size for K2HASH is minimum 8192, it will be support for long life for this.
	if(K2HShm::MIN_SYSPAGE_SIZE >= psize){
		if(K2HShm::MIN_SYSPAGE_SIZE != psize){
			MSG_K2HPRN("System Page size is %ld, but for safety K2HASH set %d", psize, K2HShm::MIN_SYSPAGE_SIZE);
			psize = K2HShm::MIN_SYSPAGE_SIZE;
		}
	}else{
		WAN_K2HPRN("System Page size is %ld, it is over minimum size %d", psize, K2HShm::MIN_SYSPAGE_SIZE);
	}

	K2HShm::SystemPageSize = static_cast<size_t>(psize);
	return static_cast<ssize_t>(K2HShm::SystemPageSize);
}

bool K2HShm::CheckSystemLimit(void)
{
	struct rlimit	rlim;

	memset(&rlim, 0, sizeof(struct rlimit));
	if(-1 == getrlimit(RLIMIT_AS, &rlim)){
		ERR_K2HPRN("Could not get RLIMIT_AS system value. errno = %d.", errno);
		return false;
	}
	// [TODO]
	// Now this function only checks(display) rlimit value.
	// Probably RLIMIT_AS value is -1, it means no limiting mmapping...
	// If you need to set value, set it here.
	//
	MSG_K2HPRN("RLIMIT_AS value : current(%jd), maximum(%jd).", static_cast<intmax_t>(rlim.rlim_cur), static_cast<intmax_t>(rlim.rlim_max));

	return true;
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HShm::InitializeFile(const char* file, bool isfullmapping, int mask_bitcnt, int cmask_bitcnt, int max_element_cnt, size_t pagesize)
{
	void*			pShmBase;
	struct timeval	tv;

	K2HShm::GetRealTimeval(tv);

	if(max_element_cnt <= 0){
		ERR_K2HPRN("Maximum element count(%d) must be over 1.", max_element_cnt);
		return false;
	}
	if(mask_bitcnt < K2HShm::MIN_MASK_BITCOUNT && K2HShm::MAX_MASK_BITCOUNT < mask_bitcnt){
		// Warning
		WAN_K2HPRN("Mask bit count(%d) for hash should be from %d to %d.", mask_bitcnt, K2HShm::MIN_MASK_BITCOUNT, K2HShm::MAX_MASK_BITCOUNT);
	}
	if(K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT < cmask_bitcnt){
		// Warning
		WAN_K2HPRN("Collision Mask bit count(%d) for hash should be under %d.", cmask_bitcnt, K2HShm::DEFAULT_COLLISION_MASK_BITCOUNT);
	}
	if(pagesize < static_cast<size_t>(K2HShm::MIN_PAGE_SIZE)){
		// Warning
		WAN_K2HPRN("Page size(%zu) is under minimum size(%d), so pagesize set minimum size.", pagesize, K2HShm::MIN_PAGE_SIZE);
		pagesize = static_cast<size_t>(K2HShm::MIN_PAGE_SIZE);
	}

	// For clear old mmapinfo before setting new file path
	//
	string	oldshmpath	= ShmPath;

	// Open file if need
	if(ISEMPTYSTR(file)){
		ShmFd			= -1;
		ShmPath			= "";
		isAnonMem		= true;
		isFullMapping	= true;
		isReadMode		= false;
	}else{
		if(-1 == (ShmFd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))){
			ERR_K2HPRN("Could not create file(%s), errno = %d", file, errno);
			return false;
		}
		ShmPath			= file;
		isAnonMem		= false;
		isFullMapping	= isfullmapping;
		isReadMode		= false;
	}

	// Clear old mmapinfo before setting new file path
	//
	if(!MmapInfos.ReplaceMapInfo(oldshmpath.c_str())){
		ERR_K2HPRN("Could not build mapping information for file(%s)", ShmPath.c_str());
		return false;
	}

	// build monitor file
	if(!isAnonMem && !isTemporary && !FileMon.Open(ShmPath.c_str(), true)){		// with write lock inode
		ERR_K2HPRN("Could not open and initialize monitor file.");
		Clean(true);
		return false;
	}

	// Initial area total size.
	//
	// K2H					K2Hash Header structure area
	// KINDEX * X			Key Index area				= Key index size * initial count
	// CKINDEX * Y			Collision Key Index area	= Collision Key Index size * initial count
	// ELEMENT * Y * Z		Element area				= Element size * initial collision key count * element coefficient
	// PAGE * (Y * Z) * P	Page area					= page size * Element count * page coefficient
	//
	// *** All area must align page size. ***
	//
	INITAREAMMAP	area_mmap[INITAREAMMAP_SIZE];
	size_t			mmap_size;
	size_t			total_size;
	{
		// KINDEX
		area_mmap[INITAREAMMAP_POS_KINDEX].file_offset	= ALIGNMENT(sizeof(K2H), K2HShm::SystemPageSize);
		area_mmap[INITAREAMMAP_POS_KINDEX].length		= sizeof(KINDEX) * INIT_KINDEX_CNT(mask_bitcnt);
		// CKINDEX
		area_mmap[INITAREAMMAP_POS_CKINDEX].file_offset	= ALIGNMENT(area_mmap[INITAREAMMAP_POS_KINDEX].file_offset + area_mmap[INITAREAMMAP_POS_KINDEX].length, K2HShm::SystemPageSize);
		area_mmap[INITAREAMMAP_POS_CKINDEX].length		= sizeof(CKINDEX) * INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt);
		// ELEMENT
		area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset	= ALIGNMENT(area_mmap[INITAREAMMAP_POS_CKINDEX].file_offset + area_mmap[INITAREAMMAP_POS_CKINDEX].length, K2HShm::SystemPageSize);
		area_mmap[INITAREAMMAP_POS_ELEMENT].length		= sizeof(ELEMENT) * INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO;
		// PAGE
		area_mmap[INITAREAMMAP_POS_PAGE].file_offset	= ALIGNMENT(area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset + area_mmap[INITAREAMMAP_POS_ELEMENT].length, K2HShm::SystemPageSize);
		area_mmap[INITAREAMMAP_POS_PAGE].length			= pagesize * (INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO) * K2HShm::PAGE_CNT_RATIO;

		// MMAP/TOTAL SIZE
		if(isFullMapping){
			mmap_size	= area_mmap[INITAREAMMAP_POS_PAGE].file_offset + area_mmap[INITAREAMMAP_POS_PAGE].length;
			total_size	= mmap_size;
		}else{
			mmap_size	= area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset + area_mmap[INITAREAMMAP_POS_ELEMENT].length;
			total_size	= area_mmap[INITAREAMMAP_POS_PAGE].file_offset + area_mmap[INITAREAMMAP_POS_PAGE].length;
		}

		// ZERO CLEAR TO FILE
		if(!isAnonMem && !K2HShm::InitializeFileZero(ShmFd, 0L, total_size)){
			ERR_K2HPRN("Could not initialize(zero clear) file(%s)", file);
			Clean(true);
			return false;
		}

		// MAPPING for initializing
		if(isAnonMem){
			if(MAP_FAILED == (pShmBase = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0))){
				ERR_K2HPRN("Could not mmap anonymous, errno = %d", errno);
				Clean(true);
				return false;
			}
		}else{
			if(MAP_FAILED == (pShmBase = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, ShmFd, 0))){
				ERR_K2HPRN("Could not mmap file(%s), errno = %d", file, errno);
				Clean(true);
				return false;
			}
		}

		// MMAP Address
		area_mmap[INITAREAMMAP_POS_KINDEX].pmmap		= ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_KINDEX].file_offset);
		area_mmap[INITAREAMMAP_POS_CKINDEX].pmmap		= ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_CKINDEX].file_offset);
		area_mmap[INITAREAMMAP_POS_ELEMENT].pmmap		= ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset);
		if(isFullMapping){
			area_mmap[INITAREAMMAP_POS_PAGE].pmmap		= ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_PAGE].file_offset);
		}else{
			area_mmap[INITAREAMMAP_POS_PAGE].pmmap		= NULL;
		}

		// SET MAPPING INFO
		MmapInfos.AddArea(K2H_AREA_K2H,	0UL, pShmBase, sizeof(K2H));
		MmapInfos.AddArea(K2H_AREA_KINDEX,	area_mmap[INITAREAMMAP_POS_KINDEX].file_offset,	ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_KINDEX].file_offset),	area_mmap[INITAREAMMAP_POS_KINDEX].length);
		MmapInfos.AddArea(K2H_AREA_CKINDEX,	area_mmap[INITAREAMMAP_POS_CKINDEX].file_offset,ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_CKINDEX].file_offset),	area_mmap[INITAREAMMAP_POS_CKINDEX].length);
		MmapInfos.AddArea(K2H_AREA_PAGELIST,area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset,ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset),	area_mmap[INITAREAMMAP_POS_ELEMENT].length);
		if(isFullMapping){
			MmapInfos.AddArea(K2H_AREA_PAGE,area_mmap[INITAREAMMAP_POS_PAGE].file_offset,	ADDPTR(pShmBase, area_mmap[INITAREAMMAP_POS_PAGE].file_offset),		area_mmap[INITAREAMMAP_POS_PAGE].length);
		}
	}

	// initialize K2H
	pHead = static_cast<PK2H>(pShmBase);

	memset(pHead->version,		0, K2H_VERSION_LENGTH);
	sprintf(pHead->version,		K2H_VERSION_FORMAT, K2H_VERSION);
	memset(pHead->hash_version,	0, K2H_HASH_FUNC_VER_LENGTH);
	sprintf(pHead->hash_version,"%s", k2h_hash_version());

	pHead->total_size					= total_size;
	pHead->page_size					= pagesize;
	pHead->max_mask						= K2HShm::MakeMask(K2HShm::MAX_MASK_BITCOUNT);
	pHead->min_mask						= K2HShm::MakeMask(K2HShm::MIN_MASK_BITCOUNT);
	pHead->cur_mask						= K2HShm::MakeMask(mask_bitcnt);
	pHead->collision_mask				= K2HShm::MakeMask(cmask_bitcnt);
	pHead->max_element_count			= static_cast<unsigned long>(max_element_cnt);
	pHead->last_update.tv_sec			= 0;				// last_update is updated only by writing data
	pHead->last_update.tv_usec			= 0;				// 
	pHead->last_area_update.tv_sec		= tv.tv_sec;
	pHead->last_area_update.tv_usec		= tv.tv_usec;
	pHead->unassign_area				= total_size;
	pHead->pextra						= NULL;				// Version 1 is NULL.

	// init Areas array
	{
		PK2HAREA pK2hArea = &(pHead->areas[0]);
		for(int	nCnt = 0; nCnt < MAX_K2HAREA_COUNT; pK2hArea++, nCnt++){
			pK2hArea->type			= K2H_AREA_UNKNOWN;
			pK2hArea->file_offset	= 0;
			pK2hArea->length		= 0;
		}
	}

	// Init Collision Key Index area
	{
		PCKINDEX	pCKIndex = static_cast<PCKINDEX>(area_mmap[INITAREAMMAP_POS_CKINDEX].pmmap);
		if(!K2HShm::InitializeCollisionKeyIndexArray(pCKIndex, INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt))){
			ERR_K2HPRN("Failed to initialize Collision Key Index Area.");
			Clean(true);
			return false;
		}
	}

	// Init Key Index area
	{
		PKINDEX		pKeyIndex	= static_cast<PKINDEX>(area_mmap[INITAREAMMAP_POS_KINDEX].pmmap);
		PCKINDEX	pCKIndex	= static_cast<PCKINDEX>(area_mmap[INITAREAMMAP_POS_CKINDEX].pmmap);
		int			cmask_bitcnt= K2HShm::GetMaskBitCount(pHead->collision_mask);
		k2h_hash_t	cur_hash	= 0UL;
		int			kindex_pos	= 0;
		int			ckindex_pos	= 0;
		for(int bitcnt = 0; bitcnt <= mask_bitcnt; bitcnt++){
			int	kindex_count = 1 << (0 < bitcnt ? bitcnt - 1 : bitcnt);			// notice: if mask_bitcnt is 0 or 1, kindex_count is 1, but it is OK!
			if(!K2HShm::InitializeKeyIndexArray(pShmBase, &pKeyIndex[kindex_pos], cur_hash, pHead->cur_mask, cmask_bitcnt, &pCKIndex[ckindex_pos], kindex_count, KINDEX_ASSIGNED)){
				ERR_K2HPRN("Failed to initialize Key Index Area.");
				Clean(true);
				return false;
			}
			pHead->key_index_area[bitcnt] = CVT_REL(pShmBase, &pKeyIndex[kindex_pos], PKINDEX);
			ckindex_pos += kindex_count * CKINDEX_BYKINDEX_CNT(cmask_bitcnt);
			kindex_pos  += kindex_count;
		}
	}

	// Init Free Element Area
	{
		PELEMENT	pElement = static_cast<PELEMENT>(area_mmap[INITAREAMMAP_POS_ELEMENT].pmmap);
		if(!K2HShm::InitializeElementArray(pShmBase, pElement, (INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO))){
			ERR_K2HPRN("Failed to initialize Element Area.");
			Clean(true);
			return false;
		}
		pHead->pfree_elements		= CVT_REL(pShmBase, pElement, PELEMENT);
		pHead->free_element_count	= INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO;
	}

	// Init Page Area
	if(isFullMapping){
		PPAGEHEAD	pPage = static_cast<PPAGEHEAD>(area_mmap[INITAREAMMAP_POS_PAGE].pmmap);
		if(!K2HShm::InitializePageArray(pShmBase, pPage, pagesize, ((INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO) * K2HShm::PAGE_CNT_RATIO))){
			ERR_K2HPRN("Failed to initialize Page Area.");
			Clean(true);
			return false;
		}
		pHead->pfree_pages		= CVT_REL(pShmBase, pPage, PPAGEHEAD);
		pHead->free_page_count	= (INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO) * K2HShm::PAGE_CNT_RATIO;

	}else{
		if(!K2HShm::InitializePageArray(ShmFd, area_mmap[INITAREAMMAP_POS_PAGE].file_offset, pagesize, ((INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO) * K2HShm::PAGE_CNT_RATIO))){
			ERR_K2HPRN("Failed to initialize Page Area.");
			Clean(true);
			return false;
		}
		pHead->pfree_pages		= reinterpret_cast<PPAGEHEAD>(area_mmap[INITAREAMMAP_POS_PAGE].file_offset);
		pHead->free_page_count	= (INIT_CKINDEX_CNT(mask_bitcnt, cmask_bitcnt) * K2HShm::ELEMENT_CNT_RATIO) * K2HShm::PAGE_CNT_RATIO;
	}

	// init Area array
	if(!K2HShm::SetAreasArray(pHead, K2H_AREA_KINDEX, area_mmap[INITAREAMMAP_POS_KINDEX].file_offset, area_mmap[INITAREAMMAP_POS_KINDEX].length)){
		ERR_K2HPRN("Failed to set Key Index Area information into Areas array.");
		Clean(true);
		return false;
	}
	if(!K2HShm::SetAreasArray(pHead, K2H_AREA_CKINDEX, area_mmap[INITAREAMMAP_POS_CKINDEX].file_offset, area_mmap[INITAREAMMAP_POS_CKINDEX].length)){
		ERR_K2HPRN("Failed to set Key Index Area information into Areas array.");
		Clean(true);
		return false;
	}
	if(!K2HShm::SetAreasArray(pHead, K2H_AREA_PAGELIST, area_mmap[INITAREAMMAP_POS_ELEMENT].file_offset, area_mmap[INITAREAMMAP_POS_ELEMENT].length)){
		ERR_K2HPRN("Failed to set Element Area information into Areas array.");
		Clean(true);
		return false;
	}
	if(!K2HShm::SetAreasArray(pHead, K2H_AREA_PAGE, area_mmap[INITAREAMMAP_POS_PAGE].file_offset, area_mmap[INITAREAMMAP_POS_PAGE].length)){
		ERR_K2HPRN("Failed to set Page Area information into Areas array.");
		Clean(true);
		return false;
	}

	// Sync
	if(-1 == msync(pShmBase, mmap_size, MS_ASYNC)){
		ERR_K2HPRN("Failed to sync to file.");
		Clean(true);
		return false;
	}

	// Update monitor file
	if(!isAnonMem && !isTemporary){
		bool is_change		= false;
		bool is_need_check	= false;
		if(!FileMon.UpdateInode(is_change) || !FileMon.UpdateArea(is_need_check)){
			WAN_K2HPRN("Could not update area in monitor file, but continue...");
		}
	}
	return true;
}

bool K2HShm::AttachFile(const char* file, bool isReadOnly, bool isfullmapping)
{
	if(ISEMPTYSTR(file)){
		ERR_K2HPRN("File name is empty.");
		return false;
	}

	// For clear old mmapinfo before setting new file path
	//
	string	oldshmpath	= ShmPath;

	if(!MmapInfos.GetFd(file, ShmFd)){
		if(isReadOnly){
			if(-1 == (ShmFd = open(file, O_RDONLY))){
				ERR_K2HPRN("Could not open(read only) file(%s), errno = %d", file, errno);
				return false;
			}
		}else{
			if(-1 == (ShmFd = open(file, O_RDWR))){
				ERR_K2HPRN("Could not open(write mode) file(%s), errno = %d", file, errno);
				return false;
			}
		}
	}else{
		MSG_K2HPRN("Already set fd(%d) for file(%s) in mmap info.", ShmFd, file);
	}
	ShmPath 		= file;
	isReadMode		= isReadOnly;
	isFullMapping	= isfullmapping;

	// read mode fd
	if(isReadMode){
		K2HLock::AddReadModeFd(ShmFd);
	}

	// Clear old mmapinfo before setting new file path
	//
	if(!MmapInfos.ReplaceMapInfo(oldshmpath.c_str())){
		ERR_K2HPRN("Could not build mapping information for file(%s)", ShmPath.c_str());
		return false;
	}

	// build monitor file
	if(!isTemporary && !FileMon.Open(ShmPath.c_str())){				// with read lock
		ERR_K2HPRN("Could not open and initialize monitor file.");
		Clean(false);
		return false;
	}

	// build all mmap info
	if(!BuildMmapInfo()){
		ERR_K2HPRN("Failed to build mmap info from %s file.", file);
		return false;
	}
	return true;
}

bool K2HShm::BuildMmapInfo(void)
{
	void*	pShmBase;

	if(MmapInfos.IsMmaped()){
		// Already has mapping

		// set PK2H
		if(NULL == (pShmBase = MmapInfos.CvtAbs(0, true, false))){
			ERR_K2HPRN("There is mmap info for file(%s), but ShmBase is NULL.", ShmPath.c_str());
			Clean(false);
			return false;
		}
		pHead = static_cast<PK2H>(pShmBase);

		// wait for loading all mapping
		K2HLock	ALObjUnArea(ShmFd, Rel(&(pHead->unassign_area)), K2HLock::RDLOCK);	// LOCK

		return true;
	}

	// mmap for head
	if(MAP_FAILED == (pShmBase = mmap(NULL, sizeof(K2H), (isReadMode ? PROT_READ : PROT_READ | PROT_WRITE), MAP_SHARED, ShmFd, 0))){
		ERR_K2HPRN("Could not mmap file(%s), errno = %d", ShmPath.c_str(), errno);
		Clean(false);
		return false;
	}

	// mmap start
	MmapInfos.AddArea(K2H_AREA_K2H, 0, pShmBase, sizeof(K2H));

	// set PK2H
	pHead = static_cast<PK2H>(pShmBase);

	// force other thread to wait for loading all mapping
	K2HLock	ALObjUnArea(ShmFd, Rel(&(pHead->unassign_area)), K2HLock::RWLOCK);		// LOCK

	// Check version
	{
		char	szTmpVer[K2H_HASH_FUNC_VER_LENGTH];

		sprintf(szTmpVer, K2H_VERSION_FORMAT, K2H_VERSION);
		if(0 != strcmp(szTmpVer, pHead->version)){
			ERR_K2HPRN("K2HASH file version(\"%s\") is not supported, this library supports only version \"%s\"", pHead->version, szTmpVer);
			ERR_K2HPRN("You can convert k2hash file to newer format by putting archive file by old version tools(libs), and load it by newer tools(libs).");
			Clean(false);
			return false;
		}

		sprintf(szTmpVer, "%s", k2h_hash_version());
		if(0 != strcmp(szTmpVer, pHead->hash_version)){
			ERR_K2HPRN("K2H Hash function version(\"%s\") is not loaded, this library\'s hash function version is \"%s\"", pHead->hash_version, szTmpVer);
			Clean(false);
			return false;
		}
	}

	// Check page size(very important)
	if(pHead->page_size < static_cast<size_t>(K2HShm::MIN_PAGE_SIZE)){
		WAN_K2HPRN("Page size(%zu) is under minimum pagesize(%d).", pHead->page_size, K2HShm::MIN_PAGE_SIZE);
	}

	// mmap loop for all index area
	if(!ExpandMmapInfo()){
		ERR_K2HPRN("Failed to mmap k2hash areas");
		Clean(false);
		return false;
	}
	return true;
}

bool K2HShm::ExpandMmapInfo(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// mmap loop for all index area
	PK2HAREA	pK2hArea = &(pHead->areas[0]);
	void*		pMmap;
	for(int	nCnt = 0; nCnt < MAX_K2HAREA_COUNT && K2H_AREA_UNKNOWN != pK2hArea->type; pK2hArea++, nCnt++){
		// check exists mmap area
		if(0 == pK2hArea->file_offset){
			MSG_K2HPRN("file offset in k2area is 0L, skip it.");
			continue;
		}
		if(NULL != (pMmap = MmapInfos.CvtAbs(pK2hArea->file_offset, false, false))){		// not allow offset=0 and not update
			continue;
		}
		if(!isFullMapping && K2H_AREA_PAGE == pK2hArea->type){
			// not full mapping mode, skip it
			continue;
		}
		// found new mmap area, mmap it
		if(MAP_FAILED == (pMmap = mmap(NULL, pK2hArea->length, PROT_READ | (isReadMode ? 0 : PROT_WRITE), MAP_SHARED, ShmFd, pK2hArea->file_offset))){
			ERR_K2HPRN("Could not mmap file(%s: %jd - %zu), errno = %d", ShmPath.c_str(), static_cast<intmax_t>(pK2hArea->file_offset), pK2hArea->length, errno);
			return false;
		}
		MmapInfos.AddArea(pK2hArea->type, pK2hArea->file_offset, pMmap, pK2hArea->length);
	}
	return true;
}

//
// must lock before calling this method
//
bool K2HShm::ContractMmapInfo(void)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// make now area information into mmapinfo list
	PK2HMMAPINFO	pAreaListTop = new K2HMMAPINFO;
	{
		// add K2H_AREA_K2H type because areas does not have it.
		pAreaListTop->type			= K2H_AREA_K2H;
		pAreaListTop->file_offset	= 0L;
		pAreaListTop->mmap_base		= NULL;
		pAreaListTop->length		= sizeof(K2H);
	}

	PK2HAREA	pK2hArea = &(pHead->areas[0]);
	for(int	nCnt = 0; nCnt < MAX_K2HAREA_COUNT && K2H_AREA_UNKNOWN != pK2hArea->type; pK2hArea++, nCnt++){
		PK2HMMAPINFO	pinfo	= new K2HMMAPINFO;
		pinfo->type				= pK2hArea->type;
		pinfo->file_offset		= pK2hArea->file_offset;
		pinfo->mmap_base		= NULL;
		pinfo->length			= pK2hArea->length;

		k2h_mmap_info_list_add(&pAreaListTop, pinfo);
	}

	// check and munmap
	bool	result = MmapInfos.Unmap(pAreaListTop);

	k2h_mmap_info_list_freeall(&pAreaListTop);

	return result;
}

/*
 * VIM modelines
 *
 * vim:set ts=4 fenc=utf-8:
 */
