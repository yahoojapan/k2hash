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
 * CREATE:   Fri Apr 25 2014
 * REVISION:
 *
 */

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

#include "k2hcommon.h"
#include "k2hshm.h"
#include "k2hpage.h"
#include "k2hpagefile.h"
#include "k2hpagemem.h"
#include "k2hshmupdater.h"
#include "k2hutil.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Area utility for area compress
//---------------------------------------------------------
//
// Returns lastest area.
// But if it is K2H_AREA_PAGELIST and K2H_AREA_PAGE, returns not lastest area.
// K2H_AREA_PAGELIST and K2H_AREA_PAGE must be over one area.
//
PK2HAREA K2HShm::GetLastestArea(void) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}

	// mmap loop for all index area
	for(int nCnt = MAX_K2HAREA_COUNT; 0 < nCnt; nCnt--){
		if(K2H_AREA_UNKNOWN != (pHead->areas[nCnt - 1]).type){
			// check same area for PAGELIST and PAGE
			if(K2H_AREA_PAGELIST == (pHead->areas[nCnt - 1]).type || K2H_AREA_PAGE == (pHead->areas[nCnt - 1]).type){
				bool	isfound = false;
				for(int nCnt2 = nCnt - 1; 0 < nCnt2; nCnt2--){
					if((pHead->areas[nCnt - 1]).type == (pHead->areas[nCnt2 - 1]).type){
						isfound = true;
						break;
					}
				}
				if(!isfound){
					// there is not same type area before pK2HArea.
					MSG_K2HPRN("### type = %ld is first area in k2hash, so it can not be removed.", (pHead->areas[nCnt - 1]).type);
					return NULL;
				}
			}
			return &pHead->areas[nCnt - 1];
		}
	}
	return NULL;
}

bool K2HShm::PrintAreaInfo(void) const
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	fprintf(stdout, "K2HASH AREA [%d] = {\n", MAX_K2HAREA_COUNT);

	for(int nCnt = 0; nCnt < MAX_K2HAREA_COUNT; nCnt++){
		string	strType;
		if(K2H_AREA_UNKNOWN == (pHead->areas[nCnt]).type){
			strType = "UNASSIGNMENT";
		}else{
			strType = "";
			if(K2H_AREA_K2H == ((pHead->areas[nCnt]).type & K2H_AREA_K2H)){
				strType += "HEAD";
			}
			if(K2H_AREA_KINDEX == ((pHead->areas[nCnt]).type & K2H_AREA_KINDEX)){
				if(strType.length()){
					strType += ",";
				}
				strType += "KEYINDEX";
			}
			if(K2H_AREA_CKINDEX == ((pHead->areas[nCnt]).type & K2H_AREA_CKINDEX)){
				if(strType.length()){
					strType += ",";
				}
				strType += "CKEYINDEX";
			}
			if(K2H_AREA_PAGELIST == ((pHead->areas[nCnt]).type & K2H_AREA_PAGELIST)){
				if(strType.length()){
					strType += ",";
				}
				strType += "ELEMENT";
			}
			if(K2H_AREA_PAGE == ((pHead->areas[nCnt]).type & K2H_AREA_PAGE)){
				if(strType.length()){
					strType += ",";
				}
				strType += "PAGE";
			}
		}
		fprintf(stdout, "    No.%d\ttype = %s AREA(%ld),\tfile offset=%jd,\tarea length=%zu\n", nCnt + 1, strType.c_str(), (pHead->areas[nCnt]).type, static_cast<intmax_t>((pHead->areas[nCnt]).file_offset), (pHead->areas[nCnt]).length);
	}
	fprintf(stdout, "}\n");

	return true;
}

