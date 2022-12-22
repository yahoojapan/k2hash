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

#include "k2hcommon.h"
#include "k2hpagemem.h"
#include "k2hdbg.h"

using namespace std;

//---------------------------------------------------------
// Class Methods
//---------------------------------------------------------
// cppcheck-suppress constParameter
bool K2HPageMem::GetData(const K2HShm* pk2hshm, PPAGEHEAD reladdr, unsigned char** ppPageData, size_t* pLength)
{
	if(!pk2hshm || !ppPageData || !pLength){
		ERR_K2HPRN("pk2hshm or ppPageData or pLength is NULL");
		return false;
	}

	K2HPageMem	k2hpage_obj;
	if(!k2hpage_obj.Initialize(pk2hshm, reladdr)){
		return false;
	}
	if(!k2hpage_obj.LoadData()){
		return false;
	}
	if(NULL == (*ppPageData = static_cast<unsigned char*>(malloc(k2hpage_obj.DataLength)))){
		ERR_K2HPRN("Could not allocation memory.");
		// cppcheck-suppress memleak
		return false;
	}
	memcpy(*ppPageData, k2hpage_obj.pPageData, k2hpage_obj.DataLength);

	// cppcheck-suppress memleak
	return true;
}

bool K2HPageMem::FreeData(unsigned char* pPageData)
{
	// cppcheck-suppress uselessAssignmentPtrArg
	K2H_Free(pPageData);
	return true;
}

//---------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------
K2HPageMem::K2HPageMem(const K2HShm* pk2hshm, PPAGEHEAD reladdr) : K2HPage(pk2hshm), pRelAddress(reladdr)
{
}

K2HPageMem::~K2HPageMem()
{
	Clean();
}

//---------------------------------------------------------
// Methods
//---------------------------------------------------------
bool K2HPageMem::Initialize(const K2HShm* pk2hshm, PPAGEHEAD reladdr)
{
	if(!reladdr){
		ERR_K2HPRN("PPAGEHEAD is null.");
		return false;
	}
	if(!K2HPage::Initialize(pk2hshm)){
		return false;
	}
	CleanMemory();
	pRelAddress	= reladdr;

	return true;
}

bool K2HPageMem::UploadPageHead(void)
{
	if(!isHeadLoaded){
		ERR_K2HPRN("Not loaded page head yet.");
		return false;
	}
	// Nothing to do.

	return true;
}

bool K2HPageMem::LoadPageHead(void)
{
	if(!pK2HShm){
		ERR_K2HPRN("K2HShm object pointer is not set.");
		return false;
	}
	if(isHeadLoaded){
		//MSG_K2HPRN("Already load page head");
		return true;
	}
	if(!pRelAddress){
		ERR_K2HPRN("PPAGEHEAD is not set.");
		return false;
	}
	pPageHead = reinterpret_cast<PPAGEHEAD>(pK2HShm->Abs(pRelAddress));

	isHeadLoaded = true;

	return true;
}

bool K2HPageMem::LoadData(bool isPageHead, bool isAllPage)
{
	if(!pK2HShm){
		ERR_K2HPRN("K2HShm object pointer is not set.");
		return false;
	}
	if(!pRelAddress){
		ERR_K2HPRN("PPAGEHEAD is not set.");
		return false;
	}

	// Clear inner memory
	if(isPageHead){
		CleanMemory();
	}else{
		CleanLoadedData();
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return false;
	}

	// allocation for reading one page
	if(NULL == AllocateData(PageSize - PAGEHEAD_SIZE)){
		ERR_K2HPRN("Could not allocation memory.");
		return false;
	}

	// copy to inner memory
	unsigned char*	pOneData = &(pPageHead->data[0]);	// so already load head
	if(pOneData && 0UL != pPageHead->length){
		memcpy(pPageData, pOneData, pPageHead->length);
		DataLength = pPageHead->length;
	}

	if(isAllPage && pPageHead->next){
		// load after this
		for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext; pNext = pTmpNext){
			K2HPageMem	PageAfter(pK2HShm, pNext);

			if(!PageAfter.LoadData(isPageHead, false)){
				ERR_K2HPRN("Failed to load data after this object.");
				return false;
			}
			// append after data
			if(PageAfter.pPageData && 0UL != PageAfter.DataLength){
				if(NULL == AppendData(PageAfter.pPageData, PageAfter.DataLength)){
					ERR_K2HPRN("Failed to append page data after this.");
					return false;
				}
			}
			pTmpNext = PageAfter.pPageHead->next;
		}
	}
	isLoaded = true;

	return true;
}

//
// This method is for iostream, and load only specified offset-length data into allocated buffer
// Returns last page object, if returned object is same as this, must not free it.
// If the result of this function is offset = next_read_pos, it means no more data.
//
K2HPage* K2HPageMem::LoadData(off_t offset, size_t length, unsigned char** byData, size_t& datalength, off_t& next_read_pos)
{
	if(!pK2HShm){
		ERR_K2HPRN("K2HShm object pointer is not set.");
		return NULL;
	}
	if(!pRelAddress){
		ERR_K2HPRN("PPAGEHEAD is not set.");
		return NULL;
	}
	if(!byData){
		ERR_K2HPRN("Parameter is wrong.");
		return NULL;
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return NULL;
	}

	// allocated
	bool	isAllocate = false;
	if(!(*byData)){
		if(NULL == (*byData = static_cast<unsigned char*>(malloc(length)))){
			ERR_K2HPRN("Could not allocate memory.");
			return NULL;
		}
		isAllocate = true;
	}

	// loop
	unsigned char*	byNext 		= *byData;
	K2HPageMem*		pPageAfter	= NULL;
	off_t			next_offset;

	for(PPAGEHEAD pTarget = pPageHead; pTarget; offset = next_offset){
		if(pTarget->length <= static_cast<size_t>(offset)){
			// skip to next
			next_offset		= offset - pTarget->length;

		}else{
			// page has specified area.
			size_t	this_length = min((pTarget->length - offset), length);

			// load data by offset and length
			unsigned char*	pOffsetData = ADDPTR(&(pTarget->data[0]), offset);
			memcpy(byNext, pOffsetData, this_length);

			length			-= this_length;
			datalength		+= this_length;
			byNext			= ADDPTR(byNext, this_length);
			offset			+= this_length;
			next_offset		= 0L;
		}

		if(0UL == length){
			break;
		}
		if(!pTarget->next){
			//MSG_K2HPRN("Could not read more length(%zu), because no more page chain.", length);
			if(0UL == datalength && isAllocate){
				K2H_Free(*byData);
			}
			break;
		}

		// set up next page
		K2H_Delete(pPageAfter);

		pPageAfter = new K2HPageMem(pK2HShm, pTarget->next);
		if(!pPageAfter->LoadPageHead() || !pPageAfter->pPageHead){
			ERR_K2HPRN("Failed to read after page head");
			K2H_Delete(pPageAfter);
			if(isAllocate){
				K2H_Free(*byData);
			}
			return NULL;
		}
		pTarget = pPageAfter->pPageHead;
	}

	if(!pPageAfter){
		pPageAfter = this;
	}
	next_read_pos = offset;

	return pPageAfter;
}