//---------------------------------------------------------
// Replace elements for area compress
//---------------------------------------------------------
// [NOTICE]
// ALObjFEC must be RWLOCK or UNLOCK before calling this method
//
PELEMENT K2HShm::ReserveElement(void* pRelExpArea, size_t ExpLength)
{
	if(!pRelExpArea || 0UL == ExpLength){
		ERR_K2HPRN("Parameter is wrong.");
		return NULL;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return NULL;
	}
	K2HLock		ALObjFEC(ShmFd, Rel(&(pHead->free_element_count)), K2HLock::RWLOCK);	// LOCK

	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress constVariablePointer
	const PELEMENT	pStartPos	= reinterpret_cast<PELEMENT>(pRelExpArea);
	// cppcheck-suppress unmatchedSuppression
	// cppcheck-suppress constVariablePointer
	const PELEMENT	pLastPos	= ADDPTR(reinterpret_cast<PELEMENT>(pRelExpArea), static_cast<off_t>(ExpLength));

	for(PELEMENT pElement = static_cast<PELEMENT>(Abs(pHead->pfree_elements)), pRelElement = pHead->pfree_elements; pElement; pElement = static_cast<PELEMENT>(Abs(pElement->same)), pRelElement = pElement->same){
		// check target area
		if(pRelElement < pStartPos || pLastPos <= pRelElement){
			// found element and cut off it.
			PELEMENT	pPrevElement = static_cast<PELEMENT>(Abs(pElement->parent));
			PELEMENT	pNextElement = static_cast<PELEMENT>(Abs(pElement->same));
			if(pPrevElement){
				pPrevElement->same = pElement->same;
			}else{
				pHead->pfree_elements = pElement->same;
			}
			if(pNextElement){
				pNextElement->parent = pElement->parent;
			}

			pElement->parent= NULL;
			pElement->same	= NULL;
			pElement->small	= NULL;
			pElement->big	= NULL;

			if(0 < pHead->free_element_count){
				pHead->free_element_count -= 1UL;
			}
			return pElement;
		}
	}
	// not found
	return NULL;
}