bool K2HPageMem::SetData(const unsigned char* byData, size_t length, bool isSetLoadedData)
{
	if(!byData && 0UL != length){
		ERR_K2HPRN("Parameters are wrong.");
		return false;
	}
	if(!pRelAddress || !pK2HShm){
		ERR_K2HPRN("PPAGEHEAD or pK2HShm is not set.");
		return false;
	}

	// Clear loaded data
	CleanLoadedData();

	// loop
	K2HPageMem*				pPageAfter	= this;
	size_t					remlength	= length;
	const unsigned char*	byRead		= byData;
	while(0UL < remlength){
		// data length
		size_t	one_length = min(remlength, (pPageAfter->PageSize - PAGEHEAD_SIZE));

		// write data length
		if(!pPageAfter->SetPageHead(SETHEAD_LENGTH, NULL, NULL, one_length)){
			ERR_K2HPRN("Could not upload page head.");
			if(pPageAfter != this){
				K2H_Delete(pPageAfter);
			}
			return false;
		}

		// write page data
		if(0UL < one_length){
			unsigned char*	pPageData = reinterpret_cast<unsigned char*>(pPageAfter->pK2HShm->Abs(ADDPTR(pPageAfter->pRelAddress, PAGEHEAD_DATA_OFFSET)));
			memcpy(pPageData, byRead, one_length);
		}

		// next pos
		byRead		= &byRead[one_length];
		remlength  -= one_length;

		// make next page object head
		PPAGEHEAD	pNext = pPageAfter->pPageHead->next;

		if(pPageAfter != this){
			K2H_Delete(pPageAfter);
		}
		if(0UL == remlength){
			break;
		}
		if(!pNext){
			ERR_K2HPRN("Failed to set data, because buffer does not exist.");
			return false;
		}
		pPageAfter = new K2HPageMem(pK2HShm, pNext);
	}

	// set inner buffer
	if(isSetLoadedData && 0UL < length){
		if(NULL == AppendData(byData, length)){
			ERR_K2HPRN("Failed to copy loaded data buffer.");
			return false;
		}
	}
	return true;
}

//
// This method is for iostream, and always clear inner data buffer.
// Returns last page object, if returned object is same as this, must not free it.
//
K2HPage* K2HPageMem::SetData(const unsigned char* byData, size_t length, off_t offset, bool& isChangeSize, off_t& next_write_pos)
{
	if(!byData || 0UL == length){
		ERR_K2HPRN("Parameters are wrong.");
		return NULL;
	}
	if(!pRelAddress || !pK2HShm){
		ERR_K2HPRN("PPAGEHEAD or pK2HShm is not set.");
		return NULL;
	}

	// Clear loaded data
	CleanLoadedData();

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return NULL;
	}

	// loop
	K2HPageMem*	pCurrentPage;
	PPAGEHEAD	pCurPageHead;
	for(pCurrentPage = this, pCurPageHead = pCurrentPage->pPageHead; pCurPageHead; pCurPageHead = pCurrentPage->pPageHead){
		// set next writing position from current object
		next_write_pos = offset + length;

		// set data to current page
		if((PageSize - PAGEHEAD_SIZE) <= static_cast<size_t>(offset)){
			// offset is over current page maximum length.
			if(pCurPageHead->length < (PageSize - PAGEHEAD_SIZE)){
				// if current page length is small by max length, grown area to maximum.
				if(!pCurrentPage->SetPageHead(SETHEAD_LENGTH, NULL, NULL, (PageSize - PAGEHEAD_SIZE))){
					ERR_K2HPRN("Could not upload page head.");
					if(this != pCurrentPage){
						K2H_Delete(pCurrentPage);
					}
					return NULL;
				}
				isChangeSize = true;

				// [NOTICE]
				// does not initialize page data
			}
			offset -= (PageSize - PAGEHEAD_SIZE);

		}else{
			// page has specified area.
			size_t	current_length = min(((PageSize - PAGEHEAD_SIZE) - offset), length);

			if(pCurPageHead->length < (current_length + offset)){
				// current page length is needed to change.
				if(!pCurrentPage->SetPageHead(SETHEAD_LENGTH, NULL, NULL, (current_length + offset))){
					ERR_K2HPRN("Could not upload page head.");
					if(this != pCurrentPage){
						K2H_Delete(pCurrentPage);
					}
					return NULL;
				}
				isChangeSize = true;

				// [NOTICE]
				// does not initialize page data
			}

			// set data from offset and length
			unsigned char*	pOffsetData = ADDPTR(&(pCurPageHead->data[0]), offset);
			memcpy(pOffsetData, byData, current_length);

			length	-= current_length;
			byData	= ADDPTR(byData, current_length);
			offset	= 0L;
		}

		if(0L == length){
			// finished.
			break;
		}

		if(!(pCurPageHead->next)){
			// expand for next object
			if(!pK2HShm->AddPages(pCurrentPage, length)){
				ERR_K2HPRN("Failed to expand(add) page objects");
				if(this != pCurrentPage){
					K2H_Delete(pCurrentPage);
				}
				return NULL;
			}
			pCurrentPage->CleanPageHead();
			if(!pCurrentPage->LoadPageHead() || !pCurrentPage->pPageHead || !pCurrentPage->pPageHead->next){
				ERR_K2HPRN("Could not reload head");
				if(this != pCurrentPage){
					K2H_Delete(pCurrentPage);
				}
				return NULL;
			}
			// force reset page head pointer.
			pCurPageHead = pCurrentPage->pPageHead;
			isChangeSize = true;
		}

		// make new page object
		if(this != pCurrentPage){
			K2H_Delete(pCurrentPage);
		}
		pCurrentPage = new K2HPageMem(pK2HShm, pCurPageHead->next);
		if(!pCurrentPage->LoadPageHead() || !pCurrentPage->pPageHead){
			ERR_K2HPRN("Failed to read page head");
			K2H_Delete(pCurrentPage);
			return NULL;
		}
	}

	return pCurrentPage;
}

//
// [NOTICE]
// This method only truncate length, so that new pages and area is not initialized.
//
bool K2HPageMem::Truncate(size_t length)
{
	if(!pK2HShm){
		ERR_K2HPRN("K2HShm object pointer is not set.");
		return false;
	}
	if(!pRelAddress){
		ERR_K2HPRN("PPAGEHEAD is not set.");
		return false;
	}
	if(0UL == length){
		ERR_K2HPRN("Parameter is wrong.");
		return false;
	}

	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return false;
	}

	if(length < pPageHead->length){
		// to short length
		if(!SetPageHead(SETHEAD_LENGTH, NULL, NULL, length)){
			ERR_K2HPRN("Could not upload page head.");
			return false;
		}
		if(pPageHead->next){
			// free next pages
			K2HPageMem	PageAfter(pK2HShm, pPageHead->next);
			if(!PageAfter.Free(NULL, NULL, true)){
				ERR_K2HPRN("Could not free pages.");
				return false;
			}
		}
	}else if(pPageHead->length < length){
		// to large length
		if(pPageHead->length < (PageSize - PAGEHEAD_SIZE)){
			size_t	one_length = min(length, (PageSize - PAGEHEAD_SIZE));
			if(!SetPageHead(SETHEAD_LENGTH, NULL, NULL, one_length)){
				ERR_K2HPRN("Could not upload page head.");
				return false;
			}
			length -= one_length;
		}
		if(0UL < length){
			// over this page, need next page
			if(!pPageHead->next){
				// expand next
				if(!pK2HShm->AddPages(this, length)){
					ERR_K2HPRN("Failed to expand(add) page objects");
					return false;
				}
				CleanPageHead();
				if(!LoadPageHead() || !pPageHead || !pPageHead->next){
					ERR_K2HPRN("Could not reload head");
					return false;
				}
			}

			for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext && 0UL < length; pNext = pTmpNext){
				K2HPageMem	PageAfter(pK2HShm, pNext);
				size_t		one_length = min(length, (PageSize - PAGEHEAD_SIZE));

				if(!PageAfter.SetPageHead(SETHEAD_LENGTH, NULL, NULL, one_length)){
					ERR_K2HPRN("Could not upload page head.");
					return false;
				}
				length	-= one_length;
				pTmpNext = PageAfter.pPageHead->next;
			}
			if(0UL < length){
				ERR_K2HPRN("There are not enough page objects.");
				return false;
			}
		}
	}
	return true;
}