// [NOTICE]
// ALObjFEC must be RWLOCK or UNLOCK before calling this method
//
bool K2HShm::ReplaceElement(PELEMENT pElement, void* pRelExpArea, size_t ExpLength)
{
	if(!pRelExpArea || 0UL == ExpLength){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// check in area
	PELEMENT	pStartPos	= reinterpret_cast<PELEMENT>(pRelExpArea);
	PELEMENT	pLastPos	= ADDPTR(reinterpret_cast<PELEMENT>(pRelExpArea), static_cast<off_t>(ExpLength));
	PELEMENT	pRelElement	= reinterpret_cast<PELEMENT>(Rel(pElement));
	if(pRelElement < pStartPos || pLastPos <= pRelElement){
		// element is not in target area
		return true;
	}

	// reserve new element
	PELEMENT	pNewElement;
	if(NULL == (pNewElement = ReserveElement(pRelExpArea, ExpLength))){		// ALObjFEC must be RWLOCK or UNLOCK
		MSG_K2HPRN("Could not get new element.(ex: no left space)");
		return false;
	}

	// lock ckindex
	K2HLock		ALObjCKI(K2HLock::RWLOCK);		// LOCK
	PCKINDEX	pCKIndex;
	if(NULL == (pCKIndex = GetCKIndex(pElement->hash, ALObjCKI))){
		MSG_K2HPRN("Could not get ckindex.");
		return false;
	}

	// copy
	pNewElement->small		= pElement->small;
	pNewElement->big		= pElement->big;
	pNewElement->parent		= pElement->parent;
	pNewElement->same		= pElement->same;
	pNewElement->hash		= pElement->hash;
	pNewElement->subhash	= pElement->subhash;
	pNewElement->key		= pElement->key;
	pNewElement->value		= pElement->value;
	pNewElement->subkeys	= pElement->subkeys;
	pNewElement->attrs		= pElement->attrs;
	pNewElement->keylength	= pElement->keylength;
	pNewElement->vallength	= pElement->vallength;
	pNewElement->skeylength	= pElement->skeylength;
	pNewElement->attrlength	= pElement->attrlength;

	// change list
	PELEMENT	pRelNew	= reinterpret_cast<PELEMENT>(Rel(pNewElement));
	PELEMENT	pSmall	= reinterpret_cast<PELEMENT>(Abs(pElement->small));
	PELEMENT	pBig	= reinterpret_cast<PELEMENT>(Abs(pElement->big));
	PELEMENT	pParent	= reinterpret_cast<PELEMENT>(Abs(pElement->parent));
	PELEMENT	pSame	= reinterpret_cast<PELEMENT>(Abs(pElement->same));
	if(pSmall){
		pSmall->parent	= pRelNew;
	}
	if(pBig){
		pBig->parent	= pRelNew;
	}
	if(pSame){
		pSame->parent	= pRelNew;
	}
	if(pParent){
		if(pParent->small == pRelElement){
			pParent->small = pRelNew;
		}else if(pParent->big == pRelElement){
			pParent->big = pRelNew;
		}else if(pParent->same == pRelElement){
			pParent->same = pRelNew;
		}else{
			ERR_K2HPRN("Found broken element list!!!");

			// [NOTICE]
			// ALObjFEC must be lock RWLOCK, or UNLOCK.
			//
			if(!PutBackElement(pNewElement)){
				ERR_K2HPRN("[FATAL] Failed to putback element, this element is leaked!!!");
			}
			return false;
		}
	}else{
		pCKIndex->element_list = pRelNew;
	}

	if(!PutBackElement(pElement)){
		ERR_K2HPRN("[FATAL] Failed to putback element, this element is leaked!!!");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// Replace pages for area compress
//---------------------------------------------------------
//
// Utility method, for compressing k2hash area.
//
// [NOTICE]
// must lock free_page_count before calling this method.
//
K2HPage* K2HShm::CutOffPage(PPAGEHEAD pCurPageHead, PPAGEHEAD* ppTopPageHead, PPAGEHEAD pSetPrevPageHead)
{
	if(!pCurPageHead){
		ERR_K2HPRN("Parameter is wrong.");
		return NULL;
	}

	// First: make page objects because it is fatal error if fail to set headers.
	PAGEHEAD	CurPageHead;
	K2HPage*	pCurPage;
	K2HPage*	pPrevPage = NULL;
	K2HPage*	pNextPage = NULL;
	if(NULL == (pCurPage = GetPageObject(pCurPageHead, false))){
		ERR_K2HPRN("Could not make current page object.");
		return NULL;
	}
	if(!pCurPage->GetPageHead(&CurPageHead)){
		ERR_K2HPRN("Could not get current page head.");
		K2H_Delete(pCurPage);
		return NULL;
	}
	if(CurPageHead.prev && NULL == (pPrevPage = GetPageObject(CurPageHead.prev, false))){
		ERR_K2HPRN("Could not make previous page object.");
		K2H_Delete(pCurPage);
		return NULL;
	}
	if(CurPageHead.next && NULL == (pNextPage = GetPageObject(CurPageHead.next, false))){
		ERR_K2HPRN("Could not make next page object.");
		K2H_Delete(pCurPage);
		K2H_Delete(pPrevPage);
		return NULL;
	}

	// change previous object's next pointer
	if(pPrevPage){
		// set next pointer to prev object's next.
		if(!pPrevPage->SetPageHead(K2HPage::SETHEAD_NEXT, NULL, CurPageHead.next)){
			// This error is not fatal.
			ERR_K2HPRN("Could not set next pointer to previous object.");
			K2H_Delete(pCurPage);
			K2H_Delete(pPrevPage);
			K2H_Delete(pNextPage);
			return NULL;
		}
	}else{
		// set next pointer to first free list.
		if(ppTopPageHead){
			(*ppTopPageHead) = CurPageHead.next;
		}
	}
	K2H_Delete(pPrevPage);

	// change next object's prev pointer
	if(pNextPage){
		// set next pointer to prev object's next.
		if(!pNextPage->SetPageHead(K2HPage::SETHEAD_PREV, CurPageHead.prev)){
			// [NOTES]
			// If ppTopPageHead is pfree_pages, this error is not fatal NOW!
			// Because the pfree_pages's normal order is not broken, but only broken inverse order, and NOW not use inverse order.
			// If using inverse order, this error is FATAL.
			//
			ERR_K2HPRN("Could not set previous pointer to next object. (please see a comment in source code!)");
			K2H_Delete(pCurPage);
			K2H_Delete(pNextPage);
			return NULL;
		}
	}else{
		// not need to set previous pointer.
	}
	K2H_Delete(pNextPage);

	// set start page object.
	if(!pCurPage->SetPageHead(K2HPage::SETHEAD_NEXT | K2HPage::SETHEAD_PREV, pSetPrevPageHead, NULL)){
		ERR_K2HPRN("[FATAL] Could not set prev and next pointer to current object, this object is leaked!!!");
		K2H_Delete(pCurPage);
		return NULL;
	}

	return pCurPage;
}

//
// pSrcTop page chain list count must be as same as pDestTop one.
//
bool K2HShm::CopyPageData(K2HPage* pSrcTop, K2HPage* pDestTop)
{
	if(!pSrcTop || !pDestTop){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	size_t			onelength = GetPageSize() - PAGEHEAD_SIZE;
	unsigned char*	byBuff;
	if(NULL == (byBuff = reinterpret_cast<unsigned char*>(malloc(onelength)))){
		ERR_K2HPRN("Could not allocate memory.");
		return false;
	}

	K2HPage*	pSrc;
	K2HPage*	pDest;
	K2HPage*	pSrcTmp					= NULL;
	K2HPage*	pDestTmp				= NULL;
	off_t		read_offset				= 0L;
	off_t		write_offset			= 0L;
	off_t		next_read_pos;
	off_t		next_write_pos;
	bool		isChangeDestPageLength	= false;
	for(pSrc = pSrcTop, pDest = pDestTop; true; read_offset = next_read_pos, write_offset = next_write_pos){
		size_t	readlength;

		// read one page data.
		readlength		= 0UL;
		next_read_pos	= 0L;
		if(NULL == (pSrcTmp = pSrc->LoadData(read_offset, onelength, &byBuff, readlength, next_read_pos))){
			ERR_K2HPRN("Failed to read data from source page object.");

			if(pSrc != pSrcTop){
				K2H_Delete(pSrc);
			}
			if(pDest != pDestTop){
				K2H_Delete(pDest);
			}
			K2H_Free(byBuff);
			return false;
		}
		if(pSrcTmp == pSrc){
			if(read_offset == next_read_pos){
				// finish to read
				if(pSrc != pSrcTop){
					K2H_Delete(pSrc);
				}
				break;
			}
		}else{
			if(pSrc != pSrcTop){
				K2H_Delete(pSrc);
			}
			pSrc = pSrcTmp;
		}
		if(0UL == readlength){
			// finish to read ?
			if(pSrc != pSrcTop){
				K2H_Delete(pSrc);
			}
			break;
		}

		// write data
		next_write_pos = 0L;
		if(NULL == (pDestTmp = pDest->SetData(byBuff, readlength, write_offset, isChangeDestPageLength, next_write_pos))){
			ERR_K2HPRN("Failed to write data to destination page object.");

			if(pSrc != pSrcTop){
				K2H_Delete(pSrc);
			}
			if(pDest != pDestTop){
				K2H_Delete(pDest);
			}
			K2H_Free(byBuff);
			return false;
		}
		if(pDestTmp != pDest){
			if(pDest != pDestTop){
				K2H_Delete(pDest);
			}
			pDest = pDestTmp;
		}
		if(isChangeDestPageLength){
			MSG_K2HPRN("destination page object length is changed, probably it is safe because of it is not added page object.");
		}
	}
	K2H_Free(byBuff);

	return true;
}

//
// Reserve pages without expectation address range.
// This method for compressing area.
//
K2HPage* K2HShm::ReservePages(size_t length, void* pRelExpArea, size_t ExpLength)
{
	if(0UL == length || !pRelExpArea || 0UL == ExpLength){
		ERR_K2HPRN("Parameter is wrong.");
		return NULL;
	}
	K2HLock	ALObjFPC(ShmFd, Rel(&(pHead->free_page_count)), K2HLock::RWLOCK);		// LOCK

	// Check enough page for length
	if(((GetPageSize() - PAGEHEAD_SIZE) * (pHead->free_page_count)) < length){
		ERR_K2HPRN("Not enough pages for reserve.");
		return NULL;
	}

	// Get page head and tail
	K2HPage*	pLastPage		= NULL;
	K2HPage*	pStartPage		= NULL;
	PPAGEHEAD	pStartExpPos	= reinterpret_cast<PPAGEHEAD>(pRelExpArea);
	PPAGEHEAD	pLastExpPos		= ADDPTR(reinterpret_cast<PPAGEHEAD>(pRelExpArea), static_cast<off_t>(ExpLength));
	PPAGEHEAD	pLastPageHead	= NULL;
	PAGEHEAD	CurPageHead;
	for(K2HPage* pCurPage = GetPageObject(pHead->pfree_pages, false); pCurPage; pCurPage = GetPageObject(CurPageHead.next, false)){
		// keep page head
		if(!pCurPage->GetPageHead(&CurPageHead)){
			ERR_K2HPRN("Could not get current page head.");
			K2H_Delete(pCurPage);
			break;
		}
		// keep current page relative page head
		PPAGEHEAD	pRelCurPageHead = pCurPage->GetPageHeadRelAddress();

		K2H_Delete(pCurPage);

		// check target area
		if(pStartExpPos <= pRelCurPageHead && pRelCurPageHead < pLastExpPos){
			// this page is expectation area, skip it.
			if(!CurPageHead.next){
				break;
			}
			continue;
		}

		//
		// cut off current page object from free list.(with set previous pointer to current object)
		//
		K2HPage*	pOneReservePage;
		if(NULL == (pOneReservePage = CutOffPage(pRelCurPageHead, &(pHead->pfree_pages), pLastPageHead))){
			ERR_K2HPRN("Could not reserve one page,");
			break;
		}

		// decrement free count
		if(0L < pHead->free_page_count){
			pHead->free_page_count--;
		}

		// set next pointer to last object
		if(pLastPage && !pLastPage->SetPageHead(K2HPage::SETHEAD_NEXT, NULL, pRelCurPageHead)){
			ERR_K2HPRN("Could not set next pointer to last object,");
			break;
		}

		// set pointer for next loop
		if(pStartPage != pLastPage){
			K2H_Delete(pLastPage);
		}
		if(!pStartPage){
			pStartPage	= pOneReservePage;
		}
		pLastPage		= pOneReservePage;
		pLastPageHead	= pRelCurPageHead;

		// check & decrement length
		if(length <= (GetPageSize() - PAGEHEAD_SIZE)){
			// finish
			length = 0UL;
			break;
		}
		length -= (GetPageSize() - PAGEHEAD_SIZE);

		if(!CurPageHead.next){
			break;
		}
	}

	// check length
	if(0UL < length){
		MSG_K2HPRN("There is no more free page. try to put back reserved pages.");

		// there is not enough pages, so try to put back pages.
		if(pStartPage && !pStartPage->Free()){
			ERR_K2HPRN("[FATAL] Could not put back reserved pages, these objects are leaked!!!");		// LEAK!!!
		}
		if(pStartPage != pLastPage){
			K2H_Delete(pLastPage);
		}
		K2H_Delete(pStartPage);
		return NULL;
	}
	if(pStartPage != pLastPage){
		K2H_Delete(pLastPage);
	}
	return pStartPage;
}

bool K2HShm::ReplacePageInElement(PELEMENT pElement, int type, void* pRelExpArea, size_t ExpLength)
{
	if(!pElement){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	// get target page head
	PPAGEHEAD	pRelPageHead= NULL;
	size_t		length		= 0UL;
	if(PAGEOBJ_KEY == type){
		if(0UL == pElement->keylength){
			return true;
		}
		pRelPageHead= pElement->key;
		length		= pElement->keylength;
	}else if(PAGEOBJ_VALUE == type){
		if(0UL == pElement->vallength){
			return true;
		}
		pRelPageHead= pElement->value;
		length		= pElement->vallength;
	}else if(PAGEOBJ_SUBKEYS == type){
		if(0UL == pElement->skeylength){
			return true;
		}
		pRelPageHead= pElement->subkeys;
		length		= pElement->skeylength;
	}else if(PAGEOBJ_ATTRS == type){
		if(0UL == pElement->attrlength){
			return true;
		}
		pRelPageHead= pElement->attrs;
		length		= pElement->attrlength;
	}else{
		ERR_K2HPRN("type(%d) is not defined.", type);
		return false;
	}
	if(!pRelPageHead){
		WAN_K2HPRN("PPAGEHEAD is NULL(something wrong), but skip this error.");
		return true;
	}

	// check target area
	PPAGEHEAD	pStartExpPos	= reinterpret_cast<PPAGEHEAD>(pRelExpArea);
	PPAGEHEAD	pLastExpPos		= ADDPTR(reinterpret_cast<PPAGEHEAD>(pRelExpArea), static_cast<off_t>(ExpLength));
	if(pRelPageHead < pStartExpPos || pLastExpPos <= pRelPageHead){
		// this page is not expectation area, skip it.
		return true;
	}

	// get page it
	K2HPage*	pTargetPage;
	if(NULL == (pTargetPage = GetPageObject(pRelPageHead, false))){
		ERR_K2HPRN("Could not make & load page object.");
		return false;
	}

	// reserve page
	K2HPage*	pNewPage;
	if(NULL == (pNewPage = ReservePages(length, pRelExpArea, ExpLength))){
		ERR_K2HPRN("Could not reserve new page area.");
		K2H_Delete(pTargetPage);
		return false;
	}

	// copy data
	if(!CopyPageData(pTargetPage, pNewPage)){
		ERR_K2HPRN("Could not copy page data to reserve new page.");

		// try to put back reserved pages
		if(!pNewPage->Free()){
			ERR_K2HPRN("[FATAL] Could not put back reserved pages, these objects are leaked!!!");
		}
		K2H_Delete(pNewPage);
		K2H_Delete(pTargetPage);

		return false;
	}

	// replace pagehead in element
	if(PAGEOBJ_KEY == type){
		pElement->key = pNewPage->GetPageHeadRelAddress();
	}else if(PAGEOBJ_VALUE == type){
		pElement->value = pNewPage->GetPageHeadRelAddress();
	}else if(PAGEOBJ_SUBKEYS == type){
		pElement->subkeys = pNewPage->GetPageHeadRelAddress();
	}else{	// PAGEOBJ_ATTRS == type
		pElement->attrs = pNewPage->GetPageHeadRelAddress();
	}

	// putback old page objects
	if(!pTargetPage->Free()){
		ERR_K2HPRN("[FATAL] Could not put back old pages, these objects are leaked!!!");
	}
	K2H_Delete(pNewPage);
	K2H_Delete(pTargetPage);

	return true;
}

bool K2HShm::ReplacePagesInElement(PELEMENT pElement, void* pRelExpArea, size_t ExpLength)
{
	if(!pElement){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}

	if(	!ReplacePageInElement(pElement, PAGEOBJ_KEY, pRelExpArea, ExpLength)	||
		!ReplacePageInElement(pElement, PAGEOBJ_VALUE, pRelExpArea, ExpLength)	||
		!ReplacePageInElement(pElement, PAGEOBJ_SUBKEYS, pRelExpArea, ExpLength)||
		!ReplacePageInElement(pElement, PAGEOBJ_ATTRS, pRelExpArea, ExpLength)	)
	{
		ERR_K2HPRN("Failed to replace page data in expectation area to no-expectation area.");
		return false;
	}
	return true;
}

//---------------------------------------------------------
// AreaCompress
//---------------------------------------------------------
bool K2HShm::AreaCompress(bool& isCompressed)
{
	if(!IsAttached()){
		ERR_K2HPRN("There is no attached K2HASH.");
		return false;
	}
	isCompressed = false;

	K2HFILE_UPDATE_CHECK(this);

	K2HLock	ALObjUnArea(ShmFd, Rel(&(pHead->unassign_area)), K2HLock::RWLOCK);	// LOCK

	// get lastest information
	PK2HAREA	pLastestArea;
	if(NULL == (pLastestArea = GetLastestArea())){
		MSG_K2HPRN("Failed to get lastest area information, probably could not find a removable area.");
		return true;
	}
	long	type		= pLastestArea->type;
	off_t	ExpOffset	= pLastestArea->file_offset;
	size_t	ExpLength	= pLastestArea->length;

	// compress
	if(type == K2H_AREA_PAGE){
		// replace pages to another area
		for(PELEMENT pElement = static_cast<PELEMENT>(MmapInfos.begin(K2H_AREA_PAGELIST)); pElement; pElement = static_cast<PELEMENT>(MmapInfos.next(pElement, sizeof(ELEMENT)))){
			if(pElement->key){
				// For lock
				K2HLock	ALObjCKI(K2HLock::RWLOCK);		// LOCK
				if(NULL != GetCKIndex(pElement->hash, ALObjCKI)){
					// check & replace page data
					if(!ReplacePagesInElement(pElement, reinterpret_cast<void*>(ExpOffset), ExpLength)){
						ERR_K2HPRN("Something error occurred in replacing page objects, one of case is nomore space for moving data.");
						return false;
					}
				}
			}
		}

		K2HLock	ALObjFPC(ShmFd, Rel(&(pHead->free_page_count)), K2HLock::RWLOCK);		// LOCK

		// check to remove lastest page area( is all expectation pages in free page list? )
		PPAGEHEAD		pStartPos		= reinterpret_cast<PPAGEHEAD>(ExpOffset);
		PPAGEHEAD		pLastPos		= ADDPTR(reinterpret_cast<PPAGEHEAD>(ExpOffset), static_cast<off_t>(ExpLength));
		unsigned long	area_page_count = static_cast<unsigned long>(ExpLength / GetPageSize());
		unsigned long	page_count		= 0UL; 
		PAGEHEAD		CurPageHead;
		for(K2HPage* pCurPage = GetPageObject(pHead->pfree_pages, false); pCurPage; pCurPage = GetPageObject(CurPageHead.next, false)){
			// cppcheck-suppress unmatchedSuppression
			// cppcheck-suppress constVariablePointer
			const PPAGEHEAD	pRelCurPageHead = pCurPage->GetPageHeadRelAddress();
			if(pStartPos <= pRelCurPageHead && pRelCurPageHead < pLastPos){
				// found
				page_count++;
			}

			if(!pCurPage->GetPageHead(&CurPageHead)){
				// why?
				K2H_Delete(pCurPage);
				break;
			}
			K2H_Delete(pCurPage);

			if(area_page_count <= page_count){
				break;
			}
			if(NULL == CurPageHead.next){
				break;
			}
		}
		if(page_count < area_page_count){
			ERR_K2HPRN("Could not compress area no more.");
			return true;
		}

		// remove expectation pages from free page list.
		for(K2HPage* pCurPage = GetPageObject(pHead->pfree_pages, false); pCurPage; pCurPage = GetPageObject(CurPageHead.next, false)){
			// at first for getting next pagehead.
			if(!pCurPage->GetPageHead(&CurPageHead)){
				// why?
				ERR_K2HPRN("[FATAL] Something error occurred, some page objects are leaked!!!");			// LEAK!!!
				K2H_Delete(pCurPage);
				break;
			}

			// get relative page head
			PPAGEHEAD	pRelCurPageHead = pCurPage->GetPageHeadRelAddress();
			K2H_Delete(pCurPage);

			// check
			if(pStartPos <= pRelCurPageHead && pRelCurPageHead < pLastPos){
				// cut off current page from list
				if(NULL == (pCurPage = CutOffPage(pRelCurPageHead, &(pHead->pfree_pages), NULL))){
					ERR_K2HPRN("[FATAL] Could not cut off a page, some page objects are leaked!!!");		// LEAK!!!
					return false;
				}
				K2H_Delete(pCurPage);

				// decrement free count
				if(0L < pHead->free_page_count){
					pHead->free_page_count--;
				}
			}

			if(NULL == CurPageHead.next){
				break;
			}
		}

		// recheck( is there no expectation pages in free page list? )
		bool	isFoundPage = false;
		for(K2HPage* pCurPage = GetPageObject(pHead->pfree_pages, false); pCurPage; pCurPage = GetPageObject(CurPageHead.next, false)){
			PPAGEHEAD	pRelCurPageHead = pCurPage->GetPageHeadRelAddress();
			if(pStartPos <= pRelCurPageHead && pRelCurPageHead < pLastPos){
				// found
				K2H_Delete(pCurPage);
				isFoundPage = true;
				break;
			}

			if(!pCurPage->GetPageHead(&CurPageHead)){
				// why?
				ERR_K2HPRN("[FATAL] Something error occurred, some page objects are leaked!!!");		// LEAK!!!
				K2H_Delete(pCurPage);
				return false;
			}
			K2H_Delete(pCurPage);

			if(NULL == CurPageHead.next){
				break;
			}
		}
		if(isFoundPage){
			ERR_K2HPRN("[FATAL] Could not free lastest area and, some page objects are leaked!!!");		// LEAK!!!
			return false;
		}

	}else if(type == K2H_AREA_PAGELIST){
		// replace element to another area
		K2HLock	ALObjFEC(ShmFd, Rel(&(pHead->free_element_count)), K2HLock::RWLOCK);	// LOCK
		for(PELEMENT pElement = static_cast<PELEMENT>(MmapInfos.begin(K2H_AREA_PAGELIST)); pElement; pElement = static_cast<PELEMENT>(MmapInfos.next(pElement, sizeof(ELEMENT)))){
			if(pElement->key){
				// replace
				if(!ReplaceElement(pElement, reinterpret_cast<PELEMENT>(ExpOffset), ExpLength)){
					MSG_K2HPRN("Something error occurred or no more element space.");
					return false;
				}
			}
		}

		// check to remove lastest element area( is all expectation element in free page list? )
		PELEMENT		pStartPos			= reinterpret_cast<PELEMENT>(ExpOffset);
		PELEMENT		pLastPos			= ADDPTR(reinterpret_cast<PELEMENT>(ExpOffset), static_cast<off_t>(ExpLength));
		unsigned long	area_element_count	= static_cast<unsigned long>(ExpLength / sizeof(ELEMENT));
		unsigned long	element_count		= 0UL; 
		for(PELEMENT pCurElement = pHead->pfree_elements; pCurElement; pCurElement = static_cast<PELEMENT>(Abs(pCurElement))->same){
			if(pStartPos <= pCurElement && pCurElement < pLastPos){
				// found
				element_count++;
			}
		}
		if(element_count < area_element_count){
			ERR_K2HPRN("Could not compress area no more.");
			return true;
		}

		// remove expectation element from free element list.
		for(PELEMENT pCurElement = pHead->pfree_elements; pCurElement; pCurElement = static_cast<PELEMENT>(Abs(pCurElement))->same){
			if(pStartPos <= pCurElement && pCurElement < pLastPos){
				// cut off current element from list
				PELEMENT	pPrevElement = static_cast<PELEMENT>(Abs(pCurElement))->parent;
				PELEMENT	pNextElement = static_cast<PELEMENT>(Abs(pCurElement))->same;
				if(pPrevElement){
					static_cast<PELEMENT>(Abs(pPrevElement))->same = pNextElement;
				}else{
					pHead->pfree_elements = pNextElement;
				}
				if(pNextElement){
					static_cast<PELEMENT>(Abs(pNextElement))->parent = pPrevElement;
				}
				// decrement free count
				if(0 < pHead->free_element_count){
					pHead->free_element_count -= 1UL;
				}
			}
		}

		// recheck( is there no expectation pages in free element list? )
		for(PELEMENT pCurElement = pHead->pfree_elements; pCurElement; pCurElement = static_cast<PELEMENT>(Abs(pCurElement))->same){
			if(pStartPos <= pCurElement && pCurElement < pLastPos){
				// found
				ERR_K2HPRN("[FATAL] Could not free lastest area and, some element objects are leaked!!!");		// LEAK!!!
				return false;
			}
		}

	}else{
		MSG_K2HPRN("Lastest area type(%ld) is not target type.", type);
		return true;
	}

	// munmap lastest area
	if(isFullMapping || K2H_AREA_PAGE != type){
		if(!MmapInfos.Unmap(type, ExpOffset, ExpLength)){
			MSG_K2HPRN("Failed to munmap area, because already does not have mmap area. it is no problem, then continue...");
		}
	}

	// area init
	pHead->unassign_area		= ExpOffset;
	pLastestArea->type			= K2H_AREA_UNKNOWN;
	pLastestArea->file_offset	= 0L;
	pLastestArea->length		= 0UL;

	// set notice area update by monitor file
	bool	is_need_check = false;
	if(!FileMon.UpdateArea(is_need_check)){
		WAN_K2HPRN("Failed to update area notice throw monitor file, but continue...");
	}

	// remove lastest area from file( do truncate!! )
	if(!isAnonMem && !isTemporary && 0 != ftruncate(ShmFd, static_cast<size_t>(ExpOffset))){
		ERR_K2HPRN("Could not truncate file, errno = %d", errno);
		return false;
	}
	isCompressed = true;

	ALObjUnArea.Unlock();					// UNLOCK

	// need to check area update
	if(is_need_check){
		if(!DoAreaUpdate()){
			ERR_K2HPRN("Something error occurred during updating area information, but continue...");
		}
	}
	return true;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