PPAGEHEAD K2HPageMem::GetPageHeadRelAddress(void) const
{
	if(!pK2HShm){
		ERR_K2HPRN("There is no K2HShm object pointer, so this object is not initialized.");
		return NULL;
	}
	return pRelAddress;
}

PPAGEHEAD K2HPageMem::GetLastPageHead(unsigned long* pagecount, bool isAllPage)
{
	if(!pRelAddress || !pK2HShm){
		ERR_K2HPRN("PPAGEHEAD or pK2HShm is not set.");
		return NULL;
	}
	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return NULL;
	}
	if(pagecount){
		(*pagecount)++;
	}
	PPAGEHEAD	pReturn = pRelAddress;

	// next pages
	if(isAllPage && pPageHead->next){
		for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext; pNext = pTmpNext){
			K2HPageMem	PageAfter(pK2HShm, pNext);

			if(NULL == (pReturn = PageAfter.GetLastPageHead(pagecount, false))){
				return NULL;
			}
			if(!(PageAfter.pPageHead->next)){
				break;
			}
			pTmpNext = PageAfter.pPageHead->next;
		}
	}
	return pReturn;
}

bool K2HPageMem::Free(PPAGEHEAD* ppRelLastPageHead, unsigned long* pPageCount, bool isAllPage)
{
	if(!pRelAddress || !pK2HShm){
		WAN_K2HPRN("PPAGEHEAD or pK2HShm is not set.");
		return true;
	}
	// load page head
	if(!LoadPageHead() || !pPageHead){
		ERR_K2HPRN("Failed to read page head");
		return false;
	}

	// Free Page count
	unsigned long LocalPageCount = 1UL;
	if(!pPageCount){
		pPageCount = &LocalPageCount;
	}else{
		(*pPageCount)++;
	}

	// set own length.
	if(!SetPageHead(SETHEAD_LENGTH, NULL, NULL, 0UL)){
		WAN_K2HPRN("Failed to clear length, but continue...");
	}

	PPAGEHEAD	pRelPageHead	= pRelAddress;
	PPAGEHEAD	pRelLastPageHead= pRelAddress;

	// Free next pages
	if(isAllPage && pPageHead->next){
		for(PPAGEHEAD pNext = pPageHead->next, pTmpNext = NULL; pNext; pNext = pTmpNext){
			K2HPageMem	PageAfter(pK2HShm, pNext);

			if(!PageAfter.Free(&pRelLastPageHead, pPageCount, false)){
				WAN_K2HPRN("Failed to free pages, but continue...");
			}
			if(!(PageAfter.pPageHead->next)){
				break;
			}
			pTmpNext = PageAfter.pPageHead->next;
		}
	}
	if(ppRelLastPageHead){
		*ppRelLastPageHead = pRelLastPageHead;
	}

	// Re-chain into free list
	if(!ppRelLastPageHead){
		if(!pK2HShm->PutBackPages(pRelPageHead, pRelLastPageHead, *pPageCount)){
			ERR_K2HPRN("Failed to replace free page head.");
			return false;
		}
	}
	return true;
}

const unsigned char* K2HPageMem::GetData(const K2HShm* pk2hshm, PPAGEHEAD reladdr, size_t* pLength)
{
	if(!Initialize(pk2hshm, reladdr)){
		return NULL;
	}
	if(!isLoaded){
		if(!LoadData()){
			return NULL;
		}
	}
	if(pLength){
		*pLength = DataLength;
	}
	return pPageData;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noexpandtab sw=4 ts=4 fdm=marker
 * vim<600: noexpandtab sw=4 ts=4
 */
